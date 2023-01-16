/*
* Author:	Angelo Frasca Caccia (lem0nSec_)
* Date:		16/01/2023
* Title:	CreateRemoteThreadPlus.exe
* Website:	https://github.com/lem0nSec/CreateRemoteThreadPlus
*/


#include "CreateRemoteThreadPlus.h"

#pragma optimize ("", off)
DWORD WINAPI U32MessageBoxGenerateFunctionInstructions(PUSER32_LIB_DATA lpParameter)
{
	lpParameter->output.outputStatus = ((PMESSAGEBOXA)0x4141414141414141)(lpParameter->input.hwnd, lpParameter->input.text, lpParameter->input.title, lpParameter->input.uType);
	return STATUS_SUCCESS;
}
DWORD U32MessageBoxGenerateFunctionInstructions_End()
{
	return 0;
}
#pragma optimize ("", on)

PUSER32_LIB_INPUT_DATA U32MessageBoxCreateInputParameters(HANDLE hProcess, HWND inputHwnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	LPVOID lpTextAllocation, lpCaptionAllocation;
	PUSER32_LIB_INPUT_DATA iData;
	
	if (lpTextAllocation = VirtualAllocEx(hProcess, NULL, strlen(lpText), MEM_COMMIT, PAGE_READWRITE))
	{
		if (!WriteProcessMemory(hProcess, lpTextAllocation, lpText, strlen(lpText), NULL))
		{
			VirtualFreeEx(hProcess, lpTextAllocation, strlen(lpText), MEM_DECOMMIT);
			return 0;
		}
		if (lpCaptionAllocation = VirtualAllocEx(hProcess, NULL, strlen(lpCaption), MEM_COMMIT, PAGE_READWRITE))
		{
			if (!WriteProcessMemory(hProcess, lpCaptionAllocation, lpCaption, strlen(lpCaption), NULL))
			{
				VirtualFreeEx(hProcess, lpCaptionAllocation, strlen(lpCaption), MEM_DECOMMIT);
				VirtualFreeEx(hProcess, lpTextAllocation, strlen(lpText), MEM_DECOMMIT);
				return 0;
			}
		}

		iData = (PUSER32_LIB_INPUT_DATA)LocalAlloc(LPTR, FIELD_OFFSET(USER32_LIB_INPUT_DATA, uType) + sizeof(UINT));
		iData->hwnd = inputHwnd;
		iData->uType = uType;
		iData->text = (LPCSTR)lpTextAllocation;
		iData->title = (LPCSTR)lpCaptionAllocation;

	}

	return iData;

}

LPVOID U32MessageBoxCreateRemoteFunction(HANDLE hProcess, LPCVOID buffer, DWORD bufferSize)
{
	PVOID address = GetProcAddress(LoadLibraryW(L"user32.dll"), "MessageBoxA");
	LPVOID functionAllocation = 0, tmpBuffer;

	if (address == 0)
	{
		return 0;
	}

	if (tmpBuffer = (LPVOID)LocalAlloc(LPTR, (SIZE_T)bufferSize))
	{
		RtlCopyMemory(tmpBuffer, buffer, bufferSize);

		for (DWORD i = 0; i < bufferSize - sizeof(PVOID); i++)
		{
			if ((*(PVOID*)((PBYTE)tmpBuffer + i)) == (PVOID)0x4141414141414141)
			{
				(*(PVOID*)((PBYTE)tmpBuffer + i)) = address;
				i += sizeof(PVOID) - 1;
			}
		}
		

		if (functionAllocation = VirtualAllocEx(hProcess, NULL, bufferSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE))
		{
			if (!WriteProcessMemory(hProcess, functionAllocation, tmpBuffer, bufferSize, NULL))
			{
				VirtualFreeEx(hProcess, functionAllocation, bufferSize, MEM_DECOMMIT);
			}
		}

		LocalFree(tmpBuffer);

	}

	return functionAllocation;

}



BOOL U32MessageBoxCreateRemoteThreadPlus(HANDLE hProcess, LPVOID funcAddress, DWORD funcSize, DWORD lpTextSize, DWORD lpCaptionSize, PUSER32_LIB_INPUT_DATA input)
{
	BOOL status = FALSE;
	HANDLE hThread;
	PUSER32_LIB_DATA data;
	DWORD size = FIELD_OFFSET(USER32_LIB_DATA, input.uType) + sizeof(UINT);
	LPVOID remoteAllocation;


	if (data = (PUSER32_LIB_DATA)LocalAlloc(LPTR, size))
	{
		RtlCopyMemory(&data->input, input, FIELD_OFFSET(USER32_LIB_INPUT_DATA, uType) + sizeof(UINT));
		
		if (remoteAllocation = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT, PAGE_READWRITE))
		{
			if (WriteProcessMemory(hProcess, remoteAllocation, (LPCVOID)data, size, NULL))
			{
				hThread = CreateRemoteThread(hProcess, NULL, 0, (PTHREAD_START_ROUTINE)funcAddress, remoteAllocation, 0, NULL);

				if (hThread)
				{
					WaitForSingleObject(hThread, INFINITE);
					CloseHandle(hThread);
					VirtualFreeEx(hProcess, remoteAllocation, size, MEM_DECOMMIT);
					VirtualFreeEx(hProcess, funcAddress, funcSize, MEM_DECOMMIT);
					VirtualFreeEx(hProcess, (LPVOID)input->text, lpTextSize, MEM_DECOMMIT);
					VirtualFreeEx(hProcess, (LPVOID)input->title, lpCaptionSize, MEM_DECOMMIT);
					LocalFree(input);
					status = TRUE;
				}
			}
		}

		LocalFree(data);
	
	}
	
	return status;
}


int wmain(int argc, wchar_t* argv[])
{
	DWORD processID = 0;
	LPCVOID buffer = U32MessageBoxGenerateFunctionInstructions;
	DWORD bufferSize = ((PBYTE)U32MessageBoxGenerateFunctionInstructions_End - (PBYTE)U32MessageBoxGenerateFunctionInstructions);
	LPVOID funcAddress, paramAddress;
	PUSER32_LIB_INPUT_DATA iData;
	USER32_LIB_OUTPUT_DATA oData;
	PUSER32_LIB_DATA data;
	HANDLE hProcess;
	char text[] = "CreateRemoteThreadPlus", title[] = "Success!";


	if (argv[1])
	{
		processID = _wtoi(argv[1]);
		if (processID != 0)
		{
			if (hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID))
			{
				funcAddress = U32MessageBoxCreateRemoteFunction(hProcess, buffer, bufferSize);
				if (funcAddress != 0)
				{
					iData = U32MessageBoxCreateInputParameters(hProcess, NULL, text, title, MB_OK);
					if (U32MessageBoxCreateRemoteThreadPlus(hProcess, funcAddress, bufferSize, strlen(text), strlen(title), iData) == TRUE)
					{
						CloseHandle(hProcess);
						return 1;
					}
				}
			}
		}
		else
		{
			goto help;
		}
	}
	else
	{
		goto help;
	}

help:
	wprintf(L"Usage: %s <pid>\n", argv[0]);

	return 0;

}