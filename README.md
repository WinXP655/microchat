# MicroChat

<img width="506" height="313" alt="image" src="https://github.com/user-attachments/assets/49f07f29-8cfe-4429-9380-fe1e393e4e51" />

**MicroChat** is an open-source and ultra-light chat framework, written on pure C with raw WinAPI. 
No heavy libraries and extra dependencies.

## Features
- **No dependencies:** works everywhere (Windows NT 4.0/98 SE - Windows 11).
- **Lightweight:** uses only 2 MB of memory.

## Requirements for building
For successful compilation you need:
- **GCC** (WinGW/MSYS2) under Windows.
- **Resource Hacker** (for editing resources).
- Basic knowledge of **CMD/CLI**.
- Knowledge of C language.
- OS: Windows 7 and higher (for building tools).

## Compiling

### 1. Compiling code
Run a command for basic binary (you can change any optimization flags):
```
gcc microchat.c srvconn_base.o -o microchat.exe -m32 -lws2_32 -lwinmm -mwindows
```

### 2. Preparing resources
Open file `srvconn.rc` with any text editor (for example, Notepad) and copy its contents.

### 3. Changing default dialog
- Open **Resource Hacker**.
- Open compiled `microchat.exe`.
- Replace `DIALOG` resource with `srvconn.rc` contents you've copied.
<img width="786" height="433" alt="image" src="https://github.com/user-attachments/assets/0e8ae69b-a9d6-43b8-a9b4-d80351e22a5c" />

### 4. Customization (optional)
Using Resource Hacker you also can add:
- Your icon (Action -> Add an Image or Other Binary Resource)
- Your version info (Action -> Add using Script Template)
- And anything else.

## License
This application is distributed under MIT license. Explore, learn, enhance.

---

*Created by WinXP655, 2025-2026*
