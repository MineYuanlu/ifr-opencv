#include <iostream>
#include "umt/umt.hpp"
#include <thread>
#include "defs.h"
#include "Camera.h"
#include "Record.h"
#include "FinderEM.h"
#include "DataWaiter.h"
#include "AimEM.h"
#include "OutputEM.h"
#include "Signal.h"

using namespace std;
using namespace cv;

#if DEBUG_TIME
void writeTime();

struct TimeInfo {
    map<string, double> data;
    uint64_t id;
};
#endif

void start();

int main() {
    system("pwd");

    start();

    return -1;
}

void start() {
    OUTPUT("开始启动")
    ifr::Signal::open_dog_thread(4.0);
#if DATA_IN_CAMERA
    while (ifr::Camera::runCamera() < 0) {
        ifr::Camera::stopCamera();
        cout << "Open camera faile.\n";
    }
#endif
#if DATA_IN_VIDEO
    //TODO
#endif
    ifr::DataWaiter<uint64_t, datas::TargetInfo> dw;

    thread MainProgressThread = thread([&dw]() {//识别 - 预测 - 发送
        umt::Publisher<datas::OutInfo> goOut(MSG_OUTPUT);//发布者
        while (true) {
            const auto data = dw.pop();
            const auto out = EM::aimEm.handle(data.first, data.second);
            goOut.push(out);
        }
    });

    while (!MainProgressThread.joinable());
    //MainProgressThread.detach();

    thread DataOutputThread = thread([] {//输出
        EM::Output::open();
        umt::Subscriber<datas::OutInfo> goIn(MSG_OUTPUT);
        while (true)EM::Output::output(goIn.pop());
    });

    while (!DataOutputThread.joinable());
    //DataOutputThread.detach();

    thread *VisionThreads;
    VisionThreads = new thread[THREAD_IDENTITY];

    for (int i = 1; i <= THREAD_IDENTITY; i++) {
        cout << "启动识别线程 " << i << endl;
        VisionThreads[i - 1] = thread([&dw, i]() {
            umt::Subscriber<datas::FrameData> fdIn(MSG_CAMERA);
            EM::Finder finder(i);
#if DEBUG_TIME
            umt::Publisher<TimeInfo> timeOut(MSG_DEBUG_TIME);//发布者
#endif
            while (true) {
                const auto data = fdIn.pop();
                dw.start(data.id);
#if DEBUG_TIME
                map<string, double> times;
#endif
                auto ti = finder.run(data.mat
#if DEBUG_TIME
                        , times
#endif
                );
#if DEBUG_TIME
                timeOut.push({times, data.id});
#endif
                ti.time = data.time;
                ti.receiveTick = data.receiveTick;
                dw.finish(data.id, ti);
            }
        });

        while (!VisionThreads[i - 1].joinable());
        //VisionThreads[i - 1].detach();
    }

    while (true);
}

#if DEBUG_TIME

void writeTime() {
    umt::Subscriber<TimeInfo> sub(MSG_DEBUG_TIME);
    auto fp = fopen("times.csv", "w");
    fprintf(fp, "id,type,time\n");
    while (true) {
        try {
            const auto data = sub.pop();
            for (const auto &e: data.data) {
                fprintf(fp, "%lu,%s,%f\n", data.id, e.first.c_str(), e.second);
            }
            if (data.id > 10000)break;
        } catch (...) {
            break;
        }
    }
    fclose(fp);
}

#endif