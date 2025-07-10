/*
* Author:	Angelo Frasca Caccia (lem0nSec_)
* Date:		16/01/2023
* Title:	CreateRemoteThreadPlus.exe
* Website:	https://github.com/lem0nSec/CreateRemoteThreadPlus
*/


#include "CreateRemoteThreadPlus.h"


#pragma optimize ("", off)
static DWORD WINAPI MessageBox_Start(_In_ PUSER32_LIB_DATA lpParameter)
{
	lpParameter->output.ReturnValue = ((PMESSAGEBOXA)0x4141414141414141)(
		lpParameter->input.hWnd, 
		lpParameter->input.lpText, 
		lpParameter->input.lpCaption, 
		lpParameter->input.uType);
	
	return 0;
}
static DWORD MessageBox_End()
{
	return 0;
}
#pragma optimize ("", on)


static BOOL InsertInputParameter(
	_In_ HANDLE hProcess, 
	_In_ LPVOID pInputParameter, 
	_In_ SIZE_T szInputParameter, 
	_Inout_ PVOID *pRemoteInput)
{
	BOOL status = FALSE;
	PVOID pRemote = 0;

	if (!hProcess ||
		hProcess == INVALID_HANDLE_VALUE) {
		goto Exit;
	}

	pRemote = VirtualAllocEx(hProcess, NULL, szInputParameter, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pRemote) {
		goto Exit;
	}

	status = WriteProcessMemory(hProcess, pRemote, pInputParameter, szInputParameter, NULL);

	if (status) {
		*pRemoteInput = pRemote;
	}
	else {
		VirtualFreeEx(hProcess, pRemote, 0, MEM_RELEASE);
	}

Exit:

	return status;
}


static LPVOID CreateRemoteFunction(
	_In_ HANDLE hProcess, 
	_In_ PVOID function, 
	_In_ LPCVOID buffer, 
	_In_ SIZE_T bufferSize)
{
	LPVOID functionAllocation = 0;
	LPVOID tmpBuffer = 0;
	DWORD i = 0;

	tmpBuffer = (LPVOID)LocalAlloc(LPTR, bufferSize);
	if (!tmpBuffer) {
		goto Exit;
	}

	RtlCopyMemory(tmpBuffer, buffer, bufferSize);

	for (i = 0; i < bufferSize - sizeof(PVOID); i++) {
		if ((*(PVOID*)((PBYTE)tmpBuffer + i)) == (PVOID)0x4141414141414141)
		{
			(*(PVOID*)((PBYTE)tmpBuffer + i)) = function;
			i += sizeof(PVOID) - 1;
		}
	}

	functionAllocation = VirtualAllocEx(hProcess, NULL, bufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!functionAllocation) {
		goto Exit;
	}

	if (!WriteProcessMemory(hProcess, functionAllocation, tmpBuffer, bufferSize, NULL)) {
		VirtualFreeEx(hProcess, functionAllocation, 0, MEM_RELEASE);
		goto Exit;
	}

Exit:
	LocalFree(tmpBuffer);

	return functionAllocation;
}


int wmain(int argc, wchar_t* argv[])
{
	DWORD processId = 0;
	LPCVOID buffer = MessageBox_Start;
	SIZE_T bufferSize = (SIZE_T)((PBYTE)MessageBox_End - (PBYTE)MessageBox_Start);
	LPVOID pRemoteFunction = 0;
	PVOID function = 0;
	USER32_LIB_INPUT_DATA iData = { 0 };
	USER32_LIB_OUTPUT_DATA oData = { 0 };
	USER32_LIB_DATA data = { 0 };
	PUSER32_LIB_DATA pRemoteData = 0;
	HANDLE hProcess = 0, hRemoteThread = 0;
	HMODULE hUser32 = 0;
	char text[] = "CreateRemoteThreadPlus", caption[] = "Success!";
	int ReturnValue = 0;

	if (!argv[1]) {
		wprintf(L"Usage: %s pid\n", argv[0]);
		return 0;
	}

	processId = _wtoi(argv[1]);
	if (!processId) {
		return 0;
	}

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (hProcess == INVALID_HANDLE_VALUE ||
		!hProcess) {
		goto Exit;
	}

	hUser32 = LoadLibrary(L"user32.dll");
	if (!hUser32) {
		goto Exit;
	}

	function = (PVOID)GetProcAddress(hUser32, "MessageBoxA");
	if (!function) {
		goto Exit;
	}

	pRemoteFunction = CreateRemoteFunction(hProcess, function, buffer, bufferSize);
	if (!pRemoteFunction) {
		goto Exit;
	}

	wprintf(L"[+] Remote function created: 0x%-016p\n", pRemoteFunction);

	if (!InsertInputParameter(hProcess, &text, strlen(text), &iData.lpText) ||
		!InsertInputParameter(hProcess, &caption, strlen(caption), &iData.lpCaption)) {
		goto Exit;
	}

	iData.hWnd = NULL;
	iData.uType = MB_OK;
	RtlCopyMemory(&data.input, &iData, sizeof(iData));

	if (!InsertInputParameter(hProcess, &data, sizeof(data), &pRemoteData)) {
		goto Exit;
	}

	wprintf(L"[+] Remote parameters created: 0x%-016p\n", pRemoteData);

	hRemoteThread = CreateRemoteThread(
		hProcess, 
		NULL, 
		0, 
		pRemoteFunction, 
		pRemoteData, 
		0, 
		NULL);
	if (!hRemoteThread) {
		goto Exit;
	}
	
	wprintf(L"[+] Remote thread created: 0x%lx\n", HandleToUlong(hRemoteThread));

	WaitForSingleObject(hRemoteThread, INFINITE);

	if (ReadProcessMemory(
		hProcess, 
		Add2Ptr(pRemoteData, FIELD_OFFSET(USER32_LIB_DATA, output)), 
		&ReturnValue, 
		sizeof(int), 
		NULL)) {
		wprintf(L"[+] Remote thread returned: %d\n", ReturnValue);
	}

// Cleanup	
Exit:
	if (hRemoteThread) {
		CloseHandle(hRemoteThread);
	}
	if (pRemoteFunction) {
		VirtualFreeEx(hProcess, pRemoteFunction, 0, MEM_RELEASE);
	}
	if (pRemoteData) {
		VirtualFreeEx(hProcess, pRemoteData, 0, MEM_RELEASE);
	}
	if (iData.lpText) {
		VirtualFreeEx(hProcess, (LPVOID)iData.lpText, 0, MEM_RELEASE);
	}
	if (iData.lpCaption) {
		VirtualFreeEx(hProcess, (LPVOID)iData.lpCaption, 0, MEM_RELEASE);
	}
	if (hProcess) {
		CloseHandle(hProcess);
	}

	return 0;
}