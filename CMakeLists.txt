cmake_minimum_required(VERSION 3.28)
project(Adso VERSION 1.0 LANGUAGES CXX)

# --- 1. Qt Configuration ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.6.1/msvc2019_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)

# --- 2. Icon Configuration ---
set(WIN32_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/logo.ico")
set(MACOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/logo.icns")

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE "logo.icns")
    set_source_files_properties(${MACOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set(APP_ICON_RESOURCE ${MACOS_ICON})
elseif(WIN32)
    set(APP_ICON_RESOURCE ${WIN32_ICON})
endif()

# --- 3. Dependencies (FetchContent) ---
include(FetchContent)
set(CMAKE_SKIP_INSTALL_RULES TRUE)

FetchContent_Declare(zlib GIT_REPOSITORY https://github.com/madler/zlib.git GIT_TAG v1.3.1 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(zlib)

set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}" CACHE PATH "" FORCE)
set(ZLIB_LIBRARY zlibstatic CACHE STRING "" FORCE)

set(NIFTI_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(nifti GIT_REPOSITORY https://github.com/NIFTI-Imaging/nifti_clib.git GIT_TAG master EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(nifti)

FetchContent_Declare(eigen GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git GIT_TAG 3.4.0 SOURCE_SUBDIR "dummy_dir_to_bypass_cmake")
FetchContent_MakeAvailable(eigen)
add_library(Eigen3::Eigen INTERFACE IMPORTED)
set_target_properties(Eigen3::Eigen PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${eigen_SOURCE_DIR}")

# --- 4. Build the Executable ---
add_executable(${PROJECT_NAME} 
    MACOSX_BUNDLE 
    WIN32
    main.cpp
    MainWindow.cpp
    MainWindow.h
    niftiVolume.cpp
    niftiVolume.h
    ${APP_ICON_RESOURCE}
)

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.youwan.adso"
        MACOSX_BUNDLE_BUNDLE_NAME "Adso"
    )
endif()

# --- 5. Link Everything Together ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
    Eigen3::Eigen niftiio znz zlibstatic
)

# --- 6. Deployment ---
get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe" --no-translations --compiler-runtime "$<TARGET_FILE:${PROJECT_NAME}>"
        VERBATIM
    )
elseif(APPLE)
    # The flag "-Wl,-no_warn_search_path" was removed because it caused build failures.
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT_BIN_DIR}/macdeployqt" "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>" 
                -verbose=1 
                -codesign=-
        COMMENT "Deploying Qt dependencies and plugins into the App Bundle..."
    )
endif()