//
// Created by yuanlu on 2022/12/9.
//

#ifndef IFR_OPENCV_TASK_REFEREEDATA_H
#define IFR_OPENCV_TASK_REFEREEDATA_H

#include <utility>
#include "../datas/referee_datas.h"
#include "CRC.h"
#include "plan/Plans.h"
#include "logger/logger.hpp"
#include "msg/msg.hpp"
#include "../defs.h"
#include "serial_port.h"

#ifndef  OUTPUT_DEFAULT_PORT
#if __OS__ == __OS_Windows__
#define OUTPUT_DEFAULT_PORT "COM5"
#elif __OS__ == __OS_Linux__
#define OUTPUT_DEFAULT_PORT "/dev/ttyTHS2"
#endif
#endif
namespace ifr {
    class RefereeData {
    private:
#pragma pack(1)
        struct FrameHeader {
            static const constexpr uint8_t HEAD = 0xA5;
            uint8_t SOF;//数据帧起始字节，固定值为 0xA5
            uint16_t data_length;//数据帧中 data 的长度
            uint8_t seq;//包序号
            uint8_t CRC8;//帧头 CRC8 校验
        };
#pragma pack()

        serial_port_t device{};//设备
        const std::string port;//串口
        char *port_a;//串口 (会内存泄漏, 但懒得管)


        /**打开串口*/
        void open() {
            if (device)return;
            device = open_serial_port(port_a);
            IFR_LOGGER("RefereeData", IOflush_serial_port(device, FLUSH_I | FLUSH_O));
            serial_port_attr_t attr;
            IFR_LOGGER("RefereeData", require_serial_port_attr(device, &attr));
            IFR_LOGGER("RefereeData", standardize_serial_port_attr(&attr));
            IFR_LOGGER("RefereeData", set_serial_port_attr_baud_rate(&attr, BAUD_RATE(115200)));
            IFR_LOGGER("RefereeData", set_serial_port_attr_data_bits(&attr, OPT_DATA_BITS(8)));
            IFR_LOGGER("RefereeData", set_serial_port_attr_parity(&attr, OPT_PARITY_NONE));
            IFR_LOGGER("RefereeData", set_serial_port_attr_stop_bits(&attr, OPT_STOP_BITS(1)));
#if __OS__ == __OS_Windows__
            IFR_LOGGER("RefereeData", set_serial_port_attr_read_settings(&attr, 0, 0, 0));
#else
            IFR_LOGGER("RefereeData", set_serial_port_attr_read_settings(&attr, 1, 1, 0));
#endif
            IFR_LOGGER("RefereeData", apply_serial_port_attr(device, &attr, TCSANOW));
            if (is_device_valid(device))
                ifr::logger::log("RefereeData", "Serial port turned on successfully(" + port + ")", device);
            else throw std::runtime_error("[RefereeData] Can not open serial port: " + port);
        }

        /**关闭串口*/
        void close() {
            close_serial_port(device);
            device = 0;
        }

        explicit RefereeData(std::string port) : port(std::move(port)) {
            port_a = new char[this->port.length() + 1];
            for (size_t i = 0; i < this->port.length(); i++)port_a[i] = this->port[i];
            port_a[this->port.length()] = '\0';

            open();
        }

        ~RefereeData() {
            delete[]port_a;
            close();
        }

        /**
         * @brief 读取数据
         * @param device 串口
         * @param dst 缓冲区
         * @param size 字节数
         * @return 读取是否成功(简化返回值)
         */
        FORCE_INLINE bool sp_read(const serial_port_t &device, uint8_t *const dst, const size_t &size) {
            return serial_port_read(device, dst, size) == size;
        }

        /**
         * @brief 读取一个结构体
         * @tparam T 结构体类型
         * @param device 串口
         * @param[out] dst 结构体引用
         * @return 读取是否成功(简化返回值)
         */
        template<class T>
        FORCE_INLINE bool sp_readT(const serial_port_t &device, T &dst) {
            return sp_read(device, (uint8_t *) &dst, sizeof(T));
        }

        /**
         * @brief 读取一个结构体
         * @tparam T 结构体类型
         * @param device 串口
         * @param[out] dst 结构体引用
         * @param[in] offset 偏移的字节数
         * @return 读取是否成功(简化返回值)
         */
        template<class T>
        FORCE_INLINE bool sp_readT(const serial_port_t &device, T &dst, const size_t &offset) {
            return sp_read(device, (uint8_t *) &dst + offset, sizeof(T) - offset);
        }

        /**
         * @brief 对齐数据
         * @details 自动在数据中寻找包头 (仅按值寻找, 可能不是包头, 请在对齐后进行crc校验)
         * @param[in,out] frameHeader 帧头
         * @param[in,out] cmd_id 命令ID
         * @return false: 串口读取失败
         */
        inline bool alignHead(FrameHeader &frameHeader, uint16_t &cmd_id) const {
            static const constexpr auto sFH = sizeof(frameHeader);//sizeof FrameHeader
            static const constexpr auto sCI = sizeof(cmd_id);//sizeof cmd id
            size_t mov;
            uint8_t *chunk;

            //尝试在FH内寻找包头
            chunk = (uint8_t *) &frameHeader;
            for (mov = 1; mov < sFH; mov++)
                if (chunk[mov] == FrameHeader::HEAD)break;
            if (mov < sFH) {//在FrameHeader内发现帧头
                memmove(&frameHeader, &frameHeader + mov, sFH - mov);//FH数据前移
                memcpy(&frameHeader + sFH - mov, &cmd_id, std::min(mov, sCI));//cmdid数据前移

                if (mov > sCI)//补齐FH
                    if (!sp_readT(device, frameHeader, sFH + sCI - mov))return false;

                if (mov >= sCI) {//读入cmdid (完整)
                    if (!sp_readT(device, cmd_id))return false;
                } else {//读入cmdid (部分)
                    memmove(&cmd_id, &cmd_id + mov, sCI - mov);
                    if (!sp_readT(device, cmd_id, mov))return false;
                }

                return true;
            }

            //尝试在CI内寻找包头
            chunk = (uint8_t *) &cmd_id;
            for (mov = 0; mov < sCI; mov++)
                if (chunk[mov] == FrameHeader::HEAD)break;
            if (mov < sCI) {//在cmdid内发现帧头
                static_assert(sFH >= sCI, "sizeof(FrameHeader) must more than sizeof(uint16_t)");
                memcpy(&frameHeader, &cmd_id, sCI - mov);//cmdid前移
                if (!sp_readT(device, frameHeader, sCI - mov))return false;
                if (!sp_readT(device, cmd_id))return false;
                return true;
            }

            //循环读入直到包头
            frameHeader.SOF = FrameHeader::HEAD - 1;
            while (sp_readT(device, frameHeader.SOF) && frameHeader.SOF != FrameHeader::HEAD);
            if (frameHeader.SOF != FrameHeader::HEAD)return false;

            return sp_readT(device, frameHeader, sizeof(FrameHeader::SOF)) &&
                   sp_readT(device, cmd_id);
        }

        /**
         * @brief 读取帧头, 会自动对齐帧头
         * @param[out] frameHeader 帧头
         * @param[out] cmd_id 命令id
         * @return false: 串口读取失败 / CRC校验失败
         */
        bool readHead(FrameHeader &frameHeader, uint16_t &cmd_id) const {
            if (FrameHeader::HEAD != frameHeader.SOF && cmd_id != uint16_t(-1)) [[unlikely]] {//上一次的数据未对齐
                if (!alignHead(frameHeader, cmd_id))return false;
            } else {//普通读取
                if (!sp_readT(device, frameHeader))return false;
                if (!sp_readT(device, cmd_id))return false;
            }

            if (FrameHeader::HEAD != frameHeader.SOF) [[unlikely]] {
                if (!alignHead(frameHeader, cmd_id)) return false;
            }

            static auto table = CRC::CRC_8().MakeTable();
            auto crc = CRC::Calculate(&frameHeader, sizeof(frameHeader) - sizeof(frameHeader.CRC8), table);
            if (crc != frameHeader.CRC8) [[unlikely]] {
                return false;
            }
            return true;
        }

        /**
         * @brief 读取帧内容
         * @tparam T 数据类型
         * @param[in] frameHeader 帧头数据
         * @param[in] cmd_id cmd id
         * @param[out] ok是否校验通过
         * @return 帧数据
         */
        template<class T>
        T readBody(const FrameHeader &frameHeader, const uint16_t &cmd_id, bool &ok) {
            if (frameHeader.data_length != sizeof(T)) [[unlikely]] {
                throw invalid_argument("[Referee Data] bad data size: " +
                                       to_string(frameHeader.data_length) + " <-> " + to_string(sizeof(T)));
            }
            T t;
            if (!sp_readT(device, t)) {
                ok = false;
                return t;
            };


            uint16_t tail;
            if (!sp_readT(device, tail)) {
                ok = false;
                return t;
            };
            static auto table = CRC::CRC_16_ARC().MakeTable();
            auto crc = CRC::Calculate(&frameHeader, 1 + 2 + 1, table);
            ok = crc == tail;

            return t;
        }

        /**跳过指定长度的内容*/
        void skip(size_t size) {
            static auto *const dst = new uint8_t[256];
            sp_read(device, dst, size);
        }

        /**
         * 跳过帧内容(及帧尾)
         * @param[in] frameHeader 帧头数据
         */
        void skipBody(const FrameHeader &frameHeader) {
            skip(frameHeader.data_length + 2);
        }

        inline bool handleStuI(const FrameHeader &frameHeader, const uint16_t &cmd_id, bool &crc_pass,
                               RefereeSystem::Package::StuInteractiveData &data) {
            using namespace RefereeSystem::Package;
            if (!sp_readT(device, data.head))return false;
            switch (data.head.data_cmd_id) {
#define IFR_REFEREEDATA_STUI_SW(type) \
                case StuInteractiveDataBody::type::ID: {\
                    auto sub_body = make_shared<StuInteractiveDataBody::type>();\
                    if (!sp_readT<StuInteractiveDataBody::type>(device, *sub_body))return false;\
                    data.body = sub_body;\
                    return true;\
                }
                IFR_REFEREEDATA_STUI_SW(Custom_SentryStatus)
                IFR_REFEREEDATA_STUI_SW(Delete)
                IFR_REFEREEDATA_STUI_SW(Draw1)
                IFR_REFEREEDATA_STUI_SW(Draw2)
                IFR_REFEREEDATA_STUI_SW(Draw5)
                IFR_REFEREEDATA_STUI_SW(Draw7)
                IFR_REFEREEDATA_STUI_SW(DrawChar)
#undef IFR_REFEREEDATA_STUI_SW
                default:skip(frameHeader.data_length + 2 - sizeof(StuInteractiveData::head));
                    return false;
            }
        }

    public:

        static void registerTask() {
            static const std::string arg_port = "port";
            ifr::Plans::TaskDescription description{"input", "裁判系统串口输入"};

#define IFR_REFEREEDATA_ALL_PACKAGE \
IFR_REFEREEDATA(GameStatus)\
IFR_REFEREEDATA(GameResult)\
IFR_REFEREEDATA(GameRobotHP)\
//所有的裁判系统数据包

//定义接口
#define IFR_REFEREEDATA(type) static const std::string io_##type = #type ; \
            description.io[io_##type] = {TYPE_NAME(RefereeSystem::Package::type), "输出数据: " #type, false};
            IFR_REFEREEDATA_ALL_PACKAGE
            static const std::string io_StuInteractiveData = "StuInteractiveData";
            description.io[io_StuInteractiveData] = {TYPE_NAME(RefereeSystem::Package::StuInteractiveData),
                                                     "输出数据: StuInteractiveData", false};
#undef IFR_REFEREEDATA

            description.args[arg_port] = {"串口", OUTPUT_DEFAULT_PORT, ifr::Plans::TaskArgType::STR};

            ifr::Plans::registerTask("Referee Data", description, [](auto io, auto args, auto state, auto cb) {
                ifr::Plans::Tools::waitState(state, 1);

                RefereeData rd(args[arg_port]);

                //注册发布者
#define IFR_REFEREEDATA(type) ifr::Msg::Publisher<RefereeSystem::Package::type> pub_##type(args[io_##type]);
                IFR_REFEREEDATA_ALL_PACKAGE
                ifr::Msg::Publisher<RefereeSystem::Package::StuInteractiveData>
                        pub_StuInteractiveData(args[io_StuInteractiveData]);
#undef IFR_REFEREEDATA
                ifr::Plans::Tools::finishAndWait(cb, state, 1);
                //锁定发布者
#define IFR_REFEREEDATA(type) pub_##type.lock(false);
                IFR_REFEREEDATA_ALL_PACKAGE
                pub_StuInteractiveData.lock(false);
#undef IFR_REFEREEDATA
                cb(2);

                FrameHeader frameHeader{};//帧头
                uint16_t cmd_id = -1;//命令ID  -1 代表第一次读取
                bool crc_pass;//crc校验是否通过
                while (*state < 3) {
                    try {
                        if (rd.readHead(frameHeader, cmd_id))
                            switch (cmd_id) {
                                //处理数据包
#define IFR_REFEREEDATA(type) case RefereeSystem::Package::type::ID:{            \
if (!pub_##type.hasSubscriber()) {rd.skipBody(frameHeader);break;}               \
auto body=rd.readBody<RefereeSystem::Package::type>(frameHeader,cmd_id,crc_pass);\
if (crc_pass) pub_##type.push(body);                                             \
break;                                                                           \
}
                                IFR_REFEREEDATA_ALL_PACKAGE

                                case RefereeSystem::Package::StuInteractiveDataHead::ID: {
                                    if (!pub_StuInteractiveData.hasSubscriber()) {
                                        rd.skipBody(frameHeader);
                                        break;
                                    }
                                    RefereeSystem::Package::StuInteractiveData data;
                                    if (rd.handleStuI(frameHeader, cmd_id, crc_pass, data))
                                        pub_StuInteractiveData.push(data);
                                    break;
                                }
#undef IFR_REFEREEDATA
                                default:rd.skipBody(frameHeader);
                                    break;
                            };
                        cmd_id = 0;//重置cmd id
                    } catch (ifr::Msg::MessageError_Broke &) { break; }
                }
                ifr::Plans::Tools::waitState(state, 3);
                ifr::Plans::Tools::finishAndWait(cb, state, 3);
                //auto release && cb(4)
            });
#undef IFR_REFEREEDATA_ALL_PACKAGE
        }
    };

}

#endif //IFR_OPENCV_TASK_REFEREEDATA_H
