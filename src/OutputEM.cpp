//
// Created by yuanlu on 2022/9/7.
//

#include "OutputEM.h"

namespace EM {

    namespace Output {
#if DATA_OUT_PORT
        serial_port_t device;
        uint8_t port_data_out[17] = {0x5b, 2, 0};//串口输出
#endif

#if DATA_OUT_PORT

        void output(const datas::OutInfo &info) {
            port_data_out[1] = info.targetType;
            *(short *) (port_data_out + 2) = (short) (-info.velocity.x);
            *(short *) (port_data_out + 4) = (short) (info.velocity.y);
            *(uint8_t *) (port_data_out + 6) = (uint8_t) info.delay;
            short sum_check =
                    short(-info.velocity.x) + short(info.velocity.y) +
                    short(port_data_out[1]) +
                    short(port_data_out[6]);
            *(short *) (port_data_out + 15) = sum_check;
            std::swap(port_data_out[2], port_data_out[3]);
            std::swap(port_data_out[4], port_data_out[5]);
            std::swap(port_data_out[15], port_data_out[16]);
            std::cout << "target: " << info.targetType << ", vector: " << (-info.velocity.x) << " " << (info.velocity.y)
                      << ", active: " << info.activeCount << ", delay: " << info.delay << std::endl;
            serial_port_write(device, port_data_out, 17);
        }

#elif DATA_OUT_PRINT

        void output(const datas::OutInfo &info) {
            std::cout << info.velocity << " " << info.activeCount << " " << info.delay << std::endl;
        }

#endif

        void open() {
#if DATA_OUT_PORT
            device = open_serial_port(port);
            SIMPLE_INSPECT(IOflush_serial_port(device, FLUSH_I | FLUSH_O));
            serial_port_attr_t attr;
            SIMPLE_INSPECT(require_serial_port_attr(device, &attr));
            SIMPLE_INSPECT(standardize_serial_port_attr(&attr));
            SIMPLE_INSPECT(set_serial_port_attr_baud_rate(&attr, BAUD_RATE(115200)));
            SIMPLE_INSPECT(set_serial_port_attr_data_bits(&attr, OPT_DATA_BITS(8)));
            SIMPLE_INSPECT(set_serial_port_attr_parity(&attr, OPT_PARITY_NONE));
            SIMPLE_INSPECT(set_serial_port_attr_stop_bits(&attr, OPT_STOP_BITS(1)));
#if __OS__ == __OS_Windows__
            SIMPLE_INSPECT(set_serial_port_attr_read_settings(&attr, 0, 0, 0));
#else
            SIMPLE_INSPECT(set_serial_port_attr_read_settings(&attr, 1, 1, 0));
#endif
            SIMPLE_INSPECT(apply_serial_port_attr(device, &attr, TCSANOW));
            std::cout << "Serial port turned on successfully: " << device << " - " << std::string(port) << std::endl;
#endif
        }

    };

}  // ifr