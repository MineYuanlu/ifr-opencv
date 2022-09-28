//
// Created by yuanlu on 2022/9/7.
//

#include "Camera.h"
#include "Signal.h"


typedef unsigned char BYTE;

namespace ifr {
    namespace Camera {


        GX_DEV_HANDLE m_hDevice;        //设备句柄
        BYTE *m_pBufferRaw;             //原始图像数据
        BYTE *m_pBufferRGB;             //RGB图像数据，用于显示和保存bmp图像
        int64_t m_nImageHeight;         //原始图像高
        int64_t m_nImageWidth;          //原始图像宽
        int64_t m_nPayLoadSize;         //数据大小
        int64_t m_nPixelColorFilter;    //Bayer格式
        int64_t m_nPixelSize;           //像素深度
        umt::Publisher<datas::FrameData> publisher(MSG_CAMERA);//发布者
#if DEBUG
        uint64 id = 0;//更新ID
#endif


        /*图像回调处理函数*/
        static void GX_STDC OnFrameCallbackFun(GX_FRAME_CALLBACK_PARAM *pFrame) {
            if (pFrame->status == 0) {
                cv::Mat src(m_nImageHeight, m_nImageWidth, CV_8UC1, (void *) pFrame->pImgBuf);
                publisher.push({src, pFrame->nFrameID, pFrame->nTimestamp});
                ifr::Signal::feed_dog();//喂狗
#if DEBUG
                if (id + 1 != pFrame->nFrameID)
                    std::cout << "[相机] 跳跃ID: " << id << ' ' << pFrame->nTimestamp << " " << pFrame->pImgBuf << " "
                              << pFrame->nImgSize
                              << std::endl;
                id = pFrame->nFrameID;
#endif

            }
        }

        int runCamera() {
            GX_STATUS emStatus = GX_STATUS_SUCCESS;
            GX_OPEN_PARAM openParam;
            uint32_t nDeviceNum = 0;
            openParam.accessMode = GX_ACCESS_EXCLUSIVE;
            openParam.openMode = GX_OPEN_INDEX;
            openParam.pszContent = new char[2]{'1', '\0'};
            // 初始化库
            emStatus = GXInitLib();
            if (emStatus != GX_STATUS_SUCCESS) {
                return -1;
            }
            // 枚举设备列表
            emStatus = GXUpdateDeviceList(&nDeviceNum, 1000);
            if ((emStatus != GX_STATUS_SUCCESS) || (nDeviceNum <= 0)) {
                return -1;
            }
            //打开设备
            emStatus = GXOpenDevice(&openParam, &m_hDevice);
            //设置采集模式连续采集
            emStatus = GXSetEnum(m_hDevice, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
            emStatus = GXSetInt(m_hDevice, GX_INT_ACQUISITION_SPEED_LEVEL, 1);
            emStatus = GXSetEnum(m_hDevice, GX_ENUM_BALANCE_WHITE_AUTO, GX_BALANCE_WHITE_AUTO_CONTINUOUS);
            //设置自动曝光
            emStatus = GXSetEnum(m_hDevice, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_OFF);    //关闭自动曝光
            emStatus = GXSetFloat(m_hDevice, GX_FLOAT_EXPOSURE_TIME, 2000.0000); //初始曝光时间
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
                std::cout << "Unable to open camera" << std::endl;
                return -1;
            }
            //判断相机是否支持bayer格式
            bool m_bColorFilter;
            emStatus = GXIsImplemented(m_hDevice, GX_ENUM_PIXEL_COLOR_FILTER, &m_bColorFilter);
            if (m_bColorFilter) {
                emStatus = GXGetEnum(m_hDevice, GX_ENUM_PIXEL_COLOR_FILTER, &m_nPixelColorFilter);
            }
            m_pBufferRGB = new BYTE[(size_t) (m_nImageWidth * m_nImageHeight * 3)];
            if (m_pBufferRGB == nullptr) {
                return -1;
            }
            //为存储原始图像数据申请空间
            m_pBufferRaw = new BYTE[(size_t) m_nPayLoadSize];
            if (m_pBufferRaw == nullptr) {
                delete[] m_pBufferRGB;
                m_pBufferRGB = nullptr;
                std::cout << "Unable to request memory space" << std::endl;
                return -1;
            }
            //注册图像处理回调函数
            emStatus = GXRegisterCaptureCallback(m_hDevice, nullptr, OnFrameCallbackFun);
            //发送开采命令
            emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_START);


            std::cout << "Camera turned on successfully" << std::endl;

            return 1;
        }

        void stopCamera() {

            //发送停采命令
            GX_STATUS emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_STOP);
            //注销采集回调
            emStatus = GXUnregisterCaptureCallback(m_hDevice);
            if (m_pBufferRGB != nullptr) {
                delete[] m_pBufferRGB;
                m_pBufferRGB = nullptr;
            }
            if (m_pBufferRaw != nullptr) {
                delete[] m_pBufferRaw;
                m_pBufferRaw = nullptr;
            }
            emStatus = GXCloseDevice(m_hDevice);
            emStatus = GXCloseLib();
        }

        int getHeight() {
            return m_nImageHeight;
        }

        int getWidth() {
            return m_nImageWidth;
        }

        cv::Mat getSrc() {
            return cv::Mat();
        }
    }


} // ifr