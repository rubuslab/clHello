#include <string>
#include <vector>

#include "CL/cl.hpp"

#include "Utility.h"
#include "YuvConvertOpencc.h"

void YuvI420ToNV12OpenCc::Log(std::string message) {
    // std::cout << message << std::endl;
}

void YuvI420ToNV12OpenCc::Release() {
    m_devices.clear();
    delete m_context; m_context = nullptr;
    delete m_program; m_program = nullptr;
    delete m_queue; m_queue = nullptr;
    delete m_kernel_yuvi420_to_nv12; m_kernel_yuvi420_to_nv12 = nullptr;
}

bool YuvI420ToNV12OpenCc::Init() {
    std::string UpdateImageDataCode =  "kernel void kYuvI420ToNV12(global const unsigned char* in_i420_uv,"
                                       "                         global unsigned char* nv12_uv_buff,"
                                       "                         const int width, const int height,"
                                       "                         const int lines_per_group) {"
                                       "  int gid = get_global_id(0);"
                                       "  int start_u = width * lines_per_group * gid;"
                                       "  int start_v = (width * height) + start_u;"
                                       ""
                                       "  int nv12_uv_start = (width * 2) * lines_per_group * gid;"
                                       "  int max = width * lines_per_group;"
                                       "  for (int i = 0; i < max; ++i) {"
                                       "    int index = nv12_uv_start + i * 2;"
                                       "    nv12_uv_buff[index] = in_i420_uv[start_u + i];"
                                       "    nv12_uv_buff[index + 1] = in_i420_uv[start_v + i];"
                                       "  }"
                                       "}";

    cl_int err = CL_SUCCESS;
    // get platforms
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.size() == 0) { Log("can not get any opencl platforms."); return false; }
    // std::string platform_name = platforms[0].getInfo<CL_PLATFORM_NAME>();

    // get devices of GPU
    GetDevices(platforms, CL_DEVICE_TYPE_GPU, &m_devices);
    if (m_devices.size() == 0) { Log("can not get any GPU devices."); return false; }
    cl::Device& target_device = m_devices[0];
    std::string use_device_name = target_device.getInfo<CL_DEVICE_NAME>();

    // get max workgroups, must <= CL_DEVICE_MAX_WORK_GROUP_SIZE
    int num_compute_units = 0;
    err = target_device.getInfo(CL_DEVICE_MAX_COMPUTE_UNITS, &num_compute_units);
    m_max_device_workgroups = target_device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
    int u_height = m_height / 2;  // u channels
    m_workgroups = u_height;
    for (int i = 1; i <= u_height; ++i) {
        if (u_height % i == 0) {
            int groups = u_height / i;
            if (groups <= m_max_device_workgroups) {
                m_workgroups = groups;
                break;
            }
        }
    }

    // create context
    m_context = new cl::Context({target_device});

    // build device program
    cl::Program::Sources sources;
    sources.push_back({UpdateImageDataCode.c_str(), UpdateImageDataCode.length()});
    m_program = new cl::Program(*m_context, sources);
    if ((err = m_program->build({target_device})) != CL_SUCCESS) {
        Log("Error building: " + m_program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(target_device));
        return false;
    }
    // bind kernel function
    m_kernel_yuvi420_to_nv12 = new cl::Kernel(*m_program, "kYuvI420ToNV12", &err);
    if (err != CL_SUCCESS) { return false; }

    // create queue
    m_queue = new cl::CommandQueue(*m_context, target_device, 0, &err);
    if (err != CL_SUCCESS) { return false; }

    return true;
}

bool YuvI420ToNV12OpenCc::ConvertToNV12Impl(int width, int height, unsigned char* yuv_i420_img_data) {
    cl_int err = CL_SUCCESS;
    unsigned char* input_uv = yuv_i420_img_data + (width * height);
    const int uv_width = width / 2;
    const int uv_height = height / 2;
    const int NUM = uv_width * uv_height;  // only u data or only v data
    const int U_V_BLOCKS_SIZE = NUM * 2;

    // create input and output buffer
    cl::Buffer inputBuffer = cl::Buffer(*m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (U_V_BLOCKS_SIZE) * sizeof(unsigned char), (void*)input_uv, &err);
    cl::Buffer outputBuffer = cl::Buffer(*m_context, CL_MEM_WRITE_ONLY, U_V_BLOCKS_SIZE * sizeof(unsigned char), NULL, &err);

    // upload input data to target device
    err = m_queue->enqueueWriteBuffer(inputBuffer, CL_TRUE, 0, U_V_BLOCKS_SIZE * sizeof(unsigned char), input_uv);

    // bind kernel function parameters
    const int lines_per_group = uv_height / m_workgroups;
    err = m_kernel_yuvi420_to_nv12->setArg(0, inputBuffer);
    err = m_kernel_yuvi420_to_nv12->setArg(1, outputBuffer);
    err = m_kernel_yuvi420_to_nv12->setArg(2, uv_width);
    err = m_kernel_yuvi420_to_nv12->setArg(3, uv_height);
    err = m_kernel_yuvi420_to_nv12->setArg(4, lines_per_group);

    // running kernel function
    // https://downloads.ti.com/mctools/esd/docs/opencl/execution/kernels-workgroups-workitems.html
    // global size: work-items, 需要计算的work-item总数目。
    // local-size: work-items in a group, 一个group是一次被调度到硬件一个内核(cpu, gpu, apu, dsp..)上的执行度量单元，
    //             一个group需要完成WIG(work-items in a group)个work-items的计算。当WIG > 1, 则硬件计算单元多次执行对应的
    //             kernel函数来满足完成一个group内计算WIG个work-items的要求。
    //             显然如果只执行一次kernel函数就能完成一个group的计算要求是效率最高的。只要总work-items（实际计算中可以是“逻辑work-item”）
    //             和groups相等，则可以做到一个group执行完成一个"逻辑work-item"的要求，而不用在一个group内多次执行kernel函数。
    cl::Event event;
    // 让"逻辑work-items"和Workgroups数目相等，则每个group处理的时候只需要处理一个“逻辑work-item”
    err = m_queue->enqueueNDRangeKernel(*m_kernel_yuvi420_to_nv12, cl::NullRange, cl::NDRange(m_workgroups),
                                     cl::NDRange(1), NULL, &event);
    if (err != CL_SUCCESS) { return false; }
    event.wait();

    // read result, read the result put back to host memory
    // overwrite input_uv
    err = m_queue->enqueueReadBuffer(outputBuffer, CL_TRUE, 0, U_V_BLOCKS_SIZE * sizeof(unsigned char), input_uv, 0, NULL);

    return err == CL_SUCCESS;
}
