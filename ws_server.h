//
// Created by RGAA on 25/02/2025.
//

#ifndef GAMMARAYSERVER_WS_SERVER_H
#define GAMMARAYSERVER_WS_SERVER_H

#include "tc_common_new/concurrent_hashmap.h"
#include "log.h"
#include "url_helper.h"
#include <asio2/asio2.hpp>

namespace tc
{

    // WsData
    class WsData {
    public:
        std::map<std::string, std::any> vars_;
    };
    using WsDataPtr = std::shared_ptr<WsData>;

    // WsSession
    class WsSession {
    public:
        virtual void OnConnected() {}
        virtual void OnDisConnected() {}
        virtual void OnBinMessage(std::string_view data) {}

        virtual void PostBinMessage(const std::string& m) {
            if (inner_ && inner_->is_started()) {
                inner_->ws_stream().binary(true);
                inner_->async_send(m);
            }
        }

        virtual void PostTextMessage(const std::string& m) {
            if (inner_ && inner_->is_started()) {
                inner_->ws_stream().binary(false);
                inner_->async_send(m);
            }
        }

        template<typename T>
        std::optional<T> GetVar(const std::string& n) {
            if (ws_data_->vars_.contains(n)) {
                auto v = ws_data_->vars_[n];
                return std::any_cast<T>(v);
            }
            return std::nullopt;
        }

        std::optional<std::string> GetQueryParam(const std::string& key) {
            if (query_params_.contains(key)) {
                return query_params_[key];
            }
            return std::nullopt;
        }

    public:
        std::string path_;
        int64_t socket_fd_{0};
        std::shared_ptr<asio2::http_session> inner_ = nullptr;
        WsDataPtr ws_data_ = nullptr;
        std::unordered_map<std::string, std::string> query_params_;
    };

    // WsServer
    class WsServer : public std::enable_shared_from_this<WsServer> {
    public:

        void Prepare(const std::map<std::string, std::any>& vars);
        void Start(const std::string& ip, int port);
        virtual void Exit();

    public:
        template<typename Session>
        void AddWebsocketRouter(const std::string& path) {
            auto fn_get_socket_fd = [](std::shared_ptr<asio2::http_session> &sess_ptr) -> uint64_t {
                auto& s = sess_ptr->socket();
                return (uint64_t)s.native_handle();
            };
            http_server_->bind(path, websocket::listener<asio2::http_session>{}
                .on("message", [=, this](std::shared_ptr<asio2::http_session>& http_sess, std::string_view data) {
                    auto socket_fd = fn_get_socket_fd(http_sess);
                    if (sessions_.HasKey(socket_fd)) {
                        auto sess = sessions_.Get(socket_fd);
                        sess->OnBinMessage(data);
                    }
                })
                .on("open", [=, this](std::shared_ptr<asio2::http_session>& http_sess) {
                    LOGI("App server {} open", path);
                    http_sess->ws_stream().binary(true);
                    http_sess->set_no_delay(true);
                    auto query = http_sess->get_request().get_query();
                    auto socket_fd = fn_get_socket_fd(http_sess);
                    auto session = std::make_shared<Session>();
                    session->socket_fd_ = socket_fd;
                    session->path_ = path;
                    session->ws_data_ = ws_data_;
                    session->inner_ = http_sess;
                    session->query_params_ = UrlHelper::ParseQueryString(std::string(query.data(), query.size()));
                    session->OnConnected();
                    sessions_.Insert(socket_fd, session);

                    for (const auto& [k, v] : session->query_params_) {
                        LOGI("socket: {}, query param, k: {}, v: {}", socket_fd, k, v);
                    }

                })
                .on("close", [=, this](std::shared_ptr<asio2::http_session>& http_sess) {
                    auto socket_fd = fn_get_socket_fd(http_sess);
                    if (sessions_.HasKey(socket_fd)) {
                        auto sess = sessions_.Get(socket_fd);
                        sess->OnDisConnected();
                        sessions_.Remove(socket_fd);
                    }
                })
            );
        }

        void AddHttpGetRouter(const std::string& path,
                              std::function<void(const std::string& path, http::web_request &req, http::web_response &rep)>&& cbk);

        void AddHttpPostRouter(const std::string& path,
                               std::function<void(const std::string& path, http::web_request &req, http::web_response &rep)>&& cbk);

    protected:
        WsDataPtr ws_data_ = nullptr;
        std::shared_ptr<asio2::http_server> http_server_ = nullptr;
        ConcurrentHashMap<uint64_t, std::shared_ptr<WsSession>> sessions_;

    };

}

#endif //GAMMARAYSERVER_WS_SERVER_H
