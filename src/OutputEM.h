//
// Created by yuanlu on 2022/9/7.
//

#ifndef IFR_OPENCV_OUTPUTEM_H
#define IFR_OPENCV_OUTPUTEM_H

#include "defs.h"
#include "serial_port.h"
#include "ext_types.h"


namespace EM {

    namespace Output {
#if __OS__ == __OS_Windows__
        static char port[] = "COM5";
#elif __OS__ == __OS_Linux__
        static char port[] = "/dev/ttyTHS2";
#endif

        void open();

        void output(const datas::OutInfo &info);
    };

} // ifr

#endif //IFR_OPENCV_OUTPUTEM_H
