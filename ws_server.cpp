//
// Created by RGAA on 25/02/2025.
//

#include "ws_server.h"
#include "tc_common_new/log.h"

namespace tc
{

    struct aop_log {
        bool before(http::web_request &req, http::web_response &rep) {
            asio2::ignore_unused(rep);
            return true;
        }

        bool after(std::shared_ptr<asio2::http_session> &session_ptr, http::web_request &req, http::web_response &rep) {
                    ASIO2_ASSERT(asio2::get_current_caller<std::shared_ptr<asio2::http_session>>().get() == session_ptr.get());
            asio2::ignore_unused(session_ptr, req, rep);
            return true;
        }
    };

    void WsServer::Prepare(const std::map<std::string, std::any>& vars) {
        http_server_ = std::make_shared<asio2::http_server>();
        http_server_->bind_disconnect([=, this](std::shared_ptr<asio2::http_session>& sess_ptr) {
            auto socket_fd = (uint64_t)sess_ptr->socket().native_handle();
            if (sessions_.HasKey(socket_fd)) {
                auto sess = sessions_.Get(socket_fd);
                sess->OnDisConnected();
                sessions_.Remove(socket_fd);
            }
        });

        http_server_->support_websocket(true);
        ws_data_ = std::make_shared<WsData>(WsData{
            .vars_ = vars,
        });
    }

    void WsServer::Start(const std::string &ip, int port) {
        bool ret = http_server_->start(ip, port);
        LOGI("App server start result: {}", ret);
    }

    void WsServer::Exit() {
        if (http_server_ && http_server_->is_started()) {
            http_server_->stop_all_timers();
            http_server_->stop();
        }
    }

    void WsServer::AddHttpGetRouter(const std::string &path,
                                       std::function<void(const std::string& path, http::web_request &req, http::web_response &rep)>&& cbk) {
        http_server_->bind<http::verb::get>(path, [=, this](http::web_request &req, http::web_response &rep) {
            cbk(path, req, rep);
        }, aop_log{});
    }

    void WsServer::AddHttpPostRouter(const std::string& path,
                                        std::function<void(const std::string& path, http::web_request &req, http::web_response &rep)>&& cbk) {
        http_server_->bind<http::verb::post>(path, [=, this](http::web_request &req, http::web_response &rep) {
            cbk(path, req, rep);
        }, aop_log{});
    }


}