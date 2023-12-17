//
// Created by RGAA on 2023-12-17.
//

#include <gtest/gtest.h>

#include "../uuid.h"
#include "../data.h"
#include "../md5.h"
#include "../random.h"

using namespace tc;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest();
    return RUN_ALL_TESTS();
    return 0;
}

TEST(Test_GenUUID, gen_uuid) {
    auto uuid = tc::GetUUID();
    std::cout << "UUID: " << uuid << std::endl;
}

TEST(Test_Data_String, data_string) {
    auto data = Data::Make(nullptr, 100);
    memcpy(data->DataAddr(), "Jack", 4);
    auto str = data->AsString();
    std::cout << "Str: " << str << std::endl;
}

TEST(Test_MD5, md5) {
    auto str = tc::MD5::Hex("jack");
    std::cout << "MD5: " << str << std::endl;
}

TEST(Test_Random, random) {
    for (int i = 0; i < 10; i++) {
        std::cout << "Random: " << Random::RandT(1, 10) << std::endl;
    }
}