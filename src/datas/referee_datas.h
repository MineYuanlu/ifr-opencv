//
// Created by yuanlu on 2022/12/9.
//

#ifndef IFR_OPENCV_REFEREE_DATAS_H
#define IFR_OPENCV_REFEREE_DATAS_H

namespace RefereeSystem {
    namespace Package {
#pragma pack(1)
#define RefereeSystem_BASIC(id, interval_, maxInterval_) static const constexpr uint16_t ID=id; \
static const constexpr int64_t INTERVAL=interval_; \
static const constexpr int64_t MAX_INTERVAL=maxInterval_;\
        //包ID ; 目标间隔时间(ms) ; 最大间隔时间(ms) ; 间隔时间: -1 = 不定,  -2 = 仅发送一次

        /**
         * @brief 比赛状态数据，1Hz 周期发送
         * @details 命令码: 0x0001
         * @details 数据段长度: 11
         */
        struct GameStatus {
            RefereeSystem_BASIC(0x0001, 1000, 2000)
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
        * @details 命令码: 0x0002
        * @details 数据段长度: 1
        */
        struct GameResult {
            RefereeSystem_BASIC(0x0002, -2, 0)
            uint8_t winner{};//0 平局 1 红方胜利 2 蓝方胜利
        };

        /**
        * @brief 比赛机器人血量数据，1Hz 周期发送
        * @details 命令码: 0x0003
        * @details 数据段长度: 28
        */
        struct GameRobotHP {
            RefereeSystem_BASIC(0x0003, 1000, 2000)
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

            /**
                @brief 通过机器人ID获取对应的血量
                @details 1：红方英雄机器人；
                @details 2：红方工程机器人；
                @details 3/4/5：红方步兵机器人；
                @details 6：红方空中机器人；
                @details 7：红方哨兵机器人；
                @details 8：红方飞镖机器人；
                @details 9：红方雷达站；
                @details 101：蓝方英雄机器人；
                @details 102：蓝方工程机器人；
                @details 103/104/105：蓝方步兵机器人；
                @details 106：蓝方空中机器人；
                @details 107：蓝方哨兵机器人；
                @details 108：蓝方飞镖机器人；
                @details 109：蓝方雷达站。
                @return -1: 无血量
             */
            inline uint16_t getByRobotId(const uint8_t &id) const {
                switch (id) {
                    case 1:return red_1_robot_HP;
                    case 2:return red_2_robot_HP;
                    case 3:return red_3_robot_HP;
                    case 4:return red_4_robot_HP;
                    case 5:return red_5_robot_HP;
                    case 7:return red_7_robot_HP;
                    case 101:return blue_1_robot_HP;
                    case 102:return blue_2_robot_HP;
                    case 103:return blue_3_robot_HP;
                    case 104:return blue_4_robot_HP;
                    case 105:return blue_5_robot_HP;
                    case 107:return blue_7_robot_HP;
                    case 6:
                    case 8:
                    case 9:
                    case 106:
                    case 108:
                    case 109:return -1;
                }
            }
        };

        /**
         * @brief 人工智能挑战赛加成与惩罚状态，1Hz 周期发送
         * @details 命令码: 0x0005
         * @details 数据段长度: 11
         */
        struct ICRA_BDZL_status {
            RefereeSystem_BASIC(0x0005, 1000, 2000)
            uint16_t data1{};//bit[0, 4, 8, 12, 16, 20]为 F1-F6 激活状态
            uint8_t data2{};//bit[1-3, 5-7, 9-11, 13-15, 17-19, 21-23]为 F1-F1 的状态信息
            uint16_t red1_bullet_left{};//红方 1 号剩余弹量
            uint16_t red2_bullet_left{};//红方 2 号剩余弹量
            uint16_t blue1_bullet_left{};//蓝方 1 号剩余弹量
            uint16_t blue2_bullet_left{};//蓝方 2 号剩余弹量
            uint8_t lurk_mode{};//阶段
            uint8_t res{};//保留字节

            /**
             * 判断 F1-F6 激活状态
             * @param id F1~F6 -> 1~6
             * @return false = 未激活 / true = 可激活
             */
            template<uint8_t id>
            inline bool isActive() const {
                static constexpr const uint8_t mask = 1;
                static_assert(1 <= id && id <= 6, "ID must be between 1 and 6 (inclusive)");
                if ((id - 1) < 4)return data1 & (1 << ((id - 1) * 4));
                else return data2 & (1 << (((id - 1) - 4) * 4));
            }

            /**
             * 获取 F1-F6 状态信息
             * @param id F1~F6 -> 1~6
             * @return 1bit = 激活状态 ; 3bit = 状态信息
             */
            template<uint8_t id>
            inline uint8_t getStatus() const {
                static constexpr const uint8_t mask = -1;
                static_assert(1 <= id && id <= 6, "ID must be between 1 and 6 (inclusive)");
                if ((id - 1) < 4)return (data1 >> ((id - 1) * 4)) & mask;
                else return (data2 >> (((id - 1) - 4) * 4)) & mask;
            }
        };

        /**
         * @brief 场地事件数据，1Hz 周期发送
         * @details 命令码: 0x0101
         * @details 数据段长度: 4
         */
        struct EventData {
            RefereeSystem_BASIC(0x0101, 1000, 2000)
            uint32_t data{};

            /**
             * 己方补给站 1~3 号补血点占领状态
             * @tparam id 1~3号 -> 1~3
             * @return true = 已占领
             */
            template<uint8_t id>
            inline bool isOccupy() const {
                // bit 0~2
                static_assert(1 <= id && id <= 3, "ID must be between 1 and 3 (inclusive)");
                return data & (1 << (id - 1));
            }

            /**@return 打击点占领状态; true = 占领*/
            inline bool isEMOccupy() const { return data & (1 << 3); }

            /**@return 小能量机关激活状态; true = 激活*/
            inline bool isEMsmActive() const { return data & (1 << 4); }

            /**@return 大能量机关激活状态; true = 激活*/
            inline bool isEMlgActive() const { return data & (1 << 5); }

            /**@return 己方侧 R2/B2 环形高地占领状态; true = 占领*/
            inline bool isR2B2Occupy() const { return data & (1 << 6); }

            /**@return 己方侧 R3/B3 梯形高地占领状态; true = 占领*/
            inline bool isR3B3Occupy() const { return data & (1 << 7); }

            /**@return 己方侧 R4/B4 梯形高地占领状态; true = 占领*/
            inline bool isR4B4Occupy() const { return data & (1 << 8); }

            /**@return 己方基地护盾状态; true = 基地有虚拟护盾血量*/
            inline bool hasVirtualShield() const { return data & (1 << 9); }

            /**@return 己方前哨战状态; true = 前哨战存活*/
            inline bool isOutpostBattleSurvival() const { return data & (1 << 10); }

        };

        /**
         * @brief 场地补给站动作标识数据，动作改变后发送
         * @details 命令码: 0x0102
         * @details 数据段长度: 3
         */
        struct SupplyProjectileAction {
            RefereeSystem_BASIC(0x0102, -1, 0)
            uint8_t projectile_id{};//补给站口 ID
            uint8_t robot_id{};//补弹机器人 ID：0 为当前无机器人补弹，1 为红方英雄机器人补弹，2 为红方工程机 器人补弹，3/4/5 为红方步兵机器人补弹，101 为蓝方英雄机器人补弹，102 为蓝方工程机器人补弹，103/104/105 为蓝方步兵机器人补弹
            uint8_t step{};//出弹口开闭状态：0 为关闭，1 为子弹准备中，2 为子弹下落
            uint8_t num{};//补弹数量 50 / 100 /150 /20
        };

        /**
         * @brief 裁判警告数据，警告发生后发送
         * @details 命令码: 0x0104
         * @details 数据段长度: 2
         */
        struct RefereeWarning {
            RefereeSystem_BASIC(0x0104, -1, 0)
            uint8_t level{}; // 1 = 黄牌 ; 2 = 红牌 ; 3 = 判负
            uint8_t foul_robot_id{};// 犯规机器人 ID ; 判负时为 0
        };

        /**
         * @brief 飞镖发射口倒计时，1Hz 周期发送
         * @details 命令码: 0x0105
         * @details 数据段长度: 1
         */
        struct DartRemaining {
            RefereeSystem_BASIC(0x0105, 1000, 2000)
            uint8_t dart_remaining_time{};//15s倒计时
        };

        /**
         * @brief 机器人状态数据，10Hz 周期发送
         * @details 命令码: 0x0201
         * @details 数据段长度: 15
         */
        struct GameRobotStatus {
            RefereeSystem_BASIC(0x0201, 1000 / 10, 1000 / 10 * 2)
            /**
                本机器人 ID：
                @details 1：红方英雄机器人；
                @details 2：红方工程机器人；
                @details 3/4/5：红方步兵机器人；
                @details 6：红方空中机器人；
                @details 7：红方哨兵机器人；
                @details 8：红方飞镖机器人；
                @details 9：红方雷达站；
                @details 101：蓝方英雄机器人；
                @details 102：蓝方工程机器人；
                @details 103/104/105：蓝方步兵机器人；
                @details 106：蓝方空中机器人；
                @details 107：蓝方哨兵机器人；
                @details 108：蓝方飞镖机器人；
                @details 109：蓝方雷达站。
             */
            uint8_t robot_id{};
            uint8_t robot_level{};//机器人等级： 1：一级；2：二级；3：三级。
            uint16_t remain_HP{};//机器人剩余血量
            uint16_t max_HP{};//机器人上限血量
            uint16_t shooter_id1_17mm_cooling_rate{};//机器人 1 号 17mm 枪口每秒冷却值
            uint16_t shooter_id1_17mm_cooling_limit{};//机器人 1 号 17mm 枪口热量上限
            uint16_t shooter_id1_17mm_speed_limit{};//机器人 1 号 17mm 枪口上限速度 单位 m/s
            uint16_t shooter_id2_17mm_cooling_rate{};//机器人 2 号 17mm 枪口每秒冷却值
            uint16_t shooter_id2_17mm_cooling_limit{};//机器人 2 号 17mm 枪口热量上限
            uint16_t shooter_id2_17mm_speed_limit{};//机器人 2 号 17mm 枪口上限速度 单位 m/s
            uint16_t shooter_id1_42mm_cooling_rate{};//机器人 42mm 枪口每秒冷却值
            uint16_t shooter_id1_42mm_cooling_limit{};//机器人 42mm 枪口热量上限
            uint16_t shooter_id1_42mm_speed_limit{};//机器人 42mm 枪口上限速度 单位 m/s
            uint16_t chassis_power_limit{};//机器人底盘功率限制上限
            uint8_t mains_power_output{};//主控电源输出情况

            //gimbal 口输出：1 为有 24V 输出，0 为无 24v 输出；
            inline bool mains_power_gimbal_output() const { return mains_power_output & (1 << 0); }

            //chassis 口输出：1 为有 24V 输出，0 为无 24v 输出；
            inline bool mains_power_chassis_output() const { return mains_power_output & (1 << 1); }

            //shooter 口输出：1 为有 24V 输出，0 为无 24v 输出；
            inline bool mains_power_shooter_output() const { return mains_power_output & (1 << 2); }
        };

        /**
         * @brief 实时功率热量数据，50Hz 周期发送
         * @details 命令码: 0x0202
         * @details 数据段长度: 16
         */
        struct PowerHeat {
            RefereeSystem_BASIC(0x0202, 1000 / 50, 1000 / 50 * 2)
            uint16_t chassis_volt{};//底盘输出电压 单位 毫伏
            uint16_t chassis_current{};//底盘输出电流 单位 毫安
            float chassis_power{};//底盘输出功率 单位 W 瓦
            uint16_t chassis_power_buffer{};//底盘功率缓冲 单位 J 焦耳 备注：飞坡根据规则增加至 250J
            uint16_t shooter_id1_17mm_cooling_heat{};//1 号 17mm 枪口热量
            uint16_t shooter_id2_17mm_cooling_heat{};//2 号 17mm 枪口热量
            uint16_t shooter_id1_42mm_cooling_heat{};//42mm 枪口热量
        };

        /**
         * @brief 机器人位置数据，10Hz 发送
         * @details 命令码: 0x0203
         * @details 数据段长度: 16
         */
        struct RobotPos {
            RefereeSystem_BASIC(0x0203, 1000 / 10, 1000 / 10 * 2)
            float x;//位置 x 坐标，单位 m
            float y;//位置 y 坐标，单位 m
            float z;//位置 z 坐标，单位 m
            float yaw;//位置枪口，单位度
        };

        /**
         * @brief 机器人增益数据，增益状态改变后发送
         * @details 命令码: 0x0204
         * @details 数据段长度: 1
         */
        struct Buff {
            RefereeSystem_BASIC(0x0204, -1, 0)
            uint8_t power_rune_buff{};

            //机器人血量补血状态
            inline bool getRobotRecovery() const { return power_rune_buff & (1 << 0); }

            //枪口热量冷却加速
            inline bool getCoolingSpeed() const { return power_rune_buff & (1 << 1); }

            //机器人防御加成
            inline bool getDefenseAdd() const { return power_rune_buff & (1 << 2); }

            //机器人攻击加成
            inline bool getAttackAdd() const { return power_rune_buff & (1 << 3); }
        };


        /**
         * @brief 空中机器人能量状态数据，10Hz 周期发送，只有空中机器人主控发送
         * @details 命令码: 0x0205
         * @details 数据段长度: 3
         */
        struct AerialRobotEnergy {
            RefereeSystem_BASIC(0x0205, 1000 / 10, 1000 / 10 * 2)
            uint8_t attack_time{};//可攻击时间 单位 s。30s 递减至 0
        };

        /**
         * @brief 伤害状态数据，伤害发生后发送
         * @details 命令码: 0x0206
         * @details 数据段长度: 1
         */
        struct RobotHurt {
            RefereeSystem_BASIC(0x0206, -1, 0)
            uint8_t data{};

            //当血量变化类型为装甲伤害，代表装甲 ID，其中数值为 0-4 号代表机器人的五个装甲片，其他血量变化类型，该变量数值为 0。
            inline uint8_t getArmorId() const { return data & 0xf; }

            /**
             * @brief 获取血量变化类型
             * @details 0x0 装甲伤害扣血；
             * @details 0x1 模块掉线扣血；
             * @details 0x2 超射速扣血；
             * @details 0x3 超枪口热量扣血；
             * @details 0x4 超底盘功率扣血；
             * @details 0x5 装甲撞击扣血
             */
            inline bool getHurtType() const { return (data >> 4) & 0xf; }
        };

        /**
         * @brief 实时射击数据，子弹发射后发送
         * @details 命令码: 0x0207
         * @details 数据段长度: 6
         */
        struct ShootData {
            RefereeSystem_BASIC(0x0207, -1, 0)
            uint8_t bullet_type{};//子弹类型: 1：17mm 弹丸 2：42mm 弹丸
            uint8_t shooter_id{};//发射机构 ID： 1：1 号 17mm 发射机构 2：2 号 17mm 发射机构 3：42mm 发射机构
            uint8_t bullet_freq{};//子弹射频 单位 Hz
            float bullet_speed{};//子弹射速 单位 m/s
        };
        /**
         * @brief 子弹剩余发送数，空中机器人以及哨兵机器人发送，1Hz 周期发送
         * @details 命令码: 0x0208
         * @details 数据段长度: 2
         */
        struct BulletRemaining {
            RefereeSystem_BASIC(0x0208, 1000, 2000)
            uint16_t bullet_remaining_num_17mm{};//17mm 子弹剩余发射数目
            uint16_t bullet_remaining_num_42mm{};//42mm 子弹剩余发射数目
            uint16_t coin_remaining_num{};//剩余金币数量
        };

        /**
         * @brief 机器人 RFID 状态，1Hz 周期发送
         * @details 命令码: 0x0209
         * @details 数据段长度: 4
         */
        struct RfidStatus {
            RefereeSystem_BASIC(0x0209, 1000, 2000)
            uint32_t rfid_status{};

            /**
             * @details  0：基地增益点 RFID 状态；
             * @details  1：高地增益点 RFID 状态；
             * @details  2：能量机关激活点 RFID 状态；
             * @details  3：飞坡增益点 RFID 状态；
             * @details  4：前哨岗增益点 RFID 状态；
             * @details  6：补血点增益点 RFID 状态；
             * @details  7：工程机器人复活卡 RFID 状态；
             */
            template<uint8_t id>
            inline bool get() const {
                static_assert(0 <= id && id <= 7, "ID must be between 0 and 7 (inclusive)");
                return rfid_status & (1 << id);
            }
        };

        /**
         * @brief 飞镖机器人客户端指令书，10Hz 周期发送
         * @details 命令码: 0x020A
         * @details 数据段长度: 12
         */
        struct DartClientCmd {
            RefereeSystem_BASIC(0x020A, -1, 0)
            uint8_t dart_launch_opening_status{};//当前飞镖发射口的状态 1：关闭； 2：正在开启或者关闭中 0：已经开启
            uint8_t dart_attack_target{};//飞镖的打击目标，默认为前哨站； 0：前哨站； 1：基地。
            uint16_t target_change_time{};//切换打击目标时的比赛剩余时间，单位秒，从未切换默认为 0。
            uint16_t operate_launch_cmd_time{};//最近一次操作手确定发射指令时的比赛剩余时间，单位秒, 初始值为 0。
        };
        /**
         * @brief 机器人间交互数据，发送方触发发送，上限 10Hz
         * @details 命令码: 0x0301
         * @details 数据段长度: n
         */
        struct StuInteractiveDataHead {
            RefereeSystem_BASIC(0x0301, 1000 / 10, 1000 / 10 * 2)
            uint16_t data_cmd_id{};//0x0200~0x02FF 机器人间通信 ; 0x0100~0x0110 客户端通信
            /**
@机器人ID 1，英雄(红)；
@机器人ID 2，工程(红)；
@机器人ID 3/4/5，步兵(红)；
@机器人ID 6，空中(红)；
@机器人ID 7，哨兵(红)；
@机器人ID 9，雷达站（红）；
@机器人ID 101，英雄(蓝)；
@机器人ID 102，工程(蓝)；
@机器人ID 103/104/105，步兵(蓝)；
@机器人ID 106，空中(蓝)；
@机器人ID 107，哨兵(蓝)；
@机器人ID 109，雷达站（蓝）。

@客户端ID 0x0101 为英雄操作手客户端(红)；
@客户端ID 0x0102，工程操作手客户端((红)；
@客户端ID 0x0103/0x0104/0x0105，步兵操作手客户端(红)；
@客户端ID 0x0106，空中操作手客户端((红)；
@客户端ID 0x0165，英雄操作手客户端(蓝)；
@客户端ID 0x0166，工程操作手客户端(蓝)；
@客户端ID 0x0167/0x0168/0x0169，步兵操作手客户端步兵(蓝)；
@客户端ID 0x016A，空中操作手客户端(蓝)。
             */
            uint16_t sender_ID{};
            uint16_t receiver_ID{};


        };
        namespace StuInteractiveDataBody {
#define RefereeSystem_SDB_BASIC(id) static const constexpr uint16_t ID=id;

            //自定义数据包: (操作手或其他机器人指定的)哨兵状态
            struct Custom_SentryStatus {
                RefereeSystem_SDB_BASIC(0x0200)
                uint8_t type = 0;

                /**
                 * @brief
                 * @details 0: 自由模式
                 * @details 1: P (保护基地)
                 * @details 2: S (自保)
                 * @details 3: N (默认)
                 * @details 4: A (攻击)
                 */
                inline uint8_t getStatus() const { return type & 0x7; }

//                /**
//                 * 获取维持此状态的持续时间
//                 * @details 最大31s
//                 * @return 0 = 永久, 其他: 秒
//                 */
//                inline uint8_t getTime() const { return (type >> 3) & 0x1f; }
            };

            ///图形数据
            struct GraphicData {
                uint8_t graphic_name[3] = {};//
                uint32_t data1{};//图形配置 1
                uint32_t data2{};//图形配置 2
                uint32_t data3{};//图形配置 3

                /**
                    @图形操作 0：空操作
                    @图形操作 1：增加
                    @图形操作 2：修改
                    @图形操作 3：删除
                 */
                inline uint8_t getOperation() const { return data1 & 0x7; }

                /**
                    @图形类型 0：直线
                    @图形类型 1：矩形
                    @图形类型 2：整圆
                    @图形类型 3：椭圆
                    @图形类型 4：圆弧
                    @图形类型 5：浮点数
                    @图形类型 6：整型数
                    @图形类型 7：字符
                 */
                inline uint8_t getType() const { return (data1 >> 3) & 0x7; }

                ///图层数，0~9
                inline uint8_t getLayer() const { return (data1 >> 6) & 0xf; }

                /**
                    @颜色 0：红蓝主色
                    @颜色 1：黄色
                    @颜色 2：绿色
                    @颜色 3：橙色
                    @颜色 4：紫红色
                    @颜色 5：粉色
                    @颜色 6：青色
                    @颜色 7：黑色
                    @颜色 8：白色
                 */
                inline uint8_t getColor() const { return (data1 >> 10) & 0xf; }

                ///起始角度，单位：°，范围[0,360]；
                inline uint8_t getStartAngle() const { return (data1 >> 14) & 0x1ff; }

                ///终止角度，单位：°，范围[0,360]。
                inline uint8_t getEndAngle() const { return (data1 >> 23) & 0x1ff; }

                ///线宽
                inline uint8_t getLineWidth() const { return data2 & 0x3ff; }

                ///起点 x 坐标
                inline uint8_t getStartX() const { return (data2 >> 10) & 0x7ff; }

                ///起点 y 坐标
                inline uint8_t getStartY() const { return (data2 >> 21) & 0x7ff; }

                ///字体大小或者半径
                inline uint8_t getFontSize() const { return data3 & 0x3ff; }

                ///终点 x 坐标
                inline uint8_t getEndX() const { return (data3 >> 10) & 0x7ff; }

                ///终点 y 坐标
                inline uint8_t getEndY() const { return (data3 >> 21) & 0x7ff; }

            };

            ///客户端删除图形
            struct Delete {
                RefereeSystem_SDB_BASIC(0x0100)
                /**
                 * 图形操作
                 * @details 0: 空操作；
                 * @details 1: 删除图层；
                 * @details 2: 删除所有；
                 */
                uint8_t operate_tpye{};
                uint8_t layer{};//图层数：0~9
            };
            template<size_t n>
            struct DrawN {
                GraphicData graphic_data[n];
            };
            ///客户端绘制一个图形
            struct Draw1 {
                RefereeSystem_SDB_BASIC(0x0101)
                DrawN<1> data;
            };
            struct Draw2 {
                RefereeSystem_SDB_BASIC(0x0102)
                DrawN<2> data;
            };
            struct Draw5 {
                RefereeSystem_SDB_BASIC(0x0103)
                DrawN<5> data;
            };
            struct Draw7 {
                RefereeSystem_SDB_BASIC(0x0104)
                DrawN<7> data;
            };
            struct DrawChar {
                RefereeSystem_SDB_BASIC(0x0110)
                GraphicData graphic_data;
                uint8_t data[30];
            };
#undef RefereeSystem_SDB_BASIC
        }

        /**
         * @brief 机器人间交互数据，发送方触发发送
         * @details 头部数据和实际数据的整合
         */
        struct StuInteractiveData {
            StuInteractiveDataHead head;
            std::shared_ptr<void> body{};
        };
        /**
         * @brief 自定义控制器交互数据接口，通过客户端触发发送，上限 30Hz
         * @details 命令码: 0x0302
         * @details 数据段长度: n
         */
        template<size_t x>
        struct RobotInteractiveData {
            RefereeSystem_BASIC(0x0302, 1000 / 30, 1000 / 30 * 2)
            static_assert(0 <= x && x <= 30, "Bad size");
            uint8_t data[x];
        };
        /**
         * @brief 客户端小地图交互数据，触发发送
         * @details 命令码: 0x0303
         * @details 数据段长度: 15
         */
        struct RobotCommandSend {
            RefereeSystem_BASIC(0x0303, 1000 / 10, 1000 / 10 * 2)
            float target_position_x{};//目标 x 位置坐标，单位 m 当发送目标机器人 ID 时，该项为 0
            float target_position_y{};//目标 y 位置坐标，单位 m 当发送目标机器人 ID 时，该项为 0
            float target_position_z{};//目标 z 位置坐标，单位 m 当发送目标机器人 ID 时，该项为 0
            uint8_t commd_keyboard{};//发送指令时，云台手按下的键盘信息 无按键按下则为 0
            /**
                @brief 要作用的目标机器人 ID 当发送位置信息时，该项为 0
                @机器人ID 1，英雄(红)；
                @机器人ID 2，工程(红)；
                @机器人ID 3/4/5，步兵(红)；
                @机器人ID 6，空中(红)；
                @机器人ID 7，哨兵(红)；
                @机器人ID 9，雷达站（红）；
                @机器人ID 10，前哨站（红）；
                @机器人ID 11，基地（红）；
                @机器人ID 101，英雄(蓝)；
                @机器人ID 102，工程(蓝)；
                @机器人ID 103/104/105，步兵(蓝)；
                @机器人ID 106，空中(蓝)；
                @机器人ID 107，哨兵(蓝)；
                @机器人ID 109，雷达站（蓝）；
                @机器人ID 110，前哨站（蓝）；
                @机器人ID 111，基地（蓝）。
             */
            uint16_t target_robot_ID{};
        };


        /**
         * @brief 客户端小地图接收信息
         * @details 命令码: 0x0305
         * @details 数据段长度: 10
         */
        struct RobotCommandReceive {
            RefereeSystem_BASIC(0x0304, 1000 / 30, 1000 / 30 * 2)
            int16_t mouse_x{};//鼠标 X 轴信息
            int16_t mouse_y{};//鼠标 Y 轴信息
            int16_t mouse_z{};//鼠标滚轮信息
            int8_t left_button_down{};//鼠标左键
            int8_t right_button_down{};//鼠标右键按下
            uint16_t keyboard_value{};//键盘信息
            uint16_t reserved{};//保留位

            inline bool getW() const { return keyboard_value & (1 << 0); }

            inline bool getS() const { return keyboard_value & (1 << 1); }

            inline bool getA() const { return keyboard_value & (1 << 2); }

            inline bool getD() const { return keyboard_value & (1 << 3); }

            inline bool getShift() const { return keyboard_value & (1 << 4); }

            inline bool getCtrl() const { return keyboard_value & (1 << 5); }

            inline bool getQ() const { return keyboard_value & (1 << 6); }

            inline bool getE() const { return keyboard_value & (1 << 7); }

            inline bool getR() const { return keyboard_value & (1 << 8); }

            inline bool getF() const { return keyboard_value & (1 << 9); }

            inline bool getG() const { return keyboard_value & (1 << 10); }

            inline bool getZ() const { return keyboard_value & (1 << 11); }

            inline bool getX() const { return keyboard_value & (1 << 12); }

            inline bool getC() const { return keyboard_value & (1 << 13); }

            inline bool getV() const { return keyboard_value & (1 << 14); }

            inline bool getB() const { return keyboard_value & (1 << 15); }


        };

        /**
         * @brief 键盘、鼠标信息，通过图传串口发送
         * @details 命令码: 0x0304
         * @details 数据段长度: 12
         */
        struct ClientMapCommand {
            RefereeSystem_BASIC(0x0305, 1000 / 10, 1000 / 10 * 2)
            uint16_t target_robot_ID{};//目标机器人 ID
            float target_position_x{};//目标 x 位置坐标，单位 m 当 x,y 超出界限时则不显示。
            float target_position_y{};//目标 y 位置坐标，单位 m 当 x,y 超出界限时则不显示。
        };

#undef RefereeSystem_BASIC
#pragma pack()

        /**
         * 判断某个数据包是否有效
         * @tparam T 数据包类型
         * @param lstUpdate 最后一次更新数据包的tick(来自 cv::getTickCount()), 0为无数据
         * @return 是否有效
         */
        template<class T>
        FORCE_INLINE bool is_valid(const int64_t &lstUpdate) {
            const auto now = cv::getTickCount();

            if (T::INTERVAL == -2 && lstUpdate > 0) return true;
            if (static_cast<double>(now - lstUpdate) / cv::getTickFrequency() < (T::MAX_INTERVAL / 1000.0))return true;
            return false;
        }
    }


}
#endif //IFR_OPENCV_REFEREE_DATAS_H
