# ==============================================================================
# CMake Toolchain File for MinGW-w64 (x86_64-w64-mingw32)
# Enables cross-compiling native 64-bit Windows executables from Linux or Windows.
# ==============================================================================

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross compilers
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Target environment root path
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Adjust searching behavior: never search in host directories for programs,
# search only target directories for libraries and headers
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Static linking options for pure portable zero-runtime deployment
set(CMAKE_CXX_FLAGS_INIT "-static-libgcc -static-libstdc++ -Wno-unused-parameter")
set(CMAKE_C_FLAGS_INIT "-static-libgcc -Wno-unused-parameter")
