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

#if __OS__ == __OS_Windows__

#include <Windows.h>

#elif __OS__ == __OS_Linux__

#include <unistd.h>

#endif

#ifndef DEBUG
#define DEBUG 1 //调试模式总开关
#endif

#define DEBUG_TIME (DEBUG && 0 ) //是否输出耗时信息
#define DEBUG_IMG (DEBUG && 0 ) //是否显示图片
#define DEBUG_AIM (DEBUG && 0 ) //是否调试瞄准

#if DEBUG_TIME || DEBUG_IMG
#define DEBUG_nowTime(name) auto name = cv::getTickCount();
#else
#define DEBUG_nowTime(name)
#endif


#define USE_GPU 0 //是否使用GPU加速计算

#define USE_FORECAST 1 //是否使用预测, 如果不使用预测, 则直接输出

#define DATA_IN 0 //输入方式 (相机/视频)
#define DATA_IN_CAMERA (DATA_IN==0)
#define DATA_IN_VIDEO (DATA_IN==1)
#define DATA_OUT 0 //输出方式(串口/打印)
#define DATA_OUT_PORT (DATA_OUT==0)
#define DATA_OUT_PRINT (DATA_OUT==1)

#define MSG_CAMERA "camera" //相机(或视频) 通道
#define MSG_DEBUG_TIME "DEBUG_TIME" //耗时记录 通道
#define MSG_OUTPUT "output" //输出移动矢量 通道


#define THREAD_IDENTITY 1 //识别目标的线程数量


#if DEBUG
#define SIMPLE_INSPECT(__Expr__) \
   std::cout<<"Expression " #__Expr__ " : "<<__Expr__<<std::endl;
#define OUTPUT(line) std::cout<<(line)<<std::endl;
#else
#define SIMPLE_INSPECT(__Expr__) __Expr__
#define OUTPUT(line)
#endif

#if USE_GPU
#define USE_GPU_SELECT(G, noG) G
#else
#define USE_GPU_SELECT(G, noG) noG
#endif

#if __OS__ == __OS_Windows__
#define SLEEP_TIME(t) (unsigned long) ((t) * 1000.0) //转换休眠时间, 单位: s
#define SLEEP(t) Sleep((t)) //执行休眠, 传入 SLEEP_TIME 的返回值
#define TYPE_NAME(x) (((x*)NULL),( #x )) //验证type名称, 并返回字符串形式
#elif __OS__ == __OS_Linux__
#define SLEEP_TIME(t) useconds_t((unsigned int)((t) * 1000000.0)) //转换休眠时间, 单位: s
#define SLEEP(t) usleep((t)) //执行休眠, 传入 SLEEP_TIME 的返回值
#define TYPE_NAME(x) #x //验证type名称, 并返回字符串形式
#endif


namespace datas {
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
}
#endif //OPENCV_TEST_DEFS_H
