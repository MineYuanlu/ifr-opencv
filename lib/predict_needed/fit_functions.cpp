/*
速度函数的拟合函数的定义
*/

#include "headers/fit_functions.hpp"
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <random>

//xy_data = [x, y, x, y, x, y......]

/**
* @brief 损失函数
* @param _ori_func 原函数
* @param params 函数参数
* @param xy_data 获取到的数据集
*/
long double loss_func(void *_ori_func, FParam &params, FParam &xy_data) {
    long double result, t;
    double (*ori_func)(FParam &, double);
    size_t i, N;

    result = 0;
    ori_func = (double (*)(FParam &, double)) _ori_func;
    N = xy_data.size();

    if (N == 0)return -1;

    for (i = 0; i < N; i += 2) {
        t = (long double) (ori_func(params, (double) xy_data[i])) - xy_data[i + 1];
        t = t * t;
        result += t;
    }

    result /= (long double) N;
    //result = result / (result + 1.0l);

    //printf("result:%.6Lf\n", result);

    return result;
}

/**
* @brief 损失函数
* @param _ori_func 原函数
* @param params 函数参数
* @param xy_data 获取到的数据集
*/
long double loss_func(void *_ori_func, FParam &params, DataSetType &xy_data) {
    long double result, t;
    double (*ori_func)(FParam &, double);
    size_t N;

    result = 0;
    ori_func = (double (*)(FParam &, double)) _ori_func;
    N = xy_data.size();

    if (N == 0)return -1;

    for (auto data: xy_data) {
        t = (long double) (ori_func(params, (double) data.x)) - data.y;
        result += std::exp(t * t) - 1.0l;
    }

    result /= (long double) N;
    //result = result / (result + 1.0l);

    //printf("result:%.6Lf\n", result);

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
    long double dt, Temp;

    N = func_param.size();
    lam = 1.0;
    dt = 1e-7l;

    func = (double (*)(FParam &, double)) func_p;

    for (i = 0; i < N; i++) {
        //dt = steps[i];

        //printf("dt:%.16Lf\n", dt);

        //lfunc_value_m = loss_func((void*)func, t_para, xy_data);
        t_para[i] = func_param[i] + dt;
        lfunc_value_f = loss_func((void *) func, t_para, xy_data);//cout << "lfunc_value_f: " << lfunc_value_f << endl;
        if (lfunc_value_f == -1)return false;
        t_para[i] = func_param[i] - dt;
        lfunc_value_r = loss_func((void *) func, t_para, xy_data);//cout << "lfunc_value_r: " << lfunc_value_r << endl;

        diff_1 = lfunc_value_f - lfunc_value_r;

        Temp = -0.0625l * diff_1 / dt / 2.0l;

        steps[i] = 0.2l * steps[i] + 0.8l * Temp;

        t_para[i] = t_para[i] + steps[i];

        /*
        if(diff_1 >= 0.0l){
            t_para[i] = func_param[i] - 0.01l*dt;
            steps[i] = -0.99l*steps[i];
        }else{
            t_para[i] = func_param[i] + dt;
            steps[i] = 1.001l*steps[i];
        }*/

        if (lb[i] > t_para[i])t_para[i] = lb[i];
        if (ub[i] < t_para[i])t_para[i] = ub[i];

        Temp = t_para[i];
        t_para[i] = func_param[i];
        func_param[i] = Temp;
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
            *start_param = FParam(N, 1.0l);
            steps = FParam(N, 0.1l);
            return false;
        }
        passed_time_tick = cv::getTickCount() - start_time_tick;
        if ((double) passed_time_tick / cv::getTickFrequency() > 100.0) {
            *start_param = FParam(N, 1.0l);
            steps = FParam(N, 0.1l);
            return false;
        }

        cout << "max_step:" << max_step << ' ';
        cout << "params = [" << (*start_param)[0];
        for (int i = 1; i < 4; i++) {
            cout << ", " << (*start_param)[i];
        }
        cout << "]" << endl;

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
    if (params[1] == 0)return (double) params[0] * std::sin((double) params[2]) * t + (double) params[3] * t;

    double cos_i1 = ((double) params[1] * t0 + (double) params[2]);
    double cos_i2 = ((double) params[1] * (t0 + t) + (double) params[2]);

    while (cos_i1 > CV_PI)cos_i1 -= CV_2PI;
    while (cos_i1 < -CV_PI)cos_i1 += CV_2PI;
    while (cos_i2 > CV_PI)cos_i2 -= CV_2PI;
    while (cos_i2 < -CV_PI)cos_i2 += CV_2PI;

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

Mutex fitProgress_lock;

std::random_device fit_rd;
std::default_random_engine fit_gen(fit_rd());
std::uniform_real_distribution<long double> fit_dis(0, 10000.0l);

long double gen_rand_double(long double a, long double b) {
    return fit_dis(fit_gen) * (a - b) / 10000.0l + b;
}

void dynamic_fit_process_callback(void *_params) {
    fitProgress_lock.lock();
    dynamic_fit_process_param_type *params_p = (dynamic_fit_process_param_type *) _params;
    FParam &_func_param = *(params_p->param);
    FParam lb = FParam(*(params_p->lb)), ub = FParam(*(params_p->ub));
    DataSetType &_xy_data = *(params_p->xy_data);
    oriFuncType func = params_p->func;
    long double learning_rate = params_p->learning_rate, step_multip;
    size_t N = _func_param.size(), i;

    learning_rate = std::abs(learning_rate);
    step_multip = learning_rate / (learning_rate + 0.1l);

    params_p->progress_running = true;
    fitProgress_lock.unlock();
    bool progress_running = true;

    long double dt, f_res_f, f_res_r, diff_1, temp_ld;
    fitProgress_lock.lock();
    FParam step(N, 0.1l), temp_param(N, 0), temp_param_ori(N, 0), temp_param_out(N, 0);
    FParam rand_param(N, 0);
    fitProgress_lock.unlock();

    while (progress_running) {
        fitProgress_lock.lock();
        if (_xy_data.size() < 4) {
            fitProgress_lock.unlock();
            continue;
        }
        temp_param = FParam(_func_param);
        temp_param_ori = FParam(_func_param);
        fitProgress_lock.unlock();

        for (i = 0; i < N; i++) {
            dt = step[i];

            temp_param[i] = temp_param_ori[i] + dt;
            fitProgress_lock.lock();
            f_res_f = loss_func((void *) func, temp_param, _xy_data);
            fitProgress_lock.unlock();
            temp_param[i] = temp_param_ori[i] - dt;
            fitProgress_lock.lock();
            f_res_r = loss_func((void *) func, temp_param, _xy_data);
            fitProgress_lock.unlock();

            diff_1 = f_res_f - f_res_r;

            if (diff_1 > 0) {
                temp_ld = -step_multip * step[i];
                step[i] = 0.75l * temp_ld + 0.25l * step[i];
            } else {
                step[i] = 1.01 * step[i];
            }

            //step[i] = -learning_rate * diff_1;

            //printf("diff_1==%Lf\n", diff_1);

            if (step[i] > 1.0l)step[i] = 1.0l;
            if (step[i] < -1.0l)step[i] = -1.0l;

            if (step[i] < PRECISION && step[i] >= 0)step[i] = PRECISION;
            if (step[i] > -PRECISION && step[i] <= 0)step[i] = -PRECISION;

            if (std::abs(step[i]) > PRECISION)temp_param_out[i] = temp_param_ori[i] + step[i];
            temp_param[i] = temp_param_ori[i];

            if (temp_param_out[i] > ub[i])temp_param_out[i] = ub[i];
            if (temp_param_out[i] < lb[i])temp_param_out[i] = lb[i];

            rand_param[i] = gen_rand_double(lb[i], ub[i]);
        }

        fitProgress_lock.lock();
        f_res_f = loss_func((void *) func, rand_param, _xy_data);
        f_res_r = loss_func((void *) func, temp_param_out, _xy_data);
        fitProgress_lock.unlock();

        if (f_res_f - f_res_r < 0)temp_param_out = rand_param;

        fitProgress_lock.lock();
        progress_running = params_p->progress_running;
        _func_param = FParam(temp_param_out);
        fitProgress_lock.unlock();
    }

    return;
}

DynFitter::DynFitter() {
    this->param = FParam(4, 1.0l);
    this->xy_data = DataSetType();
    this->lb = FParam(4, -4.0l);
    this->ub = FParam(4, 4.0l);
    this->func = Int_velocity_func_t0_is0;
    this->max_getted_time = 5.0l;
    this->learning_rate = 0.01l;

    this->pre_T = 0.0l;
    this->pre_P = 0.0l;
    this->change_P = 0.0l;
    this->change_T = 0.0l;
    this->pre_change_P = 0.0l;
    this->cur_vel = 0.0l;
    this->start_T = 0.0l;

    this->FitThreadParam = dynamic_fit_process_param_type({0});
}

DynFitter::DynFitter(FParam &start_param, FParam &lb, FParam &ub, oriFuncType func_ptr, long double max_getted_time,
                     long double learning_rate) {
    this->param = FParam(start_param);
    this->xy_data = DataSetType();
    this->lb = FParam(lb);
    this->ub = FParam(ub);
    this->func = func_ptr;
    this->max_getted_time = max_getted_time;
    this->learning_rate = learning_rate;

    this->pre_T = 0.0l;
    this->pre_P = 0.0l;
    this->change_P = 0.0l;
    this->change_T = 0.0l;
    this->pre_change_P = 0.0l;
    this->cur_vel = 0.0l;
    this->start_T = 0.0l;

    this->FitThreadParam = dynamic_fit_process_param_type({0});
}

void DynFitter::insertData(Data &data) {
    Data TempData;

    fitProgress_lock.lock();
    if (this->xy_data.size() < 1) {
        TempData.x = data.x;
        TempData.y = data.y;

        this->start_T = 0.0l;
        this->start_P = 0.0l;

        this->xy_data.insert(TempData);

        this->pre_T = data.x;
        this->pre_P = data.y;
    } else {
        this->change_T = data.x - this->pre_T;
        this->change_P = data.y - this->pre_P;
        this->cur_vel = 0.9l * this->change_P / this->change_T + 0.1l * this->cur_vel;

        if (this->change_P > CV_PI)this->change_P -= CV_2PI;
        if (this->change_P < -CV_PI)this->change_P += CV_2PI;

        if (std::abs(this->change_P) > 0.5l) {
            this->change_P = this->cur_vel * this->change_T;
        }

        TempData.x = this->xy_data.rbegin()->x + this->change_T;
        TempData.y = this->xy_data.rbegin()->y + this->change_P;

        this->xy_data.insert(TempData);

        this->pre_T = data.x;
        this->pre_P = data.y;

        this->pre_change_P = this->change_P;
    }

    auto iter1 = this->xy_data.rbegin();
    auto iter2 = this->xy_data.begin();

    if (iter1->x - iter2->x > this->max_getted_time) {

        //printf("\nPop Out [%Lf,%Lf]\n", iter2->x, iter2->y);
        this->xy_data.erase(iter2);
    }

    fitProgress_lock.unlock();

    return;
}

void DynFitter::OpenProcess() {
    this->FitThreadParam.func = this->func;
    this->FitThreadParam.lb = &(this->lb);
    this->FitThreadParam.ub = &(this->ub);
    this->FitThreadParam.learning_rate = this->learning_rate;
    this->FitThreadParam.param = &(this->param);
    this->FitThreadParam.xy_data = &(this->xy_data);
    this->FitThreadParam.progress_running = false;

    this->FitThread_p = new thread(dynamic_fit_process_callback, (void *) (&FitThreadParam));

    while (!(this->FitThread_p->joinable()));

    return;
}

void DynFitter::CloseProcess() {
    fitProgress_lock.lock();
    this->FitThreadParam.progress_running = false;
    fitProgress_lock.unlock();
    this->FitThread_p->join();

    return;
}

void DynFitter::get_now_param(FParam &_param, long double &start_T) {
    fitProgress_lock.lock();
    _param = this->param;
    start_T = this->start_T;
    fitProgress_lock.unlock();
}