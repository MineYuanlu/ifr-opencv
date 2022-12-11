//
// Created by yuanlu on 2022/11/2.
//

#ifndef IFR_OPENCV_PICTURE_H
#define IFR_OPENCV_PICTURE_H

#include "../defs.h"
#include "plan/Plans.h"
#include "msg/msg.hpp"

namespace ifr {

    class Picture {
    public:
        /**
         * 注册任务
         */
        static void registerTask() {
            static const std::string io_src = "src";
            static const std::string arg_path = "path";
            static const std::string arg_loop = "loop";
            static const std::string arg_period = "period";
            Plans::TaskDescription description{"input", "图片输入, 模拟相机输入"};
            description.io[io_src] = {TYPE_NAME(datas::FrameData), "输出一帧数据", false};
            description.args[arg_path] = {"图片文件的路径", "", ifr::Plans::TaskArgType::STR};
            description.args[arg_loop] = {"正整数, 代表循环次数, 0代表无限循环", "0", ifr::Plans::TaskArgType::NUMBER};
            description.args[arg_period] = {"正整数, 帧间隔(ms)", "20", ifr::Plans::TaskArgType::NUMBER};

            ifr::Plans::registerTask("picture", description, [](auto io, auto args, auto state, auto cb) {
                Plans::Tools::waitState(state, 1);

                const auto mat = cv::imread(args[arg_path], cv::ImreadModes::IMREAD_COLOR);
                const auto loop = std::stoull(args[arg_loop]);
                const auto period = std::stoull(args[arg_period]);
                ifr::Msg::Publisher<datas::FrameData> fdOut(io[io_src].channel);
                Plans::Tools::finishAndWait(cb, state, 1);
                fdOut.lock();
                cb(2);

                uint64_t id = 0;
                const auto delay = SLEEP_TIME((period > 1 ? period : 1) / 1000.0);//延时
                while (*state < 3 && (loop == 0 || id < loop)) {
                    fdOut.push({mat.clone(), id, id * period, cv::getTickCount(), datas::FrameType::BGR});
                    if (delay) SLEEP(delay);
                    id++;
                }
                //Plans::Tools::finishAndWait(cb, state, 3);
                //auto release && cb(4)
            });
        }
    };

} // ifr

#endif //IFR_OPENCV_PICTURE_H
