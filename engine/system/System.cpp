#include <stdio.h>
#include <Windows.h>
#include <array>

namespace System
{

	bool LaunchProcess(const char* commandLine)
	{
		// create a pipe to get possible errors from the process
		//
		HANDLE hChildStdOutRead = NULL;
		HANDLE hChildStdOutWrite = NULL;

		SECURITY_ATTRIBUTES saAttr = {};
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		if (!CreatePipe(&hChildStdOutRead, &hChildStdOutWrite, &saAttr, 0))
			return false;

		// launch process
		//
		PROCESS_INFORMATION pi = {};
		STARTUPINFOA        si = {};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdError = hChildStdOutWrite;
		si.hStdOutput = hChildStdOutWrite;
		si.wShowWindow = SW_HIDE;

		std::array<char, 4096> buffer;
		sprintf_s(buffer.data(), buffer.size(), "%s", commandLine);

		if (CreateProcessA(NULL, buffer.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(hChildStdOutWrite);

			ULONG exitCode;
			if (GetExitCodeProcess(pi.hProcess, &exitCode))
			{
				HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
				DWORD  dwRead, dwWritten;
				char   chBuf[4096];

				for (;;)
				{
					BOOL bSuccess = ReadFile(hChildStdOutRead, chBuf, _countof(chBuf), &dwRead, NULL);

					if (!bSuccess || dwRead == 0)
						break;

					bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
					if (!bSuccess)
						break;
				}

				if (exitCode == 0)
				{
					return true;
				}
			}

			CloseHandle(hChildStdOutRead);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		return false;
	}
}
