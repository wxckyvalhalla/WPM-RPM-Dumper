#include "imports.h"
#include "globals.h"

namespace fs = std::filesystem;

std::mutex g_fileMutex;

std::wstring string_to_wstring(const std::string& str)
{
    if (str.empty()) return L"";

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    if (size_needed == 0) return L"";

    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

std::string wstring_to_string(const std::wstring& wstr)
{
    if (wstr.empty()) return "";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed == 0) return "";

    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

std::string GenerateUniqueFilename(const std::string& baseName, const std::string& extension,
    const std::string& directory = globals::dumpPath)
{
    std::lock_guard<std::mutex> lock(g_fileMutex);

    for (int i = 1; i <= 1000; ++i) {
        std::string path = directory + baseName + "." + std::to_string(i) + extension;
        if (!fs::exists(path)) {
            return path;
        }
    }
    return "";
}

std::wstring GenerateUniqueFilenameW(const std::wstring& baseName, const std::wstring& extension,
    const std::wstring& directory = string_to_wstring(globals::dumpPath))
{
    std::lock_guard<std::mutex> lock(g_fileMutex);

    for (int i = 1; i <= 1000; ++i) {
        std::wstring path = directory + baseName + L"." + std::to_wstring(i) + extension;
        if (!fs::exists(path)) {
            return path;
        }
    }
    return L"";
}

bool SafeWriteToFile(const std::string& filename, const void* data, size_t size)
{
    if (filename.empty() || !data || size == 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_fileMutex);

    try {
        fs::path filePath(filename);
        if (!fs::exists(filePath.parent_path())) {
            fs::create_directories(filePath.parent_path());
        }

        std::ofstream outputFile(filename, std::ios::binary);
        if (outputFile.is_open()) {
            outputFile.write(reinterpret_cast<const char*>(data), size);
            return outputFile.good();
        }
    }
    catch (const std::exception& e) {
        std::cerr << _XOR_("File write error: ") << e.what() << std::endl;
    }

    return false;
}

bool SafeWriteToFileW(const std::wstring& filename, const void* data, size_t size)
{
    if (filename.empty() || !data || size == 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_fileMutex);

    try {
        fs::path filePath(filename);
        if (!fs::exists(filePath.parent_path())) {
            fs::create_directories(filePath.parent_path());
        }

        std::ofstream outputFile(filename, std::ios::binary);
        if (outputFile.is_open()) {
            outputFile.write(reinterpret_cast<const char*>(data), size);
            return outputFile.good();
        }
    }
    catch (const std::exception& e) {
        std::wcerr << _XOR_(L"File write error: ") << e.what() << std::endl;
    }

    return false;
}

typedef BOOL(WINAPI* tGetThreadContext)(HANDLE hThread, LPCONTEXT lpContext);
tGetThreadContext pGetThreadContext = nullptr;

BOOL WINAPI hookedGetThreadContext(HANDLE hThread, LPCONTEXT lpContext)
{
    BOOL result = pGetThreadContext(hThread, lpContext);

    if (result && lpContext && (lpContext->ContextFlags & (CONTEXT_DEBUG_REGISTERS | 0x7F))) {
        lpContext->ContextFlags &= ~CONTEXT_DEBUG_REGISTERS;
        lpContext->Dr0 = 0;
        lpContext->Dr1 = 0;
        lpContext->Dr2 = 0;
        lpContext->Dr3 = 0;
        lpContext->Dr6 = 0;
        lpContext->Dr7 = 0;
    }

    return result;
}

typedef BOOL(WINAPI* tWriteProcessMemory)(HANDLE hProcess, LPVOID lpBaseAddress,
    LPCVOID lpBuffer, SIZE_T nSize,
    SIZE_T* lpNumberOfBytesWritten);
tWriteProcessMemory pWriteProcessMemory = nullptr;

BOOL WINAPI hookedWriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress,
    LPCVOID lpBuffer, SIZE_T nSize,
    SIZE_T* lpNumberOfBytesWritten)
{
    if (lpBuffer && nSize > 0) {
        std::string filename = GenerateUniqueFilename("writeprocessmemory_buffer", ".bin");
        if (!filename.empty() && SafeWriteToFile(filename, lpBuffer, nSize)) {
            std::cout << _XOR_(" [WriteProcessMemory-Dumped] ") << filename << std::endl;
        }
    }

    return pWriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

typedef BOOL(WINAPI* tReadProcessMemory)(HANDLE hProcess, LPCVOID lpBaseAddress,
    LPVOID lpBuffer, SIZE_T nSize,
    SIZE_T* lpNumberOfBytesRead);
tReadProcessMemory pReadProcessMemory = nullptr;

BOOL WINAPI hookedReadProcessMemory(HANDLE hProcess, LPCVOID lpBaseAddress,
    LPVOID lpBuffer, SIZE_T nSize,
    SIZE_T* lpNumberOfBytesRead)
{

    BOOL result = pReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);

    if (result && lpBuffer && nSize > 0 && lpNumberOfBytesRead && *lpNumberOfBytesRead > 0) {
        std::string filename = GenerateUniqueFilename("readprocessmemory_buffer", ".bin");
        if (!filename.empty() && SafeWriteToFile(filename, lpBuffer, *lpNumberOfBytesRead)) {
            std::cout << _XOR_(" [ReadProcessMemory-Dumped] ") << filename << std::endl;
        }
    }

    return result;
}

typedef BOOL(WINAPI* tDeleteFileW)(LPCWSTR lpFileName);
tDeleteFileW pDeleteFileW = nullptr;

BOOL WINAPI hookedDeleteFileW(LPCWSTR lpFileName)
{
    if (lpFileName) {
        std::wstring wideDumpPath = string_to_wstring(globals::dumpPath);
        std::wstring filename = GenerateUniqueFilenameW(L"DeleteFileW", L".bin", wideDumpPath);

        if (!filename.empty()) {
            if (CopyFileW(lpFileName, filename.c_str(), FALSE)) {
                std::wcout << _XOR_(L"[DeleteFileW] ") << lpFileName
                    << _XOR_(L" -> ") << filename << std::endl;
            }
        }
    }

    return pDeleteFileW(lpFileName);
}

typedef BOOL(WINAPI* tDeleteFileA)(LPCSTR lpFileName);
tDeleteFileA pDeleteFileA = nullptr;

BOOL WINAPI hookedDeleteFileA(LPCSTR lpFileName)
{
    if (lpFileName) {
        std::string filename = GenerateUniqueFilename("DeleteFileA", ".bin");

        if (!filename.empty()) {
            if (CopyFileA(lpFileName, filename.c_str(), FALSE)) {
                std::cout << _XOR_("[DeleteFileA] ") << lpFileName
                    << _XOR_(" -> ") << filename << std::endl;
            }
        }
    }

    return pDeleteFileA(lpFileName);
}

typedef NTSTATUS(NTAPI* tNtRaiseHardError)(NTSTATUS ErrorStatus, ULONG NumberOfParameters,
    ULONG UnicodeStringParameterMask, PULONG_PTR Parameters,
    ULONG ResponseOption, PULONG Response);
tNtRaiseHardError pNtRaiseHardError = nullptr;

NTSTATUS NTAPI hookedNtRaiseHardError(NTSTATUS ErrorStatus, ULONG NumberOfParameters,
    ULONG UnicodeStringParameterMask, PULONG_PTR Parameters,
    ULONG ResponseOption, PULONG Response)
{
    if (Response) {
        *Response = ResponseOption;
    }
    return 0;
}