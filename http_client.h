#pragma once 

#include <map>
#include <string>
#include <memory>

//#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "http/httplib.h"

namespace tc
{

    class HttpResponse {
    public:
        int status = -1;
        std::string body;
    };

    class HttpClient {
    public:

        static std::shared_ptr<HttpClient> Make(const std::string& host, const std::string& path, int timeout = 3);
        static std::shared_ptr<HttpClient> MakeSSL(const std::string& host, const std::string& path, int timeout = 3);
        static std::shared_ptr<HttpClient> MakeDownloadHttp(const std::string& url);
        static std::shared_ptr<HttpClient> MakeDownloadHttps(const std::string& url);

        HttpClient(const std::string& host, const std::string& path, bool ssl, int timeout = 3);
        ~HttpClient();

        HttpResponse Request();
        HttpResponse Request(const std::map<std::string, std::string>& query);
        HttpResponse Post();
        HttpResponse Post(const std::map<std::string, std::string>& query);

        HttpResponse Download(std::function<void(const std::string& body, bool success)>&& download_cbk);
        
        int HeadFileSize();

    private:

        std::string host; 
        std::string path;
        bool ssl = false;

        std::shared_ptr<httplib::Client> client = nullptr;
#if 0
        std::shared_ptr<httplib::SSLClient> ssl_client = nullptr;
#endif

    };

}