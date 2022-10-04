//
// Created by yuanlu on 2022/9/7.
//

#ifndef IFR_OPENCV_CAMERA_H
#define IFR_OPENCV_CAMERA_H

#include "defs.h"


#include <GxIAPI.h>
#include <DxImageProc.h>
#include<opencv2/opencv.hpp>
#include<iostream>
#include <umt/umt.hpp>

typedef unsigned char BYTE;
namespace ifr {

    /**
     * 代表相机接口
     */
    namespace Camera {


        /**启动相机*/
        int runCamera();

        /**停止相机*/
        GX_STATUS stopCamera();

        /**@return 图像高度*/
        int getHeight();

        /**@return 图像宽度*/
        int getWidth();
    };

} // ifr

#endif //IFR_OPENCV_CAMERA_H
