//
// Created by yuanlu on 2022/10/4.
//

#ifndef IFR_OPENCV_IMGDISPLAY_H
#define IFR_OPENCV_IMGDISPLAY_H

#include <opencv2/opencv.hpp>
#include "defs.h"

using namespace std;
using namespace cv;

namespace ifr {
#if DEBUG_IMG
    namespace ImgDisplay {
        /**
                * 显示图像
                * @param name 图像名称
                * @param getter 生成图像
                */
        void setDisplay(const std::string &name, const std::function<cv::Mat()> &getter);
    };
#endif
} // ifr

#endif //IFR_OPENCV_IMGDISPLAY_H