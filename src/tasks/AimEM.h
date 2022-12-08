//
// Created by yuanlu on 2022/9/21.
//

#include <map>
#include "../defs.h"
#include "plan/Plans.h"
#include "msg/msg.hpp"
#include "config/config.h"
#include "../ImgDisplay.h"
#include "predict_needed/headers/predict_model.hpp"

#ifndef IFR_OPENCV_FORECASTEM_H
#define IFR_OPENCV_FORECASTEM_H

#if DEBUG_IMG || DEBUG_AIM
#define WINNAME_AIM_VAL "aim values"
#define WINNAME_AIM_IMG "aim img"
#define AIM_TBAR_MIDDLE 5000
#define AIM_TBAR_MAX 10000
#endif

namespace EM {


    class AimEM {
    public:
        const bool useForecast;//是否使用预测
        cv::Point2f target;//目标位置
        cv::Size size;//画幅大小
        cv::Point2f go;//瞄准位置
        cv::Point2f velocity;//移动矢量

        struct AimOffset {//瞄准偏移
            int offset_x, offset_y;
        };
        AimOffset aimOffset;

        /**
         * 处理识别结果
         * @param id 帧ID
         * @param info 目标信息
         */
        datas::OutInfo handle(const datas::TargetInfo &info);

        AimEM(bool useForecast, AimOffset offset);


    public:
        /**
    * 注册任务
    */
        static void registerTask() {
            static const std::string io_src = "target";
            static const std::string io_output = "output";
            static const std::string arg_offx = "offx";
            static const std::string arg_offy = "offy";
            {
                ifr::Plans::TaskDescription description{"aim", "能量机关目标瞄准(直接)"};
                description.io[io_src] = {TYPE_NAME(datas::TargetInfo), "目标信息", true};
                description.io[io_output] = {TYPE_NAME(datas::OutInfo), "输出数据", false};
                description.args[arg_offx] = {"偏移量x", "0", ifr::Plans::TaskArgType::NUMBER};
                description.args[arg_offy] = {"偏移量y", "0", ifr::Plans::TaskArgType::NUMBER};

                ifr::Plans::registerTask("AimEM direct", description, [](auto io, auto args, auto state, auto cb) {
                    ifr::Plans::Tools::waitState(state, 1);

                    AimEM aimEm(false, {stoi(args[arg_offx]), stoi(args[arg_offy])});
                    ifr::Msg::Subscriber<datas::TargetInfo> tiIn(io[io_src].channel);
                    ifr::Msg::Publisher<datas::OutInfo> oiOut(io[io_output].channel);

                    ifr::Plans::Tools::finishAndWait(cb, state, 1);

                    oiOut.lock();

                    cb(2);
                    while (*state < 3) {
                        try {
                            oiOut.push(aimEm.handle(tiIn.pop_for(COMMON_LOOP_WAIT)));
                        } catch (ifr::Msg::MessageError_NoMsg &) {
                            ifr::logger::err("AimEM", "direct",
                                             "数据等待超时:" + std::to_string(COMMON_LOOP_WAIT) + " ms");
                        } catch (ifr::Msg::MessageError_Broke &) {
                            break;
                        }
                    }
                    ifr::Plans::Tools::waitState(state, 3);
                    ifr::Plans::Tools::finishAndWait(cb, state, 3);
                    //auto release && cb(4)
                });
            }
            {
                ifr::Plans::TaskDescription description{"aim", "能量机关目标瞄准(预测)"};
                description.io[io_src] = {TYPE_NAME(datas::TargetInfo), "目标信息", true};
                description.io[io_output] = {TYPE_NAME(datas::OutInfo), "输出数据", false};
                description.args[arg_offx] = {"偏移量x", "0", ifr::Plans::TaskArgType::NUMBER};
                description.args[arg_offy] = {"偏移量y", "0", ifr::Plans::TaskArgType::NUMBER};

                ifr::Plans::registerTask("AimEM forecast", description, [](auto io, auto args, auto state, auto cb) {
                    ifr::Plans::Tools::waitState(state, 1);

                    AimEM aimEm(true, {stoi(args[arg_offx]), stoi(args[arg_offy])});
                    ifr::Msg::Subscriber<datas::TargetInfo> tiIn(io[io_src].channel);
                    ifr::Msg::Publisher<datas::OutInfo> oiOut(io[io_output].channel);

                    ifr::Plans::Tools::finishAndWait(cb, state, 1);

                    oiOut.lock();

                    cb(2);
                    while (*state < 3) {
                        try {
                            oiOut.push(aimEm.handle(tiIn.pop_for(COMMON_LOOP_WAIT)));
                        } catch (ifr::Msg::MessageError_NoMsg &) {
                            ifr::logger::err("AimEM", "forecast",
                                             "数据等待超时:" + std::to_string(COMMON_LOOP_WAIT) + " ms");
                        } catch (ifr::Msg::MessageError_Broke &) {
                            break;
                        }
                    }
                    ifr::Plans::Tools::waitState(state, 3);
                    ifr::Plans::Tools::finishAndWait(cb, state, 3);
                    //auto release && cb(4)
                });
            }
        }
    };

} // EM

#endif //IFR_OPENCV_FORECASTEM_H
