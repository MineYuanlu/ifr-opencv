//
// Created by yuanlu on 2022/10/15.
//

#ifndef IFR_OPENCV_CONFIG_H
#define IFR_OPENCV_CONFIG_H

#include "defs.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include <algorithm>
#include <string>
#include <fstream>

using namespace rapidjson;
namespace ifr {
    namespace Config {
        static const std::string dir = "runtime/config/";

        /**
         * 配置文件控制器
         */
        struct ConfigController {
            std::function<void()> save;//保存
            std::function<void()> load;//加载
        };
        /**
         * 配置文件信息
         * @tparam T 配置文件数据类型
         */
        template<typename T>
        struct ConfigInfo {
            std::function<void(T *, rapidjson::Writer<OStreamWrapper> &)> serialize;
            std::function<void(T *, const rapidjson::Document &)> deserialize;
        };

        /**
         * 创建文件夹 (递归创建, 调用系统命令)
         * @param path 文件夹路径
         */
        void mkDir(std::string path);

        /**
        * 获取路径的文件夹
        * @param fname 文件路径
        * @return 所在文件夹
        */
        std::string getDir(std::string fname);

        /**
         * 注册一个配置文件
         * @tparam T 配置文件数据类型
         * @param name 配置文件名称
         * @param info 配置文件信息
         * @return
         */
        template<typename T>
        ConfigController createConfig(const std::string &name, T *data, const ConfigInfo<T> &info) {
            const auto fname = dir + name + ".json";
            mkDir(getDir(fname));
            ConfigController cc = {
                    [&info, fname, &data]() {
                        try {
                            std::ofstream fout(fname);
                            if (fout.is_open()) {
                                rapidjson::OStreamWrapper osw(fout);
                                rapidjson::Writer<OStreamWrapper> w(osw);

                                try {
                                    info.serialize(data, w);
                                } catch (...) {
                                    OUTPUT("[Config] Can not serialize file: " + fname);
                                }

                                w.Flush();
                                osw.Flush();
                                fout.flush();
                                fout.close();

                            } else {
                                OUTPUT("[Config] Can not open file (w): " + fname)
                                return;
                            }
                        } catch (...) {
                            OUTPUT("[Config] Can not write file: " + fname);
                        }
                    },
                    [&info, fname, &data]() {
                        try {
                            std::ifstream fin(fname);
                            if (fin.is_open()) {
                                Document d;
                                rapidjson::IStreamWrapper isw(fin);
                                d.ParseStream(isw);

                                try {
                                    info.deserialize(data, d);
                                } catch (...) {
                                    OUTPUT("[Config] Can not deserialize file: " + fname);
                                }
                                fin.close();
                            } else {
                                OUTPUT("[Config] Can not open file (r): " + fname)
                                return;
                            }
                        } catch (...) {
                            OUTPUT("[Config] Can not read file: " + fname);
                        }
                    }
            };
            return cc;
        }

    }
} // ifr

#endif //IFR_OPENCV_CONFIG_H
