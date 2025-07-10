/*
* Author:	Angelo Frasca Caccia (lem0nSec_)
* Date:		16/01/2023
* Title:	CreateRemoteThreadPlus.exe
* Website:	https://github.com/lem0nSec/CreateRemoteThreadPlus
*/


#pragma once

#include <windows.h>
#include <stdio.h>

#define	Add2Ptr(P, I)   ((PVOID)((PUCHAR)(P) + (I)))

typedef int(WINAPI* PMESSAGEBOXA) (__in HWND hwnd, __in LPCSTR lpText, __in LPCSTR lpCaption, __in UINT uType);

typedef struct _USER32_LIB_INPUT_DATA {
	LPCSTR	lpText;
	LPCSTR	lpCaption;
	HWND	hWnd;
	UINT	uType;
} USER32_LIB_INPUT_DATA, * PUSER32_LIB_INPUT_DATA;



typedef struct _USER32_LIB_OUTPUT_DATA {
	int ReturnValue;
} USER32_LIB_OUTPUT_DATA, * PUSER32_LIB_OUTPUT_DATA;



typedef struct _USER32_LIB_DATA {
	USER32_LIB_INPUT_DATA	input;
	USER32_LIB_OUTPUT_DATA	output;
} USER32_LIB_DATA, * PUSER32_LIB_DATA;
