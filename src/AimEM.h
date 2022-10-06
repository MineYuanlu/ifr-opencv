//
// Created by yuanlu on 2022/9/21.
//

#include <map>
#include "defs.h"
#include <opencv2/opencv.hpp>

#ifndef IFR_OPENCV_FORECASTEM_H
#define IFR_OPENCV_FORECASTEM_H

#if DEBUG_IMG || DEBUG_AIM
#define WINNAME_AIM_VAL "aim values"
#define WINNAME_AIM_IMG "aim img"
#define AIM_TBAR_MIDDLE 5000
#define AIM_TBAR_MAX 10000
#endif

#define AIM_VALUE_FILE "aim.txt"
namespace EM {


    namespace AimEM {
        /**
         * 处理识别结果
         * @param id 帧ID
         * @param info 目标信息
         */
        datas::OutInfo handle(const uint64_t id, const datas::TargetInfo info);

        /**
         * 初始化瞄准器
         */
        void init();

        /**
         * 保存参数
         */
        void saveValue();

        /**
         * 读取参数
         */
        void readValue();

    };

} // EM

#endif //IFR_OPENCV_FORECASTEM_H
