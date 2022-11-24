//
// Created by yuanlu on 2022/11/17.
//

#ifndef IFR_OPENCV_AIMARMOR_H
#define IFR_OPENCV_AIMARMOR_H

#include <map>
#include "../defs.h"
#include "plan/Plans.h"
#include "msg/msg.hpp"
#include "config/config.h"
#include "../ImgDisplay.h"

namespace Armor {

    class AimArmor {
    private:
        const bool useForecast;//是否使用预测
        cv::Point2f target;//目标位置
        cv::Size size;//画幅大小
        cv::Point2f go;//瞄准位置
        cv::Point2f velocity;//移动矢量
    public:
        AimArmor(bool useForecast) : useForecast(useForecast) {
        }

        /**
         * 处理识别结果
         * @param id 帧ID
         * @param info 目标信息
         */
        datas::OutInfo handle(const datas::ArmTargetInfos info);

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
                ifr::Plans::TaskDescription description{"aim", "装甲板目标瞄准(直接)"};
                description.io[io_src] = {TYPE_NAME(datas::ArmTargetInfos), "目标信息", true};
                description.io[io_output] = {TYPE_NAME(datas::OutInfo), "输出数据", false};
                description.args[arg_offx] = {"偏移量x", "0", ifr::Plans::TaskArgType::NUMBER};
                description.args[arg_offy] = {"偏移量y", "0", ifr::Plans::TaskArgType::NUMBER};

                ifr::Plans::registerTask("AimArmor direct", description, [](auto io, auto args, auto state, auto cb) {
                    ifr::Plans::Tools::waitState(state, 1);

                    AimArmor aimArmor(false);
                    ifr::Msg::Subscriber<datas::ArmTargetInfos> tiIn(io[io_src].channel);
                    ifr::Msg::Publisher<datas::OutInfo> oiOut(io[io_output].channel);

                    ifr::Plans::Tools::finishAndWait(cb, state, 1);

                    oiOut.lock();

                    cb(2);
                    while (*state < 3) {
                        try {
                            oiOut.push(aimArmor.handle(tiIn.pop_for(COMMON_LOOP_WAIT)));
                        } catch (ifr::Msg::MessageError_NoMsg &) {
                            ifr::logger::err("AimArmor", "direct",
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

} // Armor

#endif //IFR_OPENCV_AIMARMOR_H
