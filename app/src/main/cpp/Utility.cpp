#include <sys/time.h>

#include <iostream>
#include <string>
#include <vector>

#include "Utility.h"


//#if __OPENCL_VERSION__ <= CL_VERSION_1_1
//#pragma OPENCL EXTENSION cl_khr_fp64 : enable
//#endif

#include "CL/cl.hpp"

uint64_t GetMilliseconds() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

std::string gHelloWorldC =  "kernel void helloworld(global const int* in, global int* out) {\n"
                            "  int num = get_global_id(0);"
                            "  out[num] = in[num] * 2.0;"
                            "}";

// CL_DEVICE_TYPE_ALL
// CL_DEVICE_TYPE_CPU
// CL_DEVICE_TYPE_GPU
void GetDevices(const std::vector<cl::Platform>& platforms,
        cl_device_type type,
        std::vector<cl::Device>* devices) {
    for(int i = 0; i < platforms.size(); ++i) {
        std::vector<cl::Device> temp_devices;
        cl_int err = platforms[i].getDevices(type, &temp_devices);
        if (err != CL_SUCCESS) {
            LOGI("Can not get any devices of type: %d, error code: %d", type, err);
        }
        devices->insert(devices->end(), temp_devices.begin(), temp_devices.end());
    }
}

void TryAdd() {
    // get platforms
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.size() == 0) { LOGI("can not get any opencl platforms."); return; }
    // std::string platform_name = platforms[0].getInfo<CL_PLATFORM_NAME>();

    // get devices of GPU
    std::vector<cl::Device> devices;
    GetDevices(platforms, CL_DEVICE_TYPE_GPU, &devices);
    if (devices.size() == 0) { LOGI("can not get any GPU devices."); return; }
    cl::Device& target_device = devices[0];
    std::string use_device_name = target_device.getInfo<CL_DEVICE_NAME>();

    // create context
    cl_int err = CL_SUCCESS;
    cl::Context context({target_device});

    // build device program
    cl::Program::Sources sources;
    sources.push_back({gHelloWorldC.c_str(), gHelloWorldC.length()});
    cl::Program program_ = cl::Program(context, sources);
    if ((err = program_.build({target_device})) != CL_SUCCESS) {
        std::string s = "Error building: " + program_.getBuildInfo<CL_PROGRAM_BUILD_LOG>(target_device);
        LOGI("%s", s.c_str());
        return;
    }

    // prepare buffer
    /**Step 7: Initial input,output for the host and create memory objects for the kernel*/
    const int NUM = 512000;
    int* input = new int[NUM];
    for(int i = 0; i < NUM; ++i)
        input[i] = i;
    int* output = new int[NUM];

    cl::Buffer inputBuffer = cl::Buffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, (NUM) * sizeof(int), (void*)input, &err);
    cl::Buffer outputBuffer = cl::Buffer(context, CL_MEM_WRITE_ONLY, NUM * sizeof(int), NULL, &err);

    // create queue
    cl::CommandQueue queue(context, devices[0], 0, &err);
    // upload input data to target device
    err = queue.enqueueWriteBuffer(inputBuffer, CL_TRUE, 0, sizeof(int) * NUM, input);

    /**Step 9: Sets Kernel arguments.*/
    // bind kernel function
    cl::Kernel kernel(program_, "helloworld", &err);
    err = kernel.setArg(0, inputBuffer);
    err = kernel.setArg(1, outputBuffer);

    /**Step 10: Running the kernel.*/
    // run kernel function
    cl::Event event;
    err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(NUM), cl::NullRange, NULL, &event);
    // event.wait();
    queue.finish();

    /**Step 11: Read the cout put back to host memory.*/
    // read result, read the result put back to host memory
    queue.enqueueReadBuffer(outputBuffer, CL_TRUE, 0, NUM * sizeof(int), output, 0, NULL);
    std::cout << output[NUM-1] << std::endl;

    /**Step 12: Clean the resources.*/
    //cl_int status = clReleaseMemObject(inputBuffer);//Release mem object.
    //status = clReleaseMemObject(outputBuffer);

    delete []input;
    delete []output;
}

std::string GetDevicesName() {
    std::string sz = "devices : ";

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.size() == 0) {
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

    TryAdd();

    return sz;
}