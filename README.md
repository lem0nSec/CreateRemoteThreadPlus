# CreateRemoteThreadPlus
**CreateRemoteThread: how to pass multiple lpParameters to the remote thread function.**

-----------------------------------------------------------------------------------------------------------------------------------------------------------------

## The Function
As stated by the related MSDN page, the CreateRemoteThread API from kernel32.dll *creates a thread that runs in the virtual address space of another process.* This API is often used for process or shellcode injection purposes. Standard dll injection is perhaps the most common amongst these techniques. CreateRemoteThread can 'force' the remote process to load an arbitrary .dll by opening a new thread in it. The LoadLibrary address is passed to the API as LPTHREAD_START_ROUTINE (4th parameter), while the a pointer to the string (.dll to be loaded) written in the remote process is passed as LPVOID (5th parameter).

```c++
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
Standard .dll injection works right off the bath because the LoadLibrary API expects one parameter only. As previously stated, the 'LPVOID lpParameter' allows for passing this single parameter to the remote thread function. But what if 'LPTHREAD_START_ROUTINE lpStartAddress' needs to be a pointer to a function which expects multiple parameters?

-----------------------------------------------------------------------------------------------------------------------------------------------------------------

## The Solution
This repository shows how to handle situations where CreateRemoteThread is used execute a thread function which expects multiple parameters. 
