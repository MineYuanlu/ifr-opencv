//
// Created by yuanlu on 2022/9/7.
//

#ifndef IFR_OPENCV_SIGNAL_H
#define IFR_OPENCV_SIGNAL_H

namespace ifr {

    namespace Signal {

        /**
        * @brief 启动看门狗线程
        * @param timed_out 喂狗超时时间
        */
        void open_dog_thread(double timed_out = 5.0);

        /**
        * @brief 喂狗
        */
        void feed_dog();

    };

} // ifr

#endif //IFR_OPENCV_SIGNAL_H
