**MicroChat (Current, 17 October 2026)**:
- Fixed recursive AddMessage call.
- Added setting subsystem version to 5.0 (Windows 2000) in build script.
- Added WaitForSingleObject to CleanupAndExit using method that doesn't hang app.
- Added check for connect and getsockname in GetDefaultIP.
- Replaced ExitProcess with returning false.
- Simplified InitializeNetwork mode choice.
- StartServer and StartClient changed to bool.
- Added setting 5 seconds timeout to server side.
- Added setting client_socket to INVALID_SOCKET after closesocket.
- Added check of MultiByteToWideChar.
- Removed unused server_ip_global.
- Replaced direct AddMessage with wrapper UIAddMessage.

**MicroChat (9 October 2026)**:
- Added loop exit in accept error loop.
- Added a 100ms timeout before trying accept again in StartServer().
- Fixed wcscpy in StartClient() without size check.
- Added truncating large message in AddMessage().
- Added _WIN32_WINNT define to start of header file.
- Added checking getsockname() result in StartClient().
- Added explicit wchar.h header for wide functions.
- Added truncating long message instead preventing showing.
- Added suppressing Tab and Enter as characters.

**MicroChat (21 September 2026)**:
- Separated large parts of some functions into smaller functions.\
  InitializeNetwork -> InitializeNetwork, StartServer, StartClient\
  WndProc -> WndProc, CreateAppFonts, CreateAppControls, CleanupGdi, ResizeMainWindow
- Removed error message for listen, subclassing.
- Added optimization flags: -Os -ffunction-sections -fdata-sections

**MicroChat (5 September 2026)**:
- Fixed some static sizes in swprintf.
- Removed unneeded extra tab char in main.c.
- Enlarged buffer for send message error from 32 chars to 256 chars.

**MicroChat (27 August 2026)**:
- Added debug information.\
  To display it, compile it with /dbg flag.\
  After compile, press Ctrl+D when in message edit.
- Removed /sym flag - /dbg includes /sym functionality + extra menu.
- Renamed variables to single style.
- Added "void" arg to functions which do not accept args.
- Removed unused IsWindow call from AddMessage.

**MicroChat (Current, 26 July 2026)**:
- Added .gitignore.
- Added /clean, /sym, /help to build system.\
(updated only build system, no main code update)

**MicroChat (Current, 18 July 2026):**:
- isRunning is now bool.
- Removed dublicating code.
- Replaced all ANSI functions with Unicode.
- Removed Windows 9x compatibility code.
- Buffer is 8192 characters because of Unicode.
- Separated single monolithic code into separated parts.
- Added display and send message length limit.
- Introduced automatic build system.

**MicroChat (8 July 2026)**:
- Renamed functions to be more readable.
- Removed unused hwnd_global from CleanupAndExit.
- Added more comments.
Last version for Windows 9x.

**MicroChat (15 June 2026)**:
- Added "MicroChat - " to title instead just peer name.
- Replaced explicit A (GetComputerNameA) variations to generic (GetComputerName).


**MicroChat (2 April 2026)**:
- Text trimming extended from just caret and newline to caret, newline, tab, space.

**MicroChat (27 May 2026)**:
- Added comments.

**MicroChat (21 May 2026)**:
- Added error handling when main window failed to create.
- Added error handling when subclass failed.
- Added error handling when message receiver failed to start.

**MicroChat (18 May 2026)**:
- Removed MB_ICONERROR.
- Compacted code a bit.

**MicroChat (14 February 2026)**:
- sprintf replaced with snprintf.
- strcpy replaced with strncpy.
- Added check for recv length.
- Messages longer than 4095 characters no longer displayed.
- Mostly security update.
- First version to be opened to public access.

*Versons below was private releases*

**MicroChat (20 January 2026)**:
- Added sleep to allow ReceiveMessages to exit.
- Added error handling for send.

**MicroChat (15 January 2026)**:
- Initial release: GUI, Error Handling, Raw TCP Pipe
- Wasn't open to public yet.
