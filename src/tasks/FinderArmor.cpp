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


    FORCE_INLINE void drawRotatedRect(cv::Mat &mask, const cv::RotatedRect &rr, const cv::Scalar &color, int thickness,
                                      int lineType) {
        cv::Point2f ps[4];
        rr.points(ps);
        std::vector<std::vector<cv::Point>> tmpContours;
        std::vector<cv::Point> contours;
        for (const auto &p: ps) { contours.emplace_back(cv::Point2i(p)); }
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
        cv::waitKey(1);
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


        VALUES_PREFIX float maxSizeRatio = 3000;//最大面积比(画面大小除以轮廓框大小), 超过此值则认为是噪声
        VALUES_PREFIX float minSizeRatio = 4;//最小面积比(画面大小除以轮廓框大小), 低于此值则认为非法框
        VALUES_PREFIX float maxAspectRatio = 50;//最大长宽比(灯条)
        VALUES_PREFIX float minAspectRatio = 2;//最大长宽比(灯条)
        VALUES_PREFIX float maxBetweenSizeRatio = 1;//(灯条)轮廓间最大面积比(相除-1取绝对值)
        VALUES_PREFIX float maxBetweenWHRatio = 0.5;//(灯条)轮廓间最大长或宽比(相除-1取绝对值)
        VALUES_PREFIX float maxAngleDistance = 15;//最大角度差(超过此值则认为两个轮廓不平行)
        VALUES_PREFIX float maxAngleMiss = 12;//最大角度差值(灯条中心点连线的角度与灯条角度)

        static const auto r2d = 45.0 / atan(1.0);//弧度转角度

        VALUES_PREFIX auto arm_l_h = 53.72F; //装甲板 灯条高度
        VALUES_PREFIX auto arm_h = 125.0F;//装甲板高度
        VALUES_PREFIX auto arm_sm_w = 135.0F;//小装甲板宽度
        VALUES_PREFIX auto arm_lg_w = 230.0F;//大装甲板宽度
        VALUES_PREFIX auto arm_min_r = arm_sm_w / (arm_h * 2);//装甲板长宽比最低值
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

        VALUES_PREFIX auto model_threshold = 0.9F; // 模型结果阈值

        void init() {
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
        }
    }


    void FinderArmor::handler(const cv::Mat &src, int type, std::vector<datas::ArmTargetInfo> &targets) {
        using namespace Values;


#if DEBUG_IMG_FA
        static const auto imshow_delay = cv::getTickFrequency() * 1;
        static auto imshow_lst = cv::getTickCount();
        static bool imshow_show = false;
        const auto imshow_now = cv::getTickCount();
        if ((imshow_now - imshow_lst) > imshow_delay) imshow_show = true, imshow_lst = imshow_now;
        else imshow_show = false;

        if (writer == nullptr) {
            writer = new cv::VideoWriter("output.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 20.0,
                                         src.size());
            ifr::logger::log("FindArmor", "Record", writer->isOpened());
        }

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


        cv::threshold(gray, b_mat, 100, 255, cv::ThresholdTypes::THRESH_BINARY);
//      cv::threshold(gray, b_mat, 100, 255, cv::ThresholdTypes::THRESH_OTSU);
#if DEBUG_IMG_FA && 0
        if (imshow_show) imshow("thr", b_mat);
#endif
        FinderArmor_tw(2);


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

                float jw = irr.size.width, jh = irr.size.height;
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
                                    + miss_a
                                   });
            }
        }
        //TODO 二重循环遍历goodPair, 剔除内部包含其它ContourPair的ContourPair
        std::sort(goodPair.begin(), goodPair.end(), [](const auto &l, const auto &r) { return l.bad < r.bad; });

        FinderArmor_tw(6);


//        static auto t = std::to_string(time(nullptr));
//        static int64_t index = 0;
//        auto _index = std::to_string(++index);

#if DEBUG_IMG_FA
        std::vector<datas::ArmTargetInfo> bad_targets;
#endif

//        for (size_t i = 0; i < goodPair.size(); i++) {//将所有可能的装甲板做仿射变换提取图像
        for (auto &p: goodPair) {//将所有可能的装甲板做仿射变换提取图像
            const auto &r1 = rrs[p.i1], &r2 = rrs[p.i2];
            cv::Point2f ps[8];
            r1.points(ps), r2.points(ps + 4);
            auto rr = cv::minAreaRect(std::vector<cv::Point2f>(ps, ps + 8));//装甲板 包围
            if (rr.size.height < rr.size.width)//将高度设为装甲板的高度(原高度为灯条高度), 缩小宽度(减少两侧灯条)
                rr.size.height = p.h, rr.size.width -= std::min(r1.size.width, r1.size.height) * 3;
            else rr.size.width = p.h, rr.size.height -= std::min(r1.size.width, r1.size.height) * 3;

            cv::Mat mat;
            affineTransform(gray, rr, mat, p.is_large ? arm_to_lg : arm_to_sm, p.angle);
//            cv::equalizeHist(mat, mat);
//            cv::imshow("target t " + std::to_string(i), mat);
            cv::threshold(mat, mat, 0, 255, cv::THRESH_OTSU);

            auto result_type = predict(mat, p.is_large);

            if (result_type != 0)
                targets.push_back({rr, result_type, p.angle, p.is_large, p.bad});
#if DEBUG_IMG_FA
            else
                bad_targets.push_back({rr, result_type, p.angle, p.is_large, p.bad});
#endif

//            cv::imwrite("assets\\smat_" + t + "_" + _index + "_" + std::to_string(i) + ".png", mat);
//            cv::imshow("target " + std::to_string(i), mat);//TODO
//            predict(mat, p.is_large);
//            cv::Mat debug_mat = mat.clone();
//            static const auto dilateKernel = getStructuringElement(0, cv::Size(3, 3));
//            cv::dilate(debug_mat, debug_mat, dilateKernel, cv::Point(-1, -1), 1);
//            c1to3(debug_mat);
//            cv::putText(debug_mat, std::to_string(predict(mat, p.is_large)) + " " + std::to_string(p.bad),
//                        mat.size() / 2, cv::FONT_HERSHEY_COMPLEX,
//                        0.5, cv::Scalar(255, 0, 100));
//            cv::imshow("target x " + std::to_string(i), debug_mat);
        }
        FinderArmor_tw(7);


//        static std::list<double> all_fps;
//        static long double cnt_fps;

//        auto fps = 1 / ((t_5 - t_0) / cv::getTickFrequency());
//        all_fps.push_back(fps), cnt_fps += fps;
//        if (all_fps.size() > 10) {
//            cnt_fps -= all_fps.front();
//            all_fps.pop_front();
//        }

//        std::cout << (cnt_fps / all_fps.size()) << " " << fps << out_tdiff(t_1, t_0) << out_tdiff(t_2, t_1)
//                  << out_tdiff(t_3, t_2)
//                  << out_tdiff(t_4, t_3) << out_tdiff(t_5, t_4) << std::endl;

#if DEBUG_IMG_FA
        if (imshow_show) {
            DEBUG_nowTime(t_end);
            auto fps = 1 / ((t_end - imshow_now) / cv::getTickFrequency());

            cv::Mat img = gray.clone();
//            if (type == datas::FrameType::BGR) {
//                cv::cvtColor(src, img, cv::COLOR_BGR2RGB);
//            } else if (type == datas::FrameType::BayerRG) {
//                cv::cvtColor(src, img, cv::COLOR_BayerRG2RGB);
//            } else throw std::runtime_error("[FinderArmor] Bad src type: " + std::to_string(type));
            c1to3(img);
//            cv::Mat channels[3];
//            cv::split(img, channels);
//            for (int i = 0; i < 3; i++)cv::equalizeHist(channels[i], channels[i]);
//            cv::merge(channels, 3, img);
            cv::drawContours(img, contours, -1, cv::Scalar(0, 255, 0), 2);
            for (size_t i = 0; i < targets.size(); i++) {
                const auto &t = targets[i];
                cv::putText(img,
                            std::to_string((int) i) + " " + std::to_string((int) t.type) + " " + std::to_string(t.bad),
                            t.target.center, cv::FONT_HERSHEY_COMPLEX, 0.8, cv::Scalar(255, 0, 255));
                drawRotatedRect(img, t.target, cv::Scalar(0, 255, 0), 5, 16);
            }
            for (size_t i = 0; i < bad_targets.size(); i++) {
                const auto &t = bad_targets[i];
                cv::putText(img, std::to_string((int) i) + " " + std::to_string(t.bad),
                            t.target.center, cv::FONT_HERSHEY_COMPLEX, 0.8, cv::Scalar(255, 0, 0));
                drawRotatedRect(img, t.target, cv::Scalar(0, 0, 255), 5, 16);
            }
            cv::putText(img, "fps:" + std::to_string(fps), cv::Size(0, 30),
                        cv::FONT_HERSHEY_COMPLEX, 0.8, cv::Scalar(255, 0, 100));
//            cv::imshow("view", img);
            writer->write(img);
        }
        if (imshow_show)cv::waitKey(1);
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
        static const auto to1 = 1 / 255.0;
        auto blob = cv::dnn::blobFromImage(mat, to1, is_lg ? arm_to_lg : arm_to_sm, mean, true, false);
        cv::Mat out;
        {
//            std::unique_lock<std::mutex> lock(is_lg ? net_lg_mtx : net_sm_mtx);
            (is_lg ? net_lg : net_sm).setInput(blob);
            out = (is_lg ? net_lg : net_sm).forward((is_lg ? net_lg_out : net_sm_out));
        }

        int max = 0;//最大置信度对应的结果
        float max_v = out.at<float>(max);//最大置信度

        for (int i = 1, l = is_lg ? arm_lg_ids_size : arm_sm_ids_size; i < l; i++) {
            auto v = out.at<float>(i);
            if (v > max_v)max = i, max_v = v;
        }
        if (max_v < model_threshold)return 0;
        else return (is_lg ? arm_lg_ids : arm_sm_ids)[max];
    }

}