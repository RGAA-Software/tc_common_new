#include "tc_aes.h"
#include <iostream>

namespace tc {

	bool AesEncryptPcks7Cbc128(const unsigned char* plaintext, int plaintext_len, const unsigned char* key, const unsigned char* iv, std::vector<unsigned char>& ciphertext) {

		EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new(); // 创建加密上下文
		if (!ctx) {
			std::cerr << "Failed to create cipher context." << std::endl;
			return false;
		}

		// 初始化加密操作 (AES-128-CBC)
		if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key, iv) != 1) {
			std::cerr << "Failed to initialize AES encryption." << std::endl;
			EVP_CIPHER_CTX_free(ctx);
			return false;
		}

		// 设置填充模式为 PKCS7（默认情况下 OpenSSL 使用 PKCS7 填充）
		EVP_CIPHER_CTX_set_padding(ctx, 1);

		// 分配缓冲区：密文长度需要考虑填充大小
		int block_size = EVP_CIPHER_block_size(EVP_aes_128_cbc());
		int ciphertext_len = plaintext_len + block_size; // 最大可能长度
		ciphertext.resize(ciphertext_len);

		int len = 0;
		int total_len = 0;

		// 加密数据
		if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext, plaintext_len) != 1) {
			std::cerr << "Failed to encrypt data." << std::endl;
			EVP_CIPHER_CTX_free(ctx);
			return false;
		}
		total_len += len;

		// 加密最后的填充块
		if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total_len, &len) != 1) {
			std::cerr << "Failed to finalize encryption." << std::endl;
			EVP_CIPHER_CTX_free(ctx);
			return false;
		}
		total_len += len;

		// 调整密文大小
		ciphertext.resize(total_len);

		EVP_CIPHER_CTX_free(ctx); // 释放上下文
		return true;
	}
}