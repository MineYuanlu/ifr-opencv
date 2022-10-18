//
// Created by yuanlu on 2022/9/7.
//

#include "Record.h"
#include <thread>

namespace ifr {
    namespace Record {

        cv::VideoWriter writer;
        std::thread t;

        std::string getFile(const std::string &dir) {
            char now[64];
            std::time_t tt;
            struct tm *ttime;
            tt = time(nullptr);
            ttime = localtime(&tt);
            strftime(now, 64, "%Y-%m-%d_%H_%M_%S", ttime);  // 以时间为名字
            std::string now_string(now);
            std::string path(std::string(dir + now_string).append(".avi"));
            return path;
        }

        void recorder() {
            umt::Subscriber<uint64> sub(MSG_CAMERA);
            while (true) {
                try {
                    sub.pop();
//                    writer.write(Camera::getSrc());
                }
                catch (umt::MessageError &e) {
                    std::cerr << "[WARN] Can not get " << MSG_CAMERA << ": " << e.what() << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        }

        void startRecord(const std::string &dir) {
//            const auto file = getFile(dir);
//            writer = cv::VideoWriter(file, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 20.0,
//                                     cv::Size(Camera::getWidth(), Camera::getHeight()));
//            if (!writer.isOpened()) {
//                std::cerr << "Can't open [record] file: " << file << std::endl;
//                return;
//            }
//            t = std::thread(recorder);
//            t.detach();
        }

        void stopRecord() {
            writer.release();
        }

        void join() {
            t.join();
        }
    }
} // ifr