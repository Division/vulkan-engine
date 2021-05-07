#pragma once


#include <string>
#include <iostream>
#include <sstream>


namespace utils
{
    enum class MessageBoxStyle {
        OK,
        OKCancel
    };

    bool ShowMessageBox(const char* title, const char* text, MessageBoxStyle style = MessageBoxStyle::OK);

    std::string BrowseFolder(std::string saved_path);
    
}