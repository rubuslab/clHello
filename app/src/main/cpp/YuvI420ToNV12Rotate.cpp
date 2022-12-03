//
// Created by Wookin on 12/3/2022.
//

#include "YuvI420ToNV12Rotate.h"

#include <string>
#include <vector>

#include "CL/cl.hpp"

#include "Utility.h"

void YuvI420ToNV12Rotate::Log(std::string message) {
    // std::cout << message << std::endl;
}

void YuvI420ToNV12Rotate::Release() {
    m_devices.clear();
    delete m_context; m_context = nullptr;
    delete m_program; m_program = nullptr;
    delete m_queue; m_queue = nullptr;
    delete m_kernel_yuvi420_to_nv12; m_kernel_yuvi420_to_nv12 = nullptr;
    delete m_input_buff_yuv; m_input_buff_yuv = nullptr;
    delete m_output_buff_yuv; m_output_buff_yuv = nullptr;
}

bool YuvI420ToNV12Rotate::Init() {
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

     // before use mask
     // --------------------------------------------------
         std::string UpdateImageDataCode{R"CLC(
      // for each target device, modify this printf function name.
      // #pragma OPENCL EXTENSION cl_arm_printf : enable

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


              // printf("Hello from opencl...\n");

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
     // --------------------------------------------------
    */

    /*
    get_group_id -- Work-group ID
    get_global_id --  Global work-item ID
    get_local_id -- Local work-item ID

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
      const uchar16 g_mask_8uv = (uchar16)(0,8,  1,9,  2,10,  3,11,  4,12,  5,13,  6,14,  7,15);
      const uchar8  g_mask_4uv = (uchar8)(0,4,  1,5,  2,6,  3,7);

      // process 1 u/v line once this function called
      kernel void
      kYuvI420ToNV12(__global const unsigned char* in_i420_uv,  // 0, m_input_buff_yuv
        __global unsigned char* nv12_uv_buff,                   // 1, m_output_buff_yuv
        const int y_width,                                      // 2, y_width
        const int y_height,                                     // 3, y_height
        const int y_block_size,                                 // 4, y_width * y_height
        const int u_width,                                      // 5, u_width
        const int u_height,                                     // 6, u_height
        const int u_block_size,                                 // 7, u_width * u_height
        const int max_u_size_each_work_item,                    // 8, m_max_u_size_each_work_item
        const int max_valid_group_id) {                         // 9, max_valid_group_id
            int gid = get_global_id(0);
            int u_total_bytes = u_width * u_height;

            // calculate u bytes must been processed in this work-item
            int u_bytes = max_u_size_each_work_item;
            if (gid == max_valid_group_id) u_bytes = u_total_bytes % max_u_size_each_work_item;
            // if (gid > max_valid_group_id) u_bytes = 0;

            // start u, v position
            int start_u = gid * max_u_size_each_work_item;
            int start_v = u_total_bytes + start_u;
            // uv,uv,uv,uv...
            int nv12_uv_start = start_u * 2;

            // kernel function each routine process u_bytes of u channel data
            // each loop process 8 u bytes
            // uchar16 mask_8uv = (uchar16)(0,8,  1,9,  2,10,  3,11,  4,12,  5,13,  6,14,  7,15);
            uchar16 uv16;    // uv data, u0v0-u1v1-u2v2...
            uchar8 u8;       // u channel data, u0-u1-u2-u3...
            uchar8 v8;       // v channel data, v0-v1-v2-v3...
            // loops is 0, when u_bytes < 8.
            int loops = u_bytes / 8;
            int u_offset = 0;
            for (int l = 0; l < loops; ++l) {
              u8 = vload8(0, in_i420_uv + start_u + u_offset);  // 0, 1,  2,  3,  4,  5,  6,  7
              v8 = vload8(0, in_i420_uv + start_v + u_offset);  // 8, 9, 10, 11, 12, 13, 14, 15
              uv16 = shuffle2(u8, v8, g_mask_8uv);
              vstore16(uv16, 0, nv12_uv_buff + nv12_uv_start + u_offset * 2);
              u_offset += 8;
            }

            int remain_bytes = u_bytes % 8;

            // if last work-item's remain u bytes less than 8 and >= 4
            // if remain u bytes >= 4
            int f_loops = remain_bytes / 4;
            for (int l = 0; l < f_loops; ++l) {
                uchar4 u4 = vload4(0, in_i420_uv + start_u + u_offset);  // 0, 1, 2, 3
                uchar4 v4 = vload4(0, in_i420_uv + start_v + u_offset);  // 4, 5, 6, 7
                uchar8 uv8 = shuffle2(u4, v4, g_mask_4uv);
                vstore8(uv8, 0, nv12_uv_buff + nv12_uv_start + u_offset * 2);
                u_offset += 4;
             }

            // remain bytes must less than 4
            remain_bytes = u_bytes % 4;
            for (int r = 0; r < remain_bytes; ++r) {
              int uv_index = nv12_uv_start + u_offset * 2;
              nv12_uv_buff[uv_index]     = in_i420_uv[start_u + u_offset];  // u
              nv12_uv_buff[uv_index + 1] = in_i420_uv[start_v + u_offset];  // v
              u_offset++;
            }
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
    // u channel bytes
    uint32_t u_total_bytes = (m_width / 2) * (m_height / 2);

    int PROCESS_BYTES_IN_WI_STEP = 1;
    m_max_u_size_each_work_item = PROCESS_BYTES_IN_WI_STEP;
    m_global_work_items = u_total_bytes / m_max_u_size_each_work_item + (u_total_bytes % m_max_u_size_each_work_item > 0 ? 1 : 0);
    if (m_global_work_items <= kDefaultLocalGroupSize) {
        // global work items is too small, decrease local group size
        m_local_group_size = m_global_work_items;
    } else {
        m_local_group_size = kDefaultLocalGroupSize;
        // set fake value
        int workgroups = m_max_device_workgroups + 1;
        for (int i = 1; i < u_total_bytes && workgroups > m_max_device_workgroups; ++i) {
            // each work-item process some u bytes, each work item process 8 u bytes
            m_max_u_size_each_work_item = PROCESS_BYTES_IN_WI_STEP * i;
            // each work-group process how many u bytes
            uint32_t u_len_per_wg = m_local_group_size * m_max_u_size_each_work_item;
            workgroups = u_total_bytes / u_len_per_wg + (u_total_bytes % u_len_per_wg > 0 ? 1 : 0);
        }
        m_global_work_items = u_total_bytes / m_max_u_size_each_work_item + (u_total_bytes % m_max_u_size_each_work_item > 0 ? 1 : 0);
    }
    m_max_valid_group_id = m_global_work_items -1;

    // get extensions info
    auto info = target_device.getInfo<CL_DEVICE_EXTENSIONS>();
    LOGI("Opencl extensions: %s\n", info.c_str());

    // create context
    m_context = new cl::Context({target_device});

    // build device program
    cl::Program::Sources sources;
    sources.push_back({UpdateImageDataCode.c_str(), UpdateImageDataCode.length()});
    m_program = new cl::Program(*m_context, sources);
    if ((err = m_program->build({target_device}, "-cl-std=CL2.0")) != CL_SUCCESS) {
        std::string build_error = "Building error: " + m_program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(target_device);
        LOGI("%s\n", build_error.c_str());
        return false;
    }
    // bind kernel function
    m_kernel_yuvi420_to_nv12 = new cl::Kernel(*m_program, "kYuvI420ToNV12", &err);
    if (err != CL_SUCCESS) { return false; }

    // create input / output buffer
    const int u_width = m_width / 2;
    const int u_height = m_height / 2;
    const int Y_UV_BLOCKS_SIZE = m_width * m_height * 1.5;
    m_input_buff_yuv = new cl::Buffer(*m_context, CL_MEM_READ_ONLY, (Y_UV_BLOCKS_SIZE) * sizeof(unsigned char),nullptr, &err);
    m_output_buff_yuv = new cl::Buffer(*m_context, CL_MEM_WRITE_ONLY, Y_UV_BLOCKS_SIZE * sizeof(unsigned char), NULL, &err);

    // bind kernel function parameters
    err = m_kernel_yuvi420_to_nv12->setArg(0, *m_input_buff_yuv);
    err = m_kernel_yuvi420_to_nv12->setArg(1, *m_output_buff_yuv);
    err = m_kernel_yuvi420_to_nv12->setArg(2, m_width);
    err = m_kernel_yuvi420_to_nv12->setArg(3, m_height);
    err = m_kernel_yuvi420_to_nv12->setArg(4, m_width * m_height);
    err = m_kernel_yuvi420_to_nv12->setArg(5, u_width);
    err = m_kernel_yuvi420_to_nv12->setArg(6, u_height);
    err = m_kernel_yuvi420_to_nv12->setArg(7, u_width * u_height);
    err = m_kernel_yuvi420_to_nv12->setArg(8, m_max_u_size_each_work_item);
    err = m_kernel_yuvi420_to_nv12->setArg(9, m_max_valid_group_id);

    // create queue
    m_queue = new cl::CommandQueue(*m_context, target_device, 0, &err);
    if (err != CL_SUCCESS) { return false; }

    return true;
}

bool YuvI420ToNV12Rotate::ConvertToNV12RotateImpl(int width, int height, unsigned char* yuv_i420_img_data) {
    cl_int err = CL_SUCCESS;
    unsigned char* input_uv = yuv_i420_img_data + (width * height);
    const int u_width = width / 2;
    const int u_height = height / 2;
    const int Y_UV_BLOCKS_SIZE = width * height * 1.5; // y_block + u_block + v_block

    // upload input data to target device
    err = m_queue->enqueueWriteBuffer(*m_input_buff_yuv, CL_TRUE, 0, Y_UV_BLOCKS_SIZE * sizeof(unsigned char), input_uv);

    // running kernel function
    // https://downloads.ti.com/mctools/esd/docs/opencl/execution/kernels-workgroups-workitems.html
    // global size: work-items, 需要计算的work-item总数目。
    // local-size: work-items in a group, 一个group是一次被调度到硬件一个内核(cpu, gpu, apu, dsp..)上的执行度量单元，
    //             一个group需要完成WIG(work-items in a group)个work-items的计算。当WIG > 1, 则硬件计算单元多次执行对应的
    //             kernel函数来满足完成一个group内计算WIG个work-items的要求。
    //             硬件上一次local-group(work-group)执行时一般是32或64个硬件线程同时执行。
    //             xx显然如果只执行一次kernel函数就能完成一个group的计算要求是效率最高的。只要总work-items（实际计算中可以是“逻辑work-item”）
    //             xx和groups相等，则可以做到一个group执行完成一个"逻辑work-item"的要求，而不用在一个group内多次执行kernel函数。
    cl::Event event;
    // 让"逻辑work-items"和Workgroups数目相等，则每个group处理的时候只需要处理一个“逻辑work-item”
    err = m_queue->enqueueNDRangeKernel(*m_kernel_yuvi420_to_nv12, cl::NullRange, cl::NDRange(m_global_work_items),
                                        cl::NDRange(m_local_group_size), NULL, &event);
    if (err != CL_SUCCESS) { return false; }
    event.wait();

    // read result, read the result put back to host memory
    // overwrite input_uv
    err = m_queue->enqueueReadBuffer(*m_output_buff_yuv, CL_TRUE, 0, Y_UV_BLOCKS_SIZE * sizeof(unsigned char), input_uv, 0, NULL);

    return err == CL_SUCCESS;
}
