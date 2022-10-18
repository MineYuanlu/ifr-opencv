//
// Created by yuanlu on 2022/9/22.
//

#include "API.h"
#include "Plans.h"

using namespace std;
namespace ifr {

    namespace API {
        string url = "0.0.0.0:8000";
#define COMMON_JSON_HEADER "Access-Control-Allow-Origin:*\nContent-Type: application/json;charset:utf-8;\n" //通用头(json)
#define COMMON_TEXT_HEADER "Access-Control-Allow-Origin:*\nContent-Type: text/plain;charset:utf-8;\n" //通用头(text)
#define STR_MG2STD(s) std::string(s.ptr,s.len)
#define STR_RBUF2MG(buf) mg_str_n(buf.GetString(),buf.GetLength())
        const size_t npos = -1;

        struct handler_data {
            const char *pattern;//匹配模式串
            const char *method;//限定请求类型
            mg_event_handler_t fn;//处理器
        };

        vector <handler_data> http_route;

        map<unsigned long, struct mg_connection *> wsClients;
        mutex mtx;


        void sendWS(std::string str) {
            mtx.lock();
            for (const auto &client: wsClients) {
                mg_ws_send(client.second, str.c_str(), str.length(), WEBSOCKET_OP_TEXT);
            }
            mtx.unlock();
        }


        static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
            switch (ev) {
                case MG_EV_WS_OPEN: {
                    auto hm = (struct mg_http_message *) ev_data;
                    mtx.lock();
                    wsClients[c->id] = c;
                    mtx.unlock();
                    sendWS("connected " + to_string(c->rem.ip));
                    cout << "已连接ws " << c->rem.ip << endl;
                    break;
                }
                case MG_EV_CLOSE: {
                    wsClients.erase(c->id);
                    break;
                }
                case MG_EV_HTTP_MSG: {
                    auto hm = (struct mg_http_message *) ev_data;
                    for (const auto &r: http_route) {
                        if (!mg_http_match_uri(hm, r.pattern))continue;
                        if (mg_vcmp(&hm->method, r.method))continue;
                        r.fn(c, ev, ev_data, fn_data);
                        return;
                    }
                    if (!mg_vcmp(&hm->method, "OPTIONS")) {
//                        mg_http_reply(c, 501, "", "Not Implemented");
                        mg_http_reply(c, 204,
                                      "Allow: OPTIONS, GET, POST\r\nAccess-Control-Allow-Origin:*\nAccess-Control-Allow-Headers:*\n",
                                      "");
                        return;
                    }
                    if (mg_http_match_uri(hm, "/ws")) {
                        mg_ws_upgrade(c, hm, NULL);
                    } else {
                        mg_http_reply(c, 200, "", (STR_MG2STD(hm->uri) + "\n" + STR_MG2STD(hm->proto) + "\n" +
                                                   STR_MG2STD(hm->query) + "\n" +
                                                   STR_MG2STD(hm->method)).c_str());;
                    }
                }
            }
        }

        void registerRoute() {
            http_route.push_back(
                    {
                            "/task/descriptions",
                            "GET",
                            [](auto c, int ev, auto ev_data, auto fn_data) {
                                mg_http_reply(c, 200, COMMON_JSON_HEADER,
                                              ifr::Plans::getTaskDescriptionsJson().c_str());
                            }
                    });
            http_route.push_back(
                    {
                            "/plan/list",
                            "GET",
                            [](auto c, int ev, auto ev_data, auto fn_data) {
                                mg_http_reply(c, 200, COMMON_JSON_HEADER, ifr::Plans::getPlanListJson().c_str());
                            }
                    });
            http_route.push_back(
                    {
                            "/plan/get",
                            "GET",
                            [](auto c, int ev, auto ev_data, auto fn_data) {
                                auto hm = (mg_http_message *) ev_data;
                                auto pname = mgx_getquery(hm->query, "pname");
                                if (!pname.len) {
                                    mg_http_reply(c, 401, COMMON_TEXT_HEADER, "no query: pname");
                                    return;
                                }
                                auto plan = ifr::Plans::getPlanInfo(STR_MG2STD(pname));
                                if (!plan.loaded)
                                    mg_http_reply(c, 404, COMMON_TEXT_HEADER, "Not Found");
                                else {
                                    rapidjson::StringBuffer buf;
                                    rapidjson::Writer<StringBuffer> w(buf);
                                    plan(w);
                                    w.Flush();
                                    mg_http_reply(c, 200, COMMON_JSON_HEADER, buf.GetString());
                                }


                            }
                    });
            http_route.push_back(
                    {
                            "/plan/save",
                            "POST",
                            [](auto c, int ev, auto ev_data, auto fn_data) {
                                auto hm = (mg_http_message *) ev_data;
                                try {
                                    rapidjson::Document d;
                                    d.Parse(hm->body.ptr, hm->body.len);
                                    auto plan = ifr::Plans::PlanInfo::read(d);
                                    ifr::Plans::savePlanInfo(plan);
                                    mg_http_reply(c, 200, COMMON_JSON_HEADER, "true");
                                } catch (...) {
                                    mg_http_reply(c, 400, COMMON_TEXT_HEADER, "Can not parse PlanInfo");
                                    return;
                                }

                            }
                    });
        }

        void init(bool async) {
            if (async) {
                thread t(init, false);
                while (!t.joinable());
                t.detach();
            } else {
                registerRoute();
                OUTPUT("启动API")
                struct mg_mgr mgr{};
                mg_mgr_init(&mgr);
                mg_http_listen(&mgr, url.c_str(), fn, nullptr);     // Create listening connection

                OUTPUT("开始监听: " + url)
                for (;;) mg_mgr_poll(&mgr, 1000);                   // Block forever
            }
        }

        inline size_t getNext(const mg_str &str, const char c, const size_t start = 0) {
            for (size_t i = start; i < str.len; i++)if (str.ptr[i] == c)return i;
            return npos;
        }

        mg_str mgx_getquery(mg_str query, std::string key) {
            bool fst = true;
            for (size_t i = 0; i < query.len; i++) {
                if (query.len - i <= key.length())return mg_str("");
                size_t j;
                for (j = 0; j < key.length(); i++, j++) {
                    if (query.ptr[i] != key[j])break;

                }
                if (j != key.length() || query.ptr[i] != '=') {
                    i = getNext(query, '&', i);
                    if (i == npos)return mg_str("");
                    else continue;
                } else {
                    auto next = getNext(query, '&', ++i);
                    if (next == npos)next = query.len;
                    return mg_str_n(query.ptr + i, next - i);
                }
            }
            return mg_str("");
        }


    }

} // ifr