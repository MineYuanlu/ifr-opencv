/*
速度函数的拟合函数的定义
*/

#include "headers/fit_functions.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

//xy_data = [x, y, x, y, x, y......]

/**
* @brief 损失函数
* @param _ori_func 原函数
* @param params 函数参数
* @param xy_data 获取到的数据集
*/
long double loss_func(void *_ori_func, FParam &params, FParam &xy_data) {
    static long double result, t;
    static double (*ori_func)(FParam &, double);
    static int i, N;

    result = 0;
    ori_func = (double (*)(FParam &, double)) _ori_func;
    N = xy_data.size();

    if (N == 0)return -1;

    for (i = 0; i < N; i += 2) {
        t = (long double) (ori_func(params, (double) xy_data[i])) - xy_data[i + 1];
        result += std::exp(t * t) - 1.0l;
    }

    result /= N;

    return result;
}

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
bool decrease_direction(void *func_p, FParam &func_param, FParam &xy_data, FParam &steps, FParam &lb, FParam &ub) {
    static double (*func)(FParam &, double);
    FParam t_para(func_param);//, temp_steps(steps), func_param_temp(func_param);
    static int N, i, j;
    static long double diff_front, diff_rear, diff_1;//, diff_1f, diff_1r, diff_2_ij;
    static long double lfunc_value_f, lfunc_value_m, lfunc_value_r, lam;
    static long double dt;

    N = func_param.size();
    lam = 1.0;
    //dt = 5e-4l;

    func = (double (*)(FParam &, double)) func_p;

    for (i = 0; i < N; i++) {
        dt = steps[i];

        //lfunc_value_m = loss_func((void*)func, t_para, xy_data);
        t_para[i] = func_param[i] + dt;
        lfunc_value_f = loss_func((void *) func, t_para, xy_data);//cout << "lfunc_value_f: " << lfunc_value_f << endl;
        if (lfunc_value_f == -1)return false;
        t_para[i] = func_param[i] - dt;
        lfunc_value_r = loss_func((void *) func, t_para, xy_data);//cout << "lfunc_value_r: " << lfunc_value_r << endl;

        diff_1 = lfunc_value_f - lfunc_value_r;

        //steps[i] = -0.001l * diff_1;

        //t_para[i] += steps[i];


        if (diff_1 >= 0) {
            t_para[i] = func_param[i] - dt;
            steps[i] = -0.95l * steps[i];
        } else {
            t_para[i] = func_param[i] + dt;
            steps[i] = 1.0001l * steps[i];
        }

        if (steps[i] > 1.0l)steps[i] = 1.0l;
        else if (steps[i] < -1.0l)steps[i] = -1.0l;

        if (lb[i] > t_para[i])t_para[i] = lb[i];
        else if (ub[i] < t_para[i])t_para[i] = ub[i];

        func_param[i] = t_para[i];
        //cout << "diff_1: " << diff_1 << endl;

        //temp_steps[i] = -1.0 * lam * diff_1;
    }

    return true;
}

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
bool func_fit(void *func_p, FParam *start_param, FParam *xy_data, FParam *_steps, FParam *lb, FParam *ub) {
    bool steps_have_nan = false;//all_step_zero = false,
    int N = start_param->size(), i;
    //double pre_loss_func_value = 0, loss_func_value = 0, loss_changes;
    static long double max_step;
    static int64 start_time_tick, passed_time_tick;
    FParam steps(*_steps);

    start_time_tick = cv::getTickCount();
    do {

        if (!decrease_direction(func_p, *start_param, *xy_data, steps, *lb, *ub))return true;
        //loss_func_value = loss_func(func_p, start_param, xy_data);

        //start_param[0] = start_param[0] + steps[0];
        steps_have_nan = steps[0] == NAN || steps[0] == INFINITY;
        max_step = std::abs(steps[0]);

        for (i = 1; i < N; i++) {
            (*start_param)[i] = (*start_param)[i] + steps[i];
            steps_have_nan = steps_have_nan || steps[i] == NAN || steps[i] == INFINITY;
            if (max_step < std::abs(steps[i]))max_step = std::abs(steps[i]);
        }
        if (steps_have_nan)return false;
        if (max_step > 100) {
            start_param->resize(N, 0.1l);
            steps.resize(N, 0.1);
            return false;
        }
        passed_time_tick = cv::getTickCount() - start_time_tick;
        if ((double) passed_time_tick / cv::getTickFrequency() > 5.0) {
            start_param->resize(N, 0.1l);
            steps.resize(N, 0.5);
            return false;
        }

        /*
        cout << "max_step:"<<max_step<<' ';
        cout << "params = [" << (*start_param)[0];
        for(int i=1;i<4;i++){
            cout << ", " << (*start_param)[i];
        }
        cout << "]" << endl;*/


        //loss_changes = std::abs(loss_func_value - pre_loss_func_value);
        //pre_loss_func_value = loss_func_value;
        //printf("max_step:%Lf\n", max_step);

    } while (max_step > PRECISION);

    return true;
}

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
bool func_fit(void *func_p, FParam &start_param, FParam &xy_data, double lb = -10.0, double ub = 10.0) {
    static const double decrese_scale = 0.99;
    static double increace_step, min_loss_value, cur_loss_value;
    static int i, j, N;

    static double (*func)(FParam &, double);
    func = (double (*)(FParam &, double)) func_p;

    increace_step = 1.0 - decrese_scale;
    N = start_param.size();

    FParam now_loc(start_param), cur_lb(start_param.size(), lb), cur_ub(start_param.size(), ub);
    static double now_lb_ub_range, min_lb_ub_range, min_location;

    min_loss_value = loss_func((void *) func, now_loc, xy_data);

    do {
        for (i = 0; i < N; i++) {
            now_lb_ub_range = cur_ub[i] - cur_lb[i];
            if (i == 0 || now_lb_ub_range < min_lb_ub_range)min_lb_ub_range = now_lb_ub_range;
            min_location = start_param[i];
            for (now_loc[i] = cur_lb[i]; now_loc[i] <= cur_ub[i]; now_loc[i] += now_lb_ub_range * increace_step) {
                cur_loss_value = loss_func((void *) func, now_loc, xy_data);
                if (min_loss_value > cur_loss_value) {
                    min_loss_value = cur_loss_value;
                    min_location = now_loc[i];
                }
            }
            start_param[i] = min_location;
            now_loc[i] = start_param[i];

            cur_lb[i] = now_loc[i] - decrese_scale * now_lb_ub_range * 0.25;
            cur_ub[i] = now_loc[i] + decrese_scale * now_lb_ub_range * 0.25;
        }

        //cout << "min_lb_ub_range: " << min_lb_ub_range << endl;
    } while (min_lb_ub_range > PRECISION);

    return true;

}

/**
* @brief 速度函数定义
* @param params 函数参数
* @param x 自变量
*/
double velocity_func(FParam &params, double x) {
    //double result = params[0] + params[1]*x + params[2]*x*x + params[3]*x*x*x;
    //double result = params[0] * std::sin(params[1] * x + params[2]) + 7.0 - params[0];
    return (double) params[0] * std::sin(double(params[1]) * x + double(params[2])) + double(params[3]);
}

#if USE_THE_FUNC_FOR_TEST
/**
* @brief 速度函数的积分函数
* @param params 函数参数
* @param t0 积分下限
* @param t t0+t为积分上限
*/
double Int_velocity_func(FParam& params, double t0, double t){
    double param_for_test[4] = {0.785, 1.884, (double)params[0], 1.305};

#define cos_i1 (param_for_test[1] * t0 + param_for_test[2])
#define cos_i2 (param_for_test[1] * (t0 + t) + param_for_test[2])

    return (((param_for_test[0] * (std::cos(cos_i1) - std::cos(cos_i2))) / param_for_test[1]) + param_for_test[3] * t);
}
#else

/**
* @brief 速度函数的积分函数
* @param params 函数参数
* @param t0 积分下限
* @param t t0+t为积分上限
*/
double Int_velocity_func(FParam &params, double t0, double t) {
    if (params[1] == 0.0)return (double) params[0] * std::sin((double) params[2]) * t + (double) params[3] * t;

#define cos_i1 ((double)params[1] * t0 + (double)params[2])
#define cos_i2 ((double)params[1] * (t0 + t) + (double)params[2])

    return ((((double) params[0] * (std::cos(cos_i1) - std::cos(cos_i2))) / (double) params[1]) +
            (double) params[3] * t);
}

#endif

/**
* @brief t0 = 0时速度函数的积分函数
* @param params 函数参数
* @param t t0+t为积分上限
*/
double Int_velocity_func_t0_is0(FParam &params, double t) {
    return Int_velocity_func(params, 0, t);
}
