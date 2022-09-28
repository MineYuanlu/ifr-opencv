//
// Created by yuanlu on 2022/9/21.
//

#include "AimEM.h"

namespace EM {

    datas::OutInfo AimEM::handle(const uint64_t id, const datas::TargetInfo info) {
#if USE_FORECAST
#else
        target = info.nowTargetAim.center;
//        std::cout << id << " " << target.x << ", " << target.y << std::endl;
        size = info.size;
#endif
        go = cv::Point(size.width / 2 + offset_x, size.height / 2 + offset_y);
        velocity = go - target;

        return {info.findTarget ? 2 : 0, info.activeCount, velocity, info.delay};
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
#if DEBUG_AIM
//        cv::namedWindow(WINNAME_AIM_VAL, cv::WINDOW_NORMAL);
        cv::namedWindow(WINNAME_AIM_IMG, cv::WINDOW_NORMAL);
//        cv::resizeWindow(WINNAME_AIM_VAL, 200, 200);
        std::thread([this] {
            while (true) {
                if (target.x <= 0 || target.y <= 0 || go.x <= 0 || go.y <= 0 || size.area() <= 0)continue;
                const auto size0 = size.area() > 0 ? size : cv::Size(1920, 1200);
                cv::Mat mat(size0, CV_8UC3, cv::Scalar(0, 0, 0));
                cv::line(mat, target, go, cv::Scalar(100, 0, 255), 5);
                circle(mat, target, 10, cv::Scalar(255, 55, 0), -1);  //画点，其实就是实心圆
                circle(mat, go, 10, cv::Scalar(0, 155, 255), -1);  //画点，其实就是实心圆
                cv::imshow(WINNAME_AIM_IMG, mat);
                cv::waitKey(1);
            }
        }).detach();
//        std::thread([this] {
////            readValue();
//            cv::Mat mat(200, 200, CV_8UC3, cv::Scalar(0, 0, 0));
//            cv::putText(mat, std::to_string(offset_x) + ", " + std::to_string(offset_y), cv::Point(0, 50),
//                        cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0, 255, 0));
//            cv::imshow(WINNAME_AIM_VAL, mat);
//            cv::waitKey(500);
//        }).detach();
#endif
    }
} // EM