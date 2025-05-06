#include "image.h"
#include "log.h"

#ifdef WIN32
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
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

    std::shared_ptr<Image> Image::Make(const DataPtr& data, int width, int height, const RawImageType& rt) {
        auto image = Image::Make(data, width, height);
        image->raw_img_type_ = rt;
        return image;
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
        // Use OpenCV
#if 0
        std::vector<char> buffer;
        buffer.resize(img_data->Size());
        memcpy(buffer.data(), img_data->DataAddr(), img_data->Size());
        //CV_LOAD_IMAGE_COLOR , this flag no alpha ??
        //auto img = cv::imdecode(buffer, -1);
        //this->width = img.cols;
        //this->height = img.rows;
        //this->channels = img.channels();
        //this->data = Data::Make((char*)img.data, img.rows * img.cols * img.channels());
#else
    // Use stb image
        auto buffer = stbi_load_from_memory((stbi_uc*)img_data->DataAddr(), img_data->Size(), &this->width, &this->height, &this->channels, 0);
        if (buffer == nullptr) {
            LOGE("stbi load image failed !");
            return;
        }
        this->data = Data::Make((char*)buffer, this->width * this->height * this->channels);
#endif
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

    std::shared_ptr<Image> Image::Duplicate(const std::shared_ptr<Image> image) {
        if (!image) {
            return nullptr;
        }
        auto new_image = Image::Make(image->data, image->width, image->height);
        return new_image;
    }
}
