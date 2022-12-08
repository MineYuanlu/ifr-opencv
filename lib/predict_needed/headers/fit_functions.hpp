/*
速度函数的拟合函数的声明
*/

#ifndef FIT_FUNCTIONS_INCLUDED
#define FIT_FUNCTIONS_INCLUDED

#include <iostream>
#include <cmath>
#include <vector>
#include <thread>

#include "data_process.hpp"

#define PRECISION 1e-5

using namespace std;

struct dynamic_fit_process_param_type {
    FParam *param;
    FParam *lb;
    FParam *ub;
    DataSetType *xy_data;
    oriFuncType func;
    long double learning_rate;
    bool progress_running;
};

//xy_data = [x, y, x, y, x, y......]

/**
* @brief 损失函数
* @param _ori_func 原函数
* @param params 函数参数
* @param xy_data 获取到的数据集
*/
long double loss_func(void *_ori_func, FParam &params, FParam &xy_data);

/**
* @brief 损失函数
* @param _ori_func 原函数
* @param params 函数参数
* @param xy_data 获取到的数据集
*/
long double loss_func(void *_ori_func, FParam &params, DataSetType &xy_data);

/**
* @brief 拟合函数func_fit所用的子函数
* @param func_p 原函数
* @param params 函数参数
* @param xy_data 获取到的数据集
* @param _steps 初始步长
* @param lb 下界
* @param ub 上界
* @return 是否计算成功
*/
bool decrease_direction(void *func_p, FParam &func_param, FParam &xy_data, FParam &steps, FParam &lb, FParam &ub);

/**
* @brief 拟合函数
* @param func_p 原函数
* @param params 函数参数
* @param xy_data 获取到的数据集
* @param _steps 初始步长
* @param lb 下界
* @param ub 上界
* @return 是否计算成功
*/
bool func_fit(void *func_p, FParam *start_param, FParam *xy_data, FParam *_steps, FParam *lb, FParam *ub);

/**
* @brief 拟合函数2（精度不太够）
* @param func_p 原函数
* @param params 函数参数
* @param xy_data 获取到的数据集
* @param _steps 初始步长
* @param lb 下界
* @param ub 上界
* @return 是否计算成功
*/
//bool func_fit(void* func_p, FParam& start_param, FParam& xy_data,double lb, double ub);

/**
* @brief 速度函数定义
* @param params 函数参数
* @param x 自变量
*/
double velocity_func(FParam &params, double x);

/**
* @brief 速度函数的积分函数
* @param params 函数参数
* @param t0 积分下限
* @param t t0+t为积分上限
*/
double Int_velocity_func(FParam &params, double t0, double t);

/**
* @brief t0 = 0时速度函数的积分函数
* @param params 函数参数
* @param t t0+t为积分上限
*/
double Int_velocity_func_t0_is0(FParam &params, double t);

//动态拟合模型
class DynFitter {
private:
    FParam param;
    FParam lb;
    FParam ub;
    DataSetType xy_data;
    oriFuncType func;
    long double learning_rate, max_getted_time;
    bool progress_running;

    thread *FitThread_p;

    dynamic_fit_process_param_type FitThreadParam;

    long double pre_T, change_T, pre_P, pre_change_P, change_P, cur_vel;
    long double start_T, start_P;
public:
    DynFitter();

    DynFitter(FParam &start_param, FParam &lb, FParam &ub, oriFuncType func_ptr, long double max_getted_time = 5.0,
              long double learning_rate = 0.01l);

    void insertData(Data &data);

    void get_now_param(FParam &_param, long double &start_T);

    void OpenProcess();

    void CloseProcess();
};

#endif //FIT_FUNCTIONS_INCLUDED