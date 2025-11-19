
#include "zip_util.h"
#include "miniz/miniz.h"
#include "string_util.h"
#include <filesystem>
#include <iostream>

namespace tc
{

    bool ZipUtil::ZipFolder(const std::string& directory_path, const std::string& zip_file_path) {
        namespace fs = std::filesystem;
        // 创建zip归档
        mz_zip_archive zip_archive;
        memset(&zip_archive, 0, sizeof(zip_archive));

        // 初始化zip归档
        if (!mz_zip_writer_init_file(&zip_archive, zip_file_path.c_str(), 0)) {
            std::cerr << "Failed to initialize zip archive" << std::endl;
            return false;
        }

        // 成功压缩的文件计数
        size_t success_count = 0;
        size_t skip_count = 0;

        try {
            // 递归遍历目录
            for (const auto& entry : fs::recursive_directory_iterator(directory_path)) {
                if (entry.is_regular_file()) {
                    // 获取文件路径
                    std::string file_path = entry.path().string();

                    // 计算在zip中的相对路径
                    std::string relative_path = fs::relative(entry.path(), directory_path).string();
                    std::replace(relative_path.begin(), relative_path.end(), '\\', '/');

                    // 尝试添加文件到zip，忽略任何错误
                    bool added = mz_zip_writer_add_file(&zip_archive, relative_path.c_str(),
                                                        file_path.c_str(), nullptr, 0,
                                                        MZ_BEST_COMPRESSION);

                    if (added) {
                        success_count++;
                        std::cout << "✓ Added: " << relative_path << std::endl;
                    } else {
                        skip_count++;
                        std::cout << "✗ Skipped (may be locked): " << relative_path << std::endl;
                        // 继续处理下一个文件，不退出
                    }
                }
                else if (entry.is_directory()) {
                    // 对于目录，需要在zip中创建对应的目录条目
                    std::string dir_path = fs::relative(entry.path(), directory_path).string();
                    std::replace(dir_path.begin(), dir_path.end(), '\\', '/');

                    // 确保目录路径以斜杠结尾
                    if (!dir_path.empty() && dir_path.back() != '/') {
                        dir_path += '/';
                    }

                    // 添加目录条目到zip，忽略错误
                    mz_zip_writer_add_mem(&zip_archive, dir_path.c_str(),
                                          nullptr, 0, MZ_BEST_COMPRESSION);
                    // 目录创建失败也不影响继续处理
                }
            }

            // 完成并关闭zip归档
            if (!mz_zip_writer_finalize_archive(&zip_archive)) {
                std::cerr << "Warning: Failed to finalize zip archive properly" << std::endl;
                // 即使finalize失败，也尝试结束
            }

        } catch (const fs::filesystem_error& ex) {
            std::cerr << "Filesystem error (continuing): " << ex.what() << std::endl;
            // 继续执行，不立即返回
        } catch (const std::exception& ex) {
            std::cerr << "Error (continuing): " << ex.what() << std::endl;
            // 继续执行，不立即返回
        }

        // 结束zip写入
        if (!mz_zip_writer_end(&zip_archive)) {
            std::cerr << "Warning: Failed to end zip writer properly" << std::endl;
        }

        std::cout << "\nCompression summary:" << std::endl;
        std::cout << "Successfully compressed: " << success_count << " files" << std::endl;
        std::cout << "Skipped (possibly locked): " << skip_count << " files" << std::endl;
        std::cout << "Output: " << zip_file_path << std::endl;

        // 只要成功创建了zip文件就返回true，即使有些文件被跳过
        auto w_path = StringUtil::ToWString(zip_file_path);
        return fs::exists(w_path) && fs::file_size(w_path) > 0;
    }

}