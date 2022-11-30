#pragma once

#include <stdint.h>
#include <string>

#include "CL/cl.hpp"
#include <android/log.h>

#define LOGV(...)__android_log_print(ANDROID_LOG_VERBOSE, " Tag", __VA_ARGS__)   // VERBOSE
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , " Tag ", __VA_ARGS__)    // DEBUG
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO  , " Tag ",__VA_ARGS__)          // INFO
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN  , " Tag ", __VA_ARGS__)    //WARN
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , " Tag ",__VA_ARGS__)      // ERROR

std::string GetDevicesName();

uint64_t GetMilliseconds();
void Log(std::string err);

// CL_DEVICE_TYPE_ALL
// CL_DEVICE_TYPE_CPU
// CL_DEVICE_TYPE_GPU
void GetDevices(const std::vector<cl::Platform>& platforms, cl_device_type type, std::vector<cl::Device>* devices);
