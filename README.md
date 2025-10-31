# Carpa Diem

Carpa Diem is a simple and relaxing 3D fishing game written in C++ using OpenGL.

## Requirements

- C++ compiler (GCC/Clang or MSVC)
- CMake (recommended) or Make
- OpenGL 3.3+ drivers
- GLFW3 (development headers)
- Common Linux build tools: build-essential, make

### Package Installation

Different distributions have different package names. Here's how to install the dependencies:

#### Debian/Ubuntu:
```bash
sudo apt-get install cmake build-essential make libx11-dev libxrandr-dev \
                     libxinerama-dev libxcursor-dev libxcb1-dev libxext-dev \
                     libxrender-dev libxfixes-dev libxau-dev libxdmcp-dev
```

Additional packages for Linux Mint:
```bash
sudo apt-get install libmesa-dev libxxf86vm-dev
```

#### RedHat/Fedora:
```bash
sudo dnf install cmake g++ glfw-devel libXxf86vm-devel
```

#### Windows (MSYS2/MinGW):
```bash
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain
```

#### macOS:
Install dependencies using Homebrew:
```bash
brew install glfw
```

## Build & Run

There are several supported workflows. Choose the one that fits your environment.

### 1) Using the Makefile (Linux/macOS)

From the project root:

```bash
make
make run
```

On macOS the supplied `Makefile.macOS` can be used:

```bash
make -f Makefile.macOS
make -f Makefile.macOS run
```

### 2) Using CMake (recommended)

```bash
mkdir build
cd build
cmake ..
make
make run
```

After building the binary is typically placed under `bin/Linux` (or the appropriate platform subfolder). You can also run it directly from the `build` directory if the CMake targets are configured to copy/run the executable.

### 3) Using VSCode

Install the extensions: `ms-vscode.cpptools` and `ms-vscode.cmake-tools`.
Open the project folder in VSCode and use the CMake "Play" button in the status bar to configure, build and run. On first use CMake Tools will ask you to choose a compiler.

If you want VSCode on Windows to detect a MinGW/GCC installation, add its `bin` folder to the CMake Tools setting `additionalCompilerSearchDirs`.

### 4) Windows with VSCode (MinGW)

1. Install a GCC toolchain (MinGW-w64 or MSYS2). See https://code.visualstudio.com/docs/cpp/config-mingw
2. Install CMake.
3. Install the `ms-vscode.cpptools` and `ms-vscode.cmake-tools` extensions.
4. Configure CMake Tools to include the GCC `bin` folder in `additionalCompilerSearchDirs` if needed (e.g. `C:\Program Files\CodeBlocks\MinGW\bin`).
5. Use the CMake Tools Play button to configure/build/run.

## Troubleshooting

- If the program fails to run, try updating your GPU drivers.
- On Windows, do not extract the project into a path that contains spaces. Some toolchains and build scripts fail when there are spaces in directory names (e.g. "dont do this").
- If libraries are missing on Linux, make sure you installed the development packages listed above.

## Project Structure

```
├── src/                    # Source code files
├── include/                # Header files
├── data/                   # Game assets and resources
├── lib/                    # Platform-specific libraries
├── CMakeLists.txt          # CMake build configuration
├── Makefile                # Linux build rules
└── Makefile.macOS          # macOS build rules
```
