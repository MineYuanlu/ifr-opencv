//
// Created by yuanlu on 2022/9/21.
//

#include "defs.h"
#include "AimEM.h"
#include"ext_funcs.h"
#include "ImgDisplay.h"

#if DEBUG_AIM && (__OS__ == __OS_Linux__)

#include <unistd.h>

#endif

#if USE_FORECAST

#include "predict_needed/headers/predict_model.hpp"

#endif

namespace EM {
    namespace AimEM {

        cv::Point2f target;//目标位置
        cv::Size size;//画幅大小
        cv::Point2f go;//瞄准位置
        cv::Point2f velocity;//移动矢量
        int offset_x, offset_y;//瞄准偏移



        datas::OutInfo handle(const uint64_t id, const datas::TargetInfo info) {
#if USE_FORECAST
            static PredictModelDynFit fit;
            fit.get_data(/*TODO 能量机关中心*/info.nowTarget.center, info.nowTargetAim.center, info.nowTarget.center,
                                              info.time / 1000.0);
            fit.predict((cv::getTickCount() - info.receiveTick) / cv::getTickFrequency());
            target = fit.get_predict_position();
#else
            target = info.nowTargetAim.center;
#endif
//            std::cout << id << " " << target.x << ", " << target.y << std::endl;
            size = info.size;
            static cv::Point2f zero(0, 0);
            go = cv::Point(size.width / 2 + offset_x, size.height / 2 + offset_y);
            velocity = info.findTarget ? go - target : zero;

#if DEBUG_IMG
            if (target.x > 0 && target.y > 0 && go.x > 0 && go.y > 0 && size.area() > 0)
                ifr::ImgDisplay::setDisplay(WINNAME_AIM_IMG, [info]() -> cv::Mat {
                    cv::Mat mat(size, CV_8UC3, cv::Scalar(0, 0, 0));
                    cv::line(mat, target, go, cv::Scalar(100, 0, 255), 5);
                    circle(mat, target, 10, cv::Scalar(255, 55, 0), -1);  //画点
                    circle(mat, go, 10, cv::Scalar(0, 155, 255), -1);  //画点
#if USE_FORECAST
                    cv::line(mat, info.nowTargetAim.center, go, cv::Scalar(255, 0, 100), 5);
                    circle(mat, info.nowTargetAim.center, 10, cv::Scalar(0, 155, 255), -1);
#endif
                    return mat;
                });
#endif

            return {info.findTarget ? 2 : 0, info.activeCount, velocity, info.receiveTick};
        }


        void saveValue() {
            auto file = fopen(AIM_VALUE_FILE, "w");
            if (file == nullptr) {
                std::cout << "Can not write file: " << AIM_VALUE_FILE << std::endl;
                exit(-1);
            }
            fprintf(file, "%d %d", offset_x, offset_y);
            fclose(file);
            file = nullptr;
        }


        void readValue() {
            auto file = fopen(AIM_VALUE_FILE, "r");
            if (file == nullptr) {
                offset_x = offset_y = 0;
                saveValue();
            } else {
                fscanf(file, "%d %d", &offset_x, &offset_y);
                fclose(file);
                file = nullptr;
            }
        }


        void init() {
            readValue();
#if DEBUG_IMG
            cv::namedWindow(WINNAME_AIM_IMG, cv::WINDOW_NORMAL);
#endif
        }
    }
} // EM