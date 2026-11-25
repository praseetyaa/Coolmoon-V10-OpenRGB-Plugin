cmake_minimum_required(VERSION 3.16)
project(OpenRGBCoolmoonPlugin LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

include(FetchContent)

# 1. Unduh & build hidapi secara statis untuk Windows HID
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    hidapi
    GIT_REPOSITORY https://github.com/libusb/hidapi.git
    GIT_TAG        hidapi-0.14.0
)
set(HIDAPI_BUILD_HIDTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(hidapi)

# 2. Deteksi Qt6 atau Qt5 (sesuaikan dengan versi OpenRGB yang terpasang)
find_package(Qt6 COMPONENTS Core Widgets QUIET)
if(Qt6_FOUND)
    set(QT_LIBS Qt6::Core Qt6::Widgets)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
    set(QT_LIBS Qt5::Core Qt5::Widgets)
endif()

set(OPENRGB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/OpenRGB")

# 3. Target Plugin DLL
add_library(OpenRGBCoolmoonPlugin SHARED
    OpenRGBCoolmoonPlugin.h
    OpenRGBCoolmoonPlugin.cpp
    RGBController_CoolmoonUSB.h
    RGBController_CoolmoonUSB.cpp
    CoolmoonUSBDriver.h
    CoolmoonUSBDriver.cpp
)

target_include_directories(OpenRGBCoolmoonPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENRGB_DIR}
    ${OPENRGB_DIR}/RGBController
    ${OPENRGB_DIR}/dependencies/json
)

target_link_libraries(OpenRGBCoolmoonPlugin PRIVATE
    ${QT_LIBS}
    hidapi::hidapi
    setupapi
)
