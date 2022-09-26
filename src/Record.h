//
// Created by yuanlu on 2022/9/7.
//

#ifndef IFR_OPENCV_RECORD_H
#define IFR_OPENCV_RECORD_H

#include "Camera.h"

namespace ifr {

    namespace Record {
        void startRecord(const std::string &dir = "/home/nvidia/2022-yuanlu/assets/record/");

        void stopRecord();

        void join();
    };

} // ifr

#endif //IFR_OPENCV_RECORD_H
