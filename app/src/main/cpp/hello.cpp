#include <string>
#include <vector>

#include "hello.h"
#include "CL/cl.hpp"

std::string Hello() {
    std::string sz = "devices : ";

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.size() == 0) {
        // std::cout << "Platform size 0\n";
        return "no platforms";
    }
    for (int i = 0; i < platforms.size(); ++i) {
        std::vector<cl::Device> devices;
        platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices);
        for (int j = 0; j < devices.size(); ++j) {
            sz += "name ";
            sz += std::string(devices[j].getInfo<CL_DEVICE_NAME>()); // device name
            sz += ", vendor ";
            sz += std::string(devices[j].getInfo<CL_DEVICE_VENDOR>());
            sz += " | ";
        }
    }

    return sz;
    return "Hello";
}