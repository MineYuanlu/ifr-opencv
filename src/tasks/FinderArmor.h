//
// Created by yuanlu on 2022/11/9.
//

#ifndef IFR_OPENCV_FINDERARMOR_H
#define IFR_OPENCV_FINDERARMOR_H

#include "../defs.h"
#include "opencv2/opencv.hpp"
#include <memory>
#include<opencv2/dnn.hpp>
#include "../Plans.h"
#include "../DataWaiter.h"

#if USE_GPU
typedef cv::cuda::GpuMat XMat;
#define __CV_NAMESPACE cv::cuda::
#else
typedef cv::Mat XMat;
#define __CV_NAMESPACE cv::
#endif


namespace Armor {
    class FinderArmor {
    private:
        cv::dnn::Net net_sm;//图像分类网络
        std::string net_sm_out;//图像分类输出层
//        mutable std::mutex net_sm_mtx;//网络锁
        cv::dnn::Net net_lg;//图像分类网络
        std::string net_lg_out;//图像分类输出层
//        mutable std::mutex net_lg_mtx;//网络锁

    public:
        const int thread_id;

        explicit FinderArmor(int thread_id) : thread_id(thread_id) {
#if DEBUG_IMG
            const int m = 8;
            const int w = 192 * m, h = 120 * m;
            cv::namedWindow("gray", cv::WINDOW_NORMAL);
            cv::resizeWindow("gray", w, h);
            cv::namedWindow("eqHist", cv::WINDOW_NORMAL);
            cv::resizeWindow("eqHist", w, h);
            cv::namedWindow("thr", cv::WINDOW_NORMAL);
            cv::resizeWindow("thr", w, h);
            cv::namedWindow("Contours", cv::WINDOW_NORMAL);
            cv::resizeWindow("Contours", w, h);
            cv::namedWindow("maybe target", cv::WINDOW_NORMAL);
          cv::resizeWindow("maybe target", w, h);
#endif
            initNet();
        }

        /**
         * 图像处理实际部分
         * @param src 输入
         * @param type 色彩空间转换
         * @param targets 装甲板识别输出
         */
        void handler(const cv::Mat &src, int type, std::vector<datas::ArmTargetInfo> &targets);

        /**初始化神经网络*/
        void initNet();

        /**
         * 使用神经网络预测图像类别
         * @param mat 输入图像
         * @param is_lg 是否是大装甲板
         * @return 装甲板类型
         */
        char predict(const cv::Mat &mat, bool is_lg);

        datas::ArmTargetInfos run(const datas::FrameData &data);

        static void registerTask() {
            static const std::string io_src = "src";
            static const std::string io_output = "target";
            static const std::string arg_thread_amount = "thread amount";
            ifr::Plans::TaskDescription description{"find", "装甲板识别"};
            description.io[io_src] = {TYPE_NAME(datas::FrameData), "输入的图像数据", true};
            description.io[io_output] = {TYPE_NAME(datas::ArmTargetInfos), "输出的目标信息", false};
            description.args[arg_thread_amount] = {"识别线程数量", "1", ifr::Plans::TaskArgType::NUMBER};

            ifr::Plans::registerTask("FinderArmor", description, [](auto io, auto args, auto state, auto cb) {
                ifr::Plans::Tools::waitState(state, 1);

                const auto finder_thread_amount = stoi(args[arg_thread_amount]);
                //初始化
//                shared_ptr<Assets> assets_ptr(new Assets());    //资源
                std::vector<std::shared_ptr<FinderArmor>> finders;             //识别器
                finders.reserve(finder_thread_amount);
                for (int i = 0; i < finder_thread_amount; ++i)
                    finders.push_back(std::make_shared<FinderArmor>(i));
                std::unique_ptr<std::thread[]> finder_threads(new std::thread[finder_thread_amount]); //识别线程


                //运行
                const auto &cname_src = io[io_src].channel;
                ifr::DataWaiter<uint64_t, datas::ArmTargetInfos> dw;  //数据整合


                for (const auto &finder: finders) {//识别线程
                    finder_threads[finder->thread_id] = thread([&cname_src, &dw, &finder, &state]() {
                        umt::Subscriber<datas::FrameData> fdIn(cname_src);
                        ifr::Plans::Tools::waitState(state, 2);
                        while (*state < 3) {
                            try {
                                const auto data = fdIn.pop_for(COMMON_LOOP_WAIT);
                                dw.start(data.id);
                                auto ti = finder->run(data);
                                dw.finish(data.id, ti);
                            } catch (umt::MessageError_Timeout &x) {
                                OUTPUT("[FinderArmor] Finder" + to_string(finder->thread_id) + "输出数据等待超时 " +
                                       std::to_string(COMMON_LOOP_WAIT) + "ms")
                            }
                        }
                    });
                    while (!finder_threads[finder->thread_id].joinable());
                }


                umt::Publisher<datas::ArmTargetInfos> tiOut(io[io_output].channel);//发布者
                ifr::Plans::Tools::finishAndWait(cb, state, 1);

                cb(2);
                while (*state < 3) {
                    try {
                        const auto data = dw.pop_for(COMMON_LOOP_WAIT);
//                        tiOut.push(data.second); TODO
                        std::cout << "[FinderArmor] result: " << data.first << ' ' << data.second.targets.size()
                                  << " = ";
                        for (const auto &x: data.second.targets)
                            std::cout << '[' << ((int) x.type) << ", " << x.angle << ", " << x.bad << "] ";
                        std::cout << std::endl;
                    } catch (ifr::Timeout &) {
                        OUTPUT("[FinderArmor] DW 输出数据等待超时 " + std::to_string(COMMON_LOOP_WAIT) + "ms")
                    }
                }
                for (int i = 0; i < finder_thread_amount; ++i)finder_threads[i].join();//等待识别线程退出
                ifr::Plans::Tools::finishAndWait(cb, state, 3);
                //auto release && cb(4)

            });
        };
    };
}


#endif //IFR_OPENCV_FINDERARMOR_H
