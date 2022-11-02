//
// Created by yuanlu on 2022/9/21.
//

#include "AimEM.h"


#if USE_FORECAST

#include "predict_needed/headers/predict_model.hpp"

#endif

namespace EM {


    datas::OutInfo AimEM::handle(const datas::TargetInfo info) {
        if (useForecast) {
            static PredictModelDynFit fit;
            fit.get_data(info.emCenter, info.nowTargetAim.center, info.nowTarget.center,
                         info.time / 1000.0);
            fit.predict((cv::getTickCount() - info.receiveTick) / cv::getTickFrequency());
            target = fit.get_predict_position();
        } else {
            target = info.nowTargetAim.center;
        }
//            std::cout << id << " " << target.x << ", " << target.y << std::endl;
        size = info.size;
        static cv::Point2f zero(0, 0);
        go = cv::Point(size.width / 2 + aimOffset.offset_x, size.height / 2 + aimOffset.offset_y);
        velocity = info.findTarget ? go - target : zero;
#if DEBUG_IMG
        if (target.x > 0 && target.y > 0 && go.x > 0 && go.y > 0 && size.area() > 0)
            ifr::ImgDisplay::setDisplay(WINNAME_AIM_IMG, [&info, this]() -> cv::Mat {
                cv::Mat mat(size, CV_8UC3, cv::Scalar(0, 0, 0));
                cv::line(mat, target, go, cv::Scalar(100, 0, 255), 5);
                circle(mat, target, 10, cv::Scalar(255, 55, 0), -1);  //画点
                circle(mat, go, 10, cv::Scalar(0, 155, 255), -1);  //画点
                if (useForecast) {
                    if (info.nowTargetAim.center.x > 0 && info.nowTargetAim.center.y > 0) {
                        cv::line(mat, info.nowTargetAim.center, go, cv::Scalar(255, 0, 100), 5);
                        circle(mat, info.nowTargetAim.center, 10, cv::Scalar(0, 155, 255), -1);
                    }
                }
                return mat;
            });
#endif

        return {info.findTarget ? 2 : 0, info.activeCount, velocity, info.receiveTick};
    }


    AimEM::AimEM(bool useForecast, AimOffset offset) : useForecast(useForecast), aimOffset(offset) {
#if DEBUG_IMG
        cv::namedWindow(WINNAME_AIM_IMG, cv::WINDOW_NORMAL);
#endif
    }
} // EM