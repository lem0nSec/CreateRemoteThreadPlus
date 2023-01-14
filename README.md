# CreateRemoteThreadPlus
**CreateRemoteThread: how to pass multiple lpParameters to the remote thread function without shellcode.**

-----------------------------------------------------------------------------------------------------------------------------------------------------------------

## The Function
As stated by the related MSDN page, the CreateRemoteThread API from kernel32.dll *creates a thread that runs in the virtual address space of another process.* This API is often used for process or shellcode injection purposes. Standard dll injection is perhaps the most common amongst these techniques. CreateRemoteThread can 'force' the remote process to load an arbitrary .dll by opening a new thread in it. The LoadLibrary address is passed to the API as LPTHREAD_START_ROUTINE (4th parameter), while the a pointer to the string (.dll to be loaded) written in the remote process is passed as LPVOID (5th parameter).

```c
HANDLE CreateRemoteThread(
  [in]  HANDLE                 hProcess,
  [in]  LPSECURITY_ATTRIBUTES  lpThreadAttributes,
  [in]  SIZE_T                 dwStackSize,
  [in]  LPTHREAD_START_ROUTINE lpStartAddress,
  [in]  LPVOID                 lpParameter,
  [in]  DWORD                  dwCreationFlags,
  [out] LPDWORD                lpThreadId
);
```

-----------------------------------------------------------------------------------------------------------------------------------------------------------------

## The Problem
Standard .dll injection works right off the bath because the LoadLibrary API expects one parameter only. As previously stated, the 'LPVOID lpParameter' allows for passing this single parameter to the remote thread function. But what if 'LPTHREAD_START_ROUTINE lpStartAddress' needs to be a pointer to a function which expects multiple parameters? What if the remote thread function is MessageBox for instance? (MessageBox expects four parameters).

-----------------------------------------------------------------------------------------------------------------------------------------------------------------

## The Solution
When the remote thread function needs more than one argument, then a data structure containing these arguments has to be passed as LPVOID lpParameter in CreateRemoteThread. 


### Creating a data structure that holds parameters
MessageBox expects a HWND as first parameter, the second and third parameters are pointers to constant strings (messagebox text and title), the fourth parameter is a UINT (content and behaviour of the messagebox). The data structure will be the following (the order of the values within the struct is irrelevant):


```c
typedef struct _USER32_LIB_INPUT_DATA {

	LPCSTR	text;
	LPCSTR	title;
	HWND	hwnd;
	UINT	uType;

} USER32_LIB_INPUT_DATA, * PUSER32_LIB_INPUT_DATA;
```

HWND and UINT are integers, writing them into the struct is as simple as follows:


```c
<snip>

iData = (PUSER32_LIB_INPUT_DATA)LocalAlloc(LPTR, FIELD_OFFSET(USER32_LIB_INPUT_DATA, uType)); // Allocate the struct
iData->hwnd = inputHwnd;
iData->uType = uType;

<snip>
```


The LPCSTR parameters are pointers to constant strings in the remote process. So the strings are first written into the target process, then the related addresses will be written into the struct like this (lpTextAllocation and lpCaptionAllocation are the returning values of previous calls to VirtualAllocEx):


```c
<snip>

iData->text = (LPCSTR)lpTextAllocation;
iData->title = (LPCSTR)lpCaptionAllocation;

<snip>
```


The result will be a data structure that has to be written into the remote process. The address of this structure will be passed to the LPVOID lpParameter parameter of CreateRemoteThread.


### Creating a __stdcall instruction set before the function call
The remote thread first has to execute instructions that populate the right registers with the right values from the previously written struct. 
