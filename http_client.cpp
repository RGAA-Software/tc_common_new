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
        session.SetTimeout(cpr::Timeout{5000});
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
            .status = response.status_code,
            .body = response.text,
        };
    }

    HttpResponse HttpClient::Post() {
        std::map<std::string, std::string> params;
        return Post(params);
    }

    HttpResponse HttpClient::Post(const std::map<std::string, std::string>& query, const std::string& body) {
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
        session.SetTimeout(cpr::Timeout{5000});
        session.SetHeader(cpr::Header{{"Authorization", "Bearer token"}});
        session.SetParameters(params);

        cpr::Response response = session.Post();
        req_path_ = response.url.str();

        // EXPECT_EQ(expected_text, response.text);
        // EXPECT_EQ(Url{url + "?key=value&hello=world&test=case"}, response.url);
        // EXPECT_EQ(std::string{"text/html"}, response.header["content-type"]);
        // EXPECT_EQ(200, response.status_code);
        // EXPECT_EQ(cpr::ErrorCode::OK, response.error.code);
        return HttpResponse {
            .status = response.status_code,
            .body = response.text,
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

    std::string HttpClient::GetReqPath() {
        return req_path_;
    }

}