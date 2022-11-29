#pragma once

#include <stdint.h>
#include <string>

#include "CL/cl.hpp"

std::string GetDevicesName();

uint64_t GetMilliseconds();
void Log(std::string err);

// CL_DEVICE_TYPE_ALL
// CL_DEVICE_TYPE_CPU
// CL_DEVICE_TYPE_GPU
void GetDevices(const std::vector<cl::Platform>& platforms, cl_device_type type, std::vector<cl::Device>* devices);
