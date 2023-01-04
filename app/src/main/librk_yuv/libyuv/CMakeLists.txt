# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.1)

# Declares and names the project.
project("lib_rk_yuv")

SET ( ly_base_dir	${PROJECT_SOURCE_DIR} )
SET ( ly_inc_dir	${ly_base_dir}/include )
SET ( ly_lib_name	rk_yuv )
SET ( ly_lib_static	${ly_lib_name} )

INCLUDE_DIRECTORIES( BEFORE ${ly_inc_dir} )


# 打印日志信息
#       ${ANDROID_ABI} 的作用是获取当前的 CPU 指令集架构
#           当本次编译 armeabi-v7a CPU 架构时 , ${ANDROID_ABI} 值为 armeabi-v7a
#           当本次编译 x86 CPU 架构时 , ${ANDROID_ABI} 值为 x86
message("CMAKE_SOURCE_DIR : ${CMAKE_SOURCE_DIR}, ANDROID_ABI : ${ANDROID_ABI}")

add_definitions("-DLIBYUV_NEON")
#set_property(SOURCE ${SRC_FILE}  APPEND_STRING PROPERTY COMPILE_FLAGS " -mfpu=neon")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
# 设置函数库编译
add_library( # Sets the name of the library.
        ${ly_lib_static}
        # Sets the library as a shared library.
        STATIC
        # Provides a relative path to your source file(s).
        source/compare.cc
        source/compare_common.cc
        source/convert.cc
        source/convert_argb.cc
        source/convert_from.cc
        source/convert_from_argb.cc
        source/convert_to_argb.cc
        source/convert_to_i420.cc
        source/cpu_id.cc
        source/planar_functions.cc
        source/rotate.cc
        source/rotate_any.cc
        source/rotate_argb.cc
        source/rotate_common.cc
        source/row_any.cc
        source/row_common.cc
        source/scale.cc
        source/scale_any.cc
        source/scale_argb.cc
        source/scale_common.cc
        source/video_common.cc
        # neon implementation
        source/compare_neon64.cc
        source/rotate_neon64.cc
        source/row_neon64.cc
        source/scale_neon64.cc)

