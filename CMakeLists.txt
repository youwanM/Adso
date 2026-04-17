cmake_minimum_required(VERSION 3.28)
project(Adso VERSION 1.0 LANGUAGES CXX)

# --- 1. Qt Configuration ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Tell CMake exactly where to find Qt. 
# IMPORTANT: Change this to your exact Qt path!
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.6.1/msvc2019_64")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# --- 2. Download Dependencies (FetchContent) ---
include(FetchContent)

# FIX: Disable Installation Rules for all third-party libraries globally.
# This stops NIfTI from panicking about the local ZLIB injection.
set(CMAKE_SKIP_INSTALL_RULES TRUE)

# 1. Fetch ZLIB
FetchContent_Declare(zlib GIT_REPOSITORY https://github.com/madler/zlib.git GIT_TAG v1.3.1 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(zlib)

# 2. Bridge our fetched ZLIB to NIfTI
set(ZLIB_FOUND TRUE CACHE BOOL "Force ZLIB found" FORCE)
set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}" CACHE PATH "ZLIB Include Dir" FORCE)
set(ZLIB_LIBRARY zlibstatic CACHE STRING "ZLIB Static Library Target" FORCE)

# 3. Fetch NIfTI
set(NIFTI_BUILD_SHARED_LIBS OFF CACHE BOOL "Build static" FORCE)
FetchContent_Declare(nifti GIT_REPOSITORY https://github.com/NIFTI-Imaging/nifti_clib.git GIT_TAG master EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(nifti)

# 4. Fetch Eigen3 (Using SOURCE_SUBDIR bypasses Eigen's CMakeLists to avoid target collisions)
FetchContent_Declare(eigen 
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git 
    GIT_TAG 3.4.0
    SOURCE_SUBDIR "dummy_dir_to_bypass_cmake"
)
FetchContent_MakeAvailable(eigen)
add_library(Eigen3::Eigen INTERFACE IMPORTED)
set_target_properties(Eigen3::Eigen PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${eigen_SOURCE_DIR}")

# --- 3. Build the Executable ---
add_executable(${PROJECT_NAME} WIN32
    main.cpp
    MainWindow.cpp
    MainWindow.h
    niftiVolume.cpp
    niftiVolume.h
)

# --- 4. Link Everything Together ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
    Eigen3::Eigen
    niftiio
    znz
    zlibstatic
)

# --- 5. Windows Deployment (Fixing Missing DLLs) ---
# Automatically run windeployqt after a successful build to copy required DLLs
if(WIN32)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe"
                --no-translations
                --no-system-d3d-compiler
                --compiler-runtime
                "$<TARGET_FILE:${PROJECT_NAME}>"
        VERBATIM
    )
endif()