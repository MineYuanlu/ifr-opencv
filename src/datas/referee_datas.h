//
// Created by yuanlu on 2022/12/9.
//

#ifndef IFR_OPENCV_REFEREE_DATAS_H
#define IFR_OPENCV_REFEREE_DATAS_H

namespace RefereeSystem {
    namespace Package {
#pragma pack(1)
#define RefereeSystem_ID(id) static const constexpr uint8_t ID=id;

        /**
         * @brief 比赛状态数据，1Hz 周期发送
         * @details 0x0001
         */
        struct GameStatus {
            RefereeSystem_ID(0x0001)
            uint8_t data1{};//0-3 bit 比赛类型 ; 4-7 bit 当前比赛阶段
            uint16_t stage_remain_time{};//当前阶段剩余时间，单位 s
            uint64_t SyncTimeStamp{};// 机器人接收到该指令的精确 Unix 时间，当机载端收到有效的 NTP 服务器授时后生效

            /**
             * @brief 比赛类型
             * @details 1：RoboMaster 机甲大师赛；
             * @details 2：RoboMaster 机甲大师单项赛；
             * @details 3：ICRA RoboMaster 人工智能挑战赛
             * @details 4：RoboMaster 联盟赛 3V3
             * @details 5：RoboMaster 联盟赛 1V1
             * */
            uint8_t game_type() const { return data1 & 0xf; }

            /**
             * @brief 当前比赛阶段
             * @details 0：未开始比赛；
             * @details 1：准备阶段；
             * @details 2：自检阶段；
             * @details 3：5s 倒计时；
             * @details 4：对战中；
             * @details 5：比赛结算中
             * */
            uint8_t game_progress() const { return (data1 >> 4) & 0xf; }
        };

        /**
         * @brief 比赛结果数据，比赛结束后发送
         * @details 0x0002
         */
        struct GameResult {
            RefereeSystem_ID(0x0002)
            uint8_t winner{};//0 平局 1 红方胜利 2 蓝方胜利
        };

        /**
         * @brief 比赛机器人血量数据，1Hz 周期发送
         * @details 0x0003
         */
        struct GameRobotHP {
            RefereeSystem_ID(0x0003)
            uint16_t red_1_robot_HP{};//红 1 英雄机器人血量，未上场以及罚下血量为 0
            uint16_t red_2_robot_HP{};//红 2 工程机器人血量
            uint16_t red_3_robot_HP{};//红 3 步兵机器人血量
            uint16_t red_4_robot_HP{};//红 4 步兵机器人血量
            uint16_t red_5_robot_HP{};//红 5 步兵机器人血量
            uint16_t red_7_robot_HP{};//红 7 哨兵机器人血量
            uint16_t red_outpost_HP{};//红方前哨战血量
            uint16_t red_base_HP{};//红方基地血量
            uint16_t blue_1_robot_HP{};//蓝 1 英雄机器人血量，未上场以及罚下血量为 0
            uint16_t blue_2_robot_HP{};//蓝 2 工程机器人血量
            uint16_t blue_3_robot_HP{};//蓝 3 步兵机器人血量
            uint16_t blue_4_robot_HP{};//蓝 4 步兵机器人血量
            uint16_t blue_5_robot_HP{};//蓝 5 步兵机器人血量
            uint16_t blue_7_robot_HP{};//蓝 7 哨兵机器人血量
            uint16_t blue_outpost_HP{};//蓝方前哨战血量
            uint16_t blue_base_HP{};//蓝方基地血量
        };
#undef RefereeSystem_ID
#pragma pack()
    }

    FORCE_INLINE void read() {

    }

}
#endif //IFR_OPENCV_REFEREE_DATAS_H
