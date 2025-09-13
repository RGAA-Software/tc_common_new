#ifndef DATA_H
#define DATA_H

#include <memory>
#include <string>

namespace tc
{

    class Data {
    public:
        static std::shared_ptr<Data> Make(const char* data, int64_t size);
        static std::shared_ptr<Data> From(const std::string& data);

        Data(const char *data, int64_t size);
        ~Data();

        const char *CStr();
        char* DataAddr();
        int Size();
        char At(int64_t offset);
        std::string AsString();
        void ConvertToStr(std::string& out);
        std::shared_ptr<Data> Dup();
        bool Append(char* data, int64_t size);
        int64_t Offset();
        void Reset();
        void Save(const std::string& path);
        std::shared_ptr<Data> Clone();

    private:

        char* data_{nullptr};
        int64_t size_ = 0;
        int64_t offset_ = 0;
};


typedef std::shared_ptr<Data> DataPtr;


}
#endif // DATA_H
