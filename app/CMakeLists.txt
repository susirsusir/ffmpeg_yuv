# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(lib_src_DIR ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI})
include_directories(${CMAKE_SOURCE_DIR}/libs/include)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libavcodec-57.so)

add_library(avdevice SHARED IMPORTED)
set_target_properties(avdevice PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libavdevice-57.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libavformat-57.so)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libavutil-55.so)

add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libavfilter-6.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libswresample-2.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${lib_src_DIR}/libswscale-4.so)


add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/native-lib.cpp
             src/main/cpp/ShaderUtils.cpp)


find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )



target_link_libraries( # Specifies the target library.
                       native-lib
                        GLESv2
                        EGL
                        android

                        avcodec
                        avdevice
                        avformat
                        avutil
                        avfilter
                        swresample
                        swscale
                       ${log-lib} )