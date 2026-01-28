cmake_minimum_required(VERSION 3.28)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------
# 1. Force Clang & Target MinGW
# -----------------------
set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe")

set(CMAKE_C_COMPILER_TARGET   "x86_64-w64-mingw32")
set(CMAKE_CXX_COMPILER_TARGET "x86_64-w64-mingw32")

# -----------------------
# 2. Initialize Project
# -----------------------
project(MyProject LANGUAGES CXX C)

# -----------------------
# 3. C++20 Modules Setup
# -----------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# -----------------------
# 4. Sources (Standard C/C++ files only)
# -----------------------
file(GLOB SRC_FILES "src/*.cpp" "src/*.c") 

# -----------------------
# 5. Executable (Name it app.exe)
# -----------------------
add_executable(app
    main.cpp
    "src/glad.c"
    ${SRC_FILES}
)

# Force the executable to be output in parent folder of build dir
set_target_properties(app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.."
)

# -----------------------
# 6. REGISTER MODULES
# -----------------------
target_sources(app PUBLIC
    FILE_SET CXX_MODULES FILES 
    utilities.cppm
)

# -----------------------
# 7. Includes & Libraries
# -----------------------
target_include_directories(app PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(app PRIVATE
    "${CMAKE_SOURCE_DIR}/glfw3.dll"
    opengl32
    gdi32
    user32
    kernel32
)

# -----------------------
# 8. Auto-copy DLL
# -----------------------
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/glfw3.dll"
    $<TARGET_FILE_DIR:app>
)
