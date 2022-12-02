#include "defs.h"
#include <iostream>
#include <thread>
#include "tasks/AimEM.h"
#include "tasks/OutputMotor.h"
#include "tasks/FinderEM.h"
#include "tasks/Camera.h"
#include "tasks/AimArmor.h"
#include "tasks/FinderArmor.h"
#include "tasks/Video.h"
#include "api/API.h"
#include "argparse.hpp"

using namespace std;
using namespace cv;

int main(int argc, char const *argv[]) {
    string log_file, err_file, plan, listen;
    bool no_api = false, exit_on_reset = false;
    if (argc > 1) {
        auto args = util::args::argparser("IFR OPENCV");
        args = args.set_program_name("IFR_OPENCV")
                .add_help_option()
                .add_option("-n", "--no-api", "Close api")
                .add_option<std::string>("-l", "--listen", "Specify the url of API listening", "0.0.0.0:8000")
                .add_option<std::string>("-p", "--plan", "Specify the plan to run", "")
                .add_option("-e", "--exit-on-reset", "Exit the program when the process is reset")
                .add_option<std::string>("", "--file-log", "Log output file", "")
                .add_option<std::string>("", "--file-err", "Err output file", "")
                .add_sc_option("-h", "--help-show", "show this help message", [&args]() {
                    args.print_help();
                })
                .parse(argc, argv);

        log_file = args.get_option<std::string>("--file-log");
        err_file = args.get_option<std::string>("--file-err");
        plan = args.get_option<std::string>("-p");
        listen = args.get_option<std::string>("-l");
        no_api = args.get_option<bool>("-n");
        exit_on_reset = args.get_option<bool>("-e");
    }
    if (!log_file.empty()) freopen(log_file.c_str(), "w", stdout);
    if (!err_file.empty()) freopen(err_file.c_str(), "w", stderr);
    if (exit_on_reset) ifr::Plans::setExitOnReset(true);


    cout << __FILE__ << " compile time: " << __DATE__ << " " << __TIME__ << endl;
    IFR_LOC_LOGGER(RAPIDJSON_HAS_CXX17);
    IFR_LOC_LOGGER(DEBUG);
    IFR_LOC_LOGGER(USE_GPU);

#if __OS__ == __OS_Linux__
    system("pwd");
#elif __OS__ == __OS_Windows__
    system("chdir");
#else
    UNSUPPORT !!!
    cout<<"未知系统: "<<__OS__<<endl;
#endif
    std::cout << std::endl;

    ifr::Plans::init();
    ifr::Camera::registerTask();
    ifr::Video::registerTask();
    EM::Finder::registerTask();
    EM::AimEM::registerTask();
    Motor::Output::registerTask();
    Armor::FinderArmor::registerTask();
    Armor::AimArmor::registerTask();

    Armor::Values::init();

    if (!plan.empty()) {
        ifr::logger::log("Main", "Plan to run has been specified", plan);
        ifr::Plans::usePlanInfo(plan);
        ifr::Plans::startPlan();
    }

    if (!no_api) {
        if (!listen.empty()) ifr::API::init(listen);
        else ifr::API::init();
    }

    auto delay = SLEEP_TIME(1);
    while (1)SLEEP(delay);
}

