//
// Created by yuanlu on 2022/9/7.
//

#include "OutputEM.h"
#include <stdio.h>

namespace EM {

#if __OS__ == __OS_Windows__
    static char port[] = "COM5";
#elif __OS__ == __OS_Linux__
    static char port[] = "/dev/ttyTHS2";
#endif


    void Output::outputSerialPort(const datas::OutInfo &info) {
        int64 delay = (cv::getTickCount() - info.receiveTick) / cv::getTickFrequency() * 1000;
        port_data_out[1] = info.targetType;
        *(short *) (port_data_out + 2) = (short) (-info.velocity.x);
        *(short *) (port_data_out + 4) = (short) (info.velocity.y);
        *(uint8_t *) (port_data_out + 6) = (uint8_t) delay;
        short sum_check =
                short(-info.velocity.x) + short(info.velocity.y) +
                short(port_data_out[1]) +
                short(port_data_out[6]);
        *(short *) (port_data_out + 15) = sum_check;
        std::swap(port_data_out[2], port_data_out[3]);
        std::swap(port_data_out[4], port_data_out[5]);
        std::swap(port_data_out[15], port_data_out[16]);
        serial_port_write(device, port_data_out, 17);
    }


    void Output::outputConsole(const datas::OutInfo &info) {
        long long int delay = (cv::getTickCount() - info.receiveTick) / cv::getTickFrequency() * 1000;
        printf("T: %d,vec: [%9.3f, %9.3f], a: %d, delay: %lld\n", info.targetType, -info.velocity.x,
               info.velocity.y,
               info.activeCount, delay);
    }


    void Output::open() {
        if (device)return;
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
    }

    void Output::close() {
        close_serial_port(device);
        device = 0;
    }


}  // ifr