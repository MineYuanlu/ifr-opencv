/*
预测模型类的声明
*/

#ifndef PREDICT_MODEL_INCLUDED
#define PREDICT_MODEL_INCLUDED

#include <iostream>
#include <ctime>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include "fit_functions.hpp"

#define DEGREE_OF_CONFIDENCE 0.15
#define PREDICT_MODEL_RANK 8
#define SAMPLING_TIME 100                   //Unit: 毫秒

using namespace std;
using namespace cv;

extern FParam UB_VAL;      //拟合参数上界
extern FParam LB_VAL;      //拟合参数下界

//低精度预测模型类
class PredictModel {
public:
    double *now_Df_set, *pre_Df_set, *now_moment_set, *pre_moment_set;      //各时刻的预测模型参数和各阶差商，分别为当前时刻各阶差商、前一时刻各阶差商、当前时刻模型参数、前一时刻模型参数。
    Point2f now_center, now_target;                                         //当前能量机关中心点，当前目标点

    int model_complex = 9;                                                  //模型阶数
    double predict_angle = 0;                                               //预测角度
    double now_path_radius = 1;                                             //能量机关臂长
    Point2f predict_linear_vel_vec = Point2f(0, 0);                          //预测目标线速度
    double predict_angular_vel = 0, predict_linear_vel = 0;                 //预测目标预测目标角速度，预测目标线速度的模

    /**
    * @brief PredictModel构造函数
    * @param _model_complex 模型阶数
    */
    PredictModel(int _model_complex) {
        this->model_complex = _model_complex + 1;
        this->now_Df_set = new double[this->model_complex];
        this->pre_Df_set = new double[this->model_complex];
        this->now_moment_set = new double[this->model_complex];
        this->pre_moment_set = new double[this->model_complex];
        for (int i = 0; i < this->model_complex; i++) {
            this->now_Df_set[i] = -1;
            this->pre_Df_set[i] = -1;
            this->now_moment_set[i] = -1;
            this->pre_moment_set[i] = -1;
        }
    }

    /**
    * @brief PredictModel默认构造函数
    */
    PredictModel() {
        this->now_Df_set = new double[this->model_complex];
        this->pre_Df_set = new double[this->model_complex];
        this->now_moment_set = new double[this->model_complex];
        this->pre_moment_set = new double[this->model_complex];
        for (int i = 0; i < this->model_complex; i++) {
            this->now_Df_set[i] = -1;
            this->pre_Df_set[i] = -1;
            this->now_moment_set[i] = -1;
            this->pre_moment_set[i] = -1;
        }
    }

    /**
    * @brief PredictModel获取数据方法
    * @param center 能量机关中心点
    * @param target 目标装甲板中心点
    * @param leap_center 用于计算旋转角度的稳定的旋转中心点
    * @param time 当前目标所处时刻，单位为可任意，但计算目标预测位置时所用单位必须与此相同
    */
    void get_data(Point2f &center, Point2f &target, Point2f &leap_center, double time);

    /**
    * @brief PredictModel预测位置方法
    * @param time 更新目标time单位时间后的位置信息，所用单位与获取的数据的单位相同
    */
    double predict(double time);

    /**
    * @brief PredictModel预测速度方法
    * @param time 计算目标time单位时间后的速度，所用单位与获取的数据的单位相同
    */
    Point2f predict_vel(double time);

    /**
    * @brief PredictModel获取预测目标像素位置方法，在调用predict()方法后更新
    */
    Point2f get_predict_position();
};

//较高精度的预测模型，需要获取足够的数据后才可用
class PredictModelFit {
private:
    FParam tp_data, ti_data;            //收集到的数据集
    DataSetType tp_data_set;            //收集到的数据的Set
    FParam now_param;                   //当前模型参数
    FParam step, lb, ub;                //初始训练步长，参数上界，参数下界

    Point2f now_center, now_target;     //当前能量机关中心点，当前目标位置

    long double max_sec;                        //获取的数据集从开始到结束的时间之差
    long double now_start_time, now_time;       //当前模型所用数据集的起始时刻，当前时刻
    bool data_collected;                        //标记数据集是否获取完毕

    double predict_angle = 0, now_angle = 0;    //预测角度，当前角度
    double now_path_radius = 1;                 //当前能量机关臂长
public:

    bool model_available;                       //标记模型是否已可用

    /**
    * @brief PredictModelFit类构造函数
    */
    PredictModelFit();

    /**
    * @brief PredictModelFit数据获取方法
    * @param center 能量机关中心点
    * @param target 目标装甲板中心点
    * @param leap_center 用于计算旋转角度的稳定的旋转中心点
    * @param time 当前目标所处时刻，单位为可任意，但计算目标预测位置时所用单位必须与此相同(根据当前上下界的设置，若该上下界不是使用输入的数据的单位时的函数参数的上下界，可能会影响参数的计算)
    */
    void get_data(Point2f &center, Point2f &target, Point2f &leap_center, double time);

    /**
    * @brief PredictModelFit预测方法
    * @param time 更新目标time单位时间后的位置信息，所用单位与获取的数据的单位相同
    */
    double predict(double time);

    /**
    * @brief PredictModelFit速预测方法
    * @param time 计算目标time单位时间后的线速度，所用单位与获取的数据的单位相同
    */
    Point2f predict_vel(double time);

    /**
    * @brief PredictModelFit获取预测目标像素位置方法，在调用predict()方法后更新
    */
    Point2f get_predict_position();

    /**
    * @brief PredictModelFit类打印当前模型内数据和函数参数的方法
    */
    void show_data_and_params();
};

//动态拟合预测模型，测试最佳
class PredictModelDynFit {
private:
    DynFitter fitter;
    FParam now_param;                   //当前模型参数
    FParam lb, ub;                //参数上界，参数下界

    Point2f now_center, now_target;     //当前能量机关中心点，当前目标位置

    long double predict_angle = 0, now_angle = 0;    //预测角度，当前角度
    long double now_time, start_time;
    long double now_path_radius = 1;                 //当前能量机关臂长
public:
    PredictModelDynFit();

    /**
    * @brief PredictModelDynFit构造函数
    * @param learning_rate 学习率,测试0.9左右最佳
    * @param start_param 起始参数
    * @param lb 参数下确界
    * @param ub 参数上确界
    * @param max_sec 队列内数据最长存在时间，单位为秒
    */
    PredictModelDynFit(long double learning_rate, FParam start_param, FParam lb = LB_VAL, FParam ub = UB_VAL,
                       long double max_sec = 5.0l);

    ~PredictModelDynFit();

    /**
    * @brief PredictModelDynFit数据获取方法
    * @param center 能量机关中心点
    * @param target 目标装甲板中心点
    * @param leap_center 用于计算旋转角度的稳定的旋转中心点
    * @param time 当前目标所处时刻，单位为可任意，但计算目标预测位置时所用单位必须与此相同(根据当前上下界的设置，若该上下界不是使用输入的数据的单位时的函数参数的上下界，可能会影响参数的计算)
    */
    void get_data(const Point2f &center, const Point2f &target, const Point2f &leap_center, const double time);

    /**
    * @brief PredictModelDynFit预测方法
    * @param time 更新目标time单位时间后的位置信息，所用单位与获取的数据的单位相同
    */
    double predict(double time);

    /**
    * @brief PredictModelDynFit速度预测方法
    * @param time 计算目标time单位时间后的线速度，所用单位与获取的数据的单位相同
    */
    Point2f predict_vel(double time);

    /**
    * @brief PredictModelDynFit获取预测目标像素位置方法，在调用predict()方法后更新
    */
    Point2f get_predict_position();

    /**
    * @brief PredictModelDynFit类打印当前模型内数据和函数参数的方法
    */
    void show_data_and_params();
};

#endif //PREDICT_MODEL_INCLUDED