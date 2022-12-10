//
// Created by yuanlu on 2022/9/15.
//

#ifndef OPENCV_TEST_DEFS_H
#define OPENCV_TEST_DEFS_H

#include<opencv2/opencv.hpp>
#include <stdio.h>
#include <iostream>
#include "ext_funcs.h"
#include "ext_types.h"
#include "tools/tools.hpp"

#ifndef DEBUG
#define DEBUG 1 //调试模式总开关
#endif

#define DEBUG_TIME (DEBUG && 1 ) //是否输出耗时信息
#define DEBUG_IMG (DEBUG && 0 ) //是否显示图片
#define DEBUG_AIM (DEBUG && 0 ) //是否调试瞄准

#if DEBUG_TIME || DEBUG_IMG
#define DEBUG_nowTime(name) auto name = cv::getTickCount();
#else
#define DEBUG_nowTime(name)
#endif

#ifndef USE_GPU
#define USE_GPU 0 //是否使用GPU加速计算
#endif

#if DEBUG
#define OUTPUT(line) std::cout<<(line)<<std::endl;
#else
#define OUTPUT(line)
#endif

#if USE_GPU
#define USE_GPU_SELECT(G, noG) G
#else
#define USE_GPU_SELECT(G, noG) noG
#endif

#if __OS__ == __OS_Windows__ && DEBUG
#define TYPE_NAME(x) (((x*)NULL),( #x )) //验证type名称, 并返回字符串形式
#else
#define TYPE_NAME(x) #x //验证type名称, 并返回字符串形式
#endif


namespace datas {
    /**
     * 一个多边形
     * @tparam TP 点数据类型
     * @tparam n 点数量
     * */
    template<typename TP, size_t n>
    struct Polygon {
        typedef cv::Point_<TP> Point;
        Point points[n];

        Polygon() = default;

        template<size_t len>
        Polygon(const Point (&ps)[len]) {
            static_assert(len >= n);
            for (size_t i = 0; i < n; i++)points[i] = ps[i];
        }

        inline Point center() const {
            Point center(0, 0);
            for (const auto &p: points)center += p;
            center /= static_cast<int>(n);
            return center;
        }
    };

    enum FrameType {
        BayerRG = cv::COLOR_BayerRG2GRAY,
        BGR = cv::COLOR_BGR2GRAY
    };
    /**一帧数据*/
    struct FrameData {
        cv::Mat mat;//图像
        uint64_t id;//帧ID (来自相机)
        uint64_t time; //帧时间戳(来自相机)
        int64 receiveTick;// 接收到图像时的tick
        FrameType type;
    };
    /**装甲板目标信息*/
    struct ArmTargetInfo {
        cv::RotatedRect target;//装甲板位置
        char type;//类型
        float angle;//旋转角度(0~180)
        bool is_large;//是否是大装甲板
        float bad;//怀分
    };
    /**装甲板目标信息*/
    struct ArmTargetInfos {
        cv::Size size;//画面大小
        uint64_t time; //帧时间戳(来自相机)
        int64 receiveTick;// 接收到图像时的tick
        std::vector<ArmTargetInfo> targets;//所有装甲板数据
    };
    /**能量机关目标信息*/
    struct TargetInfo {
        int activeCount;//激活数量
        bool findTarget;//目标类型 0 :未找到 1:找到但不确定 2: 确定
        cv::RotatedRect nowTarget;//目标机关臂位置
        cv::RotatedRect nowTargetAim;//目标装甲板位置
        cv::Point2f emCenter;//能量机关中心点
        cv::Size size;//画面大小
        uint64_t time; //帧时间戳(来自相机)
        int64 receiveTick;// 接收到图像时的tick
    };
    struct OutInfo {
        int targetType;//目标类型 0 :未找到 1:找到但不确定 2: 确定
        int activeCount;//激活数量
        cv::Point2f velocity;//移动矢量
        int64 receiveTick;// 接收到图像时的tick
    };

    /**游戏基础信息*/
    struct GameBasicInfo {
        bool gaming;
    };
}
#endif //OPENCV_TEST_DEFS_H
