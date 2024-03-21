#ifndef IMAGE_H
#define IMAGE_H

#include "data.h"

#include <string>

namespace tc
{

    class Image {
    public:
        static std::shared_ptr<Image> Make(const char* data, int width, int height, int channels, int monitor_index = 0);
        static std::shared_ptr<Image> Make(const DataPtr& data, int width, int height, int channels, int monitor_index = 0);
        static std::shared_ptr<Image> Make(const DataPtr&, int width, int height, int monitor_index = 0);
#ifdef WIN32
        // 图片是jpg png等有压缩格式的
        static std::shared_ptr<Image> MakeByCompressedImage(const DataPtr& data, int monitor_index = 0);
#endif

        Image() = delete;
        ~Image();
        Image(const char* data, int width, int height, int channels, int monitor_index = 0);
        Image(const DataPtr& img_data, int width, int height, int channels, int monitor_index = 0);
        Image(const DataPtr& img_data, int monitor_index = 0);

        int GetWidth();
        int GetHeight();
        int GetChannels();
        DataPtr GetData();
        int GetInternalFormat();
        std::string GetPath();

        void SetPath(const std::string& path);

    public:
        int width;
        int height;
        int channels;
        DataPtr data;

        std::string path;

        std::shared_ptr<Image> Make(const DataPtr &data);
        // 当多屏幕采集时候，表示当前采集屏幕的索引
        int monitor_index_ = 0;
    };

    typedef std::shared_ptr<Image> ImagePtr;

}

#endif // IMAGE_H
