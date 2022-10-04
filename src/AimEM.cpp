//
// Created by yuanlu on 2022/9/21.
//

#include "AimEM.h"
#include"ext_funcs.h"
#include "ImgDisplay.h"

#if DEBUG_AIM && (__OS__ == __OS_Linux__)

#include <unistd.h>

#endif

namespace EM {

    datas::OutInfo AimEM::handle(const uint64_t id, const datas::TargetInfo info) {
#if USE_FORECAST
#else
        target = info.nowTargetAim.center;
//        std::cout << id << " " << target.x << ", " << target.y << std::endl;
        size = info.size;
#endif
        static cv::Point2f zero(0, 0);
        go = cv::Point(size.width / 2 + offset_x, size.height / 2 + offset_y);
        velocity = info.findTarget ? go - target : zero;

#if DEBUG_IMG
        if (target.x > 0 && target.y > 0 && go.x > 0 && go.y > 0 && size.area() > 0)
            ifr::ImgDisplay::setDisplay(WINNAME_AIM_IMG, [this]() -> cv::Mat {
                cv::Mat mat(size, CV_8UC3, cv::Scalar(0, 0, 0));
                cv::line(mat, target, go, cv::Scalar(100, 0, 255), 5);
                circle(mat, target, 10, cv::Scalar(255, 55, 0), -1);  //画点，其实就是实心圆
                circle(mat, go, 10, cv::Scalar(0, 155, 255), -1);  //画点，其实就是实心圆
                return mat;
            });
#endif

        return {info.findTarget ? 2 : 0, info.activeCount, velocity, info.delay, info.receiveTick};
    }


    void AimEM::saveValue() {
        auto file = fopen(AIM_VALUE_FILE, "w");
        if (file == nullptr) {
            std::cout << "Can not write file: " << AIM_VALUE_FILE << std::endl;
            exit(-1);
        }
        fprintf(file, "%d %d", offset_x, offset_y);
        fclose(file);
        file = nullptr;
    }


    void AimEM::readValue() {
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


    AimEM::AimEM() {
        readValue();
#if DEBUG_IMG
        cv::namedWindow(WINNAME_AIM_IMG, cv::WINDOW_NORMAL);
#endif
    }
} // EM