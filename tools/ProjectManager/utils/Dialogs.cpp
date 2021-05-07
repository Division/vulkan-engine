#include <windows.h>
#include "Dialogs.h"
#include "Engine.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "glfw/glfw3native.h"
#include <shlobj.h>

namespace utils {

    bool ShowMessageBox(const char* title, const char* text, MessageBoxStyle style)
    {
        auto window = Engine::Get()->GetWindow();

        UINT type = MB_OK;
        if (style == MessageBoxStyle::OKCancel)
            type = MB_OKCANCEL;

        int result = MessageBox(glfwGetWin32Window(window), text, title, type);
        return result == IDOK;
    }

    static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
    {

        if (uMsg == BFFM_INITIALIZED)
        {
            std::string tmp = (const char*)lpData;
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        }

        return 0;
    }

    std::string BrowseFolder(std::string saved_path)
    {
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

        return "";
    }
}
