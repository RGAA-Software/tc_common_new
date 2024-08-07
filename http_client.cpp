#include "http_client.h"

#include <any>
#include "log.h"

namespace tc
{

    std::shared_ptr<HttpClient> HttpClient::Make(const std::string& host, const std::string& path) {
        return std::make_shared<HttpClient>(host, path, false);
    }

    std::shared_ptr<HttpClient> HttpClient::MakeSSL(const std::string& host, const std::string& path) {
        return std::make_shared<HttpClient>(host, path, true);
    }

    std::shared_ptr<HttpClient> HttpClient::MakeDownloadHttp(const std::string& url) {
        auto remove_prefix_url = url.substr(7, url.size());
        int separated_pos = remove_prefix_url.find('/');
        auto host = remove_prefix_url.substr(0, separated_pos);
        auto path = remove_prefix_url.substr(separated_pos, remove_prefix_url.size());
        LOGI("download, host: {}, path: {}", host.c_str(), path.c_str());
        return std::make_shared<HttpClient>(host, path, false);
    }

    std::shared_ptr<HttpClient> HttpClient::MakeDownloadHttps(const std::string& url) {
        auto remove_prefix_url = url.substr(8, url.size());
        int separated_pos = remove_prefix_url.find('/');
        auto host = remove_prefix_url.substr(0, separated_pos);
        auto path = remove_prefix_url.substr(separated_pos, remove_prefix_url.size());
        return std::make_shared<HttpClient>(host, path, true);
    }

    HttpClient::HttpClient(const std::string& host, const std::string& path, bool ssl) {
        this->host = host;
        this->path = path;
        this->ssl = ssl;

        //NLog::Write(Level::Info, Fmt("ssl : %d, host : %s, path : %s", ssl, this->host.c_str(), this->path.c_str()));

        if (ssl) {
#if 0
            ssl_client = std::make_shared<httplib::SSLClient>(host);
            ssl_client->set_follow_location(true);
            ssl_client->set_keep_alive(true);
            ssl_client->enable_server_certificate_verification(false); 
            ssl_client->set_connection_timeout(std::chrono::seconds(15));
#endif
        }
        else {
            client = std::make_shared<httplib::Client>(host);
            client->set_connection_timeout(std::chrono::seconds(15));
        }
    }

    HttpClient::~HttpClient() {

    }

    HttpResponse HttpClient::Request() {
        std::map<std::string, std::string> params;
        return Request(params);
    }

    HttpResponse HttpClient::Request(const std::map<std::string, std::string>& query) {
        //NLog::Write(Level::Info, Fmt("Request: %s", path.c_str()));
        auto res =/* ssl ? ssl_client->Get(path) :*/ client->Get(path);
        if (res.error() != httplib::Error::Success) {
            LOGE("HttpError : {}, {}", (int)res.error(), httplib::to_string(res.error()).c_str());
            return HttpResponse {
                .status = -1,
                .body = "",
            };
        }
        return HttpResponse {
            .status = res->status,
            .body = res->body,
        };
    }

    HttpResponse HttpClient::Post() {
        std::map<std::string, std::string> params;
        return Post(params);
    }

    HttpResponse HttpClient::Post(const std::map<std::string, std::string>& query) {
        //NLog::Write(Level::Info, Fmt("Post: %s", path.c_str()));
        auto res = /*ssl ? ssl_client->Post(path) :*/ client->Post(path);
        if (res.error() != httplib::Error::Success) {
            LOGE("HttpError : {}, {}", (int)res.error(), httplib::to_string(res.error()).c_str());
            return HttpResponse {
                .status = -1,
                .body = "",
            };
        }
        return HttpResponse {
            .status = res->status,
            .body = res->body,
        };
    }

    HttpResponse HttpClient::Download(std::function<void(const std::string& body, bool success)>&& download_cbk) {
        LOGI("Download: {}", path.c_str());
        std::string body;

        bool success = false;
        auto res = /*ssl ?
        ssl_client->Get(path, 
            [&](const char *data, size_t data_length) {
                body.append(data, data_length);
                return true;
            }, 
            [&](uint64_t len, uint64_t total) {
                success = (len == total);
                //NLog::Write(Level::Info, Fmt("%lld / %lld bytes => %d%% complete", len, total, (int)(len*100/total)));
                return true;
            }
        )
        :*/
        client->Get(path, 
            [&](const char *data, size_t data_length) {
                body.append(data, data_length);
                //NLog::Write(Level::Info, Fmt("body size : %d", body.size()));
                return true;
            }, 
            [&](uint64_t len, uint64_t total) {
                success = (len == total);
                //NLog::Write(Level::Info, Fmt("%lld / %lld bytes => %d%% complete", len, total, (int)(len*100/total)));
                return true;
            }
        );
        
        // for (auto& pair : res.res_->headers) {
        //     NLog::Write(Level::Info, Fmt("- k: %s, v: %s", pair.first.c_str(), pair.second.c_str()));
        // }

        if (res.error() != httplib::Error::Success) {
            LOGE("HttpError : {}, {}", (int)res.error(), httplib::to_string(res.error()).c_str());
            return HttpResponse {
                .status = -1,
                .body = "",
            };
        }

        if (download_cbk) {
            download_cbk(body, success);
        }

        return HttpResponse {
            .status = res->status,
            .body = res->body,
        };
    }

    int HttpClient::HeadFileSize() {
        //auto result = ssl ? ssl_client->Head(path) : client->Head(path);
        auto result = client->Head(path);
        // for (auto& pair : result.res_->headers) {
        //    NLog::Write(Level::Info, Fmt("* k: %s, v: %s", pair.first.c_str(), pair.second.c_str()));
        // }
        std::string key_content_length = "Content-Length";
        if (result->has_header(key_content_length)) {
            auto value = result->get_header_value(key_content_length);
            return std::atoi(value.c_str());
        }
        return 0;
    }

}