/**-----------------------------------------------------------------------------
; @file external_caller.c
;
; @project
;   external_caller
;
; @language
;   C (C99)
;
; @architecture
;   i686
;
; @platform
;   Windows NT: Windows 7, Windows 8, Windows 8.1, Windows 10
;
; @brief
;   Forcibly calls functions in remote processes.
;
; @usage
;   ExternalCaller.exe PROCESS_ID FUNCTION_ADDRESS ARGUMENTS_NUMBER [ARGUMENTS]
;
; @parameters
;   PROCESS_ID          | An identifier of a local process, a function of which
;                       | should be called from outside.
;   FUNCTION_ADDRESS    | A hexdecimal address of the function to be called.
;   ARGUMENTS_NUMBER    | A number of arguments that the function takes.
;   ARGUMENTS           | Function arguments (space-separated), if any.
;
; @date   November 2021
; @author Eph
;
; // TODO: Implement callers for x86 stdcall, thiscall.
;
-----------------------------------------------------------------------------**/



/** @includes  --------------------------------------------------------------**/

#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>



/** @functions  -------------------------------------------------------------**/

/**-----------------------------------------------------------------------------
; @call_external_cdecl_function main
;
; @brief
;   Allocates a buffer with execution rights in the remote process with id =
;   'process_id'. Writes to this buffer a set of i686 instructions that call a
;   function at 'function_address' with arguments 'args' using the 'cdecl' call
;   convention. Begins a thread that executes the code in this buffer. Waits for
'   this thread to finish executing. Returns a value returned by the function at
;   the 'function_address' address (or the value of the eax register for void
;   functions).
;
;   Caller structure:
;       BYTES           INSTRUCTIN      SIZE    COMMENT
;       68 XXXXXXXX     push XXXXXXXX   5       XXXXXXXX = the last arg. value
;       ...
;       68 XXXXXXXX     push XXXXXXXX   5       XXXXXXXX = the 2-nd arg. value
;       68 XXXXXXXX     push XXXXXXXX   5       XXXXXXXX = the 1-st arg. value
;       E8 XXXXXXXX     call XXXXXXXX   5       XXXXXXXX = function address
;       83 C4 XX        add esp, XX     3       Restore stack. XX = 4 * argc
;       C3              ret             1       Return (terminate the thread)
;
; @params
;   process_id          | An identifier of a local process, a function of which
;                       | should be called from outside.
;   function_address    | An address of the function to be called.
;   argc                | A number of arguments that the function takes.
;   args                | Function arguments (if any).
;
; @return
;   A value returned by the called function (or a value of the eax register for
;   functions of type 'void').
;
; // TODO: 'GetLastError' after WinAPI functions.
;
-----------------------------------------------------------------------------**/
DWORD call_external_cdecl_function(DWORD process_id, DWORD function_address,
    DWORD argc, DWORD* args, ...)
{
    DWORD caller_size =
        (1 + 4) * argc +    /* (push-instruction size + argument size) * argc */
        1 + 4 +             /* call-instruction size + address size */
        3 +                 /* restore-stack-instructions size */
        1;                  /* ret-instruction size */

    /* Allocate space for the caller in the remote process's address space. */
    HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
    void* caller_address = VirtualAllocEx(process_handle, NULL, caller_size,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    /* Create a buffer to store the caller bytes. */
    unsigned __int8* caller_bytes = (unsigned __int8*)malloc(caller_size);

    /* Write push-args instructions in the caller. */
    for (int i = 0; i < argc; i++)
    {
        caller_bytes[i * 5] = 0x68;         /* push */
        *(unsigned __int32*)(caller_bytes + i * 5 + 1) = args[argc - i - 1];
    }

    /* Write call-instruction to the caller bytes. */
    caller_bytes[argc * 5] = 0xe8;          /* call */

    /* Calculate and write an address for the near call instruction. */
    *(unsigned __int32*)(caller_bytes + argc * 5 + 1) = function_address -
        ((unsigned __int32)caller_address + argc * 5) - 5;

    /* Write restore-stack-instructions to the caller bytes. */
    caller_bytes[argc * 5 + 5] = 0x83;      /* add */
    caller_bytes[argc * 5 + 6] = 0xc4;      /* esp */
    caller_bytes[argc * 5 + 7] = 4 * argc;  /* args size */

    /* Write ret-instruction to the caller bytes. */
    caller_bytes[argc * 5 + 8] = 0xc3;      /* ret */

    /* Write caller bytes to the remote process's memory. */
    WriteProcessMemory(process_handle, caller_address, caller_bytes,
        caller_size, NULL);

    /* Create a thread in the remote process. */
    HANDLE thread_handle = CreateRemoteThread(process_handle, NULL, 0,
        caller_address, NULL, 0, NULL);

    /* Wait for a return from the function. */
    while (WaitForSingleObject(thread_handle, 0) != 0) {}

    /* Get the returned value. */
    DWORD result = 0;
    GetExitCodeThread(thread_handle, &result);

    CloseHandle(thread_handle);
    free(caller_bytes);
    VirtualFreeEx(process_handle, caller_address, caller_size, MEM_FREE);
    CloseHandle(process_handle);
    return result;
}


/**-----------------------------------------------------------------------------
; @func main
;
; @brief
;   Entry point. Parses command line arguments, passes them to the caller.
;
; @return
;   A value returned by the called function (or a value of the eax register for
;   functions of type 'void').
;
-----------------------------------------------------------------------------**/
int main(int argc, char* argv[])
{
    /* Check arguments number. */
    if (argc < 4)
    {
        printf("Invalid arguments number.\n");
        printf("Usage: ");
        printf("PROCESS_ID FUNCTION_ADDRESS ARGUMENTS_NUMBER [ARGUMENTS]\n");
        printf("\tPROCESS_ID       - An identifier of a local process, a "
            "function of which should be called.\n");
        printf("\tFUNCTION_ADDRESS - A hexdecimal address of the function to "
            "be called.\n");
        printf("\tARGUMENTS_NUMBER - A number of arguments that the function "
            "takes.\n");
        printf("\tARGUMENTS        - Function arguments (space-separated), if "
            "any.\n");
        return -1;
    }

    /* Parse arguments. */
    DWORD process_id = atoi(argv[1]);
    DWORD function_address = strtoul(argv[2], NULL, 16);
    DWORD arguments_number = atoi(argv[3]);
    unsigned __int32* arguments = (unsigned __int32*)malloc(
        sizeof(unsigned __int32) * arguments_number);
    for (int i = 0; i < arguments_number; i++)
        arguments[i] = atoi(argv[4 + i]);

    /* Pass the arguments to the caller, save the returned value. */
    DWORD result = call_external_cdecl_function(process_id, function_address,
        arguments_number, arguments);

    printf("Result: 0x%x\n", result);

    free(arguments);

    return result;
}
