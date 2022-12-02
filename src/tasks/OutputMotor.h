//
// Created by yuanlu on 2022/9/7.
//

#ifndef IFR_OPENCV_OUTPUTMOTOR_H
#define IFR_OPENCV_OUTPUTMOTOR_H

#include <utility>

#include "../defs.h"
#include "serial_port.h"
#include "plan/Plans.h"
#include "msg/msg.hpp"

#if __OS__ == __OS_Windows__
#define OUTPUT_DEFAULT_PORT "COM5"
#elif __OS__ == __OS_Linux__
#define OUTPUT_DEFAULT_PORT "/dev/ttyTHS2"
#endif
namespace Motor {

    class Output {
    private:
        const bool isSerial;
        serial_port_t device{};//设备(仅在串口模式下有用)
        uint8_t port_data_out[17] = {0x5b, 2, 0};//串口输出(仅在串口模式下有用)
        const std::string port;//串口(仅在串口模式下有用)
        char *port_a;//串口(仅在串口模式下有用)

        const float align_delay;//延时对齐
    public:
        explicit Output(bool isSerial, float align_delay, std::string port = "")
                : isSerial(isSerial), port(std::move(port)), align_delay(align_delay) {
            port_a = new char[this->port.length() + 1];
            for (size_t i = 0; i < this->port.length(); i++)port_a[i] = this->port[i];
            port_a[this->port.length()] = '\0';

            if (isSerial)open();
        };

        Output(const Output &obj) = delete;

        Output(Output &&obj) = delete;

        ~Output() {
            if (isSerial)close();
            delete[] port_a;
        }

        void open();

        void close();

        void outputSerialPort(const datas::OutInfo &info);

        void outputConsole(const datas::OutInfo &info) const;

        static void registerTask() {
            static const std::string io_output = "output";
            static const std::string arg_port = "port";
            static const std::string arg_align_delay = "align delay";
            {
                ifr::Plans::TaskDescription description{"output", "通过串口输出电机数据"};
                description.io[io_output] = {TYPE_NAME(datas::OutInfo), "输出数据", true};
                description.args[arg_port] = {"串口", OUTPUT_DEFAULT_PORT, ifr::Plans::TaskArgType::STR};
                description.args[arg_align_delay] = {"延时对齐(ms)", "5.0", ifr::Plans::TaskArgType::NUMBER};

                ifr::Plans::registerTask("Motor SPort", description, [](auto io, auto args, auto state, auto cb) {
                    ifr::Plans::Tools::waitState(state, 1);

                    Output output(true, std::stof(args[arg_align_delay]), args[arg_port]);
                    ifr::Msg::Subscriber<datas::OutInfo> goIn(io[io_output].channel);
                    ifr::Plans::Tools::finishAndWait(cb, state, 1);
                    cb(2);
                    while (*state < 3) {
                        try {
                            output.outputSerialPort(goIn.pop_for(COMMON_LOOP_WAIT));
                        } catch (ifr::Msg::MessageError_NoMsg &x) {
                            ifr::logger::err("serial port", "输出数据等待超时",
                                             std::to_string(COMMON_LOOP_WAIT) + " ms");
                        } catch (ifr::Msg::MessageError_Broke &) {
                            break;
                        }
                    }
                    ifr::Plans::Tools::waitState(state, 3);
                    ifr::Plans::Tools::finishAndWait(cb, state, 3);
                    //auto release && cb(4)
                });
            }
            {
                ifr::Plans::TaskDescription description{"output", "通过控制台打印电机数据"};
                description.io[io_output] = {TYPE_NAME(datas::OutInfo), "输出数据", true};
                ifr::Plans::registerTask("Motor Print", description, [](auto io, auto args, auto state, auto cb) {
                    ifr::Plans::Tools::waitState(state, 1);

                    Output output(false, std::stof(args[arg_align_delay]));
                    ifr::Msg::Subscriber<datas::OutInfo> goIn(io[io_output].channel);
                    ifr::Plans::Tools::finishAndWait(cb, state, 1);
                    cb(2);
                    while (*state < 3) {
                        try {
                            output.outputConsole(goIn.pop_for(COMMON_LOOP_WAIT));
                        } catch (ifr::Msg::MessageError_NoMsg &x) {
                            ifr::logger::err("console print", "输出数据等待超时",
                                             std::to_string(COMMON_LOOP_WAIT) + " ms");
                        } catch (ifr::Msg::MessageError_Broke &) {
                            break;
                        }
                    }
                    ifr::Plans::Tools::waitState(state, 3);
                    ifr::Plans::Tools::finishAndWait(cb, state, 3);
                    //auto release && cb(4)
                });
            }
        }
    };

} // ifr

#endif //IFR_OPENCV_OUTPUTMOTOR_H
