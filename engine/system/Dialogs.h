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
    
    void EnqueueMessage(const char* title, const char* text, MessageBoxStyle style = MessageBoxStyle::OK);
    bool DequeueAndShowMessage();
}