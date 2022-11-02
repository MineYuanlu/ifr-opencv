//
// Created by yuanlu on 2022/10/4.
//

#include "ImgDisplay.h"

namespace ifr {
#if DEBUG_IMG
    namespace ImgDisplay {
        std::mutex state_mtx;
        map<std::string, std::function<cv::Mat()>> datas;

        int init() {
            thread t = thread([]() {
                OUTPUT("画面显示线程 已运行")
                while (true) {
                    state_mtx.lock();
                    cout << "啊???" << endl;
                    for (const auto &d: datas) {
                        waitKey(30);
                        imshow(d.first, d.second());
                        waitKey(30);
                    }
                    cout << "啊???1" << endl;
                    datas.clear();
                    state_mtx.unlock();
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
            state_mtx.lock();
            datas[name] = getter;
            state_mtx.unlock();
        }
    }
#endif
} // ifr