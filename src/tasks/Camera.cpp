//
// Created by yuanlu on 2022/9/7.
//

#include "Camera.h"


typedef unsigned char BYTE;

using namespace std::placeholders;

namespace ifr {

    Camera *Camera::instance = nullptr;

    /*图像回调处理函数*/
    static void GX_STDC OnFrameCallbackFun(GX_FRAME_CALLBACK_PARAM *pFrame) {
        if (pFrame->status == 0) {
            cv::Mat src(Camera::instance->m_nImageHeight, Camera::instance->m_nImageWidth, CV_8UC1,
                        (void *) pFrame->pImgBuf);
            Camera::instance->publisher.push(
                    {src, pFrame->nFrameID, pFrame->nTimestamp, cv::getTickCount(), datas::FrameType::BayerRG}
            );
            Camera::instance->feed_dog = true;
#if DEBUG
//            static uint64 nextId = pFrame->nFrameID;//更新ID
//            if (nextId != pFrame->nFrameID)
//                std::cout << "[相机] 跳跃ID: " << nextId << ' ' << pFrame->nTimestamp << " " << pFrame->pImgBuf
//                          << " "
//                          << pFrame->nImgSize
//                          << std::endl;
//            nextId = pFrame->nFrameID + 1;
#endif

        }
    }

    GX_STATUS Camera::runCamera() {
        //发送开采命令
        GX_STATUS emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_START);

        ifr::logger::log("Camera", "Run camera", emStatus == GX_STATUS_SUCCESS ? "success" : "fail");

        return emStatus;
    }

    GX_STATUS Camera::initCamera() {
        ifr::logger::log("Camera", "Start init camera...");
        GX_STATUS emStatus = GX_STATUS_SUCCESS;
        GX_OPEN_PARAM openParam;
        uint32_t nDeviceNum = 0;
        openParam.accessMode = GX_ACCESS_EXCLUSIVE;
        openParam.openMode = GX_OPEN_INDEX;
        openParam.pszContent = new char[2]{'1', '\0'};
        // 初始化库
        emStatus = GXInitLib();
        if (emStatus != GX_STATUS_SUCCESS) {
            ifr::logger::err("Camera", "Init Lib fail", emStatus);
            return emStatus;
        }
        // 枚举设备列表
        emStatus = GXUpdateDeviceList(&nDeviceNum, 1000);
        if ((emStatus != GX_STATUS_SUCCESS) || (nDeviceNum <= 0)) {
            ifr::logger::err("Camera", "Can not found any camera", emStatus);
            return emStatus ? emStatus : GX_STATUS_ERROR;
        }
        //打开设备
        emStatus = GXOpenDevice(&openParam, &m_hDevice);

        if (use_trigger) {
            emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON);
            emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_SWITCH, GX_TRIGGER_SWITCH_ON);
            emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE2);
            emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_ACTIVATION, GX_TRIGGER_ACTIVATION_RISINGEDGE);
            emStatus = GXSetFloat(m_hDevice, GX_FLOAT_TRIGGER_FILTER_RAISING, 0.0f);
            emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_SELECTOR, GX_ENUM_TRIGGER_SELECTOR_FRAME_START);
        } else {
            emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_SWITCH, GX_TRIGGER_SWITCH_OFF);
            emStatus = GXSetEnum(m_hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF);
        }

        //设置采集模式连续采集
        emStatus = GXSetEnum(m_hDevice, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
        emStatus = GXSetInt(m_hDevice, GX_INT_ACQUISITION_SPEED_LEVEL, 1);
        emStatus = GXSetEnum(m_hDevice, GX_ENUM_BALANCE_WHITE_AUTO, GX_BALANCE_WHITE_AUTO_CONTINUOUS);
        //设置自动曝光
        emStatus = GXSetEnum(m_hDevice, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_OFF);    //关闭自动曝光
        emStatus = GXSetFloat(m_hDevice, GX_FLOAT_EXPOSURE_TIME, exposure); //初始曝光时间
        //emStatus = GXSetEnum(m_hDevice, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_CONTINUOUS);//自动曝光
        //emStatus = GXSetFloat(m_hDevice, GX_FLOAT_AUTO_EXPOSURE_TIME_MIN, 20.0000);//自动曝光最小值
        //emStatus = GXSetFloat(m_hDevice, GX_FLOAT_AUTO_EXPOSURE_TIME_MAX, 100000.0000);//自动曝光最大值
        //emStatus = GXSetInt(m_hDevice, GX_INT_GRAY_VALUE, 150);//期望灰度值
        //设置自动白平衡
        emStatus = GXSetEnum(m_hDevice, GX_ENUM_BALANCE_WHITE_AUTO,
                             GX_BALANCE_WHITE_AUTO_CONTINUOUS);                //自动白平衡
        emStatus = GXSetEnum(m_hDevice, GX_ENUM_AWB_LAMP_HOUSE,
                             GX_AWB_LAMP_HOUSE_ADAPTIVE);                    //自动白平衡光源

        //bool      bColorFliter = false;
        emStatus = GXGetInt(m_hDevice, GX_INT_PAYLOAD_SIZE, &m_nPayLoadSize);  // 获取图像大小
        emStatus = GXGetInt(m_hDevice, GX_INT_WIDTH, &m_nImageWidth); // 获取宽度
        emStatus = GXGetInt(m_hDevice, GX_INT_HEIGHT, &m_nImageHeight);// 获取高度
        emStatus = GXGetEnum(m_hDevice, GX_ENUM_PIXEL_SIZE, &m_nPixelSize);
        //GXSetFloat(m_hDevice, GX_FLOAT_EXPOSURE_TIME, 20000);
        if (!(m_nImageHeight > 0 && m_nImageWidth > 0)) {
            ifr::logger::err("Camera", "Open camera fail", emStatus);
            return GX_STATUS_ERROR;
        }
        //判断相机是否支持bayer格式
        bool m_bColorFilter;
        emStatus = GXIsImplemented(m_hDevice, GX_ENUM_PIXEL_COLOR_FILTER, &m_bColorFilter);
        if (m_bColorFilter) {
            emStatus = GXGetEnum(m_hDevice, GX_ENUM_PIXEL_COLOR_FILTER, &m_nPixelColorFilter);
        }

        //注册图像处理回调函数
        emStatus = GXRegisterCaptureCallback(m_hDevice, nullptr, OnFrameCallbackFun);
        //发送开采命令
        emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_START);

        ifr::logger::log("Camera", "Success init");

        return GX_STATUS_SUCCESS;
    }

    GX_STATUS Camera::stopCamera() {
        ifr::logger::log("Camera", "Start stop Camera...");
        //发送停采命令
        GX_STATUS emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_STOP);
        //注销采集回调
        emStatus = GXUnregisterCaptureCallback(m_hDevice);

        emStatus = GXCloseDevice(m_hDevice);
        emStatus = GXCloseLib();
        ifr::logger::log("Camera", "Success stop Camera");
        return emStatus;
    }

    int Camera::getHeight() {
        return m_nImageHeight;
    }

    int Camera::getWidth() {
        return m_nImageWidth;
    }


} // ifr