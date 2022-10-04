//
// Created by yuanlu on 2022/10/4.
//

#include "ImgDisplay.h"

namespace ifr {
#if DEBUG_IMG
    namespace ImgDisplay {
        std::mutex mtx;
        map<std::string, std::function<cv::Mat()>> datas;

        int init() {
            thread t = thread([]() {
                OUTPUT("画面显示线程 已运行")
                while (true) {
                    mtx.lock();
                    for (const auto &d: datas) {
                        imshow(d.first, d.second());
                    }
                    datas.clear();
                    mtx.unlock();
                    waitKey(30);
                }
            });
            while (!t.joinable());
            t.detach();
            OUTPUT("成功启动 画面显示进程")
            return 1;
        }

        void setDisplay(const std::string &name, const std::function<cv::Mat()> &getter) {
            static auto _ = init();
            mtx.lock();
            datas[name] = getter;
            mtx.unlock();
        }
    }
#endif
} // ifr