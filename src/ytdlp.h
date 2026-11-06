#pragma once

// Helpers for the external yt-dlp/ffmpeg tools. Everything here is plain
// Win32 so the console download harness can link the same download engine.

#include "common.h"

#include <atomic>
#include <functional>

// Folder that holds the per-user copies of yt-dlp.exe and ffmpeg.exe
// (%LOCALAPPDATA%\SpiderDownloadManager). Created on demand.
std::wstring ToolDir();

// Resolves yt-dlp.exe: next to the executable, in a "tools" subfolder, in the
// per-user tool folder, or on PATH. Empty when it is not installed.
std::wstring FindYtDlp();

// Resolves ffmpeg.exe using the same search order.
std::wstring FindFfmpeg();

// Resolves aria2c.exe using the same search order.
std::wstring FindAria2c();

// Downloads the Windows build published on gyan.dev and unpacks ffmpeg.exe
// into ToolDir(). The caller opens (and may abort) the HTTP client.
// progress(done, total) runs while the archive downloads; total is 0 when the
// server does not report a length.
bool InstallFfmpeg(HttpClient& client,
                   const std::function<void(uint64_t, uint64_t)>& progress,
                   const std::atomic<bool>& cancel, std::wstring& error);

// Downloads the single-file yt-dlp.exe release straight into ToolDir().
// yt-dlp ships as one exe (no archive), so there is nothing to unpack: the
// download goes to a temporary name, is checked to be a real PE (MZ header,
// larger than 5 MB), and only then is moved into place, so a failed or
// half-written download can never be picked up by FindYtDlp(). The caller
// opens (and may abort) the HTTP client. progress works like InstallFfmpeg.
bool InstallYtDlp(HttpClient& client,
                  const std::function<void(uint64_t, uint64_t)>& progress,
                  const std::atomic<bool>& cancel, std::wstring& error);

// Downloads the official aria2 Windows x64 zip and unpacks aria2c.exe into
// ToolDir(). The zip is small (~2.5 MB); the exe inside is checked to be a
// real PE (MZ header, larger than 1 MB) before it is moved into place, so a
// failed or half-written download can never be picked up by FindAria2c().
// The caller opens (and may abort) the HTTP client. progress works like
// InstallFfmpeg. Returns false (with error set) on any failure; callers
// that only want aria2c for speed must fall back to yt-dlp's built-in
// downloader instead of failing the download.
bool InstallAria2c(HttpClient& client,
                   const std::function<void(uint64_t, uint64_t)>& progress,
                   const std::atomic<bool>& cancel, std::wstring& error);

// A child process with stdout/stderr merged into one pipe. Pump() polls the
// pipe in short slices so a cancel request is honoured even when the child is
// silent (merging, network stalls).
class ConsoleProcess {
public:
    ~ConsoleProcess();

    ConsoleProcess(const ConsoleProcess&) = delete;
    ConsoleProcess& operator=(const ConsoleProcess&) = delete;
    ConsoleProcess() = default;

    bool Start(const std::wstring& exe, const std::wstring& arguments);

    // Returns the child exit code, or -1 when it could not be started.
    // timeoutMs < 0 waits forever.
    int Pump(const std::function<void(const std::string&)>& onLine,
             const std::atomic<bool>& cancel, int timeoutMs = -1);

private:
    void Close();

    void* process_ = nullptr;
    void* thread_ = nullptr;
    void* pipe_ = nullptr;
};

// Convenience wrappers used by the tool installer.
int RunToolLines(const std::wstring& exe, const std::wstring& arguments,
                 const std::function<void(const std::string&)>& onLine,
                 const std::atomic<bool>& cancel);
bool RunToolCapture(const std::wstring& exe, const std::wstring& arguments,
                    int timeoutMs, std::string& output, int& exitCode);
