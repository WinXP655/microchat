# MicroChat

<img width="506" height="313" alt="image" src="https://github.com/user-attachments/assets/2cb991f4-f0e3-4d4f-8301-1814831c30e8" />

**MicroChat** is an open-source and ultra-light chat framework, written on pure C with raw WinAPI. 
No heavy libraries and extra dependencies.

## Features
- **No dependencies:** Works everywhere (Windows 2000 - Windows 11).
- **Lightweight:** Single-file executable, low memory footprint.
- **Pure C implementation:** No external libraries beyond standard Win32 API
- **Open source:** Fully transparent, easy to audit or extend.
- **Cross-version compatible:** Designed to run on legacy and modern systems.

> **Security Notice:**
> MicroChat transmits all data in plain text by default.  
> If you plan to fork or extend it, it is strongly recommended to add your own encryption layer (XOR/AES/custom protocol).

> Compatibility with Windows NT 4.0 has been dropped since it have problems with SSE/SSE2 binaries.

## Requirements for Building
- **Compiler:** 
  - **GCC/MinGW-w64** (via MSYS2 or Cygwin) - recommended for MicroChat builds.
- **Resource Editing Tool:** 
  - **Resource Hacker** (optional) - required only if you want to customize connection dialog or icons (not used in build process anymore).
- **Basic Skills:**
  - Knowledge of **CMD/CLI** navigation.
  - Intermediate understanding of **C programming, WinAPI and WinSock**.
- **Build Environment:**
  - Any **Windows OS** starting from Windows 7 (Win 7, 10, 11 recommended for build tools).
- **Testing Environment:**
  - Virtual Machines preferred: 2000, XP, 7 through 11.

## Compiling (using gcc from MinGW-w64 as example)

### 1. Compiling code
- Download or clone this repository.
- Run `build.bat`.
  Make sure you have GCC in PATH.

### 2. Customization (optional)
Using Resource Hacker you also can add:
- Your icon (Action -> Add an Image or Other Binary Resource)
- Your version info (Action -> Add using Script Template)
- And anything else.

## 3. Extend MicroChat
Build anything from this raw engine:
1. **Copy** `microchat.c` into your project.
2. **Adapt**: Change the port, swap the UI (Qt/WPF/Console), or strip it for a library.
3. **Customize**: Implement your own protocol, encryption, or binary data handling.
4. **Build & Deploy**: Compile with GCC/MSVC and run anywhere.

## License
This application is distributed under MIT license. Explore, learn, enhance.

---

*Created by WinXP655, 2025-2026*
