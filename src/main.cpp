#include <iostream>
#include <thread>
#include "defs.h"
#include "tasks/AimEM.h"
#include "tasks/OutputEM.h"
#include "tasks/FinderEM.h"
#include "tasks/Camera.h"
#include "tasks/AimArmor.h"
#include "tasks/FinderArmor.h"
#include "tasks/Video.h"
#include "api/API.h"

using namespace std;
using namespace cv;

int main() {
//    freopen("ifr-opencv.log", "w", stdout);
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
    Armor::FinderArmor::registerTask();
    Armor::AimArmor::registerTask();

    ifr::API::init();

    auto delay = SLEEP_TIME(1);
    while (1)SLEEP(delay);
//    start();

    return -1;
}

