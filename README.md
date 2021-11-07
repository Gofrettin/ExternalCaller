# ExternalCaller
#### Brief:
Forcibly calls functions in remote processes.
<hr />

#### Language:
C (c99)
<hr />

#### Architecture:
i686
<hr />

#### Platform:
Windows NT: Windows 7, Windows 8, Windows 8.1, Windows 10
<hr />

#### Usage:
`ExternalCaller.exe PROCESS_ID FUNCTION_ADDRESS ARGUMENTS_NUMBER [ARGUMENTS]`
<hr />

#### Parameters:
* **PROCESS_ID** - An identifier of a local process, a function of which should be called from outside.
* **FUNCTION_ADDRESS** - A hexdecimal address of the function to be called.
* **ARGUMENTS_NUMBER** - A number of arguments that the function takes.
* **ARGUMENTS** - Function arguments (space-separated), if any.
