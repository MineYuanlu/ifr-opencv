/*
数据处理函数的定义
*/

#include "headers/data_process.hpp"

using namespace std;
using namespace cv;

/**
* @brief 将x和y数据合为一个数据
* @param x x数据
* @param y y数据
* @param xy_data 合并后的数据，格式为：[x,y,x,y,x,y,...]
* @return 合并是否成功
*/
bool merge_data_to_xy_data(FParam &x, FParam &y, FParam &xy_data) {
    static size_t i;
    static size_t N;

    N = x.size();
    xy_data = FParam((size_t) N * 2);
    try {
        for (i = 0; i < N; i++) {
            xy_data[2 * (size_t) i] = x[(size_t) i];
            xy_data[2 * (size_t) i + 1] = y[(size_t) i];
        }
    }
    catch (...) {
        return false;
    }

    return true;
}

/**
* @brief 计算弧度制角度
* @param target 目标位置
* @param center 目标的环绕中心位置
* @return 计算的角度
*/
long double get_angle(const Point2f &target, const Point2f &center) {

    return (long double) (std::atan2(target.y - center.y, target.x - center.x));

}

/**
* @brief 对位置数据做滤波
* @param src_tp_data 输入的原始位置数据
* @param dst_tp_data 输出的滤波后的位置数据
*/
void position_data_filt(FParam &src_tp_data, FParam &dst_tp_data) {
    static size_t i;
    static size_t N;
    static long double temp;
    FParam src_copy(src_tp_data);

    N = src_copy.size();

    dst_tp_data.clear();

    temp = 0;
    dst_tp_data.push_back(src_copy[0]);
    dst_tp_data.push_back(src_copy[1]);
    for (i = 2; i < N; i += 2) {
        temp = std::abs(src_copy[i + 1] - src_copy[i - 1]);
        if (temp > 1e-4 && temp < CV_PI / 5.0) {
            dst_tp_data.push_back(src_copy[i]);
            dst_tp_data.push_back(src_copy[i + 1]);
        }
    }
}

#define MAX_TIMEOUT 1.0 //秒

/**
* @brief 获取位置数据，保存为[时间，角度，时间，角度……]，时间单位为秒
* @param target 目标点位置
* @param center 目标的环绕中心位置
* @param tp_data 用于保存数据的容器
* @param this_data_time 当前获取到的目标的时刻
* @param get_continue_time 持续获取数据直到当前数据距第一个数据的时间大于此值
* @return 数据是否获取完毕 
*/
bool get_position_data(Point2f &target, Point2f &center, FParam &tp_data, long double this_data_time,
                       double get_continue_time = 3.0) {
    long double start_time;
    size_t tp_data_size;

    tp_data_size = tp_data.size();

    if (tp_data_size > 0) {
        start_time = tp_data[0];
        //cout << "this_data_time - tp_data[tp_data_size - 2] : " << this_data_time - tp_data[tp_data_size - 2] << endl;
        if (this_data_time - tp_data[tp_data_size - 2] > MAX_TIMEOUT)tp_data.clear();
        if (tp_data[tp_data_size - 2] - start_time > get_continue_time)return false;
#if 0
        cout << "start_time: " << start_time << ' ';
        cout << "passed_time:" << tp_data[tp_data_size - 2] - start_time;
#endif
    }

    //if (tp_data_size > 0)cout << "tp_data[tp_data_size - 1] - start_time: " << tp_data[tp_data_size - 1] - start_time << endl;

    tp_data.push_back(this_data_time);
    tp_data.push_back(get_angle(target, center));

    return true;
}

/**
* @brief 获取位置数据，保存为Data结构体的set，时间单位为秒
* @param target 目标点位置
* @param center 目标的环绕中心位置
* @param tp_data_set 用于保存数据的容器
* @param this_data_time 当前获取到的目标的时刻
* @param get_continue_time 持续获取数据直到当前数据距第一个数据的时间大于此值
* @return 数据是否获取完毕
*/
bool get_position_data_to_Set(Point2f &target, Point2f &center, DataSetType &tp_data_set, long double this_data_time,
                              double get_continue_time = 3.0) {
    long double start_time, end_time;
    size_t tp_data_size = tp_data_set.size();
    Data TempData;

    if (tp_data_size > 0) {
        start_time = tp_data_set.begin()->x;
        end_time = (--tp_data_set.end())->x;
        tp_data_size = tp_data_set.size();
        //cout << "this_data_time - tp_data[tp_data_size - 2] : " << this_data_time - tp_data[tp_data_size - 2] << endl;
        if (this_data_time - end_time > MAX_TIMEOUT)tp_data_set = DataSetType();
        if (end_time - start_time > get_continue_time)return false;
        //cout << "end_time: " << end_time << endl;
#if 0
        cout << "start_time: " << start_time << ' ';
        cout << "passed_time:" << tp_data[tp_data_size - 2] - start_time;
#endif
    }

    //if (tp_data_size > 0)cout << "tp_data[tp_data_size - 1] - start_time: " << tp_data[tp_data_size - 1] - start_time << endl;
    TempData.x = this_data_time;
    TempData.y = get_angle(target, center);
    tp_data_set.insert(TempData);

    return true;
}

/**
* @brief 将保存位置数据的Data类型的Set转换为FParam类型的数据集
* @param tp_data_set 保存位置数据的Data类型的Set
* @param tp_data FParam类型的数据集
*/
void convert_DataSet_to_tp_Data(DataSetType &tp_data_set, FParam &tp_data) {
    tp_data.clear();
    for (; tp_data_set.size() > 0;) {
        tp_data.push_back((const long double) (tp_data_set.begin()->x));
        tp_data.push_back((const long double) (tp_data_set.begin()->y));
        tp_data_set.erase(tp_data_set.begin());
    }
}

/**
* @brief 速度数据滤波
* @param src_tv_data 输入速度数据
* @param dst_tv_data 输出速度数据
*/
void velocity_data_filt(FParam &src_tv_data, FParam &dst_tv_data) {
    static size_t i;
    static size_t N;
    static long double pre_change, temp, change_time;
    FParam src_copy(src_tv_data);

    N = src_copy.size();

    dst_tv_data.clear();

    //pre_change = 0;
    temp = 0;
    for (i = 2; i < N; i += 2) {
        temp = std::abs(src_copy[i + 1] - src_copy[i - 1]);
        change_time = src_copy[(size_t) i] - src_copy[i - 2];
        temp /= change_time;
        if (i != 2 && std::abs(temp - pre_change) / pre_change > 3.0)continue;
        pre_change = temp;
        if (std::abs(src_copy[(size_t) i + 1]) < 7.0) {
            dst_tv_data.push_back(src_copy[ i]);
            dst_tv_data.push_back(src_copy[ i + 1]);
        }
    }

    return;
}

/**
* @brief 通过位置数据集计算速度数据集
* @param tp_data 输入位置数据集
* @param tv_data 输出速度数据
* @param start_time 保存tp_data中的第一个时间数据的值
*/
void calc_velocity_data(FParam &tp_data, FParam &tv_data, long double *start_time) {
    static size_t i;
    static size_t N;
    static long double now_changes, now_T, now_V;

    N = tp_data.size();
    tv_data.clear();
    *start_time = tp_data[0];

    for (i = 2; i < N; i += 2) {
        now_changes = tp_data[i + 1] - tp_data[i - 1];

        if (now_changes > CV_PI) {
            now_changes -= CV_2PI;
        } else if (now_changes < -CV_PI) {
            now_changes += CV_2PI;
        }
        if (std::abs(now_changes) > CV_PI / 4.0 || std::abs(now_changes) < 0.01)continue;

        now_T = (tp_data[i - 2] + tp_data[i]) / 2.0;
        now_T -= tp_data[0];
        now_V = now_changes / (tp_data[i] - tp_data[i - 2]);

        tv_data.push_back(now_T);
        tv_data.push_back(now_V);
    }

    //velocity_data_filt(tv_data, tv_data);

    return;
}

/**
* @brief 位置变化的数值积分数据滤波
* @param src_ti_data 输入位置变化的数值积分数据集
* @param dst_ti_data 输出位置变化的数值积分数据集
*/
void intg_data_filt(FParam &src_ti_data, FParam &dst_ti_data) {
    static size_t i;
    static size_t N;
    static long double temp;
    FParam src_copy(src_ti_data);

    N = src_copy.size();

    dst_ti_data = FParam(0, 0);

    temp = 0;
    dst_ti_data.push_back(src_copy[0]);
    dst_ti_data.push_back(src_copy[1]);
    for (i = 2; i < N; i += 2) {
        temp = std::abs(src_copy[i + 1] - src_copy[i - 1]);
        if (temp > 0) {
            dst_ti_data.push_back(src_copy[i]);
            dst_ti_data.push_back(src_copy[i + 1]);
        }
    }

    return;
}

/**
* @brief 通过位置数据集计算位置变化的数值积分数据集
* @param tp_data 输入位置数据集
* @param ti_data 输出位置变化的数值积分数据
* @param start_time 保存tp_data中的第一个时间数据的值
*/
void calc_intg_data(FParam &tp_data, FParam &ti_data, long double *start_time) {
    volatile long double cur_T = 0, cur_In = 0, change_P = 0, pre_change_P = 0, pre_vel = 0, change_T = 1;
    size_t i, N;

    N = tp_data.size();
    ti_data.clear();
    ti_data.push_back(0.0);
    ti_data.push_back(0.0);
    *start_time = tp_data[0];

    for (i = 2; i < N; i += 2) {

        pre_vel = 0.85 * change_P / change_T + 0.15 * pre_vel;
        change_T = tp_data[i] - tp_data[i - 2];

        pre_change_P = pre_vel * change_T;

        //cout << "pre_change_P:" << pre_change_P << endl;

        cur_T += change_T;

        change_P = tp_data[i + 1] - tp_data[i - 1];
        if (change_P > CV_PI)change_P -= CV_PI * 2.0;
        else if (change_P < -CV_PI)change_P += CV_PI * 2.0;
        if (std::abs(change_P) > 0.2 || change_P * pre_change_P < 0) {
            change_P = pre_change_P;
        }
        cur_In += change_P;

        ti_data.push_back((long double) cur_T);
        ti_data.push_back((long double) cur_In);
    }

    //intg_data_filt(ti_data, ti_data);

    return;
}
