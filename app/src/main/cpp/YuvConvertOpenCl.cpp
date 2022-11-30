#include <string>
#include <vector>

#include "CL/cl.hpp"

#include "Utility.h"
#include "YuvConvertOpenCl.h"

void YuvI420ToNV12OpenCc::Log(std::string message) {
    // std::cout << message << std::endl;
}

void YuvI420ToNV12OpenCc::Release() {
    m_devices.clear();
    delete m_context; m_context = nullptr;
    delete m_program; m_program = nullptr;
    delete m_queue; m_queue = nullptr;
    delete m_kernel_yuvi420_to_nv12; m_kernel_yuvi420_to_nv12 = nullptr;
    delete m_input_buff_uv; m_input_buff_uv = nullptr;
    delete m_output_buff_uv; m_output_buff_uv = nullptr;
}

bool YuvI420ToNV12OpenCc::Init() {
    /*std::string UpdateImageDataCode =  "kernel void kYuvI420ToNV12(global const unsigned char* in_i420_uv,"
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

    std::string UpdateImageDataCode{R"CLC(
      kernel void
      kYuvI420ToNV12(__global unsigned char* in_i420_uv,
        __global unsigned char* nv12_uv_buff,
        const int u_width, const int u_height,
        const int u_lines_per_group) {
            int gid = get_global_id(0);
            int start_u = u_width * u_lines_per_group * gid;
            int start_v = (u_width * u_height) + start_u;

            int nv12_uv_start = (u_width * 2) * u_lines_per_group * gid;
            int max = u_width * u_lines_per_group;
            for (int i = 0; i < max; ++i) {
              int index = nv12_uv_start + i * 2;
              nv12_uv_buff[index] = in_i420_uv[start_u + i];
              nv12_uv_buff[index + 1] = in_i420_uv[start_v + i];
            }
      }
    )CLC"};
    */
    /*
     // i420
     U0U1U2U3U4
     ...
     -----
     V0V1V2V3V4
     ...
     // nv12
     U0V0U1V1U2V2U3V3U4V4
     */
    std::string UpdateImageDataCode{R"CLC(
      kernel void
      kYuvI420ToNV12(__global unsigned char* in_i420_uv,
        __global unsigned char* nv12_uv_buff,
        const int u_width, const int u_height,
        const int u_lines_per_group) {
            int gid = get_global_id(0);
            int local_line_idx = get_local_id(0);
            int start_u = u_width * (u_lines_per_group * gid + local_line_idx);
            int start_v = (u_width * u_height) + start_u;

            // each kernel function deal one line u channel data
            int nv12_uv_start = (u_width * 2) * (u_lines_per_group * gid + local_line_idx);

            int loops = u_width / 16;
            loops = 1;
            for (int l = 0; l < loops; ++l) {
              int offset = l * 16;
              uchar16 u16 = vload16(0, in_i420_uv + start_u + offset);
              uchar16 v16 = vload16(0, in_i420_uv + start_v + offset);

              ushort16 uv16 = upsample(u16, v16);

              uchar16 bytes16_uv0;
              //bytes16_uv0.s01 = uv16.s0;
              //bytes16_uv0.s01 = 0x0506;
              bytes16_uv0.s0 = 0x05;
              bytes16_uv0.s1 = 0x06;

              bytes16_uv0.s23 = uv16.s1;
              bytes16_uv0.s45 = uv16.s2;
              bytes16_uv0.s67 = uv16.s3;
              bytes16_uv0.s89 = uv16.s4;
              bytes16_uv0.sab = uv16.s5;
              bytes16_uv0.scd = uv16.s6;
              bytes16_uv0.sef = uv16.s7;
              // vstore16(bytes16_uv0, 0, nv12_uv_buff + nv12_uv_start + offset);
              vstore16(bytes16_uv0, 0, nv12_uv_buff + nv12_uv_start + l * 16 * 2);

              uchar16 bytes16_uv1;
              bytes16_uv1.s01 = uv16.s8;
              bytes16_uv1.s23 = uv16.s9;
              bytes16_uv1.s45 = uv16.sa;
              bytes16_uv1.s67 = uv16.sb;
              bytes16_uv1.s89 = uv16.sc;
              bytes16_uv1.sab = uv16.sd;
              bytes16_uv1.scd = uv16.se;
              bytes16_uv1.sef = uv16.sf;
              // vstore16(bytes16_uv1, 0, nv12_uv_buff + nv12_uv_start + offset + 16);

              //  ok- vstore16(bytes16_uv1, 0, nv12_uv_buff + nv12_uv_start + l * 16 * 2 + 16);

              // vstore16(uv16, 0, nv12_uv_buff + nv12_uv_start + offset * 2);


              //uchar8 v1 = convert_uchar8_sat(uv16.hi);
              //uchar8 v2 =  convert_uchar8_sat(uv16.lo);
              //u16_val.hi = v1;
              //u16_val.lo = convert_uchar8_sat(uv16.lo);

              //vstore8(u16_val.lo, 0, nv12_uv_buff + nv12_uv_start + offset * 2);

              //uchar16 uv_bytes0;
              //uv_bytes0.lo = convert_uchar8_sat(uv16.s0123);
              //uv_bytes0.hi = convert_uchar8_sat(uv16.s4567);

              // uv16.s0123;
              //vstore16(u16_val, 0, nv12_uv_buff + nv12_uv_start + offset * 2);

              //vstore16(convert_uchar16_sat_rtz(uv16.hi), 0, nv12_uv_buff + nv12_uv_start + offset * 2);
              //vstore16(convert_uchar16_sat_rtz(uv16.lo), 0, nv12_uv_buff + nv12_uv_start + offset * 2 + 16);

              // ok
              // vstore8(convert_uchar8_sat(uv16.hi), 0, nv12_uv_buff + nv12_uv_start + offset * 2);
            }
            // for (int i = 0; i < u_width; ++i) {
            //  int index = nv12_uv_start + i * 2;
            //  nv12_uv_buff[index] = in_i420_uv[start_u + i];
            //  nv12_uv_buff[index + 1] = in_i420_uv[start_v + i];
            // }
      }
    )CLC"};

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
    int u_channel_height = m_height / 2;  // u channels
    m_workgroups = u_channel_height;
    for (int i = 1; i <= u_channel_height; ++i) {
        if (u_channel_height % i == 0) {
            int groups = u_channel_height / i;
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
    if ((err = m_program->build({target_device}, "-cl-std=CL2.0")) != CL_SUCCESS) {
        std::string build_error = "Building error: " + m_program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(target_device);
        Log(build_error);
        return false;
    }
    // bind kernel function
    m_kernel_yuvi420_to_nv12 = new cl::Kernel(*m_program, "kYuvI420ToNV12", &err);
    if (err != CL_SUCCESS) { return false; }

    // create input / output buffer
    const int u_width = m_width / 2;
    const int u_height = m_height / 2;
    const int NUM = u_width * u_height;  // only u data or only v data
    const int U_V_BLOCKS_SIZE = NUM * 2;
    m_input_buff_uv = new cl::Buffer(*m_context, CL_MEM_READ_ONLY, (U_V_BLOCKS_SIZE) * sizeof(unsigned char),nullptr, &err);
    m_output_buff_uv = new cl::Buffer(*m_context, CL_MEM_WRITE_ONLY, U_V_BLOCKS_SIZE * sizeof(unsigned char), NULL, &err);

    // bind kernel function parameters
    m_u_lines_per_group = u_height / m_workgroups;
    err = m_kernel_yuvi420_to_nv12->setArg(0, *m_input_buff_uv);
    err = m_kernel_yuvi420_to_nv12->setArg(1, *m_output_buff_uv);
    err = m_kernel_yuvi420_to_nv12->setArg(2, u_width);
    err = m_kernel_yuvi420_to_nv12->setArg(3, u_height);
    err = m_kernel_yuvi420_to_nv12->setArg(4, m_u_lines_per_group);

    // create queue
    m_queue = new cl::CommandQueue(*m_context, target_device, 0, &err);
    if (err != CL_SUCCESS) { return false; }

    return true;
}

bool YuvI420ToNV12OpenCc::ConvertToNV12Impl(int width, int height, unsigned char* yuv_i420_img_data) {
    cl_int err = CL_SUCCESS;
    unsigned char* input_uv = yuv_i420_img_data + (width * height);
    const int u_width = width / 2;
    const int u_height = height / 2;
    const int NUM = u_width * u_height;  // only u data or only v data
    const int U_V_BLOCKS_SIZE = NUM * 2;

    // upload input data to target device
    err = m_queue->enqueueWriteBuffer(*m_input_buff_uv, CL_TRUE, 0, U_V_BLOCKS_SIZE * sizeof(unsigned char), input_uv);

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
                                     cl::NDRange(m_u_lines_per_group), NULL, &event);
    if (err != CL_SUCCESS) { return false; }
    event.wait();

    // read result, read the result put back to host memory
    // overwrite input_uv
    err = m_queue->enqueueReadBuffer(*m_output_buff_uv, CL_TRUE, 0, U_V_BLOCKS_SIZE * sizeof(unsigned char), input_uv, 0, NULL);

    return err == CL_SUCCESS;
}
