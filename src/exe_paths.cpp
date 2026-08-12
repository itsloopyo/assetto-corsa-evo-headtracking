#include "exe_paths.h"

#include <windows.h>

namespace ace_ht {

bool DirectoryOf(const std::wstring& module_path, std::wstring& directory) {
    const size_t separator = module_path.find_last_of(L'\\');
    if (separator == std::wstring::npos) return false;

    directory = module_path.substr(0, separator);
    return true;
}

bool NarrowToAnsi(const std::wstring& wide, std::string& narrow) {
    const int bytes = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), static_cast<int>(wide.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return false;

    std::string converted(static_cast<size_t>(bytes), '\0');
    if (WideCharToMultiByte(CP_ACP, 0, wide.c_str(), static_cast<int>(wide.size()),
                            converted.data(), bytes, nullptr, nullptr) != bytes) {
        return false;
    }

    narrow = converted;
    return true;
}

bool ExeDirectory(std::wstring& wide, std::string& narrow) {
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return false;

    std::wstring directory;
    if (!DirectoryOf(std::wstring(path, length), directory)) return false;

    std::string converted;
    if (!NarrowToAnsi(directory, converted)) return false;

    wide = directory;
    narrow = converted;
    return true;
}

}  // namespace ace_ht
