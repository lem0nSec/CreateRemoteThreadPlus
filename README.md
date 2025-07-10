# CreateRemoteThreadPlus
**CreateRemoteThread: how to pass multiple parameters to the remote thread function without shellcode.**

-----------------------------------------------------------------------------------------------------------------------------------------------------------------

## The Function
As stated by the related MSDN page, the CreateRemoteThread API from kernel32.dll *creates a thread that runs in the virtual address space of another process.* This API is often used for process or shellcode injection purposes. Standard dll injection is perhaps the most common amongst these techniques. CreateRemoteThread can 'force' the remote process to load an arbitrary .dll by opening a new thread in it. The LoadLibrary address is passed to the API as LPTHREAD_START_ROUTINE (4th parameter), while a pointer to the string (.dll to be loaded) written in the remote process is passed as 5th parameter.

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
Standard .dll injection works because the LoadLibrary API expects one parameter only. But what if the remote function expects multiple parameters? What if the function is MessageBox for instance? (MessageBox expects four parameters). I wanted to create this repository because some people on the Internet have said that passing more than one argument to the remote function is impossible.

![](pictures/argument.png)
**Figure 1. People's argument (1)**


![](pictures/argument1.png)
**Figure 2. People's argument (2)**

-----------------------------------------------------------------------------------------------------------------------------------------------------------------

## The Solution
This code shows how to handle the aforementioned situation by doing the followings:

- Crafting a data structure which contains the following two sub-structures:
	- A struct (`_USER32_LIB_INPUT_DATA`) containing the parameters to pass to the `MessageBoxA` api
 	- A struct (`_USER32_LIB_OUTPUT_DATA`) containing an integer value. This is the returning value of the api call
- Writing a function code block into the remote process. The `CreateRemoteThread` api will receive a pointer to this function as `LPTHREAD_START_ROUTINE`.


### Creating a data structure that holds parameters
MessageBox expects a HWND variable as first parameter, the second and third parameters are pointers to constant strings (messagebox text and caption), the fourth parameter is a UINT variable (content and behaviour of the messagebox). The data structure with MessageBoxA parameters will be the following:

```c
typedef struct _USER32_LIB_INPUT_DATA {
	LPCSTR	lpText;
	LPCSTR	lpCaption;
	HWND	hWnd;
	UINT	uType;
} USER32_LIB_INPUT_DATA, * PUSER32_LIB_INPUT_DATA;
```

The LPCSTR parameters are pointers to constant strings in the remote process. So the strings are first written into the target process with calls to `VirtualAllocEx` and `WriteProcessMemory`. After that, pointers to the strings are written into the struct. This will be eventually copied into the `input` field of the bigger `USER32_LIB_DATA` struct, which is injected into the remote process.

### Creating 'Shellcode' Dynamically
The remote thread first has to execute instructions that populate the right registers with the right values from the previously written struct. The following function takes the injected `USER32_LIB_DATA` struct as input parameter. All it does is just passing the parameters in the `input` field of `USER32_LIB_DATA` to `MessageBoxA` before calling it.

```c
<snip>

DWORD WINAPI U32MessageBoxGenerateFunctionInstructions(PUSER32_LIB_DATA lpParameter)
{
	lpParameter->output.outputStatus = ((PMESSAGEBOXA)0x4141414141414141)(lpParameter->input.hwnd, lpParameter->input.text, lpParameter->input.title, lpParameter->input.uType);
	return 0;
}
DWORD U32MessageBoxGenerateFunctionInstructions_End()
{
	return 0; 
}

<snip>
```

'0x4141414141414141' is just a dummy value that will be replaced with the `MessageBoxA` address before injecting the code block. The result after code injection and remote thread creation is the following:


![MessageBoxA instruction set](pictures/function_instruction_set.png)
**Figure 3. MessageBoxA instruction set**


![Remote thread executes MessageBoxA](pictures/payload_execution.png)
**Figure 4. Remote thread executes MessageBoxA**

-----------------------------------------------------------------------------------------------------------------------------------------------------------------


## References
* https://guidedhacking.com/threads/how-to-pass-multiple-arguments-with-createremotethread-to-injected-dll.15373/
* https://stackoverflow.com/questions/25354393/passing-multiple-parameters-using-createremotethread-in-c-sharp
* https://github.com/gentilkiwi/mimikatz
* https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createremotethread
