//
// Created by yuanlu on 2022/9/13.
//

#ifndef OPENCV_TEST_FINDEREM_H
#define OPENCV_TEST_FINDEREM_H


#include <iostream>
#include <opencv2/opencv.hpp>
#include <random>
#include "../defs.h"
#include "../Record.h"
#include "../ImgDisplay.h"
#include "../semaphore.h"
#include "plan/Plans.h"
#include "msg/msg.hpp"
#include "data-waiter/DataWaiter.h"


#if USE_GPU

#include <opencv2/core/cuda.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudafeatures2d.hpp>

#endif

using namespace std;
using namespace cv;

#if USE_GPU
typedef cv::cuda::GpuMat XMat;
#define __CV_NAMESPACE cv::cuda::
#else
typedef cv::Mat XMat;
#define __CV_NAMESPACE cv::
#endif


/**
 * 能量机关相关程序
 */
namespace EM {


    /**随机数引擎*/
    static default_random_engine e(time(nullptr));


    /**
     * 工具函数
     */
    namespace Tools {

        /**
         * 预处理图像, 原图->亮度的二值图->膨胀
         * @param src 原图
         * @param dist 输出图
         * @param type 原始图像类型 详见 datas::FrameType
         */
        void prepare(const XMat &src, XMat &dist, int type);

#if USE_GPU
        /**
         * 单通道转3通道
         * @param mat
         */
        void c1to3(cv::cuda::GpuMat &mat);
#endif

        /**
         * 单通道转3通道
         * @param mat
         */
        void c1to3(Mat &mat);

#if DEBUG_IMG

        /**
         * 在图像上绘制轮廓点
         * @param contours 轮廓点
         * @param mat 图像
         * @param drawRect 是否绘制最小的旋转矩形, 默认为true,
         *                  如果为false, 则绘制每个点
         */
        void drawContours(const vector<vector<Point>> &contours, Mat &mat, bool drawRect = true);

#if USE_GPU
        /**
         * 绘制旋转矩形
         * @param box 旋转矩形
         */
        void drawRotatedRect(cv::cuda::GpuMat &mat, const RotatedRect &box);
#endif

        /**
         * 绘制旋转矩形
         * @param box 旋转矩形
         */
        void drawRotatedRect(cv::Mat &mat, const RotatedRect &box);

        /**
         * 保存所有框选出的轮廓, 并在输入的图像上描绘矩形框
         * @param img 图像
         * @param contours 轮廓点
         * @param hierarchy 轮廓关系
         */
        void saveSubs(Mat &img, const vector<vector<Point>> &contours, const vector<Vec4i> &hierarchy);

#endif

        /**
         * 欧氏距离
         * @param p1 Point
         * @param p2 Point
         * @return distance
         */
        double distance(const Point &p1, const Point &p2);

        /**
         * 执行仿射变换
         * @param src 输入图像
         * @param rect 区域
         * @param dist 输出图像
         * @param size 输出大小
         * @param fst 变换方向(正向/反向)
         */
        void affineTransform(InputArray &src, const RotatedRect &rect, OutputArray &dist, const Size &size, bool fst);


        /**
         * 整理旋转区域的点顺序
         * @param src 输入
         * @param dist 输出
         */
        void format_rrect(Point2f *src, Point2f *dist);

        /**
         * 使用缓存获取旋转矩形框(带缓存)
         */
        static const RotatedRect &
        getRrect(const vector<vector<Point>> &contours, vector<RotatedRect> &cache, const size_t index) {
            if (cache[index].size.area() <= 0)cache[index] = minAreaRect(contours[index]);
            return cache[index];
        }
    }

    /**
     * 匹配器
     */
    class Matcher {
    private :
        vector<RotatedRect> rotatedRect;
        const float ignoreFactor = 100;
    public:
        explicit Matcher(const vector<vector<Point>> &contours) {
            rotatedRect = vector<RotatedRect>(contours.size());
            for (size_t i = 0; i < contours.size(); i++)rotatedRect[i] = minAreaRect(contours[i]);
            const auto maxSize = std::max_element(rotatedRect.begin(), rotatedRect.end(),
                                                  [](const auto &a, const auto &b) {
                                                      return a.size.area() < b.size.area();
                                                  })->size.area() / 2000;
            rotatedRect.erase(std::remove_if(rotatedRect.begin(), rotatedRect.end(), [&](const auto &item) {
                return item.size.area() < maxSize;
            }), rotatedRect.end());
        }


        /**
         * 获取与模板的距离
         *
         * TODO 暂时仅可处理单层关系, (对于能量机关足够)
         * @param contours 轮廓点
         * @param hierarchy 结构
         * @param index 目标
         * @param cache minAreaRect缓存, 通过此向量重复获取同一个轮廓的最小框
         * @return 相似度: 越大越相似
         */
        double getDistance(const vector<vector<Point>> &contours, const vector<Vec4i> &hierarchy,
                           size_t index, vector<RotatedRect> &cache) {
            if (cache.size() < hierarchy.size())cache.resize(hierarchy.size());
            const auto box = Tools::getRrect(contours, cache, index);
            const auto S = box.size.area();

            size_t cnt = 1;
            for (auto i = hierarchy[index][2]; i >= 0; i = hierarchy[i][0]) {
                const auto rr = Tools::getRrect(contours, cache, i);
                if (rr.size.area() * ignoreFactor < S)continue;
                cnt++;
            }
            if (cnt == rotatedRect.size())return 0;
            //TODO 过滤掉比例过小的区域(例如面积小于总体的5%)
            //TODO 比较框的数量, 若不一致则直接返回INF(不相似), 或赋极高误差值
            //TODO 按照比例映射每个点, 计算每个模板框点到最近的实际框点的距离, 将距离和作为误差值
            return INFINITY;
        }


        /**
         * 获取首个子区域 (忽略过小区域)
         * @param contours 轮廓点
         * @param hierarchy 结构
         * @param index 目标
         * @param cache minAreaRect缓存, 通过此向量重复获取同一个轮廓的最小框
         * @return 旋转矩形
         */
        const RotatedRect getFirstSubRRect(const vector<vector<Point>> &contours, const vector<Vec4i> &hierarchy,
                                           size_t index, vector<RotatedRect> &cache) {
            if (cache.size() < hierarchy.size())cache.resize(hierarchy.size());
            const auto S = Tools::getRrect(contours, cache, index).size.area();
            for (auto i = hierarchy[index][2]; i >= 0; i = hierarchy[i][0]) {
                const auto rr = Tools::getRrect(contours, cache, i);
                if (rr.size.area() * ignoreFactor < S)continue;
                return rr;
            }
            return RotatedRect();
        }

    };


    /**
     * 资源数据
     */
    class Assets {
    public:
#if __OS__ == __OS_Linux__
        /**资源文件夹*/
        const std::string dir = std::string(R"(/home/nvidia/2022-yuanlu/assets/)");
#elif __OS__ == __OS_Windows__
        /**资源文件夹*/
        const std::string dir = std::string(R"(D:\yuanlu\dev\cpp\ifr-opencv\assets\)");
#endif
        Mat active = imread(dir + "active.png", IMREAD_GRAYSCALE);
        Size activeSize = active.size();
        Mat activeHist = getHist(active);
        Matcher activeMatcher = getMatcher(active);

        Mat target = imread(dir + "target.png", IMREAD_GRAYSCALE);
        Size targetSize = target.size();
        Mat targetHist = getHist(target);
        Matcher targetMatcher = getMatcher(target);


        Assets() {
            OUTPUT("加载assets...")
        }

    private:

        static Matcher getMatcher(const Mat &mat);

        static Mat getHist(const Mat &src) {
            static const int channels[1] = {0};
            static const float inRanges[2] = {0, 255};
            static const float *ranges[1] = {inRanges};
            static const int bins[1] = {256};
            static const Mat mask = Mat();
            Mat hist;
            calcHist(&src, 1, channels, Mat(), hist, 1, bins, ranges);
            return hist;
        }
    };


    /**
     * 寻找目标
     */
    class Finder {
    public:
#if DEBUG_TIME || DEBUG_IMG
        /**处理帧率*/
        double fps = 0;
#endif
        shared_ptr<Assets> assets;
        const int thread_id;//处理线程ID
    public:
        /**
         * 运行
         * @param src 原始图像
         * @param type 原始图像类型 详见 datas::FrameType
         */
        datas::TargetInfo run(const Mat &src, int type);


        Finder(int id, shared_ptr<Assets> assets) : assets(assets), thread_id(id) {
#if DEBUG_IMG
            namedWindow("finder img " + to_string(id), WINDOW_NORMAL);
            namedWindow("finder src " + to_string(id), WINDOW_NORMAL);
#endif
        }

    private:

        /**
         * 寻找目标
         * @param mat 图片
         * @param contours 轮廓点
         * @param hierarchy 轮廓关系
         * @param activeCount [输出] 激活的数量
         * @param nowTarget [输出] 当前风扇叶位置
         * @param nowTargetAim [输出] 当前装甲板位置
         * @param threshold 相似度阈值
         */
        datas::TargetInfo
        findTargets(const XMat &mat, const vector<vector<Point>> &contours, const vector<Vec4i> &hierarchy,
                    double threshold = 1e6);

        /**
         * 获取相似度
         * @param mat 原始图像
         * @param rect 原始图像上的区域
         * @param toSize 比较图片的大小
         * @param fst 变换方向(正向/反向)
         * @param target 比较图片
         * @return 相似度
         * @deprecated 使用Matcher
         */
        inline double getSimilarity(const Mat &mat, const RotatedRect &rect, const Size &toSize, bool fst,
                                    const Mat &target);

    public:
        /**
         * 注册任务
         */
        static void registerTask() {
            static const std::string io_src = "src";
            static const std::string io_output = "target";
            static const std::string arg_thread_amount = "thread amount";
            ifr::Plans::TaskDescription description{"find", "能量机关目标识别"};
            description.io[io_src] = {TYPE_NAME(datas::FrameData), "输入的图像数据", true};
            description.io[io_output] = {TYPE_NAME(datas::TargetInfo), "输出的目标信息", false};
            description.args[arg_thread_amount] = {"识别线程数量", "1", ifr::Plans::TaskArgType::NUMBER};

            ifr::Plans::registerTask("FinderEM", description, [](auto io, auto args, auto state, auto cb) {
                ifr::Plans::Tools::waitState(state, 1);

                const auto finder_thread_amount = stoi(args[arg_thread_amount]);
                //初始化
                shared_ptr<Assets> assets_ptr(new Assets());    //资源
                vector<shared_ptr<Finder>> finders;             //识别器
                finders.reserve(finder_thread_amount);
                for (int i = 0; i < finder_thread_amount; ++i)
                    finders.push_back(shared_ptr<Finder>(new Finder(i, assets_ptr)));
                unique_ptr<thread[]> finder_threads(new thread[finder_thread_amount]); //识别线程


                //运行
                const auto &cname_src = io[io_src].channel;
                ifr::DataWaiter<uint64_t, datas::TargetInfo> dw;  //数据整合


                for (const auto &finder: finders) {//识别线程
                    finder_threads[finder->thread_id] = thread([&cname_src, &dw, &finder, &state]() {
                        ifr::Msg::Subscriber<datas::FrameData> fdIn(cname_src);
                        ifr::Plans::Tools::waitState(state, 2);
                        while (*state < 3) {
                            try {
                                const auto data = fdIn.pop_for(COMMON_LOOP_WAIT);
                                dw.start(data.id);
                                auto ti = finder->run(data.mat, data.type);
                                ti.time = data.time;
                                ti.receiveTick = data.receiveTick;
                                dw.finish(data.id, ti);
                            } catch (ifr::Msg::MessageError_NoMsg &x) {
                                OUTPUT("[FinderEM] Finder" + to_string(finder->thread_id) + "输出数据等待超时 " +
                                       std::to_string(COMMON_LOOP_WAIT) + "ms")
                            } catch (ifr::Msg::MessageError_Broke &) {
                                break;
                            }
                        }
                    });
                    while (!finder_threads[finder->thread_id].joinable());
                }


                ifr::Msg::Publisher<datas::TargetInfo> tiOut(io[io_output].channel);//发布者
                ifr::Plans::Tools::finishAndWait(cb, state, 1);

                tiOut.lock();

                cb(2);
                while (*state < 3) {
                    try {
                        const auto data = dw.pop_for(COMMON_LOOP_WAIT);
                        tiOut.push(data.second);
                    } catch (ifr::DataWaiter_Timeout &) {
                        OUTPUT("[FinderEM] DW 输出数据等待超时 " + std::to_string(COMMON_LOOP_WAIT) + "ms")
                    }
                }
                for (int i = 0; i < finder_thread_amount; ++i)finder_threads[i].join();//等待识别线程退出
                ifr::Plans::Tools::waitState(state, 3);
                ifr::Plans::Tools::finishAndWait(cb, state, 3);
                //auto release && cb(4)

            });
        }
    };
}
#endif //OPENCV_TEST_FINDEREM_H
