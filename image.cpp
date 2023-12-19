#include "image.h"
#include "log.h"

#ifdef WIN32
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#endif

namespace tc
{

    std::shared_ptr<Image> Image::Make(const char* data, int width, int height, int channels) {
        return std::make_shared<Image>(data, width, height, channels);
    }

    std::shared_ptr<Image> Image::Make(const DataPtr& data, int width, int height, int channels) {
        return std::make_shared<Image>(data, width, height, channels);
    }

    std::shared_ptr<Image> Image::Make(const DataPtr& data, int width, int height) {
        return std::make_shared<Image>(data, width, height, data ? data->Size()/width/height : 0);
    }

#ifdef WIN32
    std::shared_ptr<Image> Image::MakeByCompressedImage(const DataPtr& data) {
        return std::make_shared<Image>(data);
    }
#endif

    Image::Image(const char* data, int width, int height, int channels) {
        this->data = Data::Make(data, width*height*channels);
        this->width = width;
        this->height = height;
        this->channels = channels;
    }

    Image::Image(const DataPtr& data, int width, int height, int channels) {
        this->data = data;
        this->width = width;
        this->height = height;
        this->channels = channels;
    }

#ifdef WIN32
    Image::Image(const DataPtr& img_data) {
        std::vector<char> buffer;
        buffer.resize(img_data->Size());
        memcpy(buffer.data(), img_data->DataAddr(), img_data->Size());

        //CV_LOAD_IMAGE_COLOR , this flag no alpha ??
        auto img = cv::imdecode(buffer, -1);
        this->width = img.cols;
        this->height = img.rows;
        this->channels = img.channels();

        this->data = Data::Make((char*)img.data, img.rows * img.cols * img.channels());
    }
#endif

    Image::~Image() {
    }

    int Image::GetWidth() {
        return width;
    }

    int Image::GetHeight() {
        return height;
    }

    int Image::GetChannels() {
        return channels;
    }

    DataPtr Image::GetData() {
        return data;
    }

    int Image::GetInternalFormat() {
        //#define GL_RGB                            0x1907
        //#define GL_RGBA                           0x1908
        if (channels == 3) {
            return 0x1907;// GL_RGB;
        } else if (channels == 4) {
            return 0x1908;// GL_RGBA;
        }
        return 0x1907;// GL_RGB;
    }

    void Image::SetPath(const std::string &path) {
        this->path = path;
    }

    std::string Image::GetPath() {
        return path;
    }

}
