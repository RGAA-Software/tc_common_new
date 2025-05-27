#pragma once 

#include <map>
#include <string>
#include <memory>
#include <functional>

namespace tc
{

    class HttpResponse {
    public:
        int status = -1;
        std::string body;
    };

    class HttpClient {
    public:

        static std::shared_ptr<HttpClient> Make(const std::string& host, int port, const std::string& path, int timeout_ms = 3000);
        static std::shared_ptr<HttpClient> MakeSSL(const std::string& host, int port, const std::string& path, int timeout_ms = 3000);
        //static std::shared_ptr<HttpClient> MakeDownloadHttp(const std::string& url);
        //static std::shared_ptr<HttpClient> MakeDownloadHttps(const std::string& url);

        HttpClient(const std::string& host, int port, const std::string& path, bool ssl, int timeout_ms = 3000);
        ~HttpClient();

        HttpResponse Request();
        HttpResponse Request(const std::map<std::string, std::string>& query);
        HttpResponse Post();
        HttpResponse Post(const std::map<std::string, std::string>& query);

        HttpResponse Download(std::function<void(const std::string& body, bool success)>&& download_cbk);
        
        int HeadFileSize();

    private:
        std::string host;
        int port_ = 0;
        std::string path;
        bool ssl = false;
        int timeout_ms_ = 1000;

    };

}