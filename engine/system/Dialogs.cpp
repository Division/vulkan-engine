#include "Dialogs.h"
#include <deque>

#if defined(_WIN32)
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include "Engine.h"
#include "glfw/glfw3native.h"
#include <shlobj.h>
#endif


namespace System {

    namespace
    {
        struct MessageEntry
        {
            std::string title;
            std::string text;
            MessageBoxStyle style;
        };

        std::deque<MessageEntry> message_queue;
        std::mutex queue_mutex;
    }

    void EnqueueMessage(const char* title, const char* text, MessageBoxStyle style)
    {
        std::scoped_lock lock(queue_mutex);
        message_queue.push_back({ title, text, style });
    }

    bool DequeueAndShowMessage()
    {
        std::scoped_lock lock(queue_mutex);
        if (message_queue.empty())
            return false;
        
        auto& message = message_queue.front();
        ShowMessageBox(message.title.c_str(), message.text.c_str(), message.style);
        message_queue.pop_front();
        
        return true;
    }

    bool ShowMessageBox(const char* title, const char* text, MessageBoxStyle style)
    {
#if defined(_WIN32)
        auto window = Engine::Get()->GetWindow();

        UINT type = MB_OK;
        if (style == MessageBoxStyle::OKCancel)
            type = MB_OKCANCEL;

        int result = MessageBox(glfwGetWin32Window(window), text, title, type);
        return result == IDOK;
#endif
    }

#if defined(_WIN32)
    static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
    {
        if (uMsg == BFFM_INITIALIZED)
        {
            std::string tmp = (const char*)lpData;
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        }

        return 0;
    }
#endif

    std::string BrowseFolder(std::string saved_path)
    {
#if defined(_WIN32)
        TCHAR path[MAX_PATH];

        const char* path_param = saved_path.c_str();

        BROWSEINFO bi = { 0 };
        bi.lpszTitle = ("Browse for folder...");
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        bi.lpfn = BrowseCallbackProc;
        bi.lParam = (LPARAM)path_param;

        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

        if (pidl != 0)
        {
            //get the name of the folder and put it in path
            SHGetPathFromIDList(pidl, path);

            //free memory used
            IMalloc* imalloc = 0;
            if (SUCCEEDED(SHGetMalloc(&imalloc)))
            {
                imalloc->Free(pidl);
                imalloc->Release();
            }

            return path;
        }

#endif
        return "";
    }
}
