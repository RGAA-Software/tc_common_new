#include "data.h"

#ifdef WIN32
#include "jemalloc/jemalloc.h"
#endif

namespace tc
{

    Data::Data(const char* src, int64_t size) {
#if defined(WIN32) || (defined(__linux__) && !defined(__ANDROID__))
        this->data_ = (char*)je_malloc(size);
#elif defined(__ANDROID__)
        this->data_ = (char *) malloc(size);
#endif
        if (src) {
            memcpy(this->data_, src, size);
        }
        this->size_ = size;
    }

    std::shared_ptr<Data> Data::From(const std::string& data) {
        return std::make_shared<Data>((char*)data.data(), (int)data.size());
    }

    Data::~Data() {
        if (this->data_) {
#if defined(WIN32) || (defined(__linux__) && !defined(__ANDROID__))
            je_free(this->data_);
#elif defined(__ANDROID__)
            free(this->data_);
#endif
        }
    }

    const char *Data::CStr() {
        return this->data_;
    }

    std::string Data::AsString() {
        std::string val;
        val.resize(size_);
        memcpy(val.data(), this->data_, size_);
        return val;
    }

    void Data::ConvertToStr(std::string& out) {
        out.resize(size_);
        memcpy((char*)out.data(), this->data_, size_);
    }

    int Data::Size() {
        return this->size_;
    }

    std::shared_ptr<Data> Data::Make(const char *data_, int64_t size) {
        return std::make_shared<Data>(data_, size);
    }

    char Data::At(int64_t offset) {
        return *(this->data_ + offset);
    }

    char* Data::DataAddr() {
        return this->data_;
    }

    std::shared_ptr<Data> Data::Dup() {
        return Data::Make(this->data_, this->size_);
    }

    bool Data::Append(char* data, int64_t size) {
        if (offset_ + size > size_) {
            return false;
        }
        memcpy(data_ + offset_, data, size);
        offset_ += size;
        return true;
    }

    int64_t Data::Offset() {
        return offset_;
    }

    void Data::Reset() {
        offset_ = 0;
    }

    void Data::Save(const std::string& path) {
//        auto file = File::OpenForWriteB(path);
//        file->Write(0, data_, size_);
//        file->Close();
    }

}
