#include "http_client.h"

#include <any>
#include "log.h"
#include <asio2/asio2.hpp>

namespace tc
{

    std::shared_ptr<HttpClient> HttpClient::Make(const std::string& host, int port, const std::string& path, int timeout_ms) {
        return std::make_shared<HttpClient>(host, port, path, false, timeout_ms);
    }

    std::shared_ptr<HttpClient> HttpClient::MakeSSL(const std::string& host, int port, const std::string& path, int timeout_ms) {
        return std::make_shared<HttpClient>(host, port, path, true, timeout_ms);
    }

//    std::shared_ptr<HttpClient> HttpClient::MakeDownloadHttp(const std::string& url) {
//        auto remove_prefix_url = url.substr(7, url.size());
//        int separated_pos = remove_prefix_url.find('/');
//        auto host = remove_prefix_url.substr(0, separated_pos);
//        auto path = remove_prefix_url.substr(separated_pos, remove_prefix_url.size());
//        LOGI("download, host: {}, path: {}", host.c_str(), path.c_str());
//        return std::make_shared<HttpClient>(host, path, false);
//    }
//
//    std::shared_ptr<HttpClient> HttpClient::MakeDownloadHttps(const std::string& url) {
//        auto remove_prefix_url = url.substr(8, url.size());
//        int separated_pos = remove_prefix_url.find('/');
//        auto host = remove_prefix_url.substr(0, separated_pos);
//        auto path = remove_prefix_url.substr(separated_pos, remove_prefix_url.size());
//        return std::make_shared<HttpClient>(host, path, true);
//    }

    HttpClient::HttpClient(const std::string& host, int port, const std::string& path, bool ssl, int timeout_ms) {
        this->host_ = host;
        this->port_ = port;
        this->path = path;
        this->ssl_ = ssl;
        this->timeout_ms_ = timeout_ms;
    }

    HttpClient::~HttpClient() {

    }

    HttpResponse HttpClient::Request() {
        std::map<std::string, std::string> params;
        return Request(params);
    }

    HttpResponse HttpClient::Request(const std::map<std::string, std::string>& query, const std::string& body) {
        auto query_path = path;
        auto index = 0;
        for (const auto& [k, v] : query) {
            if (index == 0) {
                query_path += "?" + k + "=" + v;
            } else {
                query_path += "&" + k + "=" + v;
            }
            index++;
        }

        http::web_request req;
        req.method(http::verb::get);
        req.set(http::field::user_agent, "Chrome");
        req.set(http::field::content_type, "application/json");
        req.keep_alive(true);
        req.target(query_path);
        req.body() = body;
        req.prepare_payload();

        //LOGI("Request path: {}{}:{}{}", ssl ? "https://" : "http://", host, port_, query_path);
        if (ssl_) {
            auto r = asio2::https_client::execute(host_, port_, req, std::chrono::milliseconds(timeout_ms_));
            if (asio2::get_last_error()) {
                LOGE("Request failed: {}", asio2::last_error_msg());
            }
            else {
                std::string body = r.body();
                //LOGI("req success: {}", body);
                return HttpResponse {
                    .status = 200,
                    .body = body,
                };
            }
        }
        else {
            auto r = asio2::http_client::execute(host_, port_, req, std::chrono::milliseconds(timeout_ms_));
            if (asio2::get_last_error()) {
                LOGI("Request path: {}{}{}{}", ssl_ ? "https://" : "http://", host_, port_, query_path);
                LOGE("Request failed: {}", asio2::last_error_msg());
            }
            else {
                std::string body = r.body();
                //LOGI("req success: {}", body);
                return HttpResponse {
                    .status = 200,
                    .body = body,
                };
            }
        }

        return HttpResponse {
            .status = -1,
            .body = "",
        };

    }

    HttpResponse HttpClient::Post() {
        std::map<std::string, std::string> params;
        return Post(params);
    }

    HttpResponse HttpClient::Post(const std::map<std::string, std::string>& query, const std::string& body) {
        auto query_path = path;
        auto index = 0;
        for (const auto& [k, v] : query) {
            if (index == 0) {
                query_path += "?" + k + "=" + v;
            } else {
                query_path += "&" + k + "=" + v;
            }
            index++;
        }

        http::web_request req;
        req.method(http::verb::post);
        req.set(http::field::user_agent, "Chrome");
        req.set(http::field::content_type, "application/json");
        req.keep_alive(true);
        req.target(query_path);
        req.body() = body;
        req.prepare_payload();

        LOGI("Post path: {}{}:{}{}", ssl_ ? "https://" : "http://", host_, port_, query_path);
        if (ssl_) {
            auto r = asio2::https_client::execute(host_, port_, req, std::chrono::milliseconds(timeout_ms_));
            if (asio2::get_last_error()) {
                LOGE("Post failed: {}", asio2::last_error_msg());
            }
            else {
                std::string body = r.body();
                LOGI("Post SSL success: {}", body);
                return HttpResponse {
                    .status = 200,
                    .body = body,
                };
            }
        }
        else {
            auto r = asio2::http_client::execute(host_, port_, req, std::chrono::milliseconds(timeout_ms_));
            if (asio2::get_last_error()) {
                LOGI("Post path: {}{}{}{}", ssl_ ? "https://" : "http://", host_, port_, query_path);
                LOGE("Post failed: {}", asio2::last_error_msg());
            }
            else {
                std::string body = r.body();
                LOGI("Post success: {}", body);
                return HttpResponse {
                    .status = 200,
                    .body = body,
                };
            }
        }

        return HttpResponse {
            .status = -1,
            .body = "",
        };
    }

    HttpResponse HttpClient::Download(std::function<void(const std::string& body, bool success)>&& download_cbk) {
        LOGI("Download: {}", path.c_str());
        std::string body;

        return HttpResponse {
            .status = -1,
            .body = "",
        };
    }

    int HttpClient::HeadFileSize() {

        return 0;
    }

}