//
// Created by yuanlu on 2022/9/7.
//

#ifndef IFR_OPENCV_CAMERA_H
#define IFR_OPENCV_CAMERA_H

#define MSG_CAMERA "camera"

#include <GxIAPI.h>
#include <DxImageProc.h>
#include<opencv2/opencv.hpp>
#include <umt/umt.hpp>

typedef unsigned char BYTE;
namespace ifr {

    /**
     * 代表相机接口
     */
    namespace Camera {
        /**
         * 启动相机, 相机将在"camera"上推送Mat
         */
        int runCamera();

        void stopCamera();
    };

} // ifr

#endif //IFR_OPENCV_CAMERA_H
