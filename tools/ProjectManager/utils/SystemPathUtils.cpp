#include "SystemPathUtils.h"
#include <Windows.h>
#include <winbase.h>
#include <WtsApi32.h>
#include <ShlObj_core.h>

#pragma comment(lib, "shell32.lib")

namespace utils
{
    std::filesystem::path GetDocumentsFolder()
	{
        CHAR my_documents[MAX_PATH];
        HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

        if (result != S_OK)
            return L"";
        
        return my_documents;
	}
}