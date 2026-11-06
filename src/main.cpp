#include "common.h"
#include "localization.h"
#include "ui_qt.h"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QStringList>

#include <cwchar>

namespace {

BOOL CALLBACK FindMainWindowProc(HWND window, LPARAM param) {
    if (!GetPropW(window, SpiderMainWindowProperty())) return TRUE;
    *reinterpret_cast<HWND*>(param) = window;
    return FALSE;  // stop at the first tagged window
}

HWND FindMainWindow() {
    HWND found = nullptr;
    EnumWindows(FindMainWindowProc, reinterpret_cast<LPARAM>(&found));
    return found;
}

// Fallback for a running instance built before the tag existed: post the
// message to every top-level window with the application title. Only the real
// main window handles it, the Qt helper windows simply ignore it.
BOOL CALLBACK PostShowMessageProc(HWND window, LPARAM param) {
    wchar_t title[256] = {};
    if (GetWindowTextW(window, title, 256) > 0 &&
        wcscmp(title, APP_NAME) == 0) {
        PostMessageW(window, static_cast<UINT>(param), 0, 0);
    }
    return TRUE;
}

}  // namespace

int main(int argc, char** argv) {
    // A single instance keeps the registry-backed settings coherent and avoids
    // two windows writing the same temp part files.
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\SpiderDownloadManagerSingleInstance");
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        // Ask the running instance to restore its window instead of showing
        // the HWND from here: only the owning Qt process knows the window is
        // hidden, and ShowWindow() from this process would leave it blank and
        // unclosable.
        HWND existing = nullptr;
        for (int attempt = 0; attempt < 40 && !existing; ++attempt) {
            existing = FindMainWindow();
            if (!existing) Sleep(50);
        }
        if (existing) {
            DWORD owner = 0;
            GetWindowThreadProcessId(existing, &owner);
            AllowSetForegroundWindow(owner);
            PostMessageW(existing, SpiderShowMainWindowMessage(), 0, 0);
        } else {
            EnumWindows(PostShowMessageProc,
                        static_cast<LPARAM>(SpiderShowMainWindowMessage()));
        }
        CloseHandle(mutex);
        return 0;
    }

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QString::fromWCharArray(APP_NAME));
    QCoreApplication::setOrganizationName(QStringLiteral("SpiderDownloadManager"));
    QCoreApplication::setApplicationVersion(
        QString::fromWCharArray(APP_VERSION));

    applyApplicationTheme(app);
    app.setWindowIcon(applicationIcon());
    // Closing the main window hides it to the notification area; the app only
    // quits after an explicit Exit request (tray menu or Ctrl+Q).
    app.setQuitOnLastWindowClosed(false);

    DownloadManager& manager = DownloadManager::Instance();
    manager.LoadSettings();
    // Keep the logon entry in sync with the setting (and repair its path if
    // the exe moved since the entry was written).
    ApplyStartWithWindows(manager.Settings().startWithWindows);
    SetCurrentLanguage(manager.Settings().language == 1
                           ? UiLanguage::Arabic
                           : UiLanguage::English);

    MainWindow window;

    // A URL on the command line is queued immediately; browsers and other
    // launchers use this path.
    const QStringList arguments = QCoreApplication::arguments();
    // --tray starts the app in the notification area only (used by the
    // Start with Windows logon entry). The tray icon is already shown by
    // the MainWindow constructor, so the app stays alive in the tray.
    const bool startInTray = arguments.contains(QStringLiteral("--tray")) ||
                             arguments.contains(QStringLiteral("-tray")) ||
                             arguments.contains(QStringLiteral("/tray"));
    if (!startInTray) window.show();

    for (int i = 1; i < arguments.size(); ++i) {
        const std::wstring candidate = arguments[i].toStdWString();
        if (ParseUrl(candidate).valid) {
            window.addUrlFromText(candidate, false, true);
            break;
        }
    }

    const int result = app.exec();

    manager.Shutdown();
    if (mutex) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
    return result;
}
