# MicroChat

<img width="506" height="313" alt="image" src="https://github.com/user-attachments/assets/49f07f29-8cfe-4429-9380-fe1e393e4e51" />

**MicroChat** is an open-source and ultra-light chat framework, written on pure C with raw WinAPI. 
No heavy libraries and extra dependencies.

## Features
- **No dependencies:** works everywhere (Windows NT 4.0/98 SE - Windows 11).
- **Lightweight:** single-file executable, low memory footprint.
- **Pure C implementation:** no external libraries beyond standard Win32 API
- **Open source:** fully transparent, easy to audit or extend.
- **Cross-version compatible:** designed to run on legacy and modern systems.

> **Security Notice:**
> MicroChat transmits all data in plain text by default.  
> If you plan to fork or extend it, it is strongly recommended to add your own encryption layer (XOR/AES/custom protocol).

## Requirements for Building
For successful compilation and customization you need:
- **Compiler:** 
  - **GCC/MinGW-w64** (via MSYS2 or Cygwin) — recommended for lightweight builds.
  - **MSVC** (Visual Studio Build Tools) — alternative for Windows native development.
- **Resource Editing Tool:** 
  - **Resource Hacker** (optional) — required only if you want to customize connection dialog or icons.
- **Basic Skills:**
  - Knowledge of **CMD/CLI** navigation.
  - Basic understanding of **C programming** and **WinAPI**.
- **Build Environment:**
  - Any modern **Windows OS** (Win 10/11 recommended for build tools).
- **Testing Environment:**
  - Virtual Machines preferred: Windows NT 4.0, 98 SE, XP, 7 through 11.

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

## 5. Extend MicroChat
Build anything from this raw engine:
1. **Copy** `microchat.c` into your project.
2. **Adapt**: Change the port, swap the UI (Qt/WPF/Console), or strip it for a library.
3. **Customize**: Implement your own protocol, encryption, or binary data handling.
4. **Build & Deploy**: Compile with GCC/MSVC and run anywhere.

## License
This application is distributed under MIT license. Explore, learn, enhance.

---

*Created by WinXP655, 2025-2026*
