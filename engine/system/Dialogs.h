#pragma once


#include <string>
#include <iostream>
#include <sstream>


namespace System
{
    enum class MessageBoxStyle {
        OK,
        OKCancel
    };

    bool ShowMessageBox(const char* title, const char* text, MessageBoxStyle style = MessageBoxStyle::OK);

    std::string BrowseFolder(std::string saved_path);
    std::wstring BrowseFile(std::wstring initial_dir, bool path_must_exist);

    void EnqueueMessage(const char* title, const char* text, MessageBoxStyle style = MessageBoxStyle::OK);
    bool DequeueAndShowMessage();
}