#pragma once

#include <windows.h>
#include <string>
#include <shlobj.h>
#include <iostream>
#include <sstream>


namespace utils
{
    static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
    {

        if (uMsg == BFFM_INITIALIZED)
        {
            std::string tmp = (const char*)lpData;
            std::cout << "path: " << tmp << std::endl;
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