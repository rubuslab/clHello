# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.1)

# Declares and names the project.
project("yuvconvert_gpu_i420nv12")

# 打印日志信息
#       ${ANDROID_ABI} 的作用是获取当前的 CPU 指令集架构
#           当本次编译 armeabi-v7a CPU 架构时 , ${ANDROID_ABI} 值为 armeabi-v7a
#           当本次编译 x86 CPU 架构时 , ${ANDROID_ABI} 值为 x86
message("CMAKE_SOURCE_DIR : ${CMAKE_SOURCE_DIR}, ANDROID_ABI : ${ANDROID_ABI}")

add_definitions("-D__ARM_NEON__=1")
set_property(SOURCE ${SRC_FILE}  APPEND_STRING PROPERTY COMPILE_FLAGS " -mfpu=neon")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
# 设置函数库编译
add_library( # Sets the name of the library.
        jni_ocl_yuv2nv12
        # Sets the library as a shared library.
        SHARED
        # Provides a relative path to your source file(s).
        native_ocl_i420nv12_lib.cpp Utility.cpp YuvI420ToNV12Rotate.cpp cl_host_debug_kernel_fun.cpp RGBConvertNv12Rotate.cpp)

# 到预设的目录查找 log 库 , 将找到的路径赋值给 log-lib
#       这个路径是 NDK 的 ndk-bundle\platforms\android-29\arch-arm\usr\lib\liblog.so
#       不同的 Android 版本号 和 CPU 架构 需要到对应的目录中查找 , 此处是 29 版本 32 位 ARM 架构的日志库
#
# 可以不配置 :
#       可以不进行该配置, 直接在后面的 target_link_libraries 中链接 log 也不会出错
#
# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.
find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)
# 打印日志库位置
message("log library path: ${log-lib}")

# 设置变量
# CMAKE_CXX_FLAGS 表示会将 C++ 的参数传给编译器
# CMAKE_C_FLAGS 表示会将 C 参数传给编译器
#
# 参数设置 : 传递 CMAKE_CXX_FLAGS C+= 参数给编译器时 , 在 该参数后面指定库的路径
#   CMAKE_SOURCE_DIR 指的是当前的文件地址
#   -L 参数指定动态库的查找路径

# ================ start local opencl library ================
# ${CMAKE_SOURCE_DIR}/../prebuilt_cdx_ocl_cc_libs/OpenCL/include
SET(LOCAL_OPENCL_HEADER_DIR ${CMAKE_SOURCE_DIR}/../)
set(LOCAL_OPENCL_LIBS_DIR ${CMAKE_SOURCE_DIR}/../prebuilt_cdx_ocl_cc_libs/)
message("local opencl, include dir: ${LOCAL_OPENCL_HEADER_DIR}, libs dir: ${LOCAL_OPENCL_LIBS_DIR}")
# set cxx include directories and compile flags
include_directories(${LOCAL_OPENCL_HEADER_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${LOCAL_OPENCL_LIBS_DIR}")
# ================ end local opencl library ================

# ================ start prebuilt arm compute library ================
SET(ARM_LIB_DIR ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/)
SET(ARM_LIB_HEADER_DIR ${CMAKE_SOURCE_DIR}/../prebuilt_arm/)
message("ARM_LIB_DIR: ${ARM_LIB_DIR}, ARM_LIB_HEADER_DIR include: ${ARM_LIB_HEADER_DIR}include/")
# set cxx include directories and compile flags
include_directories(${ARM_LIB_HEADER_DIR} ${ARM_LIB_HEADER_DIR}include/)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${ARM_LIB_DIR}")
# ================ end prebuilt arm compute library ================

# ================ start libyuv library ================
# Sets lib-build-DIR to the path of the desired output directory.
SET(LIBGITYUV_BUILD_DIR ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/)
# Sets lib_src_DIR to the path of the target CMake project.
set(LIBGITYUV_SRC_DIR ${CMAKE_SOURCE_DIR}/../libgit_yuv/)
file(MAKE_DIRECTORY ${LIBGITYUV_BUILD_DIR})

# Adds the CMakeLists.txt file located in the specified directory
# as a build dependency.
add_subdirectory( # Specifies the directory of the CMakeLists.txt file.
        ${LIBGITYUV_SRC_DIR}
        # Specifies the directory for the build outputs.
        ${LIBGITYUV_BUILD_DIR})

# Adds the output of the additional CMake build as a prebuilt static
# library and names it lib_gmath.
add_library(target_lib_yuv_local STATIC IMPORTED)
set_target_properties(target_lib_yuv_local PROPERTIES IMPORTED_LOCATION
        ${LIBGITYUV_BUILD_DIR}/libgit_yuv.a)
include_directories(${LIBGITYUV_SRC_DIR}include/)
#message("LIBGITYUV_SRC_DIR is : ${LIBGITYUV_SRC_DIR}include1234/")
# ================ end libyuv library ================



# 链接函数库
#       参数 1 : 本构建脚本要生成的动态库目标
#       参数 2 ~ ... : 后面是之前预编译的动态库或静态库 , 或引入的动态库
#
# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.
target_link_libraries( # Specifies the target library.
        jni_ocl_yuv2nv12

        # 表示 编译 native-lib 模块, 要链接 add 模块
        cdx_OpenCL

        # arm prebuilt, arm compute library
        arm_compute_core
        arm_compute
        arm_compute_graph

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}

        # lib yuv_local
        target_lib_yuv_local
        )