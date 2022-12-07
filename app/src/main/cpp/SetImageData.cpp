#include <iostream>
#include <string>
#include <vector>

#include "Utility.h"
#include "SetImageData.h"


//#if __OPENCL_VERSION__ <= CL_VERSION_1_1
//#pragma OPENCL EXTENSION cl_khr_fp64 : enable
//#endif

#include "CL/cl.hpp"

// https://blog.csdn.net/fangyizhitc/article/details/112369120
// 遵循这个计算公式：get_global_id(dim) = get_local_size(dim) * get_group_id(dim) + get_local_id(dim)；
std::string gUpdateImageDataCode =  "kernel void kSetImageData(global int* in) {"
                                "int height = get_local_size(1) * get_group_id(1) + get_local_id(1);"
                                "int x = get_local_size(0) * get_group_id(0) + get_local_id(0);"
                                "int index = height * get_global_size(0) + x;"
                                "  in[index] = index;"
                            "}";

void SetImageData(int width, int height, int* img_data) {
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
    sources.push_back({gUpdateImageDataCode.c_str(), gUpdateImageDataCode.length()});
    cl::Program program_ = cl::Program(context, sources);
    if ((err = program_.build({target_device})) != CL_SUCCESS) {
        std::string s = "Error building: " + program_.getBuildInfo<CL_PROGRAM_BUILD_LOG>(target_device);
        LOGI("%s", s.c_str());
        return;
    }

    // prepare buffer
    /**Step 7: Initial input,output for the host and create memory objects for the kernel*/
    int* input = img_data;
    const int NUM = width * height;
    //int* input = new int[NUM];
    //for(int i = 0; i < NUM; ++i)
    //    input[i] = i;
    //int* output = new int[NUM];

    cl::Buffer inputBuffer = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, (NUM) * sizeof(int), (void*)input, &err);
    // cl::Buffer outputBuffer = cl::Buffer(context, CL_MEM_WRITE_ONLY, NUM * sizeof(int), NULL, &err);

    // create queue
    cl::CommandQueue queue(context, devices[0], 0, &err);
    // upload input data to target device
    err = queue.enqueueWriteBuffer(inputBuffer, CL_TRUE, 0, sizeof(int) * NUM, input);

    /**Step 9: Sets Kernel arguments.*/
    // bind kernel function
    cl::Kernel kernel(program_, "kSetImageData", &err);
    err = kernel.setArg(0, inputBuffer);
    // err = kernel.setArg(1, outputBuffer);

    /**Step 10: Running the kernel.*/
    // run kernel function
    cl::Event event;
    err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height),
                                     cl::NDRange(width / 10, height / 10), NULL, &event);
    // event.wait();
    queue.finish();

    /**Step 11: Read the cout put back to host memory.*/
    // read result, read the result put back to host memory
    // queue.enqueueReadBuffer(outputBuffer, CL_TRUE, 0, NUM * sizeof(int), output, 0, NULL);
    queue.enqueueReadBuffer(inputBuffer, CL_TRUE, 0, NUM * sizeof(int), input, 0, NULL);
    std::cout << input[NUM - 1] << std::endl;

    /**Step 12: Clean the resources.*/
    //cl_int status = clReleaseMemObject(inputBuffer);//Release mem object.
    //status = clReleaseMemObject(outputBuffer);
}
