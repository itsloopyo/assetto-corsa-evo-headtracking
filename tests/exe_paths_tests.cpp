// The path resolution every file the mod touches hangs off. The failure that
// matters is not a wrong directory but a plausible-looking empty one: a
// separator-less module path must NOT resolve to "", because the INI path built
// from it would then be "\HeadTracking.ini" and the mod would write the user's
// config to the root of whatever drive the process happens to be on.
//
// No game needed - only the running test EXE's own module path.

#include "exe_paths.h"

#include "test_support.h"

#include <windows.h>

#include <cstdio>
#include <string>

using namespace ace_ht;
using ace_test::Check;

namespace {

void DirectoryOfTests() {
    std::printf("DirectoryOf splits a module path\n");

    std::wstring dir;
    Check(DirectoryOf(L"C:\\Games\\AssettoCorsaEVO\\AssettoCorsaEVO.exe", dir)
              && dir == L"C:\\Games\\AssettoCorsaEVO",
          "a normal install path yields the containing directory");

    Check(DirectoryOf(L"C:\\AssettoCorsaEVO.exe", dir) && dir == L"C:",
          "an EXE at a drive root yields the drive");

    Check(DirectoryOf(L"\\\\server\\share\\game\\AssettoCorsaEVO.exe", dir)
              && dir == L"\\\\server\\share\\game",
          "a UNC path yields the containing directory");

    // The whole reason this is a separate function: the outputs below must stay
    // untouched, not become "".
    std::wstring untouched = L"sentinel";
    Check(!DirectoryOf(L"AssettoCorsaEVO.exe", untouched) && untouched == L"sentinel",
          "a separator-less path fails instead of yielding an empty directory");
    Check(!DirectoryOf(L"", untouched) && untouched == L"sentinel",
          "an empty path fails instead of yielding an empty directory");
}

void NarrowToAnsiTests() {
    std::printf("NarrowToAnsi converts for the ANSI-only INI layer\n");

    std::string narrow;
    Check(NarrowToAnsi(L"C:\\Games\\AssettoCorsaEVO", narrow)
              && narrow == "C:\\Games\\AssettoCorsaEVO",
          "an ASCII directory converts unchanged");

    Check(NarrowToAnsi(L"C:\\Games\\a b", narrow) && narrow == "C:\\Games\\a b",
          "spaces survive the conversion");

    // An empty wide string cannot be told apart from a conversion failure by
    // WideCharToMultiByte's return, so it reports failure. Reachable only from a
    // module path like "\game.exe", where a failure correctly leaves the mod
    // dormant rather than writing to a drive root.
    std::string untouched = "sentinel";
    Check(!NarrowToAnsi(L"", untouched) && untouched == "sentinel",
          "an empty directory fails instead of yielding an empty string");
}

void ExeDirectoryTests() {
    std::printf("ExeDirectory resolves the running module in both encodings\n");

    std::wstring wide;
    std::string narrow;
    if (!Check(ExeDirectory(wide, narrow), "resolves for the running test EXE")) return;

    if (!Check(!wide.empty() && !narrow.empty(), "neither encoding comes back empty")) return;
    Check(wide.back() != L'\\', "no trailing separator, so exe_dir + \"\\\" + name is well formed");

    std::string expected;
    Check(NarrowToAnsi(wide, expected) && expected == narrow,
          "the narrow form is the narrowing of the wide form");

    // The resolved directory must actually be this EXE's, not merely a
    // plausible string: appending the file name has to reconstruct the module
    // path Windows reports.
    wchar_t module_path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, module_path, MAX_PATH);
    Check(length > 0 && length < MAX_PATH, "the test EXE's own module path is readable");
    if (length == 0 || length >= MAX_PATH) return;

    const std::wstring full(module_path, length);
    Check(full.compare(0, wide.size(), wide) == 0 && full[wide.size()] == L'\\',
          "the directory is a path prefix of the running module");
    Check(GetFileAttributesW(wide.c_str()) != INVALID_FILE_ATTRIBUTES,
          "the directory exists on disk");
}

}  // namespace

int main() {
    std::printf("Assetto Corsa EVO head tracking - exe path tests\n");
    std::printf("===============================================\n");
    DirectoryOfTests();
    NarrowToAnsiTests();
    ExeDirectoryTests();
    return ace_test::Summary("exe paths");
}
