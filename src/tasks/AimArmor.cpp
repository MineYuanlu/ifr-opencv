//
// Created by yuanlu on 2022/11/17.
//

#include "AimArmor.h"

namespace Armor {
    datas::OutInfo AimArmor::handle(const datas::ArmTargetInfos info) {
        static const cv::Point2f zero(0, 0);
        //TODO useForecast

        if (info.targets.empty())return {0, 0, zero, info.receiveTick};
        else {
            target = info.targets[0].target.center;
            go = cv::Point(size.width / 2, size.height / 2);
            velocity = go - target;
            return {2, static_cast<int>(info.targets.size()), velocity, info.receiveTick};
        }
    }
} // Armor