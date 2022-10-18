#include <iostream>
#include "umt/umt.hpp"
#include <thread>
#include "defs.h"
#include "Record.h"
#include "DataWaiter.h"
#include "tasks/AimEM.h"
#include "tasks/OutputEM.h"
#include "tasks/FinderEM.h"
#include "tasks/Camera.h"
#include "API.h"

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
    cout << "程序编译时间: " << __DATE__ << " " << __TIME__ << endl;
#if __OS__ == __OS_Linux__
    system("pwd");
#elif __OS__ == __OS_Windows__
    system("chdir");
#else
    UNSUPPORT !!!
    cout<<"未知系统: "<<__OS__<<endl;
#endif
    ifr::Plans::init();
    EM::Finder::registerTask();
    EM::AimEM::registerTask();
    ifr::Camera::registerTask();
    EM::Output::registerTask();

    ifr::API::init();

    auto delay = SLEEP_TIME(1);
    while (1)SLEEP(delay);
//    start();

    return -1;
}

ifr::DataWaiter<uint64_t, datas::TargetInfo> dw;

void start() {
    OUTPUT("开始启动")

//    thread MainProgressThread = thread([]() {//识别 - 预测 - 发送
//        umt::Publisher<datas::OutInfo> goOut(MSG_OUTPUT);//发布者
//        EM::AimEM::init();
//        while (true) {
//            const auto data = dw.pop();
//            const auto out = EM::AimEM::handle(data.first, data.second);
//            goOut.push(out);
//        }
//    });
//
//    while (!MainProgressThread.joinable());
//    //MainProgressThread.detach();
//
//
//    thread *VisionThreads;
//    VisionThreads = new thread[THREAD_IDENTITY];
//
//    for (int i = 1; i <= THREAD_IDENTITY; i++) {
//        cout << "启动识别线程 " << i << endl;
//        VisionThreads[i - 1] = thread([i]() {
//            umt::Subscriber<datas::FrameData> fdIn(MSG_CAMERA);
//            EM::Finder finder(i);
//#if DEBUG_TIME
//            umt::Publisher<TimeInfo> timeOut(MSG_DEBUG_TIME);//发布者
//#endif
//            while (true) {
//                const auto data = fdIn.pop();
//                dw.start(data.id);
//#if DEBUG_TIME
//                map<string, double> times;
//#endif
//                auto ti = finder.run(data.mat
//#if DEBUG_TIME
//                        , times
//#endif
//                );
//#if DEBUG_TIME
//                timeOut.push({times, data.id});
//#endif
//                ti.time = data.time;
//                ti.receiveTick = data.receiveTick;
//                dw.finish(data.id, ti);
//            }
//        });
//
//        while (!VisionThreads[i - 1].joinable());
//        //VisionThreads[i - 1].detach();
//    }
//
//    ifr::API::init();

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
