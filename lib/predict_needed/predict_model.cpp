/*
预测模型类的定义
*/

#include "headers/predict_model.hpp"

#include <thread>
#include <mutex>

FParam UB_VAL = {2, 4, CV_PI, 4};           //拟合参数上界
FParam LB_VAL = {-2, -4, -CV_PI, -4};        //拟合参数下界

/**
* @brief 计算变化的角度
* @param now_angle 当前角度
* @param pre_angle 变化前的角度
*/
double calc_change_angle(double now_angle, double pre_angle) {
    double change;
    change = now_angle - pre_angle;

    if (change >= CV_PI)change -= CV_2PI;
    if (change <= -CV_PI)change += CV_2PI;

    return change;
}

/**
* @brief n的阶乘
*/
double factorial(int n) {
    if (n < 1)return 1;
    if (n == 1)return 1;
    if (n == 2)return 2;
    return (double) n * factorial(n - 1);
}

/**
* @brief x的y次幂
*/
double Power(double x, int y) {
    if (y <= 0)return 1.0;
    return x * Power(x, y - 1);
}

/**
* @brief PredictModel获取数据方法
* @param center 能量机关中心点
* @param target 目标装甲板中心点
* @param leap_center 用于计算旋转角度的稳定的旋转中心点
* @param time 当前目标所处时刻，单位为可任意，但计算目标预测位置时所用单位必须与此相同
*/
void PredictModel::get_data(Point2f &center, Point2f &target, Point2f &leap_center, double time) {
    this->now_Df_set[0] = get_angle(target, leap_center);
    this->now_moment_set[0] = time;

    this->now_center = center;
    this->now_target = target;

    Point2f vec_C2T = target - center;
    this->now_path_radius = (double) std::sqrt(vec_C2T.x * vec_C2T.x + vec_C2T.y * vec_C2T.y);

    static double xchange = 0.0, time_change = 0.0, prex, nowx, Deg_Of_Conf;
    if (this->pre_moment_set[0] != -1) {
        time_change = this->now_moment_set[0] - this->pre_moment_set[0];
        if (std::abs(time_change) < SAMPLING_TIME / 100.0)return;
        prex = this->pre_Df_set[0];
        nowx = this->now_Df_set[0];
        xchange = calc_change_angle(nowx, prex);
        if (std::abs(xchange) > CV_PI / 5.0)goto end;
    }

    static int i;

    for (i = 1; i < this->model_complex; i++) {
        if (this->pre_moment_set[i - 1] == -1)break;
        this->now_moment_set[i] = (this->now_moment_set[i - 1] + this->pre_moment_set[i - 1]) / 2.0;
        Deg_Of_Conf = Power(DEGREE_OF_CONFIDENCE, i);
        if (i != 1) xchange = this->now_Df_set[i - 1] - this->pre_Df_set[i - 1];
        if (this->pre_moment_set[i] != -1) {
            this->now_Df_set[i] = Deg_Of_Conf * xchange / (this->now_moment_set[i - 1] - this->pre_moment_set[i - 1]) +
                                  (1.0 - Deg_Of_Conf) * this->pre_Df_set[i];
        } else {
            this->now_Df_set[i] = xchange / (this->now_moment_set[i - 1] - this->pre_moment_set[i - 1]);
        }
    }
    end:
    for (i = 0; i < this->model_complex; i++) {
        this->pre_moment_set[i] = this->now_moment_set[i];
        this->pre_Df_set[i] = this->now_Df_set[i];
    }

    return;
}

/**
* @brief PredictModel预测位置方法
* @param time 更新目标time单位时间后的位置信息，所用单位与获取的数据的单位相同
*/
double PredictModel::predict(double time) {
    static double result;
    static int i;
    result = this->now_Df_set[0];
    for (i = 1; i < this->model_complex; i++) {
        if (this->now_moment_set[i] == -1)break;
        result += Power(time, i) * this->now_Df_set[i] / factorial(i);
    }

    this->predict_angle = result;
    return result;
}

/**
* @brief PredictModel预测速度方法
* @param time 计算目标time单位时间后的速度，所用单位与获取的数据的单位相同
*/
Point2f PredictModel::predict_vel(double time) {
    static double result;//, abs_line_vel;
    static int i;
    if (this->now_moment_set[1] == -1) {
        this->predict_linear_vel_vec.x = 0.0;
        this->predict_linear_vel_vec.y = 0.0;
        return this->predict_linear_vel_vec;
    }
    result = this->now_Df_set[1];
    for (i = 1; i < this->model_complex; i++) {
        if (this->now_moment_set[i + 1] == -1)break;
        result += Power(time, i) * this->now_Df_set[i + 1] / factorial(i);
    }

    this->predict_angular_vel = result;
    this->predict_linear_vel = result * this->now_path_radius;

    Point2f r_vec = this->now_target - this->now_center;

    this->predict_linear_vel_vec.x = -1.0 * this->predict_angular_vel * r_vec.y;
    this->predict_linear_vel_vec.y = this->predict_angular_vel * r_vec.x;
/*
    abs_line_vel = std::abs(this->predict_linear_vel);
    if(this->predict_linear_vel > 0){
        this->predict_linear_vel_vec.x = abs_line_vel*(-1.0*std::sin(this->now_Df_set[0]));
        this->predict_linear_vel_vec.y = abs_line_vel*std::cos(this->now_Df_set[0]);
    }else{
        this->predict_linear_vel_vec.x = abs_line_vel*std::sin(this->now_Df_set[0]);
        this->predict_linear_vel_vec.y = abs_line_vel*(-1.0*std::cos(this->now_Df_set[0]));
    }
*/
    return this->predict_linear_vel_vec;

}

/**
* @brief PredictModel获取预测目标像素位置方法，在调用predict()方法后更新
*/
Point2f PredictModel::get_predict_position() {
    Point2f predict_pt;
    predict_pt.x = this->now_center.x + this->now_path_radius * std::cos(this->predict_angle);
    predict_pt.y = this->now_center.y + this->now_path_radius * std::sin(this->predict_angle);

    return predict_pt;
}

//拟合线程输入参数的结构体
struct fitfuc_thread_params {
    void *func_p;
    FParam *params;
    FParam *xy_data;
    FParam *steps;
    FParam *lb;
    FParam *ub;
};

bool now_calculating_func = false,      //标记目前函数拟合计算线程是否正在运行
func_changed = false,                   //标记函数参数是否更新
fit_complete = false;                   //标记函数参数是否可用

Mutex fit_thread_lock;                  //函数拟合计算线程锁

/**
* @brief 函数拟合计算线程回调函数
* @param _params_stru 输入的参数的结构体的指针
*/
void fit_thread_CallBack(void *_params_stru) {
    fitfuc_thread_params *thr_params = (fitfuc_thread_params *) _params_stru;
    fit_thread_lock.lock();
    now_calculating_func = true;
    fit_thread_lock.unlock();
    fit_complete = func_fit(thr_params->func_p, thr_params->params, thr_params->xy_data, thr_params->steps,
                            thr_params->lb, thr_params->ub);
    fit_thread_lock.lock();
    func_changed = true;
    now_calculating_func = false;
    fit_thread_lock.unlock();
}

thread *fit_thread_p;                   //函数拟合计算线程指针

/**
* @brief PredictModelFit类构造函数
*/
PredictModelFit::PredictModelFit() {
    this->tp_data = FParam();
    this->tp_data_set = DataSetType();
    this->ti_data = FParam();
    this->now_param = FParam();
    this->step = FParam();

    this->max_sec = 3.5l;
    this->now_start_time = 0.0l;
    this->data_collected = false;
    this->model_available = false;

    this->ub = FParam(UB_VAL);
    this->lb = FParam(LB_VAL);

    this->now_param.resize(4, 1.0l);
    this->tp_data_set.clear();
    this->step.resize(4, 0.1l);
}

fitfuc_thread_params fit_needed_param;          //函数拟合计算线程所需参数保存在此变量

FParam temp_func_param;

/**
* @brief PredictModelFit数据获取方法
* @param center 能量机关中心点
* @param target 目标装甲板中心点
* @param leap_center 用于计算旋转角度的稳定的旋转中心点
* @param time 当前目标所处时刻，单位为可任意，但计算目标预测位置时所用单位必须与此相同(根据当前上下界的设置，若该上下界不是使用输入的数据的单位时的函数参数的上下界，可能会影响参数的计算)
*/
void PredictModelFit::get_data(Point2f &center, Point2f &target, Point2f &leap_center, double time) {
    if (!this->data_collected)
        this->data_collected = !(get_position_data_to_Set(target, leap_center, this->tp_data_set, (long double) (time),
                                                          this->max_sec));
    //cout << "time: " << time << endl;
    //this->now_start_time = this->tp_data_set.begin()->x;
    this->now_center = center;
    this->now_target = target;
    this->now_time = (long double) time;
    this->now_angle = get_angle(target, leap_center);
    Point2f vec_C2T = target - center;
    this->now_path_radius = (double) std::sqrt(vec_C2T.x * vec_C2T.x + vec_C2T.y * vec_C2T.y);

    fit_thread_lock.lock();
    if (func_changed) {
        func_changed = false;
        fit_thread_lock.unlock();
        if (fit_thread_p->joinable())fit_thread_p->join();
        /*cout << "3\n";
        delete [] fit_thread_p;
        cout << "4\n";*/

        this->now_param = temp_func_param;
        this->step = FParam({0.1, 0.1, 0.1, 0.1});

        //this->show_data_and_params();
        //exit(0);

        printf("Func Calculated\n");
        this->data_collected = false;
        this->model_available = fit_complete;
        this->tp_data.clear();
        this->tp_data_set = DataSetType();
        this->ti_data.clear();
    }
    if (this->data_collected && !now_calculating_func) {
        this->now_start_time = this->tp_data_set.begin()->x;

        convert_DataSet_to_tp_Data(this->tp_data_set, this->tp_data);

        calc_intg_data(this->tp_data, this->ti_data, &this->now_start_time);

        fit_needed_param.func_p = (void *) Int_velocity_func_t0_is0;
        fit_needed_param.lb = &(this->lb);
        fit_needed_param.ub = &(this->ub);
        fit_needed_param.steps = &(this->step);
        temp_func_param = FParam(now_param);
        //this->now_param.resize(this->now_param.size(), 1.0l);
        fit_needed_param.params = &(temp_func_param);
        fit_needed_param.xy_data = &(this->ti_data);
        //cout << "1\n";
        now_calculating_func = true;
        fit_thread_p = new thread(fit_thread_CallBack, (void *) (&fit_needed_param));
        while (!fit_thread_p->joinable());
        //cout << "2\n";
    }
    fit_thread_lock.unlock();

}

/**
* @brief PredictModelFit预测方法
* @param time 更新目标time单位时间后的位置信息，所用单位与获取的数据的单位相同
*/
double PredictModelFit::predict(double time) {
    this->predict_angle = this->now_angle +
                          Int_velocity_func(this->now_param, this->now_time - this->now_start_time, (long double) time);
    return this->predict_angle;
}

/**
* @brief PredictModelFit获取预测目标像素位置方法，在调用predict()方法后更新
*/
Point2f PredictModelFit::get_predict_position() {
    Point2f predict_pt;
    predict_pt.x = this->now_center.x + this->now_path_radius * std::cos(this->predict_angle);
    predict_pt.y = this->now_center.y + this->now_path_radius * std::sin(this->predict_angle);

    return predict_pt;
}

/**
* @brief PredictModelFit类打印当前模型内数据和函数参数的方法
*/
void PredictModelFit::show_data_and_params() {
    int i;

    cout << "T=[" << this->ti_data[0];
    for (i = 2; i < this->ti_data.size(); i += 2) {
        cout << "," << this->ti_data[i];
    }
    cout << "];\n" << "I=[" << this->ti_data[1];
    for (i = 3; i < this->ti_data.size(); i += 2) {
        cout << "," << this->ti_data[i];
    }
    cout << "];\n";

    cout << "func_params=[" << this->now_param[0];
    for (i = 1; i < this->now_param.size(); i++) {
        cout << "," << this->now_param[i];
    }
    cout << "];\n";
}

/**
* @brief PredictModelFit速度预测方法
* @param time 计算目标time单位时间后的线速度，所用单位与获取的数据的单位相同
*/
Point2f PredictModelFit::predict_vel(double time) {
    double predict_angular_vel;
    Point2f r_vec = this->now_target - this->now_center, predict_linear_vel_vec;

    predict_angular_vel = velocity_func(this->now_param, this->now_time - this->now_start_time + time);

    predict_linear_vel_vec.x = -1.0 * predict_angular_vel * r_vec.y;
    predict_linear_vel_vec.y = predict_angular_vel * r_vec.x;

    return predict_linear_vel_vec;
}

PredictModelDynFit::PredictModelDynFit() {
    this->now_param = FParam(4, 0.1l);
    this->lb = LB_VAL;
    this->ub = UB_VAL;
    this->fitter = DynFitter(this->now_param, this->lb, this->ub, Int_velocity_func_t0_is0, 6.0l, 0.9l);

    this->fitter.OpenProcess();
}

PredictModelDynFit::PredictModelDynFit(long double learning_rate, FParam start_param, FParam lb, FParam ub,
                                       long double max_sec) {
    this->now_param = FParam(start_param);
    this->lb = FParam(lb);
    this->ub = FParam(ub);
    this->fitter = DynFitter(this->now_param, this->lb, this->ub, Int_velocity_func_t0_is0, max_sec, learning_rate);

    this->fitter.OpenProcess();
}

PredictModelDynFit::~PredictModelDynFit() {
    this->fitter.CloseProcess();
    delete this;
}

void PredictModelDynFit::get_data(const Point2f &center, const Point2f &target, const Point2f &leap_center,
                                  const double time) {
    this->now_center = center;
    this->now_target = target;
    this->now_time = (long double) time;
    this->now_angle = get_angle(target, leap_center);
    Point2f vec_C2T = target - center;
    this->now_path_radius = (double) std::sqrt(vec_C2T.x * vec_C2T.x + vec_C2T.y * vec_C2T.y);

    Data TempData;
    TempData.x = this->now_time;
    TempData.y = this->now_angle;
    this->fitter.insertData(TempData);

    return;
}

double PredictModelDynFit::predict(double time) {
    this->fitter.get_now_param(this->now_param, this->start_time);
    this->predict_angle = this->now_angle + Int_velocity_func(this->now_param, this->now_time - this->start_time, time);
    return (double) this->predict_angle;
}

/**
* @brief PredictModelDynFit获取预测目标像素位置方法，在调用predict()方法后更新
*/
Point2f PredictModelDynFit::get_predict_position() {
    Point2f predict_pt;
    predict_pt.x = this->now_center.x + this->now_path_radius * std::cos(this->predict_angle);
    predict_pt.y = this->now_center.y + this->now_path_radius * std::sin(this->predict_angle);

    return predict_pt;
}

/**
    * @brief PredictModelDynFit速度预测方法
    * @param time 计算目标time单位时间后的线速度，所用单位与获取的数据的单位相同
    */
Point2f PredictModelDynFit::predict_vel(double time) {
    double predict_angular_vel;
    Point2f predict_target = this->get_predict_position();
    Point2f r_vec = predict_target - this->now_center, predict_linear_vel_vec;

    predict_angular_vel = velocity_func(this->now_param, this->now_time - this->start_time + time);

    predict_linear_vel_vec.x = -1.0 * predict_angular_vel * r_vec.y;
    predict_linear_vel_vec.y = predict_angular_vel * r_vec.x;

    return predict_linear_vel_vec;
}

/**
* @brief PredictModelDynFit类打印当前模型内数据和函数参数的方法
*/
void PredictModelDynFit::show_data_and_params() {
    int i;

    cout << "func_params=[" << this->now_param[0];
    for (i = 1; i < this->now_param.size(); i++) {
        cout << "," << this->now_param[i];
    }
    cout << "];\n";
}