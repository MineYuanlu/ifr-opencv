#include <iostream>
#include <thread>
#include "defs.h"
#include "Record.h"
#include "tasks/AimEM.h"
#include "tasks/OutputEM.h"
#include "tasks/FinderEM.h"
#include "tasks/Camera.h"
#include "tasks/Video.h"
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
    ifr::Video::registerTask();

    ifr::API::init();

    auto delay = SLEEP_TIME(1);
    while (1)SLEEP(delay);
//    start();

    return -1;
}

ifr::DataWaiter<uint64_t, datas::TargetInfo> dw;

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
