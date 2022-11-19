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
    public:
        explicit Video(const std::string &path) {
            if (!capture.open(path))throw std::runtime_error("Can not open video: " + path);
            frameMaxCount = (size_t) capture.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT);
            frameCount = 0;
            fps = (size_t) capture.get(cv::VideoCaptureProperties::CAP_PROP_FPS);
            id = 0;
        }

        ~Video() {
            capture.release();
        }

        void read(cv::OutputArray mat) {
            if (!capture.read(mat)) throw std::runtime_error("Can not read frame: " + std::to_string(frameCount));
            ++id;
            if (++frameCount == frameMaxCount) {
                frameCount = 0;
                capture.set(cv::CAP_PROP_POS_FRAMES, 0);
            }
        }

        Video(const Video &obj) = delete;//拷贝构造函数

        Video &operator=(const Video &obj) = delete;//赋值运算符

        /**
    * 注册任务
    */
        static void registerTask() {
            static const std::string io_src = "src";
            static const std::string arg_path = "path";
            Plans::TaskDescription description{"input", "视频输入, 模拟相机输入"};
            description.io[io_src] = {TYPE_NAME(datas::FrameData), "输出一帧数据", false};
            description.args[arg_path] = {"视频文件的路径", "", ifr::Plans::TaskArgType::STR};

            ifr::Plans::registerTask("video", description, [](auto io, auto args, auto state, auto cb) {
                Plans::Tools::waitState(state, 1);

                Video video(args[arg_path]);
                ifr::Msg::Publisher<datas::FrameData> fdOut(io[io_src].channel);
                Plans::Tools::finishAndWait(cb, state, 1);
                fdOut.lock();
                cb(2);
                const auto delay = SLEEP_TIME(1.0 / video.fps);//延时
                while (*state < 3) {
                    SLEEP(delay);
                    cv::Mat mat;
                    video.read(mat);
                    fdOut.push({mat, video.id, 1000 * video.id / video.fps, cv::getTickCount(), datas::FrameType::BGR});
                }
                Plans::Tools::finishAndWait(cb, state, 3);
                //auto release && cb(4)
            });
        }
    };

} // ifr

#endif //IFR_OPENCV_VIDEO_H
