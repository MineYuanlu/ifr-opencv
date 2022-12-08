//
// Created by yuanlu on 2022/11/2.
//

#ifndef IFR_OPENCV_VIDEO_H
#define IFR_OPENCV_VIDEO_H

#include "../defs.h"
#include "plan/Plans.h"
#include "msg/msg.hpp"

namespace ifr {

    class Video {
    private:
        cv::VideoCapture capture;
        size_t frameMaxCount, frameCount, fps, id;
        bool loop;
    public:
        explicit Video(const std::string &path, bool loop) : loop(loop) {
            if (!capture.open(path))throw std::runtime_error("Can not open video: " + path);
            frameMaxCount = (size_t) capture.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT);
            frameCount = 0;
            fps = (size_t) capture.get(cv::VideoCaptureProperties::CAP_PROP_FPS);
            id = 0;
        }

        ~Video() {
            capture.release();
        }

        bool read(cv::OutputArray mat) {
            if (!capture.read(mat)) throw std::runtime_error("Can not read frame: " + std::to_string(frameCount));
            ++id;
            if (++frameCount == frameMaxCount) {
                if (!loop) return false;
                frameCount = 0;
                capture.set(cv::CAP_PROP_POS_FRAMES, 0);
            }
            return true;
        }

        Video(const Video &obj) = delete;//拷贝构造函数

        Video &operator=(const Video &obj) = delete;//赋值运算符

        /**
    * 注册任务
    */
        static void registerTask() {
            static const std::string io_src = "src";
            static const std::string arg_path = "path";
            static const std::string arg_loop = "loop";
            static const std::string arg_delay = "delay";
            Plans::TaskDescription description{"input", "视频输入, 模拟相机输入"};
            description.io[io_src] = {TYPE_NAME(datas::FrameData), "输出一帧数据", false};
            description.args[arg_path] = {"视频文件的路径", "", ifr::Plans::TaskArgType::STR};
            description.args[arg_loop] = {"循环播放", "false", ifr::Plans::TaskArgType::BOOL};
            description.args[arg_delay] = {"帧延时(-1关闭, 0自动计算, 其它为ms)", "0", ifr::Plans::TaskArgType::NUMBER};

            ifr::Plans::registerTask("video", description, [](auto io, auto args, auto state, auto cb) {
                Plans::Tools::waitState(state, 1);

                Video video(args[arg_path], !strcmp("true", args[arg_loop].c_str()));
                ifr::Msg::Publisher<datas::FrameData> fdOut(io[io_src].channel);
                Plans::Tools::finishAndWait(cb, state, 1);
                fdOut.lock();
                cb(2);

                const auto delay_0 = stoi(args[arg_delay]);
                const auto delay = delay_0 < 0 ? 0 : SLEEP_TIME(delay_0 ? (delay_0 / 1000.0) : (1.0 / video.fps));//延时
                while (*state < 3) {
                    if (delay) SLEEP(delay);
                    cv::Mat mat;
                    bool hasNext = video.read(mat);
                    fdOut.push({mat, video.id, 1000 * video.id / video.fps, cv::getTickCount(), datas::FrameType::BGR});
                    if (!hasNext)return;
                }
                Plans::Tools::finishAndWait(cb, state, 3);
                //auto release && cb(4)
            });
        }
    };

} // ifr

#endif //IFR_OPENCV_VIDEO_H
