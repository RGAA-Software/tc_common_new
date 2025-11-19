#pragma once 

#include <map>
#include <string>
#include <memory>
#include <functional>

#include <cpr/error.h>
#include "cpr/cpr.h"
#include "cpr/cprtypes.h"
#include "cpr/redirect.h"
#include "cpr/session.h"

namespace tc
{

    class HttpResponse {
    public:
        int status = -1;
        std::string body;
    };

    class HttpClient {
    public:

        static std::shared_ptr<HttpClient> Make(const std::string& host, int port, const std::string& path, int timeout_ms = 2000);
        static std::shared_ptr<HttpClient> MakeSSL(const std::string& host, int port, const std::string& path, int timeout_ms = 2000);
        //static std::shared_ptr<HttpClient> MakeDownloadHttp(const std::string& url);
        //static std::shared_ptr<HttpClient> MakeDownloadHttps(const std::string& url);

        HttpClient(const std::string& host, int port, const std::string& path, bool ssl, int timeout_ms = 2000);
        ~HttpClient();

        HttpResponse Request();
        HttpResponse Request(const std::map<std::string, std::string>& query, const std::string& body = "");
        HttpResponse Post();
        HttpResponse Post(const std::map<std::string, std::string>& query, const std::string& body = "");
        HttpResponse PostMultiPart(const std::map<std::string, std::string>& query,
                                   const std::map<std::string, std::string>& form_parts,
                                   const std::map<std::string, std::string>& file_parts);

        HttpResponse Download(std::function<void(const std::string& body, bool success)>&& download_cbk);
        
        int HeadFileSize();
        std::string GetReqPath();

    private:
        std::string host_;
        int port_ = 0;
        std::string path;
        bool ssl_ = false;
        int timeout_ms_ = 3000;
        std::string req_path_;

    };

}