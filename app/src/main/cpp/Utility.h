#pragma once

#include <stdint.h>
#include <string>

#include "CL/cl.hpp"
#include <android/log.h>

#define LOGV(...)__android_log_print(ANDROID_LOG_VERBOSE, " Tag", __VA_ARGS__)   // VERBOSE
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , " Tag ", __VA_ARGS__)  // DEBUG
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO  , " Tag ",__VA_ARGS__)   // INFO
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN  , " Tag ", __VA_ARGS__)  // WARN
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , " Tag ",__VA_ARGS__)  // ERROR

std::string GetDevicesName();

uint64_t GetMilliseconds();
void Log(std::string err);

// CL_DEVICE_TYPE_ALL
// CL_DEVICE_TYPE_CPU
// CL_DEVICE_TYPE_GPU
void GetDevices(const std::vector<cl::Platform>& platforms, cl_device_type type, std::vector<cl::Device>* devices);

// debug
void cc_kYuvI420ToNV12Rotate_HostDebug(const unsigned char* in_buff_yuv,  // 0, m_input_buff_yuv
         unsigned char* out_buff_yuv,                    // 1, m_output_buff_yuv
         const int y_width,                              // 2, y_width
         const int y_height,                             // 3, y_height
         const int y_block_size,                         // 4, y_width * y_height
         const int u_width,                              // 5, u_width
         const int u_height,                             // 6, u_height
         const int u_block_size,                         // 7, u_width * u_height
         int uw_pos,
         int uh_pos);

void cc_kYuvI420ToNV12Rotate_HostDebug_Blocks(const unsigned char* in_buff_yuv,  // 0, m_input_buff_yuv
                                       unsigned char* out_buff_yuv,                    // 1, m_output_buff_yuv
                                       const int y_width,                              // 2, y_width
                                       const int y_height,                             // 3, y_height
                                       const int y_block_size,                         // 4, y_width * y_height
                                       const int u_width,                              // 5, u_width
                                       const int u_height,                             // 6, u_height
                                       const int u_block_size,                         // 7, u_width * u_height
                                       int uw_pos,
                                       int uh_pos);

bool ConvertToNV12RotateImpl_HostDebug(int width, int height, unsigned char* yuv_i420_img_data);