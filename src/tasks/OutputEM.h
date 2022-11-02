//
// Created by yuanlu on 2022/9/7.
//

#ifndef IFR_OPENCV_OUTPUTEM_H
#define IFR_OPENCV_OUTPUTEM_H

#include "../defs.h"
#include "serial_port.h"
#include "ext_types.h"
#include "../Plans.h"


namespace EM {

    class Output {
    private:
        const bool isSerial;
        serial_port_t device{};
        uint8_t port_data_out[17] = {0x5b, 2, 0};//串口输出
    public:
        explicit Output(bool isSerial) : isSerial(isSerial) {
            if (isSerial)open();
        };

        Output(const Output &obj) = delete;

        Output(Output &&obj) = delete;

        ~Output() {
            if (isSerial)close();
        }

        void open();

        void close();

        void outputSerialPort(const datas::OutInfo &info);

        void outputConsole(const datas::OutInfo &info);

        static void registerTask() {
            static const std::string io_output = "output";
            {
                ifr::Plans::TaskDescription description{"output", "通过串口输出数据"};
                description.io[io_output] = {TYPE_NAME(datas::OutInfo), "输出数据", true};

                ifr::Plans::registerTask("Serial PortEM", description, [](auto io, auto args, auto state, auto cb) {
                    ifr::Plans::Tools::waitState(state, 1);

                    Output output(true);
                    umt::Subscriber<datas::OutInfo> goIn(io[io_output].channel);
                    ifr::Plans::Tools::finishAndWait(cb, state, 1);
                    cb(2);
                    while (*state < 3) {
                        try {
                            output.outputSerialPort(goIn.pop_for(COMMON_LOOP_WAIT));
                        } catch (umt::MessageError_Timeout &x) {
                            OUTPUT("[serial port] 输出数据等待超时 " + std::to_string(COMMON_LOOP_WAIT) + "ms")
                        }
                    }
                    ifr::Plans::Tools::finishAndWait(cb, state, 3);
                    //auto release && cb(4)
                });
            }
            {
                ifr::Plans::TaskDescription description{"output", "通过控制台打印数据"};
                description.io[io_output] = {TYPE_NAME(datas::OutInfo), "输出数据", true};
                ifr::Plans::registerTask("Console PrintEM", description, [](auto io, auto args, auto state, auto cb) {
                    ifr::Plans::Tools::waitState(state, 1);

                    Output output(false);
                    umt::Subscriber<datas::OutInfo> goIn(io[io_output].channel);
                    ifr::Plans::Tools::finishAndWait(cb, state, 1);
                    cb(2);
                    while (*state < 3) {
                        try {
                            output.outputConsole(goIn.pop_for(COMMON_LOOP_WAIT));
                        } catch (umt::MessageError_Timeout &x) {
                            OUTPUT("[console print] 输出数据等待超时 500ms")
                        }
                    }
                    ifr::Plans::Tools::finishAndWait(cb, state, 3);
                    //auto release && cb(4)
                });
            }
        }
    };

} // ifr

#endif //IFR_OPENCV_OUTPUTEM_H
