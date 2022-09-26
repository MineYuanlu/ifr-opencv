//
// Created by yuanlu on 2022/9/13.
//

#include "FinderEM.h"

namespace EM {


    void Tools::prepare(const XMat &src, XMat &dist) {
        XMat gray, img1, img2;

        __CV_NAMESPACE cvtColor(src, gray, COLOR_BayerRG2GRAY);//空间转换

        static const auto blurSize = Size(5, 5);
#if USE_GPU //平滑图像
        static const auto blurfilter = cv::cuda::createBoxFilter(CV_8UC1, CV_8UC1, blurSize);
        blurfilter->apply(gray, img1);
#else
        blur(gray, img1, blurSize);
#endif
        __CV_NAMESPACE threshold(img1, img2, 160, 255, THRESH_BINARY);//二值化

        static const auto dilateKernel = getStructuringElement(0, Size(7, 7));
#if USE_GPU //平滑图像
        static const auto dilateFilter = cuda::createMorphologyFilter(MORPH_DILATE, CV_8UC1, dilateKernel,
                                                                      Point(-1, -1), 1);
        dilateFilter->apply(img2, dist);
#else
        dilate(img2, dist, dilateKernel, Point(-1, -1), 1);  //膨胀
#endif
    }

    datas::TargetInfo Finder::run(const Mat &src
#if DEBUG_TIME
            , map<string, double> &times
#endif
    ) {
        auto t_start = getTickCount();
        Mat img1;
        cuda::GpuMat gpuMat;
        DEBUG_nowTime(t_0)
#if USE_GPU
        gpuMat.upload(src);
#endif
        DEBUG_nowTime(t_1)
#if USE_GPU
        Tools::prepare(gpuMat, gpuMat);
#else
        Tools::prepare(src, img1);
#endif
        DEBUG_nowTime(t_2)
#if USE_GPU
        gpuMat.download(img1);
#endif
        DEBUG_nowTime(t_3)
        vector<vector<Point>> contours;  //所有轮廓
        vector<Vec4i> hierarchy;         //轮廓关系

        findContours(img1, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);  //寻找轮廓

        DEBUG_nowTime(t_4)
        auto ti = findTargets(     //寻找目标
#if USE_GPU
                gpuMat
#else
                img1
#endif
                , contours, hierarchy);
        DEBUG_nowTime(t_5)

#if DEBUG_TIME || DEBUG_IMG
        fps = 1 / ((t_5 - t_0) / getTickFrequency());
#endif
#if DEBUG_TIME
        times["总时间"] = (t_end - t_0) / getTickFrequency() * 1000;
        times["上传"] = (t_1 - t_0) / getTickFrequency() * 1000;
        times["预处理"] = (t_2 - t_1) / getTickFrequency() * 1000;
        times["下载"] = (t_3 - t_2) / getTickFrequency() * 1000;
        times["轮廓"] = (t_4 - t_3) / getTickFrequency() * 1000;
        times["寻找"] = (t_5 - t_4) / getTickFrequency() * 1000;
#endif
#if DEBUG_IMG
        Mat imgShow = img1.clone();
        Tools::c1to3(imgShow);
        putText(imgShow, "fps: " + to_string(fps), Point(0, 50),
                FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 0));
        putText(imgShow, "sub_fps: " + to_string(1 / ((t_1 - t_0) / getTickFrequency())) + ", " +
                         to_string(1 / ((t_2 - t_1) / getTickFrequency())) + ", " +
                         to_string(1 / ((t_3 - t_2) / getTickFrequency())),
                Point(0, 100),
                FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 0));
        putText(imgShow, "active: " + to_string(ti.activeCount), Point(0, 150),
                FONT_HERSHEY_COMPLEX, 1,
                Scalar(0, ti.activeCount > 0 ? 255 : 0, ti.activeCount > 0 ? 0 : 255));
        Tools::drawRotatedRect(imgShow, ti.nowTarget);
        Tools::drawRotatedRect(imgShow, ti.nowTargetAim);
        m_imshow("img", imgShow);
        waitKey(20);
#endif
        auto t_end = getTickCount();
        ti.size = img1.size();
        ti.delay = (t_end - t_start) / getTickFrequency() * 1000;
        return ti;
    }

    void Tools::c1to3(cv::cuda::GpuMat &mat) {
        if (mat.channels() == 3) return;
        if (mat.channels() != 1) throw invalid_argument("通道数不等于1或3");
        /**3通道缓冲区*/
        vector<cv::cuda::GpuMat> channels(3, mat);
        cv::cuda::merge(channels, mat);
    }

    void Tools::c1to3(cv::Mat &mat) {
        if (mat.channels() == 3) return;
        if (mat.channels() != 1) throw invalid_argument("通道数不等于1或3");
        /**3通道缓冲区*/
        vector<cv::Mat> channels(3, mat);
        cv::merge(channels, mat);
    }

    void Tools::affineTransform(InputArray &src, const RotatedRect &rect, OutputArray &dist,
                                const Size &size, bool fst) {
        Point2f srcTri[3];
        Point2f dstTri[3];
        Mat matrix;
        Point2f points[4];
        // src
        rect.points(points);
        // dist
        dstTri[0] = Point2f(0, 0);
        dstTri[1] = Point2f(size.width - 1, 0);
        dstTri[2] = Point2f(0, size.height - 1);

        //整理
        format_rrect(points, points);

        if (fst) srcTri[0] = points[0], srcTri[1] = points[1], srcTri[2] = points[3];
        else srcTri[0] = points[2], srcTri[1] = points[3], srcTri[2] = points[1];

        matrix = getAffineTransform(srcTri, dstTri);
        __CV_NAMESPACE warpAffine(src, dist, matrix, size);
    }

    void Tools::format_rrect(Point2f *src, Point2f *dist) {
        const auto pi = src[1] - src[0];
        const auto pj = src[3] - src[0];
        const auto r_i = std::abs(pi.x) + std::abs(pi.y);
        const auto r_j = std::abs(pj.x) + std::abs(pj.y);

        if (r_j > r_i) {
            const auto pt = src[1];
            dist[1] = src[3];
            dist[3] = pt;
        }
    }

    double Tools::distance(const Point &p1, const Point &p2) {
        return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
    }

    datas::TargetInfo Finder::findTargets(const XMat &mat, const vector<vector<Point>> &contours,
                                          const vector<Vec4i> &hierarchy, double threshold) {
        datas::TargetInfo ti = {0, false};
        queue<int> que;
        for (size_t i = 0; i < hierarchy.size(); i++)
            if (hierarchy[i][3] == -1) {  //寻找顶级第一个
                auto h = hierarchy[i];
                while (h[1] != -1) h = hierarchy[i = h[1]];
                que.push((int) i);
                break;
            }
        vector<RotatedRect> cache(contours.size());
        while (!que.empty()) {
            for (auto index = que.front(); index != -1; index = hierarchy[index][0]) {
                bool hasFound = true;  //是否找到一个目标 (找到目标后将不再遍历子范围)
                if (assets.activeMatcher.getDistance(contours, hierarchy, index, cache) < threshold) {
                    ti.activeCount++;
                } else if (assets.targetMatcher.getDistance(contours, hierarchy, index, cache) < threshold) {
                    ti.findTarget = true;
                    ti.nowTarget = minAreaRect(contours[index]);
                    ti.nowTargetAim = minAreaRect(contours[hierarchy[index][2]]);
                } else {
                    hasFound = false;
                }
                if (!hasFound && hierarchy[index][2] != -1) que.push(hierarchy[index][2]);
            }
            que.pop();
        }
        return ti;
    }

    double Finder::getSimilarity(const Mat &mat, const RotatedRect &rect,
                                 const Size &toSize, const bool fst,
                                 const Mat &target) {
        Mat sub;  //仿射变换得到的区域
        Tools::affineTransform(mat, rect, sub, toSize, fst);
        absdiff(sub, target, sub);
        return (cv::sum(sub))[0];
    }

#if DEBUG_IMG

    void Tools::drawContours(const vector<vector<Point>> &contours, Mat &mat, bool drawRect) {
        c1to3(mat);
        for (const auto &c: contours) {
            if (drawRect) {
                drawRotatedRect(mat, minAreaRect(c));


//                uint64_t subs = 0;
//                size_t sub = hierarchy[index][2];
//                while (sub != -1) subs++, sub = hierarchy[sub][0];
//                putText(img, to_string(subs)
//                             + ": " + to_string(rect.size.area())
////                         + ": " + to_string(rect.size.width) + "_" + to_string(rect.size.height)
//                        ,
//                        points[1], FONT_HERSHEY_COMPLEX, 0.5,
//                        Scalar(0, 99, 99));
            } else {
                const auto color = Scalar(e() % 255, e() % 255, e() % 255);
                for (const auto &p: c) {
                    circle(mat, p, 2, color, -1);  //画点，其实就是实心圆
                }
            }
        }
    }

    void Tools::drawRotatedRect(cv::cuda::GpuMat &mat, const RotatedRect &box) {
        /**4点缓冲区*/
        Point2f points[4];
        c1to3(mat);
        box.points(points);
        for (int i = 0; i < 4; i++) {
            line(mat, points[i], points[(i + 1) % 4],
                 cv::Scalar(255, 100, 200), 2, LINE_AA);
        }
    }

    void Tools::drawRotatedRect(cv::Mat &mat, const RotatedRect &box) {
        /**4点缓冲区*/
        Point2f points[4];
        c1to3(mat);
        box.points(points);
        for (int i = 0; i < 4; i++) {
            line(mat, points[i], points[(i + 1) % 4],
                 cv::Scalar(255, 100, 200), 2, LINE_AA);
        }
    }

    void Tools::saveSubs(Mat &img, const vector<vector<Point>> &contours,
                         const vector<Vec4i> &hierarchy) {
        Mat img2;
        int i = 0;
        for (size_t index = 0; index < contours.size(); index++) {
            const auto rect = minAreaRect(contours[index]);
            static cv::Point2f points[4];
            rect.points(points);
            auto h = distance(points[0], points[1]);
            auto w = distance(points[1], points[2]);
            if (h > w) swap(h, w);
            affineTransform(img, rect, img2, Size(w, h), true);

            imwrite(assets.dir + "save\\" + to_string(i++) + ".png", img2);
        }
    }

#endif

    Matcher Assets::getMatcher(const Mat &mat) {
        Mat dist;
        dilate(mat, dist, getStructuringElement(0, Size(3, 3)), Point(-1, -1), 1);  //膨胀

        vector<vector<Point>> contours;  //所有轮廓
        vector<Vec4i> hierarchy;         //轮廓关系
        findContours(dist, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);  //寻找轮廓
#if DEBUG_IMG
        //        Tools::drawContours(contours, dist);
        //        m_imshow("dist", dist);
        //        cout << "框数: " << contours.size();
        //        for (const auto &c: contours)cout << ", " << minAreaRect(c).size;
        //        cout << endl;
        //        waitKey();
#endif
        return Matcher(contours);
    }

}  // namespace EM