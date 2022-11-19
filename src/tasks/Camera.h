//
// Created by yuanlu on 2022/9/7.
//

#ifndef IFR_OPENCV_CAMERA_H
#define IFR_OPENCV_CAMERA_H

#include "../defs.h"
#include "plan/Plans.h"
#include "msg/msg.hpp"
#include "GxIAPI.h"
#include "DxImageProc.h"
#include<opencv2/opencv.hpp>
#include<iostream>
#include <functional>

typedef unsigned char BYTE;
namespace ifr {

    /**
     * 代表相机接口
     */
    class Camera {
    public:
        std::atomic_bool feed_dog = {false};//相机看门狗投喂标志
        GX_DEV_HANDLE m_hDevice = {};        //设备句柄
        int64_t m_nImageHeight = {};         //原始图像高
        int64_t m_nImageWidth = {};          //原始图像宽
        int64_t m_nPayLoadSize = {};         //数据大小
        int64_t m_nPixelColorFilter = {};    //Bayer格式
        int64_t m_nPixelSize = {};           //像素深度
        ifr::Msg::Publisher<datas::FrameData> publisher;//发布者
        static Camera *instance;

        Camera(const std::string &outName) : publisher(outName) {}

        ~Camera() {
            stopCamera();
        }

        /**启动相机*/
        GX_STATUS runCamera();

        GX_STATUS initCamera();

        /**停止相机*/
        GX_STATUS stopCamera();

        /**@return 图像高度*/
        int getHeight();

        /**@return 图像宽度*/
        int getWidth();


        /**
         * 注册任务
         */
        static void registerTask() {
            static const std::string io_src = "src";
            Plans::TaskDescription description{"input", "相机输入, 负责采集图像"};
            description.io[io_src] = {TYPE_NAME(datas::FrameData), "输出一帧数据", false};

            Plans::registerTask("camera", description, [](auto io, auto args, auto state, auto cb) {
                Plans::Tools::waitState(state, 1);

                std::unique_ptr<Camera, void (*)(Camera *)> camera(
                        instance = new Camera(io[io_src].channel),
                        [](Camera *x) {
                            delete x;
                            instance = nullptr;
                        });
                if (camera->initCamera() != GX_STATUS_SUCCESS) throw std::runtime_error("Can not init Camera");
                Plans::Tools::finishAndWait(cb, state, 1);
                camera->publisher.lock();
                if (camera->runCamera() != GX_STATUS_SUCCESS) throw std::runtime_error("Can not run Camera");
                cb(2);
                static const auto maxNoInput = 5;//最长无输入秒数
                static const auto delay = SLEEP_TIME(COMMON_LOOP_WAIT / 1000.0);//延时
                static const auto maxCnt = int(maxNoInput / (COMMON_LOOP_WAIT / 1000.0));//5秒计数
                int count = 0;
                while (*state < 3) {
                    SLEEP(delay);
                    if (++count > maxCnt) {
                        count = 0;
                        static auto flag = true;
                        if (!camera->feed_dog.compare_exchange_strong(flag, false)) {
                            OUTPUT("[Camera] Watch dog error: No Input")
                            throw std::runtime_error(
                                    "[Watch dog] Camera No input exceeds " + std::to_string(maxNoInput) +
                                    " seconds");
                        }
                    }
                }
                Plans::Tools::finishAndWait(cb, state, 3);
                //auto release && cb(4)

            });
        }
    };


} // ifr

#endif //IFR_OPENCV_CAMERA_H
