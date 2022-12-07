//
// Created by Rubus on 12/3/2022.
//

#include <string>
#include <vector>

#include "Utility.h"
#include "YuvI420ToNV12Rotate.h"

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

    ulong8 indexs8 =   (ulong8)(0, 1, 2, 3, 4, 5, 6, 7);
    ulong4 u_indexs4 = (ulong4)(0, 1, 2, 3);

    kernel void
    kYuvI420ToNV12Rotate(__global const unsigned char* in_buff_yuv,  // 0, m_input_buff_yuv
                   __global unsigned char* out_buff_yuv,             // 1, m_output_buff_yuv
                   const int y_width,                                // 2, y_width
                   const int y_height,                               // 3, y_height
                   const int y_bytes_size,                           // 4, y_width * y_height
                   const int u_width,                                // 5, u_width
                   const int u_height,                               // 6, u_height
                   const int u_bytes_size,                           // 7, u_width * u_height
                   const int out_uv_width                            // 8, rotated image's uv width equal to u_height * 2
                   ) {
        int ux = get_global_id(0);  // u-block index at x - axle.
        int uy = get_global_id(1);  // u-block index at y - axle.

        int u_block_width = 4;
        int u_block_height = 4;
        int max_u_x_index = u_width / u_block_width - 1;
        int max_u_y_index = u_height / u_block_height - 1;

        // printf("ux = %d, uy = %d\n", ux, uy);

        // origin - y image block
        int lu_start = (uy * u_block_height * 2) * y_width + (ux * u_block_width * 2);
        // ulong8 indexs8 = (ulong8)(0, 1, 2, 3, 4, 5, 6, 7);
        ulong8 offsets = indexs8 * y_width;
        offsets += (lu_start + (ulong)in_buff_yuv);
        // load y block data, 8 x 8 (width * height)
        uchar8 yl0 = vload8(0, (const unsigned char*)(offsets.s0));
        uchar8 yl1 = vload8(0, (const unsigned char*)(offsets.s1));
        uchar8 yl2 = vload8(0, (const unsigned char*)(offsets.s2));
        uchar8 yl3 = vload8(0, (const unsigned char*)(offsets.s3));
        uchar8 yl4 = vload8(0, (const unsigned char*)(offsets.s4));
        uchar8 yl5 = vload8(0, (const unsigned char*)(offsets.s5));
        uchar8 yl6 = vload8(0, (const unsigned char*)(offsets.s6));
        uchar8 yl7 = vload8(0, (const unsigned char*)(offsets.s7));

        // rotated - y image block
        uchar8 rl0 = (uchar8)(yl7.s0, yl6.s0, yl5.s0, yl4.s0, yl3.s0, yl2.s0, yl1.s0, yl0.s0);
        uchar8 rl1 = (uchar8)(yl7.s1, yl6.s1, yl5.s1, yl4.s1, yl3.s1, yl2.s1, yl1.s1, yl0.s1);
        uchar8 rl2 = (uchar8)(yl7.s2, yl6.s2, yl5.s2, yl4.s2, yl3.s2, yl2.s2, yl1.s2, yl0.s2);
        uchar8 rl3 = (uchar8)(yl7.s3, yl6.s3, yl5.s3, yl4.s3, yl3.s3, yl2.s3, yl1.s3, yl0.s3);
        uchar8 rl4 = (uchar8)(yl7.s4, yl6.s4, yl5.s4, yl4.s4, yl3.s4, yl2.s4, yl1.s4, yl0.s4);
        uchar8 rl5 = (uchar8)(yl7.s5, yl6.s5, yl5.s5, yl4.s5, yl3.s5, yl2.s5, yl1.s5, yl0.s5);
        uchar8 rl6 = (uchar8)(yl7.s6, yl6.s6, yl5.s6, yl4.s6, yl3.s6, yl2.s6, yl1.s6, yl0.s6);
        uchar8 rl7 = (uchar8)(yl7.s7, yl6.s7, yl5.s7, yl4.s7, yl3.s7, yl2.s7, yl1.s7, yl0.s7);
        // rotated u-block index
        // rotated u block height equal to u-block-width, rotated y width equal to y-height.
        int2 ru_pos = (int2)(max_u_y_index - uy, ux);
        int rlu_start = (ru_pos.y * u_block_width * 2) * y_height + (ru_pos.x * u_block_height * 2);
        ulong8 r_offsets = indexs8 * y_height;
        r_offsets += (rlu_start + (ulong)out_buff_yuv);
        // write rotated y image data
        vstore8(rl0, 0, (unsigned char*)(r_offsets.s0));
        vstore8(rl1, 0, (unsigned char*)(r_offsets.s1));
        vstore8(rl2, 0, (unsigned char*)(r_offsets.s2));
        vstore8(rl3, 0, (unsigned char*)(r_offsets.s3));
        vstore8(rl4, 0, (unsigned char*)(r_offsets.s4));
        vstore8(rl5, 0, (unsigned char*)(r_offsets.s5));
        vstore8(rl6, 0, (unsigned char*)(r_offsets.s6));
        vstore8(rl7, 0, (unsigned char*)(r_offsets.s7));

        // u - v data
        // load u data
        const unsigned char* in_u_buff = in_buff_yuv + y_bytes_size;
        int u_start = (uy * u_block_height) * u_width + (ux * u_block_width);
        // ulong4 u_indexs4 = (ulong4)(0, 1, 2, 3);
        ulong4 u_offsets = u_indexs4 * u_width;
        u_offsets += (u_start + (ulong)(in_u_buff));
        // load u block 4 lines
        uchar4 ul0 = vload4(0, (const unsigned char*)(u_offsets.s0));
        uchar4 ul1 = vload4(0, (const unsigned char*)(u_offsets.s1));
        uchar4 ul2 = vload4(0, (const unsigned char*)(u_offsets.s2));
        uchar4 ul3 = vload4(0, (const unsigned char*)(u_offsets.s3));

        // load v block 4 lines
        ulong4 v_offsets = u_offsets;
        v_offsets += u_bytes_size;
        // const unsigned char* in_v_buff = in_u_buff + u_bytes_size;

        uchar4 vl0 = vload4(0, (const unsigned char*)(v_offsets.s0));
        uchar4 vl1 = vload4(0, (const unsigned char*)(v_offsets.s1));
        uchar4 vl2 = vload4(0, (const unsigned char*)(v_offsets.s2));
        uchar4 vl3 = vload4(0, (const unsigned char*)(v_offsets.s3));

        // uv lines
        uchar8 uvl0 = (uchar8)(ul3.s0, vl3.s0, ul2.s0, vl2.s0, ul1.s0, vl1.s0, ul0.s0, vl0.s0);
        uchar8 uvl1 = (uchar8)(ul3.s1, vl3.s1, ul2.s1, vl2.s1, ul1.s1, vl1.s1, ul0.s1, vl0.s1);
        uchar8 uvl2 = (uchar8)(ul3.s2, vl3.s2, ul2.s2, vl2.s2, ul1.s2, vl1.s2, ul0.s2, vl0.s2);
        uchar8 uvl3 = (uchar8)(ul3.s3, vl3.s3, ul2.s3, vl2.s3, ul1.s3, vl1.s3, ul0.s3, vl0.s3);

        // rotated image r-width equal to y_height
        int uv_start = y_bytes_size + (ru_pos.y * u_block_height) * y_height + ru_pos.x * u_block_width * 2;  // r-width = y_height
        ulong4 uv_offsets = u_indexs4 * y_height;                                                             // r-width = y_height
        uv_offsets += (uv_start + (ulong)(out_buff_yuv));
        // write rotated uv data
        vstore8(uvl0, 0, (unsigned char*)(uv_offsets.s0));
        vstore8(uvl1, 0, (unsigned char*)(uv_offsets.s1));
        vstore8(uvl2, 0, (unsigned char*)(uv_offsets.s2));
        vstore8(uvl3, 0, (unsigned char*)(uv_offsets.s3));
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
    std::string platform_name = platforms[0].getInfo<CL_PLATFORM_NAME>();

    // get devices of GPU
    GetDevices(platforms, CL_DEVICE_TYPE_GPU, &m_devices);
    // GetDevices(platforms, CL_DEVICE_TYPE_DEFAULT, &m_devices);
    if (m_devices.size() == 0) { LOGI("can not get any GPU devices."); return false; }
    cl::Device& target_device = m_devices[0];
    std::string use_device_name = target_device.getInfo<CL_DEVICE_NAME>();

    // get compute units
    int num_compute_units = 0;
    err = target_device.getInfo(CL_DEVICE_MAX_COMPUTE_UNITS, &num_compute_units);
    int addr_bits = target_device.getInfo<CL_DEVICE_ADDRESS_BITS >();
    LOGI("platform name: %s, device name: %s, compute unites: %d, address bits: %d [32bits or 64 bits]",
         platform_name.c_str(), use_device_name.c_str(), num_compute_units, addr_bits);

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
    std::string cl_code = yuvi420_to_nv12_rotate_opencl_code();
    sources.push_back({cl_code.c_str(), cl_code.length()});

    m_program = new cl::Program(*m_context, sources);
    if ((err = m_program->build({target_device}, "-cl-std=CL2.0 -O2")) != CL_SUCCESS) {
        std::string build_error = "building error: " + m_program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(target_device);
        LOGI("build opencl code error, %s\n", build_error.c_str());
        return false;
    }
    // bind kernel function
    m_kernel_yuvi420_to_nv12 = new cl::Kernel(*m_program, "kYuvI420ToNV12Rotate", &err);
    if (err != CL_SUCCESS) { return false; }

    // https://man.opencl.org/clGetDeviceInfo.html
    // max work-items in local group, Max allowed work-items in a group.
    // Maximum number of work-items in a work-group that a device is capable of executing on a single compute unit,
    // for any given kernel-instance running on the device.
    // int max_local_group_work_items = target_device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
    int32_t dev_max_group_work_items = target_device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
    std::vector<size_t> dims_max_group_work_items = target_device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
    LOGI("max work-items in local device group: %d", dev_max_group_work_items);
    size_t kernel_max_group_work_items = m_kernel_yuvi420_to_nv12->getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(target_device);
    std::vector<size_t> temp_sizes = dims_max_group_work_items;
    temp_sizes.push_back(dev_max_group_work_items);
    temp_sizes.push_back(kernel_max_group_work_items);
    m_max_work_items_in_group = INT32_MAX;
    for (size_t num: temp_sizes) { m_max_work_items_in_group = num < m_max_work_items_in_group ? num : m_max_work_items_in_group; }
    LOGI("maximum work-items in a group: %d", m_max_work_items_in_group);

    // create input / output buffer
    const int u_width = m_width / 2;
    const int u_height = m_height / 2;
    m_yuv_buff_size = m_width * m_height * 1.5;
    m_input_buff_yuv = new cl::Buffer(*m_context, CL_MEM_READ_ONLY, m_yuv_buff_size * sizeof(unsigned char), nullptr, &err);
    m_output_buff_yuv = new cl::Buffer(*m_context, CL_MEM_WRITE_ONLY, m_yuv_buff_size * sizeof(unsigned char), nullptr, &err);

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

bool YuvI420ToNV12Rotate::ConvertToNV12RotateImpl(int width, int height, unsigned char* yuv_i420_img_data) {
    // debug only, remove comments for debug
    // return ConvertToNV12RotateImpl_HostDebug(width, height, yuv_i420_img_data);

    cl_int err = CL_SUCCESS;
    unsigned char* input_yuv = yuv_i420_img_data;

    TestCostTime cost_time;
    // upload input data to target device
    cl::Event upload_mem_ev;
    err = m_queue->enqueueWriteBuffer(*m_input_buff_yuv, CL_FALSE, 0, m_yuv_buff_size,input_yuv, nullptr, &upload_mem_ev);
    cost_time.ShowCostTime("Upload memory to device");

    // running kernel function
    // https://downloads.ti.com/mctools/esd/docs/opencl/execution/kernels-workgroups-workitems.html
    // global size: work-items, 需要计算的work-item总数目。
    // https://man.opencl.org/clEnqueueNDRangeKernel.html
    // If local_work_size is NULL and non-uniform-work-groups are enabled, the OpenCL runtime is free to implement the ND-range
    // using uniform or non-uniform work-group sizes, regardless of the divisibility of the global work size.
    // x - work-items in local group
    // y - work-items in local group

    cost_time.Reset();
    const int u_width = width / 2;
    const int u_height = height / 2;
    int u_x_blocks = u_width / kEachUBlockWidthPixels;
    int u_y_blocks = u_height / kEachUBlockHeightPixels;
    int localx = 8;
    if(u_x_blocks / 8 > 4)
        localx = 16;
    else if(u_x_blocks < 8)
        localx = u_x_blocks;

    int localy = 8;
    if(u_y_blocks / 8 > 4)
        localy = 16;
    else if (u_y_blocks < 8)
        localy = u_y_blocks;
    cost_time.ShowCostTime("Calculate local groups");

    // https://man.opencl.org/clEnqueueNDRangeKernel.html
    // The total number of work-items in a work-group is computed as local_work_size[0] *...* local_work_size[work_dim - 1].
    // The total number of work-items in the work-group must be less than or equal to the CL_KERNEL_WORK_GROUP_SIZE value specified in table of
    // OpenCL Device Queries for clGetDeviceInfo and the number of work-items specified in local_work_size[0],...local_work_size[work_dim - 1] must be
    // less than or equal to the corresponding values specified by CL_DEVICE_MAX_WORK_ITEM_SIZES[0],... CL_DEVICE_MAX_WORK_ITEM_SIZES[work_dim - 1].
    //
    // default, 1920 * 1080, 14- 20ms
    // localx = 1; localy = 64; 1920*1080 <= 20ms
    // localx * localy <= m_max_work_items_in_group
    // localx = 2; localy = 64; 11 -20ms

    cost_time.Reset();
    cl::Event enqueue_ndrange_ev;
    std::vector<cl::Event> wait_events;
    wait_events.push_back(upload_mem_ev);
    err = m_queue->enqueueNDRangeKernel(*m_kernel_yuvi420_to_nv12, cl::NullRange, cl::NDRange(u_x_blocks, u_y_blocks),
                     cl::NDRange(localx, localy), &wait_events, &enqueue_ndrange_ev);
    if (err != CL_SUCCESS) { return false; }
    enqueue_ndrange_ev.wait();
    cost_time.ShowCostTime("Finished compute on target device");

    // read result, read the result put back to host memory
    // overwrite input_yuv
    // std::vector<cl::Event> wait_finish_events;
    // wait_finish_events.push_back(enqueue_ndrange_ev);
    cost_time.Reset();
    err = m_queue->enqueueReadBuffer(*m_output_buff_yuv, CL_TRUE, 0, m_yuv_buff_size, input_yuv, nullptr, NULL);
    cost_time.ShowCostTime("Downloaded / Copied data from target device");

    return err == CL_SUCCESS;
}
