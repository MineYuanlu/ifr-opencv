//
// Created by yuanlu on 2022/11/9.
//

#include "FinderArmor.h"

namespace Armor {
    /**
     * @brief 轮廓对, 代表2个灯条的组合
     * @details 代表一个可能的装甲板, 仅供内部使用
     */
    struct ContourPair {
        size_t i1, i2;//轮廓下标
        float w, h;//装甲板尺寸
        float angle;//装甲板旋转角度(0~180)
        bool is_large;//是否是大装甲板
        float bad;//坏分, 越大代表越不可能是装甲板

        bool skip;//是否要跳过(内部包含其它轮廓对)
        cv::RotatedRect rr;//最小包围矩形
    };

    /**单通道转3通道*/
    FORCE_INLINE void c1to3(cv::Mat &mat) {
        if (mat.channels() == 3) return;
        if (mat.channels() != 1) throw std::invalid_argument("通道数不等于1或3");
        /**3通道缓冲区*/
        std::vector<cv::Mat> channels(3, mat);
        cv::merge(channels, mat);
    }

    /**@return rr的实际角度(0~180)*/
    FORCE_INLINE float getRealAngle(const cv::RotatedRect &rr) {
        if (rr.size.height > rr.size.width)return rr.angle + 90;
        else return rr.angle;
    }

    /**@return 长宽比(>=1)*/
    FORCE_INLINE float getAspectRatio(const cv::Size2f &s) {
        return (s.width > s.height) ? (s.width / s.height) : (s.height / s.width);
    }

    /**
     * @param f1 实际角度(0~180)
     * @param f2 实际角度(0~180)
     * @return 角度差(0~90)
     * */
    FORCE_INLINE float getAngleDistance(const float &f1, const float &f2) {
        return 90 - abs(abs(f1 - f2) - 90);
    }

    /**
     * 计算两点距离
     * @tparam Tp 数值类型
     * @param l 点1
     * @param r 点2
     * @return 距离
     */
    template<typename Tp>
    FORCE_INLINE Tp distanceSquare(const cv::Point_<Tp> &l, const cv::Point_<Tp> &r) {
        const auto v = l - r;
        return v.dot(v);
    }


    FORCE_INLINE void
    drawRotatedRect(const cv::Mat &mask, const cv::RotatedRect &rr, const cv::Scalar &color, int thickness = 1,
                    int lineType = cv::LineTypes::LINE_8) {
        cv::Point2f ps[4];
        rr.points(ps);
        std::vector<std::vector<cv::Point>> tmpContours;
        std::vector<cv::Point> contours;
        for (const auto &p: ps) { contours.emplace_back(cv::Point2i(p)); }
        tmpContours.insert(tmpContours.end(), contours);
        drawContours(mask, tmpContours, 0, color, thickness, lineType);
    }

    template<typename T, size_t n>
    FORCE_INLINE void
    drawRotatedRect(const cv::Mat &mask, const datas::Polygon<T, n> &poly, const cv::Scalar &color, int thickness = 1,
                    int lineType = cv::LineTypes::LINE_8) {
        std::vector<std::vector<cv::Point>> tmpContours;
        std::vector<cv::Point> contours;
        for (const auto &p: poly.points) { contours.emplace_back(cv::Point2i(p)); }
        tmpContours.insert(tmpContours.end(), contours);
        drawContours(mask, tmpContours, 0, color, thickness, lineType);
    }


    /**
      * @brief 整理点的顺序
      *
      * @param src 旋转矩形的四个角点
      * @param center 旋转矩形的中心点
      * @param angle 装甲板的角度(0~180)
      */
    FORCE_INLINE void format_rrect(cv::Point2f *src, cv::Point2f *dst, const cv::Point2f &center, float angle) {
        static const auto d2r = (float) (CV_PI / 180.0);
        const auto angle_cos = std::cos(angle * d2r), angle_sin = std::sin(angle * d2r);
        cv::Point2f vec_a, dri_vec(angle_cos, angle_sin), dri_vec_vert(angle_sin, -angle_cos);
        float a_dot_dri[4], a_dot_dri_vert[4];

        for (int i = 0; i < 4; i++) {
            vec_a = src[i] - center;
            a_dot_dri[i] = vec_a.dot(dri_vec);
            a_dot_dri_vert[i] = vec_a.dot(dri_vec_vert);
        }
        for (int i = 0; i < 4; i++) {
            if (a_dot_dri[i] > 0 && a_dot_dri_vert[i] > 0) dst[0] = src[i];
            if (a_dot_dri[i] > 0 && a_dot_dri_vert[i] <= 0) dst[1] = src[i];
            if (a_dot_dri[i] <= 0 && a_dot_dri_vert[i] <= 0) dst[2] = src[i];
            if (a_dot_dri[i] <= 0 && a_dot_dri_vert[i] > 0) dst[3] = src[i];
        }
    }

    /**
     * 执行仿射变换
     * @param src 输入图像
     * @param rect 区域
     * @param dist 输出图像
     * @param size 输出大小
     * @param angle 装甲板的角度(0~180)
     */
    static void
    affineTransform(cv::InputArray &src, const cv::RotatedRect &rect, cv::OutputArray &dist, const cv::Size &size,
                    float angle) {
        cv::Point2f srcTri[3], dstTri[3], points[8];
        cv::Mat matrix;
        // src
        rect.points(points + 4);
        //整理
        format_rrect(points + 4, points, rect.center, angle);
        srcTri[0] = points[0], srcTri[1] = points[1], srcTri[2] = points[3];

        // dist
        dstTri[0] = cv::Point2f((float) size.width - 1, (float) size.height - 1);
        dstTri[1] = cv::Point2f(0, (float) size.height - 1);
        dstTri[2] = cv::Point2f((float) size.width - 1, 0);

        matrix = cv::getAffineTransform(srcTri, dstTri);
        __CV_NAMESPACE warpAffine(src, dist, matrix, size);
    }

    /**
     * 获取灯条一侧的中心点
     * @param light 灯条旋转矩形
     * @param ps 灯条角点
     * @param center 装甲板中心点
     * @return 灯条靠近装甲板一侧的中心点
     */
    FORCE_INLINE cv::Point2f
    getLightSideCenter(const cv::RotatedRect &light, const std::vector<cv::Point2f> &ps, const cv::Point2f &center) {
        size_t min_i = 0;//最近点下标
        auto min_d = distanceSquare(center, ps[min_i]);//最近点距离平方
        for (size_t i = 1; i < 4; i++) {
            const auto d = distanceSquare(center, ps[i]);
            if (d < min_d)min_d = d, min_i = i;
        }
        if (light.size.width < light.size.height) {
            if (min_i == 0 || min_i == 1)return (ps[0] + ps[1]) / 2;
            else return (ps[2] + ps[3]) / 2;
        } else {
            if (min_i == 1 || min_i == 2)return (ps[1] + ps[2]) / 2;
            else return (ps[0] + ps[3]) / 2;
        }
    }
    /**
     * 获取两个旋转矩形的内部矩形
     * @param r1 rr1
     * @param r2 rr2
     * @return 包围2个旋转矩形内部区域的旋转矩形
     */
    FORCE_INLINE cv::RotatedRect getInnerRR(
#if DEBUG_VIDEO_FA || DEBUG_IMG_FA
            const cv::Mat &mat,
#endif
            const cv::RotatedRect &r1, const cv::RotatedRect &r2,
            const std::vector<std::vector<cv::Point2f>> &rr_points, const size_t &ri1, const size_t &ri2,
            float set_height) {
        cv::Point2f center(0, 0);
        for (const auto &point: rr_points[ri1]) center += point;
        for (const auto &point: rr_points[ri2]) center += point;
        center /= 8;

        cv::Point2f ps[] = {
                getLightSideCenter(r1, rr_points[ri1], center),
                getLightSideCenter(r2, rr_points[ri2], center)
        };

#if DEBUG_VIDEO_FA || DEBUG_IMG_FA
        if (!mat.empty()) {
            cv::line(mat, ps[0], ps[1], cv::Scalar(255, 100, 255), 3);
            drawRotatedRect(mat, r1, cv::Scalar(255, 100, 100), 3);
            drawRotatedRect(mat, r2, cv::Scalar(255, 100, 100), 3);
            cv::circle(mat, center, 1, cv::Scalar(100, 255, 100), 3);
        }
#endif
        auto rr = cv::minAreaRect(std::vector<cv::Point2f>(ps, ps + 2));//装甲板 包围
        if (rr.size.height < rr.size.width) {//将高度设为装甲板的高度(原高度为0), 缩小宽度(减少两侧灯条)
            rr.size.height = set_height;
        } else {
            rr.size.width = set_height;
        }
        return rr;
    }

    namespace NetHelper {
        std::vector<std::string> getOutputsNames(const cv::dnn::Net &net) {
            std::vector<std::string> names;
            std::vector<int> out_layer_index = net.getUnconnectedOutLayers();//得到输出层的索引号
            std::vector<std::string> all_layers_names = net.getLayerNames();//得到网络中所有层的名称

            names.resize(out_layer_index.size());
            for (size_t i = 0; i < out_layer_index.size(); i++)
                names[i] = all_layers_names[out_layer_index[i] - 1];
            return names;
        }
    }


    datas::ArmTargetInfos FinderArmor::run(const datas::FrameData &data) {
        datas::ArmTargetInfos infos = {data.mat.size(), data.time, data.receiveTick};
        handler(data.mat, data.type, infos.targets);
        return infos;
    }

    namespace Values {
#if IFRAPI_HAS_VARIABLE && DEBUG
#define VALUES_PREFIX static
#define IFR_FAV(name, min, max) IFRAPI_VARIABLE(finder-armor,name,min,max);
#else
#define VALUES_PREFIX static const constexpr
#define IFR_FAV(name, min, max)
#endif

        VALUES_PREFIX int bmat_threshold = 100;//二值图的阈值


        VALUES_PREFIX float maxSizeRatio = 5000;//最大面积比(画面大小除以轮廓框大小), 超过此值则认为是噪声
        VALUES_PREFIX float minSizeRatio = 4;//最小面积比(画面大小除以轮廓框大小), 低于此值则认为非法框
        VALUES_PREFIX float maxAspectRatio = 50;//最大长宽比(灯条)
        VALUES_PREFIX float minAspectRatio = 2;//最小长宽比(灯条)
        VALUES_PREFIX float maxBetweenSizeRatio = 6;//(灯条)轮廓间最大面积比(相除-1取绝对值)
        VALUES_PREFIX float maxBetweenWHRatio = 4;//(灯条)轮廓间最大长或宽比(相除-1取绝对值)
        VALUES_PREFIX float maxAngleDistance = 15;//最大角度差(超过此值则认为两个轮廓不平行)
        VALUES_PREFIX float maxAngleMiss = 15;//最大角度差值(灯条中心点连线的角度与灯条角度)

        static const auto r2d = 45.0 / atan(1.0);//弧度转角度

        VALUES_PREFIX auto arm_l_h = 53.72F; //装甲板 灯条高度
        VALUES_PREFIX auto arm_h = 125.0F;//装甲板高度
        VALUES_PREFIX auto arm_sm_w = 135.0F;//小装甲板宽度
        VALUES_PREFIX auto arm_lg_w = 230.0F;//大装甲板宽度
        VALUES_PREFIX auto arm_min_r = arm_sm_w / (arm_h * 4);//装甲板长宽比最低值
        VALUES_PREFIX auto arm_sm_r = arm_sm_w / arm_h;//小装甲板长宽比
        VALUES_PREFIX auto arm_lg_r = arm_lg_w / arm_h;//大装甲板长宽比
        VALUES_PREFIX auto arm_max_r = (arm_lg_w * 1.5F) / arm_h;//装甲板长宽比最高值
        VALUES_PREFIX auto arm_middle_r = (arm_sm_r + arm_lg_r) / 2;//大小装甲板长宽比的中间值

        static const cv::Size arm_to_sm = {64, 64};
        static const cv::Size arm_to_lg = {128, 64};

        static const constexpr char arm_sm_ids[] = {1, 2, 3, 4, 9, 10, 11};
        static const constexpr char arm_lg_ids[] = {5, 6, 7, 8, 12};
        static const constexpr int arm_sm_ids_size = sizeof(arm_sm_ids) / sizeof(char);
        static const constexpr int arm_lg_ids_size = sizeof(arm_lg_ids) / sizeof(char);

        VALUES_PREFIX auto model_threshold = 0.8F; // 模型结果阈值

        VALUES_PREFIX int debug_show_extra = 1;//调试: 是否展示额外数据

        void init() {
            IFR_FAV(bmat_threshold, 0, 255)

            IFR_FAV(maxSizeRatio, 1, 100000)
            IFR_FAV(minSizeRatio, 1, 100000)
            IFR_FAV(maxAspectRatio, 1, 100)
            IFR_FAV(minAspectRatio, 1, 100)
            IFR_FAV(maxBetweenSizeRatio, 0, 100)
            IFR_FAV(maxBetweenWHRatio, 0, 100)
            IFR_FAV(maxAngleDistance, 0, 360)
            IFR_FAV(maxAngleMiss, 0, 360)

            IFR_FAV(arm_l_h, 0, 500)
            IFR_FAV(arm_h, 0, 500)
            IFR_FAV(arm_sm_w, 0, 500)
            IFR_FAV(arm_lg_w, 0, 500)
            IFR_FAV(arm_min_r, -5, 5)
            IFR_FAV(arm_sm_r, -5, 5)
            IFR_FAV(arm_lg_r, -5, 5)
            IFR_FAV(arm_max_r, -5, 5)
            IFR_FAV(arm_middle_r, -5, 5)

            IFR_FAV(model_threshold, 0, 1)

            IFR_FAV(debug_show_extra, 0, 1)
        }
    }


    void FinderArmor::handler(const cv::Mat &src, int type, std::vector<datas::ArmTargetInfo> &targets) {
        using namespace Values;


#if DEBUG_IMG_FA || DEBUG_VIDEO_FA
        static const auto imshow_delay = cv::getTickFrequency() * 0.1;//两次显示间隔
        static auto imshow_lst = cv::getTickCount();//上一次更新值
        bool imshow_show = false;//本次是否展示
        const auto imshow_now = cv::getTickCount();//当前值
        if (static_cast<decltype(imshow_delay)>((imshow_now - imshow_lst)) > imshow_delay)
            imshow_show = true, imshow_lst = imshow_now;

        cv::Mat imshow_mat_view;//调试图像

#if DEBUG_VIDEO_FA
        if (writer == nullptr) {
            writer = new cv::VideoWriter("output.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                                         100.0, src.size());
            ifr::logger::log("FindArmor", "Record", writer->isOpened());
        }
#endif
#endif

        tw->start(thread_id);
        FinderArmor_tw(0);


        static const float src_size = (float) src.size().area();//原始图像的面积大小(预期: 在程序执行期间, 输入大小不会变化)

        cv::Mat gray, b_mat;
        cv::cvtColor(src, gray, type);

#if DEBUG_IMG_FA && 0
        if (imshow_show) imshow("gray", gray);
#endif
        FinderArmor_tw(1);


        cv::threshold(gray, b_mat, bmat_threshold, 255, cv::ThresholdTypes::THRESH_BINARY);
//      cv::threshold(gray, b_mat, 100, 255, cv::ThresholdTypes::THRESH_OTSU);

#if DEBUG_IMG_FA && 0
        if (imshow_show) imshow("thr", b_mat);
#endif
        FinderArmor_tw(2);

#if DEBUG_IMG_FA || DEBUG_VIDEO_FA

        if (imshow_show) {
#if  0 //使用灰度图 or 彩色图
            imshow_mat_view = gray.clone();
            c1to3(imshow_mat_view);
#else
            if (type == datas::FrameType::BGR) cv::cvtColor(src, imshow_mat_view, cv::COLOR_BGR2RGB);
            else if (type == datas::FrameType::BayerRG) cv::cvtColor(src, imshow_mat_view, cv::COLOR_BayerRG2RGB);
            else throw std::runtime_error("[FinderArmor] Bad src type: " + std::to_string(type));
            cv::Mat channels[3];
            cv::split(imshow_mat_view, channels);
            for (auto &channel: channels)cv::equalizeHist(channel, channel);
            cv::merge(channels, 3, imshow_mat_view);
#endif
        }
#endif


        std::vector<std::vector<cv::Point>> contours;  //所有轮廓
        std::vector<cv::Vec4i> hierarchy;         //轮廓关系
        cv::findContours(b_mat, contours, hierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);\

        FinderArmor_tw(3);


        std::vector<cv::RotatedRect> rrs;//所有轮廓的最小包围
        rrs.reserve(contours.size());
        for (const auto &c: contours)rrs.push_back(cv::minAreaRect(c));

        FinderArmor_tw(4);

        std::vector<size_t> goodIndex;//所有较好的轮廓下标
        goodIndex.reserve(rrs.size());
        for (size_t i = 0; i < rrs.size(); i++) {//初步筛选
            const auto size = rrs[i].size;
            const auto sr = src_size / size.area();//面积比(画面大小除以轮廓框大小)
            if (sr < minSizeRatio || maxSizeRatio < sr)continue;
            auto ar = getAspectRatio(size);
            if (ar < minAspectRatio || maxAspectRatio < ar)continue;
            goodIndex.push_back(i);
        }

        if (goodIndex.size() > 20)return;

        FinderArmor_tw(5);


        std::vector<ContourPair> goodPair;//所有较好的轮廓对
        goodPair.reserve(rrs.size());
        for (size_t i = 0; i < goodIndex.size(); i++) {//将所有较好的轮廓尝试配对
            const auto &irr = rrs[goodIndex[i]];//i的旋转矩形
            const auto ira = getRealAngle(irr);//i的实际角度
            float iw = irr.size.width, ih = irr.size.height;
            if (iw > ih)std::swap(iw, ih);
            for (size_t j = i + 1; j < goodIndex.size(); j++) {
                const auto &jrr = rrs[goodIndex[j]];//j的旋转矩形

                float jw = jrr.size.width, jh = jrr.size.height;
                if (jw > jh)std::swap(jw, jh);
                const auto sr = abs((irr.size.area() / jrr.size.area()) - 1);//面积比
                const auto wr = abs((iw / jw) - 1);//宽比
                const auto hr = abs((ih / jh) - 1);//高比
                if (sr > maxBetweenSizeRatio || wr > maxBetweenWHRatio || hr > maxBetweenWHRatio)continue;


                const auto jra = getRealAngle(jrr);//j的实际角度
                const auto dis_a = getAngleDistance(ira, jra);//灯条角度差
                if (dis_a > maxAngleDistance)continue;

                const auto link_vec = irr.center - jrr.center;//两个轮廓中心点连线
                const auto armor_h = ((ih + jh) / 2) * arm_h / arm_l_h;//装甲板高度
                const auto armor_w = sqrt(link_vec.dot(link_vec));//装甲板宽度
                const auto armor_r = armor_w / armor_h;//装甲板长宽比
                if (armor_r < arm_min_r || arm_max_r < armor_r)continue;


                auto link_a = (atan2(link_vec.y, link_vec.x) * r2d);//连线角度
                if (link_a < 0)link_a += 180;
                const auto miss_a = (float) abs(90 - abs(jra - link_a));//计算的连线角度与真实连线角度的差值


                //TODO 通过miss_a过滤    这里有问题  无法计算两个灯条的平均角度（0度和180度会被平均为90度），暂时只拿其中一个代替。

                if (miss_a > maxAngleMiss)continue;

                goodPair.push_back({goodIndex[i], goodIndex[j], armor_w, armor_h, jra, armor_r > arm_middle_r,
                                    sr / maxBetweenSizeRatio
                                    + wr / maxBetweenWHRatio
                                    + hr / maxBetweenWHRatio
                                    + dis_a / maxAngleDistance
                                    + miss_a,
                                    false
                                   });
            }
        }
        FinderArmor_tw(6);

        std::vector<std::vector<cv::Point2f>> rr_points;//rrs的角点
        rr_points.resize(rrs.size());
        {
            cv::Point2f ps[4];
            for (auto &p: goodPair) {
                auto &rp1 = rr_points[p.i1], &rp2 = rr_points[p.i2];
                if (rp1.empty()) rrs[p.i1].points(ps), rp1.insert(rp1.end(), ps, ps + 4);
                if (rp2.empty()) rrs[p.i2].points(ps), rp2.insert(rp2.end(), ps, ps + 4);
            }
        }
        for (auto &p: goodPair) {//计算灯条组的包围矩形
            const auto &r1 = rrs[p.i1], &r2 = rrs[p.i2];
            p.rr = getInnerRR(
#if DEBUG_IMG_FA || DEBUG_VIDEO_FA
                    debug_show_extra ? imshow_mat_view : cv::Mat(),
#endif
                    r1, r2, rr_points, p.i1, p.i2, p.h);//装甲板 包围
        }


        FinderArmor_tw(7);


        for (auto &p: goodPair) {
            if (p.skip)continue;
            cv::Point2f ps[4];
            p.rr.points(ps);
            std::vector<cv::Point2f> points(ps, ps + 4);
            for (const auto &sub: goodPair) {
                if (cv::pointPolygonTest(points, rrs[sub.i1].center, false) >= 0 ||
                    cv::pointPolygonTest(points, rrs[sub.i2].center, false) >= 0) {
                    p.skip = true;
                    break;
                }
            }
        }

        FinderArmor_tw(8);


        std::sort(goodPair.begin(), goodPair.end(), [](const auto &l, const auto &r) {
            if (l.skip != r.skip)return r.skip;
            return l.bad < r.bad;
        });


        FinderArmor_tw(9);


#if DEBUG_IMG_FA || DEBUG_VIDEO_FA
        std::vector<datas::ArmTargetInfo> bad_targets;//坏目标(仅在debug时存在)
        bad_targets.resize(goodPair.size());
#endif

        targets.resize(goodPair.size());
        for (const auto &p: goodPair) {//将所有可能的装甲板做仿射变换提取图像
            if (p.skip)continue;
            cv::Mat mat;
            affineTransform(gray, p.rr, mat, p.is_large ? arm_to_lg : arm_to_sm, p.angle);
//            cv::equalizeHist(mat, mat);
//            cv::imshow("target t " + std::to_string(i), mat);
            cv::threshold(mat, mat, 0, 1, cv::THRESH_OTSU);

            auto result_type = predict(mat, p.is_large);

            if (result_type != 0)
                targets.push_back({p.rr, result_type, p.angle, p.is_large, p.bad});
#if DEBUG_IMG_FA || DEBUG_VIDEO_FA
            else
                bad_targets.push_back({p.rr, result_type, p.angle, p.is_large, p.bad});
#endif
        }
        FinderArmor_tw(10);

#if DEBUG_IMG_FA || DEBUG_VIDEO_FA
        if (imshow_show) {
            auto t_end = cv::getTickCount();
            auto fps = 1 / (static_cast<double>(t_end - imshow_now) / cv::getTickFrequency());


            static const auto color_all_c = cv::Scalar(0, 255, 255);//所有轮廓
            static const auto color_good_c = cv::Scalar(255, 255, 0);//好轮廓
            static const auto color_target = cv::Scalar(0, 255, 0);//好目标
            static const auto color_bad_target = cv::Scalar(0, 0, 255);//坏目标
            static const auto color_fps = cv::Scalar(255, 0, 255);//帧率
            //所有轮廓
            cv::drawContours(imshow_mat_view, contours, -1, color_all_c, 2);
            for (const auto &index: goodIndex)
                cv::drawContours(imshow_mat_view, contours, static_cast<int>(index), color_good_c, 2);
            for (auto &t: targets) {
                cv::putText(imshow_mat_view,
                            std::to_string((int) t.type),
                            t.target.center, cv::FONT_HERSHEY_COMPLEX, 0.8, color_target);
                drawRotatedRect(imshow_mat_view, t.target, color_target, 5, 16);
            }
            if (debug_show_extra)
                for (auto &t: bad_targets) {
                    drawRotatedRect(imshow_mat_view, t.target, color_bad_target, 5, 16);
                }
            cv::putText(imshow_mat_view,
                        "fps:" + std::to_string((int) fps)
                        + ", good: " + std::to_string(goodIndex.size())
                        + "; " + std::to_string(targets.size()) + "/" + std::to_string(goodPair.size()),
                        cv::Size(0, 30), cv::FONT_HERSHEY_COMPLEX, 0.8, color_fps);
#if DEBUG_VIDEO_FA
            writer->write(imshow_mat_view);
#endif
#if DEBUG_IMG_FA

            cv::imshow("view", imshow_mat_view);
            auto key = cv::waitKey(1);
            if (key == 32) {//空格
                static const auto ti = std::to_string(time(nullptr));
                static int frame_id = 0;
                auto frame_id_str = std::to_string(frame_id++);
                int id = 0;
                for (const auto &t: targets) {
                    cv::Mat mat;
                    affineTransform(gray, t.target, mat, t.is_large ? arm_to_lg : arm_to_sm, t.angle);
                    cv::threshold(mat, mat, 0, 255, cv::THRESH_OTSU);
                    cv::imwrite("assets/sub_" + ti + "_" + frame_id_str + "_" + std::to_string(id++) + "_good.jpg",
                                mat);
                }
                for (const auto &t: bad_targets) {
                    cv::Mat mat;
                    affineTransform(gray, t.target, mat, t.is_large ? arm_to_lg : arm_to_sm, t.angle);
                    cv::threshold(mat, mat, 0, 255, cv::THRESH_OTSU);
                    cv::imwrite("assets/sub_" + ti + "_" + frame_id_str + "_" + std::to_string(id++) + "_bad.jpg", mat);
                }
            }
#endif
        }
#endif
    }

    void FinderArmor::initNet() {
        net_sm = cv::dnn::readNet(net_sm_path);
        if (net_sm.empty())throw std::runtime_error("load net failed!");
        auto net_sm_outs = NetHelper::getOutputsNames(net_sm);
        if (net_sm_outs.size() != 1)
            throw std::runtime_error("Bad net out amount: " + std::to_string(net_sm_outs.size()) + ", expect 1");
        net_sm_out = net_sm_outs[0];
        IFR_LOC_LOGGER(net_sm_out);

        net_lg = cv::dnn::readNet(net_lg_path);
        if (net_lg.empty())throw std::runtime_error("load net failed!");
        auto net_lg_outs = NetHelper::getOutputsNames(net_lg);
        if (net_lg_outs.size() != 1)
            throw std::runtime_error("Bad net out amount: " + std::to_string(net_lg_outs.size()) + ", expect 1");
        net_lg_out = net_lg_outs[0];
        IFR_LOC_LOGGER(net_lg_out);
    }

    char FinderArmor::predict(const cv::Mat &mat, bool is_lg) {
        using namespace Values;
        static const cv::Scalar mean = {0, 0, 0};
//      static const auto to1 = 1 / 255.0;
        auto blob = cv::dnn::blobFromImage(mat, 1.0, is_lg ? arm_to_lg : arm_to_sm, mean, false, false);
        cv::Mat out;
        {
//            std::unique_lock<std::mutex> lock(is_lg ? net_lg_mtx : net_sm_mtx);
            (is_lg ? net_lg : net_sm).setInput(blob);
            out = (is_lg ? net_lg : net_sm).forward((is_lg ? net_lg_out : net_sm_out));
        }

        int max = 0;//最大置信度对应的结果
        float max_v = out.at<float>(max);//最大置信度

        for (int i = 1, l = is_lg ? arm_lg_ids_size : arm_sm_ids_size; i < l; i++) {
            const auto v = out.at<float>(i);
            if (v > max_v)max = i, max_v = v;
        }
        if (max_v < model_threshold)return 0;
        else return (is_lg ? arm_lg_ids : arm_sm_ids)[max];
    }

}