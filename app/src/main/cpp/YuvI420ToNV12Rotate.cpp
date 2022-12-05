//
// Created by Wookin on 12/3/2022.
//

#include "YuvI420ToNV12Rotate.h"

#include <string>
#include <vector>

#include "CL/cl.hpp"

#include "Utility.h"

void YuvI420ToNV12Rotate::Release() {
    m_devices.clear();
    delete m_context; m_context = nullptr;
    delete m_program; m_program = nullptr;
    delete m_queue; m_queue = nullptr;
    delete m_kernel_yuvi420_to_nv12; m_kernel_yuvi420_to_nv12 = nullptr;
    delete m_input_buff_yuv; m_input_buff_yuv = nullptr;
    delete m_output_buff_yuv; m_output_buff_yuv = nullptr;
}


std::string yuvi420_to_nv12_rotate_opencl_code() { return R_CODE( // ########################## begin of OpenCL C code ####################################################################
    // Only required by version 1.0.0 of the extension, version 2.0.0 does not
    // require the following pragma.
    // #pragma OPENCL EXTENSION cl_arm_printf : enable

    kernel void
    kYuvI420ToNV12Rotate(__global const unsigned char* in_buff_yuv,  // 0, m_input_buff_yuv
                   __global unsigned char* out_buff_yuv,             // 1, m_output_buff_yuv
                   const int y_width,                                // 2, y_width
                   const int y_height,                               // 3, y_height
                   const int y_block_size,                           // 4, y_width * y_height
                   const int u_width,                                // 5, u_width
                   const int u_height,                               // 6, u_height
                   const int u_block_size,                           // 7, u_width * u_height
                   const int out_uv_width                            // 8, rotated image's uv width. u_height * 2
                   ) {
        int ux = get_global_id(0);
        int uy = get_global_id(1);

        // printf("ux = %d, uy = %d\n", ux, uy);

        // origin image, luminance pixels p0, p1, p2, p3
        ushort2 lp0 = (ushort2)(ux << 1, uy << 1);  // ux * 2, uy * 2
        ushort2 lp1 = (ushort2)(lp0.x + 1, lp0.y);
        ushort2 lp2 = (ushort2)(lp0.x, lp0.y + 1);
        ushort2 lp3 = (ushort2)(lp1.x, lp2.y);

        // rotated image, luminance
        int y_height_sub1 = y_height - 1;
        ushort2 rp0 = (ushort2)(y_height_sub1 - lp0.y, lp0.x);
        ushort2 rp1 = (ushort2)(y_height_sub1 - lp1.y, lp1.x);
        ushort2 rp2 = (ushort2)(y_height_sub1 - lp2.y, lp2.x);
        ushort2 rp3 = (ushort2)(y_height_sub1 - lp3.y, lp3.x);

        // update rotated image's 4 luminance pixels
        // out_buff_yuv[rp0.y * y_height + rp0.x] = in_buff_yuv[lp0.y * y_width + lp0.x];
        // out_buff_yuv[rp1.y * y_height + rp1.x] = in_buff_yuv[lp1.y * y_width + lp1.x];
        // out_buff_yuv[rp2.y * y_height + rp2.x] = in_buff_yuv[lp2.y * y_width + lp2.x];
        // out_buff_yuv[rp3.y * y_height + rp3.x] = in_buff_yuv[lp3.y * y_width + lp3.x];
        //
        // rotated width = un-rotate image y_height
        uchar2 lu2_in = vload2(0, in_buff_yuv + (lp0.y * y_width + lp0.x));  // 0, 1, read 2 pixels once
        out_buff_yuv[rp0.y * y_height + rp0.x] = lu2_in.x;
        out_buff_yuv[rp1.y * y_height + rp1.x] = lu2_in.y;

        lu2_in = vload2(0, in_buff_yuv + (lp2.y * y_width + lp2.x));  // 2, 3, read 2 pixels once
        out_buff_yuv[rp2.y * y_height + rp2.x] = lu2_in.x;
        out_buff_yuv[rp3.y * y_height + rp3.x] = lu2_in.y;

        // rotated image u0, v0 position
        ushort delta_x = (u_height - uy - 1) << 1;   // * 2
        ushort2 r_up0 = (ushort2)(delta_x, ux);      // x' = (u_h - y0 - 1) * 2, y' = x0
        ushort2 r_vp0 = (ushort2)(delta_x + 1, ux);  // x' = (u_h - y0 - 1) * 2, y' = x0
        // update rotated image u0, v0 values
        const unsigned char* in_uv = in_buff_yuv + y_block_size;
        unsigned char* out_uv = out_buff_yuv + y_block_size;
        // out_uv[r_up0.y * out_uv_width + r_up0.x] = in_uv[uy * u_width + ux];
        // out_uv[r_vp0.y * out_uv_width + r_vp0.x] = in_uv[(u_height + uy) * u_width + ux];
        //
        // optimize
        int skip_u_lines_pixels = uy * u_width + ux;
        out_uv[r_up0.y * out_uv_width + r_up0.x] = in_uv[skip_u_lines_pixels];
        out_uv[r_vp0.y * out_uv_width + r_vp0.x] = in_uv[u_block_size + skip_u_lines_pixels];
    }
);} // ############################################################### end of OpenCL C code #####################################################################

// printf("%.*s", length, buffer);
void opencl_printf_callback( const char* buffer, unsigned int length, size_t final, void* user_data) {
    LOGI("%.*s", length, buffer);
}

bool YuvI420ToNV12Rotate::Init() {
    cl_int err = CL_SUCCESS;
    // get platforms
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.size() == 0) { LOGI("can not get any opencl platforms."); return false; }
    // std::string platform_name = platforms[0].getInfo<CL_PLATFORM_NAME>();

    // get devices of GPU
    GetDevices(platforms, CL_DEVICE_TYPE_GPU, &m_devices);
    if (m_devices.size() == 0) { LOGI("can not get any GPU devices."); return false; }
    cl::Device& target_device = m_devices[0];
    std::string use_device_name = target_device.getInfo<CL_DEVICE_NAME>();

    int num_compute_units = 0;
    err = target_device.getInfo(CL_DEVICE_MAX_COMPUTE_UNITS, &num_compute_units);
    LOGI("compute unites: %d", num_compute_units);

    // https://man.opencl.org/clGetDeviceInfo.html
    // max work-items in local group, Max allowed work-items in a group.
    // Maximum number of work-items in a work-group that a device is capable of executing on a single compute unit,
    // for any given kernel-instance running on the device.
    int max_local_group_work_items = target_device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
    LOGI("max work-items in local group: %d", max_local_group_work_items);

    // u channel bytes
    uint32_t u_total_bytes = (m_width / 2) * (m_height / 2);

    int PROCESS_BYTES_IN_WI_STEP = 1;
    int max_u_size_each_work_item = 1;
    m_global_work_items = u_total_bytes / max_u_size_each_work_item + (u_total_bytes % max_u_size_each_work_item > 0 ? 1 : 0);
    if (m_global_work_items <= kDefaultLocalGroupSize) {
        // global work items is too small, decrease local group size
        m_local_group_size = m_global_work_items;
    } else {
        for (int i = kDefaultLocalGroupSize; i >= 1; --i) {
            if (m_global_work_items % i == 0) {
                m_local_group_size = i;
                break;
            }
        }
        /*m_local_group_size = kDefaultLocalGroupSize;
        int workgroups = m_max_device_workgroups + 1;
        for (int i = 1; i < u_total_bytes && workgroups > m_max_device_workgroups; ++i) {
            // each work-item process some u bytes, each work item process 8 u bytes
            m_max_u_size_each_work_item = PROCESS_BYTES_IN_WI_STEP * i;
            // each work-group process how many u bytes
            uint32_t u_len_per_wg = m_local_group_size * m_max_u_size_each_work_item;
            workgroups = u_total_bytes / u_len_per_wg + (u_total_bytes % u_len_per_wg > 0 ? 1 : 0);
        }
        m_global_work_items = u_total_bytes / m_max_u_size_each_work_item + (u_total_bytes % m_max_u_size_each_work_item > 0 ? 1 : 0);
        */
    }

    // get extensions info
    auto info = target_device.getInfo<CL_DEVICE_EXTENSIONS>();
    LOGI("Opencl extensions: %s\n", info.c_str());

    // create context
    cl_context_properties context_properties[] = {
      CL_PRINTF_CALLBACK_ARM, (cl_context_properties) opencl_printf_callback,
      CL_PRINTF_BUFFERSIZE_ARM, 0x100000,
      CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[0](),
      0
    };
    // disable setup printf callback function
    // m_context = new cl::Context({target_device, context_properties});
    m_context = new cl::Context({target_device});

    // build device program
    cl::Program::Sources sources;
    // sources.push_back({UpdateImageDataCode.c_str(), UpdateImageDataCode.length()});
    std::string cl_code = yuvi420_to_nv12_rotate_opencl_code();
    sources.push_back({cl_code.c_str(), cl_code.length()});

    m_program = new cl::Program(*m_context, sources);
    if ((err = m_program->build({target_device}, "-cl-std=CL2.0 -O2")) != CL_SUCCESS) {
        std::string build_error = "Building error: " + m_program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(target_device);
        LOGI("%s\n", build_error.c_str());
        return false;
    }
    // bind kernel function
    m_kernel_yuvi420_to_nv12 = new cl::Kernel(*m_program, "kYuvI420ToNV12Rotate", &err);
    if (err != CL_SUCCESS) { return false; }

    // create input / output buffer
    const int u_width = m_width / 2;
    const int u_height = m_height / 2;
    const int Y_UV_BLOCKS_SIZE = m_width * m_height * 1.5;
    m_input_buff_yuv = new cl::Buffer(*m_context, CL_MEM_READ_ONLY, (Y_UV_BLOCKS_SIZE) * sizeof(unsigned char),nullptr, &err);
    m_output_buff_yuv = new cl::Buffer(*m_context, CL_MEM_WRITE_ONLY, Y_UV_BLOCKS_SIZE * sizeof(unsigned char), NULL, &err);

    // bind kernel function parameters
    const int out_uv_width = u_height * 2;
    err = m_kernel_yuvi420_to_nv12->setArg(0, *m_input_buff_yuv);
    err = m_kernel_yuvi420_to_nv12->setArg(1, *m_output_buff_yuv);
    err = m_kernel_yuvi420_to_nv12->setArg(2, m_width);
    err = m_kernel_yuvi420_to_nv12->setArg(3, m_height);
    err = m_kernel_yuvi420_to_nv12->setArg(4, m_width * m_height);
    err = m_kernel_yuvi420_to_nv12->setArg(5, u_width);
    err = m_kernel_yuvi420_to_nv12->setArg(6, u_height);
    err = m_kernel_yuvi420_to_nv12->setArg(7, u_width * u_height);
    err = m_kernel_yuvi420_to_nv12->setArg(8, out_uv_width);

    // create queue
    m_queue = new cl::CommandQueue(*m_context, target_device, 0, &err);
    if (err != CL_SUCCESS) { return false; }

    return true;
}

struct Position{
    int x;
    int y;
    Position(int x0, int y0): x(x0), y(y0){}
};

void cc_kYuvI420ToNV12Rotate_HostDebug(const unsigned char* in_buff_yuv,  // 0, m_input_buff_yuv
    unsigned char* out_buff_yuv,                    // 1, m_output_buff_yuv
    const int y_width,                                       // 2, y_width
    const int y_height,                                      // 3, y_height
    const int y_block_size,                                  // 4, y_width * y_height
    const int u_width,                                       // 5, u_width
    const int u_height,                                      // 6, u_height
    const int u_block_size,                                  // 7, u_width * u_height
    int uw_pos,
    int uh_pos) {                          // 9, max_valid_group_id
    int ux = uw_pos; // get_global_id(0);
    int uy = uh_pos; // get_global_id(1);

    // int r_x0 = y_height - ly - 1;  // rotated image x0
    // int r_y0 = lx;                 // rotated image y0
    Position lp0(ux * 2, uy * 2);  // luminance p0
    Position lp1(lp0.x + 1, lp0.y);
    Position lp2(lp0.x, lp0.y + 1);
    Position lp3(lp0.x + 1, lp0.y + 1);

    Position rp0(y_height - lp0.y - 1, lp0.x);
    Position rp1(y_height - lp1.y - 1, lp1.x);
    Position rp2(y_height - lp2.y - 1, lp2.x);
    Position rp3(y_height - lp3.y - 1, lp3.x);

    // rotated width = y_height
    out_buff_yuv[rp0.y * y_height + rp0.x] = in_buff_yuv[lp0.y * y_width + lp0.x];
    out_buff_yuv[rp1.y * y_height + rp1.x] = in_buff_yuv[lp1.y * y_width + lp1.x];
    out_buff_yuv[rp2.y * y_height + rp2.x] = in_buff_yuv[lp2.y * y_width + lp2.x];
    out_buff_yuv[rp3.y * y_height + rp3.x] = in_buff_yuv[lp3.y * y_width + lp3.x];

    // v0 = (ushort2)(ux, uy);
    unsigned short delta_x = (u_height - uy - 1) * 2;
    Position r_up0(delta_x, ux);  // x' = (u_h - y0 - 1) * 2, y' = x0
    Position r_vp0(delta_x + 1, ux);  // x' = (u_h - y0 - 1) * 2, y' = x0
    const unsigned char* in_uv = in_buff_yuv + y_block_size;
    unsigned char* out_uv = out_buff_yuv + y_block_size;
    unsigned short uv_width = u_height * 2;
    out_uv[r_up0.y * uv_width + r_up0.x] = in_uv[uy * u_width + ux];
    out_uv[r_vp0.y * uv_width + r_vp0.x] = in_uv[(u_height + uy) * u_width + ux];
    LOGI("u(%d, %d), r_u(%d, %d) - index(src: %d, dst: %d), r_v(%d, %d) - index(src: %d, dst: %d)\n",
         ux, uy,
         r_up0.x, r_up0.y, uy * u_width + ux, r_up0.y * uv_width + r_up0.x,
         r_vp0.x, r_vp0.y, (u_height + uy) * u_width + ux, r_vp0.y * uv_width + r_vp0.x);
}

bool YuvI420ToNV12Rotate::ConvertToNV12RotateImpl_HostDebug(int width, int height, unsigned char* yuv_i420_img_data) {
    cl_int err = CL_SUCCESS;
    unsigned char* input_yuv = yuv_i420_img_data;
    const int u_width = width / 2;
    const int u_height = height / 2;

    int localx = 8;
    if(u_width / 8 > 4)
      localx = 16;
    else if(u_width < 8)
      localx = u_width;

    int localy = 8;
    if(u_height / 8 > 4)
      localy = 16;
    else if (u_height < 8)
      localy = u_height;

    // ----------------test---------------------
    unsigned char *out_yuv = new unsigned char[width * height * 1.5];
    for (int u_h = localy - 1; u_h >=0; --u_h) {
      for (int u_w = localx - 1; u_w >= 0; --u_w) {
        cc_kYuvI420ToNV12Rotate_HostDebug(input_yuv,  // 0, m_input_buff_yuv
            out_yuv,                    // 1, m_output_buff_yuv
            width,                                       // 2, y_width
            height,                                      // 3, y_height
            width * height,                                  // 4, y_width * y_height
            u_width,                                       // 5, u_width
            u_height,                                      // 6, u_height
            u_width * u_height,                                  // 7, u_width * u_height
            u_w,
            u_h);
      }
    }
    memcpy(input_yuv, out_yuv, width * height * 1.5);
    delete[]out_yuv;
    return true;
}

bool YuvI420ToNV12Rotate::ConvertToNV12RotateImpl(int width, int height, unsigned char* yuv_i420_img_data) {
    // debug only
    // return ConvertToNV12RotateImpl_HostDebug(width, height, yuv_i420_img_data);

    cl_int err = CL_SUCCESS;
    unsigned char* input_yuv = yuv_i420_img_data;
    const int u_width = width / 2;
    const int u_height = height / 2;
    const int Y_UV_BLOCKS_SIZE = width * height * 1.5; // y_block + u_block + v_block

    // upload input data to target device
    cl::Event upload_mem_ev;
    err = m_queue->enqueueWriteBuffer(*m_input_buff_yuv, CL_FALSE, 0, Y_UV_BLOCKS_SIZE * sizeof(unsigned char), input_yuv, nullptr, &upload_mem_ev);

    // running kernel function
    // https://downloads.ti.com/mctools/esd/docs/opencl/execution/kernels-workgroups-workitems.html
    // global size: work-items, 需要计算的work-item总数目。
    // local-size: work-items in a group, 一个group是一次被调度到硬件一个内核(cpu, gpu, apu, dsp..)上的执行度量单元，
    //             一个group需要完成WIG(work-items in a group)个work-items的计算。当WIG > 1, 则硬件计算单元多次执行对应的
    //             kernel函数来满足完成一个group内计算WIG个work-items的要求。
    //             硬件上一次local-group(work-group)执行时一般是32或64个硬件线程同时执行。
    //             xx显然如果只执行一次kernel函数就能完成一个group的计算要求是效率最高的。只要总work-items（实际计算中可以是“逻辑work-item”）
    //             xx和groups相等，则可以做到一个group执行完成一个"逻辑work-item"的要求，而不用在一个group内多次执行kernel函数。

    // 让"逻辑work-items"和Workgroups数目相等，则每个group处理的时候只需要处理一个“逻辑work-item”
    // https://man.opencl.org/clEnqueueNDRangeKernel.html
    // If local_work_size is NULL and non-uniform-work-groups are enabled, the OpenCL runtime is free to implement the ND-range
    // using uniform or non-uniform work-group sizes, regardless of the divisibility of the global work size.
    // x - work-items in local group
    // y - work-items in local group
    /*int x_lg_size = 0;
    for (int n = 64; n >= 1; --n) {
        if (u_width % n == 0) { x_lg_size = n; break;}
    }
    int y_lg_size = 0;
    for (int n = 64; n >= 1; --n) {
        if (u_height % n == 0) { y_lg_size = n; break;}
    }*/
    int localx = 8;
    if(u_width / 8 > 4)
        localx = 16;
    else if(u_width < 8)
        localx = u_width;

    int localy = 8;
    if(u_height / 8 > 4)
        localy = 16;
    else if (u_height < 8)
        localy = u_height;

    // default, 1920 * 1080, 14- 20ms
    // localx = 1; localy = 64; 1920*1080 <= 20ms
    cl::Event enqueue_ndrange_ev;
    std::vector<cl::Event> wait_events;
    wait_events.push_back(upload_mem_ev);
    err = m_queue->enqueueNDRangeKernel(*m_kernel_yuvi420_to_nv12, cl::NullRange, cl::NDRange(u_width, u_height), cl::NDRange(localx, localy), &wait_events, &enqueue_ndrange_ev);
    if (err != CL_SUCCESS) { return false; }
    // event.wait();

    // read result, read the result put back to host memory
    // overwrite input_yuv
    std::vector<cl::Event> wait_finish_events;
    wait_finish_events.push_back(enqueue_ndrange_ev);
    err = m_queue->enqueueReadBuffer(*m_output_buff_yuv, CL_TRUE, 0, Y_UV_BLOCKS_SIZE * sizeof(unsigned char), input_yuv, &wait_finish_events, NULL);

    return err == CL_SUCCESS;
}
