/*
数据处理函数的声明
*/

#ifndef DATA_PROCESS_INCLUDED
#define DATA_PROCESS_INCLUDED

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include <ctime>
#include <set>

using namespace std;
using namespace cv;

//保存二维数据的结构体类型
struct Data {
    long double x;
    long double y;

    bool operator==(const Data &b) const {
        return this->x == b.x && this->y == b.y;
    }
};

//Data类型的比较函数定义
class DataComp {
public:
    bool operator()(const Data &a, const Data &b) const {
        return a.x < b.x;
    };
};

//保存Data类型数据的Set类型
typedef set<Data, DataComp> DataSetType;

typedef vector<long double> FParam;

typedef double (*oriFuncType)(FParam &, double);

typedef long double (*lossFuncType)(void *, FParam &, DataSetType &);

/**
* @brief 将x和y数据合为一个数据
* @param x x数据
* @param y y数据
* @param xy_data 合并后的数据，格式为：[x,y,x,y,x,y,...]
* @return 合并是否成功
*/
bool merge_data_to_xy_data(FParam &x, FParam &y, FParam &xy_data);

/**
* @brief 计算角度制角度
* @param target 目标位置
* @param center 目标的环绕中心位置
* @return 计算的角度
*/
long double get_angle(const Point2f &target, const Point2f &center);

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
                       double get_continue_time);

/**
* @brief 通过位置数据集计算速度数据集
* @param tp_data 输入位置数据集
* @param tv_data 输出速度数据
* @param start_time 保存tp_data中的第一个时间数据的值
*/
void calc_velocity_data(FParam &tp_data, FParam &tv_data, long double *start_time);

/**
* @brief 通过位置数据集计算位置变化的数值积分数据集
* @param tp_data 输入位置数据集
* @param ti_data 输出位置变化的数值积分数据
* @param start_time 保存tp_data中的第一个时间数据的值
*/
void calc_intg_data(FParam &tp_data, FParam &ti_data, long double *start_time);

/**
* @brief 将保存位置数据的Data类型的Set转换为FParam类型的数据集
* @param tp_data_set 保存位置数据的Data类型的Set
* @param tp_data FParam类型的数据集
*/
void convert_DataSet_to_tp_Data(DataSetType &tp_data_set, FParam &tp_data);

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
                              double get_continue_time);

#endif //DATA_PROCESS_INCLUDED