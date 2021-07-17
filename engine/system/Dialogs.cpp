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

    std::wstring BrowseFile(std::wstring initial_dir, bool path_must_exist)
    {
#if defined(_WIN32)
        OPENFILENAMEW ofn;       // common dialog box structure
        wchar_t szFile[260];       // buffer for file name
        HWND hwnd = glfwGetWin32Window(Engine::Get()->GetWindow());

        // Initialize OPENFILENAME
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = szFile;
        // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
        // use the contents of szFile to initialize itself.
        ofn.lpstrFile[0] = '\0';
        ofn.lpstrFileTitle = L"Test title";
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L"All\0*.*\0Map json\0*.json\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = initial_dir.c_str();
        if (path_must_exist)
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        // Display the Open dialog box. 

        if (GetOpenFileNameW(&ofn) == TRUE)
            return ofn.lpstrFile;
#endif
        return L"";
    }
}
