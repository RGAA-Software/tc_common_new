#include "clipboard_file_builder.h"
#include "tc_common_new/folder_util.h"
#include "tc_common_new/string_util.h"
#include "tc_common_new/log.h"
#include <climits>
#include <filesystem>

namespace fs = std::filesystem;

namespace tc::clipboard
{
    namespace {

        std::optional<FileEntry> MakeFileEntry(const fs::path& base_folder, const fs::path& full_path) {
            std::error_code ec;
            if (!fs::exists(full_path, ec) || ec) {
                return std::nullopt;
            }

            const auto base_u8 = PathToUTF8(base_folder);
            const auto full_u8 = PathToUTF8(full_path);
            if (full_u8.find(base_u8) != 0) {
                LOGE("clipboard file not under base folder, {} => {}", base_u8, full_u8);
                return std::nullopt;
            }

            std::string ref_path = full_u8.substr(base_u8.size());
            while (!ref_path.empty() && (ref_path.front() == '/' || ref_path.front() == '\\')) {
                ref_path.erase(ref_path.begin());
            }

            FileEntry entry;
            entry.full_path_ = full_u8;
            entry.file_name_ = PathToUTF8(full_path.filename());
            entry.ref_path_ = ref_path;
            entry.total_size_ = static_cast<int64_t>(fs::file_size(full_path, ec));
            if (ec) {
                entry.total_size_ = 0;
            }
            return entry;
        }

        void CollectPathsRecursive(const fs::path& path, std::vector<std::string>& out) {
            std::error_code ec;
            if (fs::is_directory(path, ec)) {
                FolderUtil::VisitRecursiveFiles(U8Path(path), 0, INT_MAX, [&](VisitResult&& r) {
                    out.push_back(PathToUTF8(r.path_));
                });
            } else if (fs::exists(path, ec)) {
                out.push_back(PathToUTF8(path));
            }
        }

        fs::path ResolveBaseFolder(const std::vector<std::string>& full_paths,
                                   const std::vector<std::string>& expanded) {
            for (const auto& p : full_paths) {
                const auto path = U8Path(p);
                std::error_code ec;
                if (fs::is_directory(path, ec)) {
                    return path;
                }
            }
            if (!expanded.empty()) {
                return U8Path(expanded[0]).parent_path();
            }
            return {};
        }

    }

    std::optional<std::vector<FileEntry>> BuildFileEntriesFromPaths(const std::vector<std::string>& full_paths) {
        if (full_paths.empty()) {
            return std::nullopt;
        }

        std::vector<std::string> expanded;
        for (const auto& p : full_paths) {
            CollectPathsRecursive(U8Path(p), expanded);
        }
        if (expanded.empty()) {
            return std::nullopt;
        }

        fs::path base_folder = ResolveBaseFolder(full_paths, expanded);
        if (base_folder.empty()) {
            return std::nullopt;
        }

        std::vector<FileEntry> entries;
        for (const auto& full_u8 : expanded) {
            auto entry = MakeFileEntry(base_folder, U8Path(full_u8));
            if (entry) {
                entries.push_back(entry.value());
            }
        }
        if (entries.empty()) {
            return std::nullopt;
        }
        return entries;
    }

}
