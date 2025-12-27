#include "http_client.h"

#include "log.h"
#include <memory>
#include <string>
#include <cpr/error.h>
#include <cpr/cpr.h>
#include <cpr/cprtypes.h>
#include <cpr/redirect.h>
#include <cpr/session.h>

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
        cpr::Parameters params;
        for (const auto& [k, v] : query) {
            // params
            params.Add({k, v});
        }

        //req_path_ = std::format("{}{}:{}{}", ssl_ ? "https://" : "http://", host_, port_, query_path);
        auto url_path = std::format("{}{}:{}{}", ssl_ ? "https://" : "http://", host_, port_, path);
        cpr::Url url{url_path};
        cpr::Session session;
        session.SetUrl(url);
        session.SetBody(body);
        session.SetVerifySsl(false);
        session.SetTimeout(cpr::Timeout{this->timeout_ms_});
        session.SetHeader(cpr::Header{{"Authorization", "Bearer token"}});
        session.SetParameters(params);

        cpr::Response response = session.Get();
        req_path_ = response.url.str();

        // EXPECT_EQ(expected_text, response.text);
        // EXPECT_EQ(Url{url + "?key=value&hello=world&test=case"}, response.url);
        // EXPECT_EQ(std::string{"text/html"}, response.header["content-type"]);
        // EXPECT_EQ(200, response.status_code);
        // EXPECT_EQ(cpr::ErrorCode::OK, response.error.code);
        return HttpResponse {
            .status = (int)response.status_code,
            .body = response.text,
        };
    }

    HttpResponse HttpClient::Post() {
        std::map<std::string, std::string> params;
        return Post(params);
    }

    HttpResponse HttpClient::Post(const std::map<std::string, std::string>& query, const std::string& body, const std::string content_type) {
        cpr::Parameters params;
        for (const auto& [k, v] : query) {
            // params
            params.Add({k, v});
        }

        auto url_path = std::format("{}{}:{}{}", ssl_ ? "https://" : "http://", host_, port_, path);
        cpr::Url url{url_path};
        cpr::Session session;
        session.SetUrl(url);
        session.SetVerifySsl(false);
        session.SetBody(body);
        session.SetTimeout(cpr::Timeout{this->timeout_ms_});
        auto header = cpr::Header{ {"Authorization", "Bearer token"} };
        if (!content_type.empty()) {
            header.insert({ "Content-Type", content_type });
        }
        session.SetHeader(header);
        session.SetParameters(params);

        cpr::Response response = session.Post();
        req_path_ = response.url.str();

        // EXPECT_EQ(expected_text, response.text);
        // EXPECT_EQ(Url{url + "?key=value&hello=world&test=case"}, response.url);
        // EXPECT_EQ(std::string{"text/html"}, response.header["content-type"]);
        // EXPECT_EQ(200, response.status_code);
        // EXPECT_EQ(cpr::ErrorCode::OK, response.error.code);
        return HttpResponse {
            .status = (int)response.status_code,
            .body = response.text,
        };
    }

    //auto resp = client.PostMultiPart(
    //    // query 参数
    //    {
    //        {"uid", "123"},
    //        {"debug", "1"}
    //    },
    //
    //    // form 字段
    //    {
    //        {"type", "avatar"},
    //        {"desc", "hello multipart"}
    //    },
    //
    //    // file 字段
    //    {
    //        {"avatar", "C:/img/a.png"},
    //        {"cover",  "C:/img/b.jpg"}
    //    }
    //);
    HttpResponse HttpClient::PostMultiPart(const std::map<std::string, std::string>& query,
                                           const std::map<std::string, std::string>& form_parts,
                                           const std::map<std::string, std::string>& file_parts) {
        // 构造 URL
        auto url_path = std::format("{}{}:{}{}", ssl_ ? "https://" : "http://", host_, port_, path);

        cpr::Url url{url_path};
        cpr::Session session;
        session.SetUrl(url);
        session.SetVerifySsl(false);
        session.SetTimeout(cpr::Timeout{timeout_ms_});
        session.SetHeader(cpr::Header{{"Authorization", "Bearer token"}});

        // --- URL Query ---
        if (!query.empty()) {
            cpr::Parameters params;
            for (auto& [k, v] : query) {
                params.Add({k, v});
            }
            session.SetParameters(params);
        }

        // --- Multipart ---
        cpr::Multipart multipart{};

        // 添加表单字段
        for (auto& [k, v] : form_parts) {
            multipart.parts.emplace_back(k, v);  // text field
        }

        // 添加文件字段
        for (auto& [k, path] : file_parts) {
            multipart.parts.emplace_back(
                    k,
                    cpr::File{path}  // 自动推断 MIME
            );
        }

        // 设置 multipart
        session.SetMultipart(multipart);

        // POST
        cpr::Response response = session.Post();
        req_path_ = response.url.str();

        return HttpResponse{
            .status = (int)response.status_code,
            .body   = response.text,
        };
    }

    HttpResponse HttpClient::Download(const std::string& path, std::function<void(const std::string& body)>&& download_cbk) {
        LOGI("Download: {}", path.c_str());
        cpr::Url url{path};
        cpr::Session session;
        session.SetHeader(cpr::Header{{"Accept-Encoding", "gzip"}});
        session.SetUrl(url);
        session.SetVerifySsl(false);
        session.SetTimeout(cpr::Timeout{5000});

        auto fn_cbk = [=](std::string_view data, intptr_t /*userdata*/) -> bool {
            LOGI("Download size: {}", data.size());
            auto cpy = std::string(data.data(), data.size());
            download_cbk(cpy);
            return true;
        };

        cpr::Response response = session.Download(cpr::WriteCallback{fn_cbk, 0});

        return HttpResponse{
            .status = (int)response.status_code,
            .body   = response.text,
        };
    }

    int HttpClient::HeadFileSize() {

        return 0;
    }

    std::string HttpClient::GetReqPath() {
        return req_path_;
    }

}