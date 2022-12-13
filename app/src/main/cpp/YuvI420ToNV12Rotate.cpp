//
// Created by Rubus on 12/3/2022.
//

#include <stdint.h>

#include <string>
#include <vector>

#include "Utility.h"
#include "YuvI420ToNV12Rotate.h"

#include "libyuv.h"

#include <arm_neon.h>

// ---------------test neon---------------------------
#include "arm_compute/runtime/NEON/NEFunctions.h"

#include "arm_compute/core/Types.h"
#include "utils/ImageLoader.h"
#include "utils/Utils.h"

// --------------------------------------------------------------------
typedef unsigned char byte;

void rgb888_2_nv12_intrinsic(byte * nv12, byte * rgb, int width, int height) {
#ifndef WIN32
    const uint8x8_t u8_zero = vdup_n_u8(0);
    const uint8x8_t u8_16 = vdup_n_u8(16);
    const uint16x8_t u16_rounding = vdupq_n_u16(128);

    const int16x8_t s16_zero = vdupq_n_s16(0);
    const int8x8_t s8_rounding = vdup_n_s8(-128);
    const int16x8_t s16_rounding = vdupq_n_s16(128);

    byte* UVPtr = nv12 + width * height;
    int pitch = width >> 4;

    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < pitch; ++i)
        {
            // Load rgb 16 pixel
            // uint8x16x3_t pixel_rgb = vld3q_u8(rgb);
            uint8x16x4_t pixel_rgb = vld4q_u8(rgb);

            uint8x8_t high_r = vget_high_u8(pixel_rgb.val[0]);
            uint8x8_t low_r = vget_low_u8(pixel_rgb.val[0]);
            uint8x8_t high_g = vget_high_u8(pixel_rgb.val[1]);
            uint8x8_t low_g = vget_low_u8(pixel_rgb.val[1]);
            uint8x8_t high_b = vget_high_u8(pixel_rgb.val[2]);
            uint8x8_t low_b = vget_low_u8(pixel_rgb.val[2]);

            // NOTE:
            // declaration may not appear after executable statement in block
            uint16x8_t high_y;
            uint16x8_t low_y;

            // 1. Multiply transform matrix (Y′: unsigned, U/V: signed)
            // 2. Scale down (">>8") to 8-bit values with rounding ("+128") (Y′: unsigned, U/V: signed)
            // 3. Add an offset to the values to eliminate any negative values (all results are 8-bit unsigned)
            uint8x8_t scalar = vdup_n_u8(66);
            high_y = vmull_u8(high_r, scalar);
            low_y = vmull_u8(low_r, scalar);

            scalar = vdup_n_u8(129);
            high_y = vmlal_u8(high_y, high_g, scalar);
            low_y = vmlal_u8(low_y, low_g, scalar);

            scalar = vdup_n_u8(25);
            high_y = vmlal_u8(high_y, high_b, scalar);
            low_y = vmlal_u8(low_y, low_b, scalar);

            high_y = vaddq_u16(high_y, u16_rounding);
            low_y = vaddq_u16(low_y, u16_rounding);

            uint8x8_t u8_low_y = vshrn_n_u16(low_y, 8);
            uint8x8_t u8_high_y = vshrn_n_u16(high_y, 8);

            low_y = vaddl_u8(u8_low_y, u8_16);
            high_y = vaddl_u8(u8_high_y, u8_16);

            uint8x16_t pixel_y = vcombine_u8(vqmovn_u16(low_y), vqmovn_u16(high_y));

            // Store
            vst1q_u8(nv12, pixel_y);

            if (j % 2 == 0)
            {
                uint8x8x2_t mix_r = vuzp_u8(low_r, high_r);
                uint8x8x2_t mix_g = vuzp_u8(low_g, high_g);
                uint8x8x2_t mix_b = vuzp_u8(low_b, high_b);

                int16x8_t signed_r = vreinterpretq_s16_u16(vaddl_u8(mix_r.val[0], u8_zero));
                int16x8_t signed_g = vreinterpretq_s16_u16(vaddl_u8(mix_g.val[0], u8_zero));
                int16x8_t signed_b = vreinterpretq_s16_u16(vaddl_u8(mix_b.val[0], u8_zero));

                int16x8_t signed_u;
                int16x8_t signed_v;

                int16x8_t signed_scalar = vdupq_n_s16(-38);
                signed_u = vmulq_s16(signed_r, signed_scalar);

                signed_scalar = vdupq_n_s16(112);
                signed_v = vmulq_s16(signed_r, signed_scalar);

                signed_scalar = vdupq_n_s16(-74);
                signed_u = vmlaq_s16(signed_u, signed_g, signed_scalar);

                signed_scalar = vdupq_n_s16(-94);
                signed_v = vmlaq_s16(signed_v, signed_g, signed_scalar);

                signed_scalar = vdupq_n_s16(112);
                signed_u = vmlaq_s16(signed_u, signed_b, signed_scalar);

                signed_scalar = vdupq_n_s16(-18);
                signed_v = vmlaq_s16(signed_v, signed_b, signed_scalar);

                signed_u = vaddq_s16(signed_u, s16_rounding);
                signed_v = vaddq_s16(signed_v, s16_rounding);

                int8x8_t s8_u = vshrn_n_s16(signed_u, 8);
                int8x8_t s8_v = vshrn_n_s16(signed_v, 8);

                signed_u = vsubl_s8(s8_u, s8_rounding);
                signed_v = vsubl_s8(s8_v, s8_rounding);

                signed_u = vmaxq_s16(signed_u, s16_zero);
                signed_v = vmaxq_s16(signed_v, s16_zero);

                uint16x8_t unsigned_u = vreinterpretq_u16_s16(signed_u);
                uint16x8_t unsigned_v = vreinterpretq_u16_s16(signed_v);

                uint8x8x2_t result;
                result.val[0] = vqmovn_u16(unsigned_u);
                result.val[1] = vqmovn_u16(unsigned_v);

                vst2_u8(UVPtr, result);
                UVPtr += 16;
            }

            rgb += 3 * 16;
            nv12 += 16;
        }
    }
#endif
}

using namespace libyuv;

int y_uv_2planesRotate(const uint8_t* src_y, int src_stride_y,
               const uint8_t* src_uv, int src_stride_uv,
               uint8_t* dst_y, int dst_stride_y,
               uint8_t* dst_uv, int dst_stride_uv,
               int width,
               int height,
               enum RotationMode mode) {
    int u_width = (width + 1) >> 1;
    // int uv_height = (height + 1) >> 1;
    int uv_height = height;
    if ((!src_y && dst_y) || !src_uv || width <= 0 || height == 0 ||
        !dst_y || !dst_uv) {
        return -1;
    }

    // Negative height means invert the image.
    if (height < 0) {
        height = -height;
        uv_height = (height + 1) >> 1;
        src_y = src_y + (height - 1) * src_stride_y;
        src_uv = src_uv + (uv_height - 1) * src_stride_uv;
        src_stride_y = -src_stride_y;
        src_stride_uv = -src_stride_uv;
    }

    switch (mode) {
        case kRotate90:
            RotatePlane90(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
            RotatePlane90(src_uv, src_stride_uv, dst_uv, dst_stride_uv, u_width, uv_height);
            return 0;
        case kRotate270:
            RotatePlane270(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
            RotatePlane270(src_uv, src_stride_uv, dst_uv, dst_stride_uv, u_width, uv_height);
            return 0;
        case kRotate180:
            RotatePlane180(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
            RotatePlane180(src_uv, src_stride_uv, dst_uv, dst_stride_uv, u_width, uv_height);
            return 0;
        default:
            break;
    }
    return -1;
}

// --------------------------------------------------------------------

// #include "arm_compute/core/NEON/kernels/detail/NEColorConvertHelper.inl"
// https://arm-software.github.io/ComputeLibrary/v19.05/
void hineonn() {
    arm_compute::Image aa;

    TestCostTime cost_time;

    int SOURCE_WIDTH = 1088;
    int SOURCE_HEIGHT = 1920;
    unsigned char* argb_data = new unsigned char[SOURCE_WIDTH * SOURCE_HEIGHT * 4];
    unsigned char* i420 = new unsigned char[SOURCE_WIDTH * SOURCE_HEIGHT * 1.5];
    unsigned char* i420_270 = new unsigned char[SOURCE_WIDTH * SOURCE_HEIGHT * 1.5];
    unsigned char* nv12 = new unsigned char[SOURCE_WIDTH * SOURCE_HEIGHT * 1.5];

    // argb-to-nv12
    cost_time.Reset();
    rgb888_2_nv12_intrinsic(nv12, argb_data, SOURCE_WIDTH, SOURCE_HEIGHT);
    cost_time.ShowCostTime("rgb888_2_nv12_intrinsic cost");

    TestCostTime cost_all;

    cost_time.Reset();
    int dst_y_size = SOURCE_WIDTH * SOURCE_HEIGHT;
    int dst_u_size = (SOURCE_WIDTH >> 1) * (SOURCE_HEIGHT >> 1);
    unsigned char* dst_i420_y_data = i420;
    unsigned char* dst_i420_u_data = i420 + dst_y_size;
    unsigned char* dst_i420_v_data = i420 + dst_y_size + dst_u_size;

    int ret = libyuv::ABGRToI420((const uint8_t *) argb_data, SOURCE_WIDTH * 4,
                          (uint8_t *) dst_i420_y_data, SOURCE_WIDTH,
                          (uint8_t *) dst_i420_u_data, SOURCE_WIDTH / 2,
                          (uint8_t *) dst_i420_v_data, SOURCE_WIDTH / 2,
                          SOURCE_WIDTH, SOURCE_HEIGHT);
    cost_time.ShowCostTime("ABGRToI420 cost time");

    cost_time.Reset();
    unsigned char* dst_i420_270_y_data = i420_270;
    unsigned char* dst_i420_270_u_data = i420_270 + dst_y_size;
    unsigned char* dst_i420_270_v_data = i420_270 + dst_y_size + dst_u_size;
    int r_width = SOURCE_HEIGHT;
    int r_height = SOURCE_WIDTH;

    ret = libyuv::I420Rotate((uint8_t *) dst_i420_y_data, SOURCE_WIDTH,
                     (uint8_t *) dst_i420_u_data, SOURCE_WIDTH / 2,
                     (uint8_t *) dst_i420_v_data, SOURCE_WIDTH / 2,
                     reinterpret_cast<uint8_t *>(dst_i420_270_y_data), r_width,
                     reinterpret_cast<uint8_t *>(dst_i420_270_u_data), r_width / 2,
                     reinterpret_cast<uint8_t *>(dst_i420_270_v_data), r_width / 2,
                     SOURCE_WIDTH, SOURCE_HEIGHT, (libyuv::RotationMode)270);
    cost_time.ShowCostTime("I420Rotate cost time");
    if (ret < 0) {
        LOGI("I420Rotate failure");
        return;
    }

    cost_time.Reset();
    unsigned char* nv12_data = nv12;
    ret = libyuv::I420ToNV12((uint8_t *) dst_i420_270_y_data, r_width,
                     (uint8_t *) dst_i420_270_u_data, r_width / 2,
                     (uint8_t * )(dst_i420_270_v_data), r_width / 2,
                     reinterpret_cast<uint8_t *>(nv12_data), r_width,
                     reinterpret_cast<uint8_t *>(nv12_data + r_width * r_height), r_width,
                     r_width, r_height);
    cost_time.ShowCostTime("I420ToNV12 cost time");
    if (ret < 0) {
        LOGI("I420ToNV12 failure");
        return;
    }

    cost_all.ShowCostTime("--- all cost time ---");

    delete []i420;
    delete []argb_data;
    delete []i420_270;
    delete []nv12;
    // arm_compute::colorconvert_rgb_to_nv12();
}


unsigned char* g_i420 = nullptr;
unsigned char* g_i420_uv = nullptr;
void CheckCacheBuff(int w, int h) {
    if (g_i420 == nullptr) {
        g_i420 = new unsigned char[w * h * 1.5];
        g_i420_uv = new unsigned char[w * h * 0.5];
    }
}

// 1. abgr -> nv12
// 2. nv12 -> rotate 270 r_nv12
void hineon_2(uint8_t* src_abgr, int32_t source_width, int32_t source_height,
              uint8_t* dst_r_nv12) {
    // fake data
    //source_width = 16;
    //source_height = 8;

    // int SOURCE_WIDTH = 1088;
    // int SOURCE_HEIGHT = 1920;
    //unsigned char* i420 = new unsigned char[source_width * source_height * 1.5];
    //unsigned char* i420_uv = new unsigned char[source_width * source_height * 0.5];
    CheckCacheBuff(source_width, source_height);

    // argb-to-nv12
    TestCostTime cost_all;
    TestCostTime cost_time;

    cost_time.Reset();
    int32_t u_bytes = source_width * source_height / 4;
    int ret = libyuv::ABGRToI420((const uint8_t *)src_abgr, source_width * 4,
                       g_i420, source_width,
                       g_i420_uv, source_width / 2,
                       g_i420_uv + u_bytes, source_width / 2,
                       source_width,source_height);
    cost_time.ShowCostTime("ABGRToNV12 convert");
    if (ret < 0) {
        LOGI("ABGRToNV12 failure");
        return;
    }

    // --------fake data--------
    /*uint8_t* u = i420_uv;
    uint8_t* v = u + source_width * source_height / 4;
    int l;
    for (int h = 0 ; h < source_height / 2; h++) {
        l = source_width / 2;
        memset(u + h * l, 1, l);
        memset(v + h * l, 2, l);
    }*/
    // --------fake data--------

    // copy v, u data to i420 uv area
    // yyyy
    // yyyy
    // vv
    // uu
    // vv
    // uu
    cost_time.Reset();
    int v_height = source_height / 2;
    int halfwidth = source_width / 2;
    uint8_t* ptr = g_i420 + source_width * source_height;  // uv area
    uint8_t* src_u = g_i420_uv;
    uint8_t* src_v = g_i420_uv + u_bytes;
    for (int h = 0; h < v_height; ++h) {
        memcpy(ptr, src_v, halfwidth);
        ptr += halfwidth;
        src_v += halfwidth;

        memcpy(ptr, src_u, halfwidth);
        ptr += halfwidth;
        src_u += halfwidth;
    }
    cost_time.ShowCostTime("memcpy data");

    // nv12 rotate 270
    cost_time.Reset();
    // ok
    /*uint8_t* src_i420_uv = i420 + source_width * source_height;
    int32_t i420_u_stride = source_width / 2;

    int32_t r_y_stride = source_height;
    uint8_t* r_uv = dst_r_nv12 + source_width * source_height;
    int32_t r_uv_stride = source_height;
    ret = y_uv_2planesRotate(i420, source_width,
                   src_i420_uv, i420_u_stride,
                   dst_r_nv12, r_y_stride,
                   r_uv, r_uv_stride,
                    source_width, source_height,
                    (RotationMode)270);
    cost_time.ShowCostTime("y_uv_2planesRotate");
    if (ret < 0) {
        LOGI("y_uv_2planesRotate failure");
        return;
    }
    */
    int dst_image_width = 1080;
    uint8_t* src_i420_uv = g_i420 + source_width * source_height;
    // int32_t i420_u_stride = source_width / 2;

    int32_t r_y_stride = source_height;
    uint8_t* r_uv = dst_r_nv12 + dst_image_width * source_height;
    int32_t r_uv_stride = source_height;
    ret = y_uv_2planesRotate(g_i420, source_width,
                             src_i420_uv, source_width / 2,
                             dst_r_nv12, r_y_stride,
                             r_uv, r_uv_stride,
                             dst_image_width, source_height,
                             (RotationMode)270);
    cost_time.ShowCostTime("y_uv_2planesRotate");
    if (ret < 0) {
        LOGI("y_uv_2planesRotate failure");
        return;
    }

    cost_all.ShowCostTime("--- all cost time ---");

    //delete []g_i420;
    //delete []g_i420_uv;

    // ------fake data
}
// ---------------test neon---------------------------

void YuvI420ToNV12Rotate::Release() {
    m_devices.clear();
    delete m_context; m_context = nullptr;
    delete m_program; m_program = nullptr;
    delete m_queue; m_queue = nullptr;
    delete m_kernel_yuvi420_to_nv12; m_kernel_yuvi420_to_nv12 = nullptr;
    delete m_input_cl_buff_yuv; m_input_cl_buff_yuv = nullptr;
    delete m_output_cl_buff_yuv; m_output_cl_buff_yuv = nullptr;
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
    if ((err = m_program->build({target_device}, "-cl-std=CL2.0 -O2 -cl-fast-relaxed-math")) != CL_SUCCESS) {
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
    m_input_cl_buff_yuv = new cl::Buffer(*m_context, CL_MEM_READ_ONLY, m_yuv_buff_size * sizeof(unsigned char), nullptr, &err);
    m_output_cl_buff_yuv = new cl::Buffer(*m_context, CL_MEM_WRITE_ONLY, m_yuv_buff_size * sizeof(unsigned char), nullptr, &err);

    // bind kernel function parameters
    const int out_uv_width = u_height * 2;
    err = m_kernel_yuvi420_to_nv12->setArg(0, *m_input_cl_buff_yuv);
    err = m_kernel_yuvi420_to_nv12->setArg(1, *m_output_cl_buff_yuv);
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

bool YuvI420ToNV12Rotate::ConvertToNV12RotateImpl(int width, int height, unsigned char* img_yuv_i420_data, unsigned char* out_nv12_data) {
    // hineonn();
    // debug only, remove comments for debug
    // return ConvertToNV12RotateImpl_HostDebug(width, height, yuv_i420_img_data);

    cl_int err = CL_SUCCESS;
    unsigned char* input_yuv = img_yuv_i420_data;

    TestCostTime cost_time;
    // upload input data to target device
    cl::Event upload_mem_ev;
    err = m_queue->enqueueWriteBuffer(*m_input_cl_buff_yuv, CL_TRUE, 0, m_yuv_buff_size,input_yuv, nullptr, &upload_mem_ev);
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
    err = m_queue->enqueueReadBuffer(*m_output_cl_buff_yuv, CL_TRUE, 0, m_yuv_buff_size, out_nv12_data, nullptr, NULL);
    cost_time.ShowCostTime("Downloaded / Copied data from target device");

    return err == CL_SUCCESS;
}

bool YuvI420ToNV12Rotate::AbgrConvertToNV12RotateImpl(int width, int height, unsigned char* img_abgr_data, unsigned char* out_nv12_data) {
    // hineonn();
    int w = width;
    int h = height;
    hineon_2(img_abgr_data, w, h,
             out_nv12_data);
    return true;
}