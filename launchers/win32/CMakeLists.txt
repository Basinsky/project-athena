cmake_minimum_required(VERSION 3.9.2)

project(HQLauncher)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release"
    CACHE STRING "Configuration types" FORCE)

# Use of MFC
set(CMAKE_MFC_FLAG 1)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

add_executable(HQLauncher
  WIN32
  libs/json/json.h
  libs/json/json-forwards.h
  libs/jsoncpp.cpp
  LauncherApp.cpp
  LauncherApp.h
  Launcher.rc
  LauncherDlg.cpp
  LauncherDlg.h
  LauncherManager.cpp
  LauncherManager.h
  LauncherUtils.cpp
  LauncherUtils.h
  libs/miniz.cpp
  libs/miniz.h
  res/HiFi_Logo_Large.png
  res/HiFi_Logo_Small.png
  res/HiFi_Voxel.png
  res/HiFi_Window.png
  res/interface.ico
  res/Launcher.rc2
  Resource.h
  stdafx.cpp
  stdafx.h
  targetver.h
)

target_compile_definitions(HQLauncher PRIVATE LAUNCHER_HMAC_SECRET=$ENV{LAUNCHER_HMAC_SECRET})

# Preprocessor definitions
target_compile_definitions(HQLauncher PRIVATE
  $<$<CONFIG:Debug>:_UNICODE;_WINDOWS;_DEBUG>
  $<$<CONFIG:Release>:_UNICODE;_WINDOWS;NDEBUG>
)

# Minimal rebuild
if (MSVC)
  target_compile_options(HQLauncher PRIVATE
    "$<$<CONFIG:Debug>:/Gm->"
    "$<$<CONFIG:Release>:/Gm->"
  )
endif ()

# Precompiled header files
if (MSVC)
  target_compile_options(HQLauncher PRIVATE
    "$<$<CONFIG:Debug>:/Yu>"
    "$<$<CONFIG:Release>:/Yu>"
  )
  set_property(SOURCE stdafx.cpp
    APPEND_STRING PROPERTY COMPILE_FLAGS
    "$<$<CONFIG:Debug>:/Yc> \
     $<$<CONFIG:Release>:/Yc>")
endif ()