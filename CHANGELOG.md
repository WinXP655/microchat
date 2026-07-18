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
