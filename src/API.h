//
// Created by yuanlu on 2022/9/22.
//

#ifndef IFR_OPENCV_API_H
#define IFR_OPENCV_API_H

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "web/mongoose.h"
#include "defs.h"

using namespace rapidjson;

namespace ifr {

    namespace API {
        /**
         * 获取query的值
         * @param query query参数
         * @param key 要查找的键
         * @return 键对应的值
         */
        mg_str mgx_getquery(mg_str query, std::string key);

        void init(bool async = false);

        void sendWS(std::string str);

    };
} // ifr

#endif //IFR_OPENCV_API_H
