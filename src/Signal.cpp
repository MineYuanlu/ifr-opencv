//
// Created by yuanlu on 2022/9/7.
//

#include "Signal.h"
#include "Camera.h"

#include <thread>
#include <mutex>
#include <iostream>

#ifdef _WIN64
#include <Windows.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef linux

#include <unistd.h>

#endif

using namespace std;

namespace ifr {
    namespace Signal {
        //看门狗变量
        bool DogFeeded = true;    //是否喂狗
        mutex Dog_lock;            //线程锁
        thread *dog_thread_p;    //线程指针

        //看门狗线程回调函数
        void dog_callback(double timed_out) {
            auto usecond_time = useconds_t(int(timed_out * 1000000.0));
            bool temp_bool = true;

            while (temp_bool) {
                Dog_lock.lock();
                DogFeeded = false;
                Dog_lock.unlock();
                usleep(usecond_time);
                Dog_lock.lock();
                temp_bool = DogFeeded;
                Dog_lock.unlock();
            }

            ifr::Camera::stopCamera();
            exit(-1);
        }

        /**
        * @brief 启动看门狗线程
        * @param timed_out 喂狗超时时间
        */
        void open_dog_thread(double timed_out) {
            DogFeeded = true;
            dog_thread_p = new thread(dog_callback, timed_out);
            while (!dog_thread_p->joinable());
            dog_thread_p->detach();
        }

        /**
        * @brief 喂狗
        */
        void feed_dog() {
            Dog_lock.lock();
            DogFeeded = true;
            Dog_lock.unlock();
        }
    };
} // ifr
