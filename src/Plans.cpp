//
// Created by yuanlu on 2022/10/5.
//

#include "Plans.h"

namespace ifr {
    namespace Plans {
        using namespace rapidjson;
        /**访问锁*/
        std::recursive_mutex mtx;
        /**任务描述*/std::string taskDescriptionsJson = "{}";
        /**任务组*/std::string taskGroupJson = "[]";


        /**所有任务的注册数据*/
        std::map<std::string, const Task> tasks;

        /**所有计划信息*/
        std::map<std::string, PlanInfo> plans;

        std::string planListJson = "[]";

        Config::ConfigController cc;


        /**更新任务描述的json串*/
        void updateTaskDescriptionsJson(const std::string &name, const TaskDescription &description) {
            std::unique_lock<std::recursive_mutex> lock(mtx);
            static rapidjson::StringBuffer taskDescriptions;
            static rapidjson::Writer<rapidjson::StringBuffer, Document::EncodingType, UTF8<>> taskDescriptionsWriter(
                    taskDescriptions);
            if (taskDescriptions.GetLength() <= 0)taskDescriptionsWriter.StartObject();

            taskDescriptionsWriter.Key(name), description(taskDescriptionsWriter);

            taskDescriptionsWriter.Flush();
            taskDescriptionsJson = taskDescriptions.GetString();
            taskDescriptionsJson.push_back('}');

            //----
            static rapidjson::StringBuffer taskGroup;
            static rapidjson::Writer<rapidjson::StringBuffer, Document::EncodingType, UTF8<>> taskGroupWriter(
                    taskGroup);
            if (taskGroup.GetLength() <= 0)taskGroupWriter.StartArray();
            taskGroupWriter.String(description.group);
            taskGroupWriter.Flush();
            taskGroupJson = taskDescriptions.GetString();
            taskGroupJson.push_back(']');

        }

        /**检查plan名称是否合格*/
        inline bool checkPlanName(std::string str) {
            return std::all_of(str.begin(), str.end(), [](const auto &x) {
                return (('a' <= x && x <= 'z') || ('A' <= x && x <= 'Z') || ('0' <= x && x <= '9') || x == '_' ||
                        x == ' ' || x == '-');
            });
        }

        void readPlanInfo(const std::string &name) {
#if IO_USE_JSON
            std::ifstream fin("runtime/plans/" + name + ".json");
            if (fin.is_open()) {
                rapidjson::IStreamWrapper isw(fin);
                Document d;
                d.ParseStream(isw);
                plans.insert(std::pair<std::string, PlanInfo>(name, PlanInfo::read(d)));
            } else {
                plans.insert(std::pair<std::string, PlanInfo>(name, {}));
            }
#else
            std::ifstream fin("runtime/plans/" + name + ".data");
            if (fin.is_open()) {
                plans.insert(std::pair<std::string, PlanInfo>(name, PlanInfo::read(fin)));
            } else {
                plans.insert(std::pair<std::string, PlanInfo>(name, {}));
            }
            fin.close();
#endif
        }

        void writePlanInfo(const PlanInfo &info) {
            if (!info.loaded)return;
#if IO_USE_JSON
            std::string file = "runtime/plans/" + info.name + ".json";
            ifr::Config::mkDir(ifr::Config::getDir(file));
            std::ofstream fout(file, std::ios_base::out | std::ios_base::trunc);
            if (fout.is_open()) {
                rapidjson::OStreamWrapper osw(fout);
                rapidjson::Writer<OStreamWrapper> w(osw);
                info(w);
                w.Flush();
                osw.Flush();
                fout.flush();
                fout.close();
            }
#else
            std::string file="runtime/plans/" + info.name + ".data";
            ifr::Config::mkdir(ifr::Config::getDir(file));
            std::ofstream fout(file, std::ios_base::out | std::ios_base::trunc);
            if (fout.is_open()) {
                info(fout);
                fout.flush();
                fout.close();
            }
#endif
        }

        void registerTask(const std::string &name, const TaskDescription &description,
                          const Task &registerTask) {
            std::unique_lock<std::recursive_mutex> lock(mtx);
            updateTaskDescriptionsJson(name, description);
            tasks.insert(std::pair<std::string, const Task>(name, registerTask));
        }

        std::string getTaskDescriptionsJson() {
            std::unique_lock<std::recursive_mutex> lock(mtx);
            return taskDescriptionsJson;
        }

        std::string getTaskGroupsJson() {
            std::unique_lock<std::recursive_mutex> lock(mtx);
            return taskGroupJson;
        }

        std::string getPlanListJson() {
            std::unique_lock<std::recursive_mutex> lock(mtx);
            if (!planListJson.empty())return planListJson;
            StringBuffer buf;
            Writer<StringBuffer> w(buf);
            w.StartArray();
            for (const auto &e: plans)w.String(e.first);
            w.EndArray();
            w.Flush();
            planListJson = std::string(buf.GetString(), buf.GetLength());
            return planListJson;
        }

        void init() {
            static ifr::Config::ConfigInfo<std::map < std::string, PlanInfo> > info = {
                    [](auto *a, auto &w) {
                        w.StartArray();
                        for (const auto &e: plans)w.String(e.first);
                        w.EndArray();
                    },
                    [](auto *a, const rapidjson::Document &d) {
                        for (const auto &item: d.GetArray()) {
                            std::string name = item.GetString();
                            if (!checkPlanName(name))continue;
                            readPlanInfo(name);
                        }
                    }
            };
            cc = ifr::Config::createConfig("plan-list", &plans, info);
            cc.load();
        }

        std::vector<std::string> getPlanList() {
            std::unique_lock<std::recursive_mutex> lock(mtx);
            std::vector<std::string> vec;
            vec.reserve(plans.size());
            for (const auto &e: plans)vec.push_back(e.first);
            return vec;

        }

        PlanInfo getPlanInfo(const std::string &name) {
            std::unique_lock<std::recursive_mutex> lock(mtx);
            return plans[name];
        }

        void savePlanInfo(const PlanInfo &info) {
            std::unique_lock<std::recursive_mutex> lock(mtx);
            planListJson = "";
            plans[info.name] = info;
            writePlanInfo(info);
            cc.save();
        }

        void removePlanInfo(const std::string &name) {
            std::unique_lock<std::recursive_mutex> lock(mtx);
            planListJson = "";
            plans.erase(name);
            cc.save();
        }


        ///运行数据, 包括全部流程控制
        namespace RunData {
            const auto delay = SLEEP_TIME(0.1);// 自旋等待间隔
            std::recursive_mutex mtx;//访问锁
            std::string currentPlan;//前流程名称
            int runID;//运行ID, 每次启动任务时改变, 防止不同批次任务混淆

            bool running = false;

            int state;//当前运行阶段, 详见Task描述
            std::set<std::string> runningTasks;//运行中的task名称
            std::set<std::string> waitingTasks;//等待中的task名称 (等待运行阶段完成)
            std::set<std::string> finishingTasks;//等待完毕的task名称 (等待程序退出)


            /**@return 当前阶段是否运行完毕*/
            bool isStepFinish() {
                std::unique_lock<std::recursive_mutex> lock(RunData::mtx);
                return waitingTasks.empty();
            }

            /**
             * 进入下一阶段
             * @return false = 当前阶段还未执行, 不可进入下一阶段
             * @return true = 成功进入下一阶段 (但不代表下一阶段执行完毕)
             */
            bool nextStep() {
                std::unique_lock<std::recursive_mutex> lock(RunData::mtx);
                if (!isStepFinish())return false;
                waitingTasks = std::set<std::string>(runningTasks.begin(), runningTasks.end());
                state++;
                return true;
            }

            /**
             * 自动执行步骤, 直到到达了指定的步骤
             *
             * @param target 目标步骤
             * @param waitFinish 是否等待步骤结束
             */
            inline void untilStep(int target, bool waitFinish = false) {
                for (bool b = false; state < target; b = true) {
                    if (b)SLEEP(delay);
                    nextStep();
                }
                if (waitFinish)while (!isStepFinish())SLEEP(delay);
            }

            /**
             * 自动执行步骤, 但立即返回
             * @param target
             */
            inline void goStep(int target) {
                std::thread t = std::thread([&target]() {
                    untilStep(target);
                });
                while (!t.joinable());
                t.detach();
            }

            /**
             * 停止流程并重置
             */
            void reset() {
                std::unique_lock<std::recursive_mutex> lock(RunData::mtx);
                if (!running)return;

                untilStep(4, true);//结束
                while (!finishingTasks.empty()) SLEEP(delay);

                state = 0;
                runningTasks.clear();
                waitingTasks.clear();
                running = false;
            }

            /**
             * 启动流程
             */
            void start(const std::string &name) {
                std::unique_lock<std::recursive_mutex> lock(RunData::mtx);
                if (running)reset();
                currentPlan = name;

                const auto rid = ++runID;
                const auto plan = plans[name];

                for (const auto &ele: plan.tasks) {
                    const auto &tname = ele.first;
                    const auto &task = ele.second;
                    if (!task.enable)continue;

                    std::map<const std::string, TaskIOInfo> io(task.io.begin(), task.io.end());

                    runningTasks.insert(tname);

                    const auto &regTask = tasks[tname];
                    std::thread t = std::thread([&regTask, &rid, &tname, &io]() {
                        regTask(io, &state, [&rid, &tname](const auto finish) {
                            std::unique_lock<std::recursive_mutex> lock(RunData::mtx);
                            if (runID != rid || finish != state)return;
                            waitingTasks.erase(tname);
                        });
                        if (runID != rid)return;
                        std::unique_lock<std::recursive_mutex> lock(RunData::mtx);
                        finishingTasks.erase(tname);
                    });
                    while (!t.joinable());
                    t.detach();
                }
                finishingTasks = std::set<std::string>(runningTasks.begin(),
                                                       runningTasks.end());
                goStep(2);//进入运行阶段
                running = true;
            }
        }

        void usePlanInfo(const std::string &name) {
            std::unique_lock<std::recursive_mutex> lock(mtx);
            RunData::start(name);
        }


    }
} // ifr