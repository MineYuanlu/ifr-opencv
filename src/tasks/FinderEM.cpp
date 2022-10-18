//
// Created by yuanlu on 2022/9/13.
//

#include "FinderEM.h"
#include "web/mongoose.h"
#include "../API.h"

namespace EM {


    void Tools::prepare(const XMat &src, XMat &dist) {
        XMat gray, img1, img2;

        __CV_NAMESPACE cvtColor(src, gray, COLOR_BayerRG2GRAY);//空间转换

        static const auto blurSize = Size(3, 3);
#if USE_GPU //平滑图像
        static const auto blurfilter = cv::cuda::createBoxFilter(CV_8UC1, CV_8UC1, blurSize);
        blurfilter->apply(gray, img1);
#else
        blur(gray, img1, blurSize);
#endif
        __CV_NAMESPACE threshold(img1, img2, 160, 255, THRESH_BINARY);//二值化

        static const auto dilateKernel = getStructuringElement(0, Size(5, 5));
#if USE_GPU //膨胀图像
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
        DEBUG_nowTime(t_0)

        Mat img1;
        USE_GPU_SELECT(cuda::GpuMat gpuMat,);
        USE_GPU_SELECT(gpuMat.upload(src),);

        DEBUG_nowTime(t_1)

        Tools::prepare(USE_GPU_SELECT(gpuMat, src), USE_GPU_SELECT(gpuMat, img1));

        DEBUG_nowTime(t_2)

        USE_GPU_SELECT(gpuMat.download(img1),);

        DEBUG_nowTime(t_3)
        vector<vector<Point>> contours;  //所有轮廓
        vector<Vec4i> hierarchy;         //轮廓关系

        findContours(img1, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);  //寻找轮廓

        DEBUG_nowTime(t_4)

        auto ti = findTargets(USE_GPU_SELECT(gpuMat, img1), contours, hierarchy); //寻找目标

        DEBUG_nowTime(t_5);
        if (ti.findTarget) {
            auto x = ti.nowTarget.center - ti.nowTargetAim.center;
        }

        DEBUG_nowTime(t_6)

#if DEBUG_TIME || DEBUG_IMG
        fps = 1 / ((t_6 - t_0) / getTickFrequency());
#endif
#if DEBUG_TIME
        times["总时间"] = (t_end - t_0) / getTickFrequency() * 1000;
        times["上传"] = (t_1 - t_0) / getTickFrequency() * 1000;
        times["预处理"] = (t_2 - t_1) / getTickFrequency() * 1000;
        times["下载"] = (t_3 - t_2) / getTickFrequency() * 1000;
        times["轮廓"] = (t_4 - t_3) / getTickFrequency() * 1000;
        times["寻找"] = (t_5 - t_4) / getTickFrequency() * 1000;
        times["圆心"] = (t_6 - t_5) / getTickFrequency() * 1000;
#endif
#if DEBUG_IMG
        ifr::ImgDisplay::setDisplay("finder src " + to_string(thread_id), [src]() -> cv::Mat {
            return src;
        });
        ifr::ImgDisplay::setDisplay("finder img " + to_string(thread_id), [this, img1, ti]() -> cv::Mat {
            auto imgShow = img1.clone();
            Tools::c1to3(imgShow);
            putText(imgShow, "fps: " + to_string(fps), Point(0, 50),
                    FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 0));
            putText(imgShow, "active: " + to_string(ti.activeCount), Point(0, 100),
                    FONT_HERSHEY_COMPLEX, 1,
                    Scalar(0, ti.activeCount > 0 ? 255 : 0, ti.activeCount > 0 ? 0 : 255));
            if (ti.findTarget) {
                auto size = ti.nowTarget.size;
                putText(imgShow,
                        to_string((unsigned long) size.area()) + " " +
                        to_string(max(size.width / size.height, size.height / size.width)),
                        ti.nowTarget.center,
                        FONT_HERSHEY_COMPLEX, 0.8,
                        Scalar(255, 255, 100));
            }
            Tools::drawRotatedRect(imgShow, ti.nowTarget);
            Tools::drawRotatedRect(imgShow, ti.nowTargetAim);
            return imgShow;
        });
#endif
        ti.size = img1.size();
        return ti;
    }

#if USE_GPU

    void Tools::c1to3(cv::cuda::GpuMat &mat) {
        if (mat.channels() == 3) return;
        if (mat.channels() != 1) throw invalid_argument("通道数不等于1或3");
        /**3通道缓冲区*/
        vector<cv::cuda::GpuMat> channels(3, mat);
        cv::cuda::merge(channels, mat);
    }

#endif

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
        vector<RotatedRect> cache(contours.size());//rr矩形缓存
        vector<int> othersIndex;//其它轮廓的index (即除2种扇叶及其子集外的轮廓)
        othersIndex.reserve(contours.size());
        while (!que.empty()) {
            for (auto index = que.front(); index != -1; index = hierarchy[index][0]) {//逐层遍历
                bool hasFound = true;  //是否找到一个目标 (找到目标后将不再遍历子范围)
                if (assets->activeMatcher.getDistance(contours, hierarchy, index, cache) < threshold) {
                    ti.activeCount++;
                    const auto &rr = Tools::getRrect(contours, cache, index);
                } else if (assets->targetMatcher.getDistance(contours, hierarchy, index, cache) < threshold) {
                    ti.findTarget = true;
                    ti.nowTarget = minAreaRect(contours[index]);
                    ti.nowTargetAim = assets->targetMatcher.getFirstSubRRect(contours, hierarchy, index, cache);
//                    ti.nowTargetAim = minAreaRect(contours[hierarchy[index][2]]);
                    const auto &rr = Tools::getRrect(contours, cache, index);
                } else {
                    hasFound = false;
                    othersIndex.push_back(index);
                }
                if (!hasFound && hierarchy[index][2] != -1) que.push(hierarchy[index][2]);
            }
            que.pop();
        }

        //在找到目标装甲臂的情况下, 找到 中心R标
        if (ti.findTarget) {
            // 目标点到直线的距离 http://t.zoukankan.com/ggYYa-p-6038939.html
            const auto pt1 = ti.nowTargetAim.center, pt2 = ti.nowTarget.center;
            const auto A = pt2.y - pt1.y, B = pt1.x - pt2.x, C = pt2.x * pt1.y - pt1.x * pt2.y;
            const auto sA2B2 = sqrt(A * A + B * B);

            //目标点是否在风车臂的另一侧, 并计算距离
            const auto pt1_ = 2 * pt2 - pt1;
            const auto vec11 = pt1_ - pt1;
            const auto s11 = sqrt(vec11.dot(vec11));

            const auto aimLength = 1.5F * max(ti.nowTargetAim.size.width, ti.nowTargetAim.size.height);// 装甲板最长边1.5倍
            const auto tarSize = ti.nowTarget.size.area() / 100;// 风车臂面积的百分之一
            struct IndexAndWeight {
                float w;
                int index;
            };
            vector<IndexAndWeight> iaw;
            for (const auto &index: othersIndex) {
                const auto &rr = Tools::getRrect(contours, cache, index);
                if (rr.size.width > aimLength || rr.size.height > aimLength)continue;//过滤掉面积过大的物体
                if (rr.size.area() < tarSize)continue;//过滤掉面积过小的物体

                const auto vec13 = rr.center - pt1;
                auto α = vec11.dot(vec13);

                if (α < 0)continue;//位于 风车臂近底部 以上 (即过 装甲板中心点关于风车臂中心点的对称点 做垂直于风车臂中心线的垂线 以上的部分)


                auto disPt = α / s11;//线上的投影点 到 装甲板关于风车臂中心点的对称点 的距离
                const auto disLine = abs(A * rr.center.x + B * rr.center.y + C) / sA2B2; //点到直线的距离

                static const float weightPt = 3;//Pt 距离 的权重
                static const float weightLine = 1;//Line 距离 的权重

                iaw.push_back({disPt * weightPt + disLine * weightLine, index});
            }
            //
            const auto index = std::min_element(iaw.begin(), iaw.end(),
                                                [](const auto &a, const auto &b) { return a.w < b.w; })->index;
            ti.emCenter = Tools::getRrect(contours, cache, index).center;
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

#if USE_GPU
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
#endif

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