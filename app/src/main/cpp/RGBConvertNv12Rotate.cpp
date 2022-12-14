//
// Created by Rubus on 12/14/2022.
//

#include <stdint.h>

#include "libyuv.h"

#include "Utility.h"

#include "RGBConvertNv12Rotate.h"


void RGBConvertNv12Rotate::ReleaseBuffer() {
    delete []m_i420_buff; m_i420_buff = nullptr;
    delete []m_i420_uv_planes_buff; m_i420_uv_planes_buff = nullptr;
}

bool RGBConvertNv12Rotate::Init() {
    if (!IsSupportedWidth()) return false;
    m_i420_buff = new unsigned char[m_width * m_height * 1.5];
    m_i420_uv_planes_buff = new unsigned char[m_width * m_height * 0.5];
    return true;
}

// return value
//  0: success
// -1: failed
int RGBConvertNv12Rotate::Y_U_V_2PlanesRotate(const uint8_t* src_y, int src_stride_y,
                       const uint8_t* src_v_u, int src_stride_v,
                       uint8_t* dst_y, int dst_stride_y,
                       uint8_t* dst_u_v, int dst_stride_u_v,
                       int width,
                       int height,
                       enum libyuv::RotationMode mode) {
    int u_width = (width + 1) >> 1;
    int uv_height = height;
    if ((!src_y && dst_y) || !src_v_u || width <= 0 || height == 0 ||
        !dst_y || !dst_u_v) {
        return -1;
    }

    // Negative height means invert the image.
    if (height < 0) {
        height = -height;
        uv_height = (height + 1) >> 1;
        src_y = src_y + (height - 1) * src_stride_y;
        src_v_u = src_v_u + (uv_height - 1) * src_stride_v;
        src_stride_y = -src_stride_y;
        src_stride_v = -src_stride_v;
    }

    switch (mode) {
        case libyuv::kRotate90:
            libyuv::RotatePlane90(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
            libyuv::RotatePlane90(src_v_u, src_stride_v, dst_u_v, dst_stride_u_v, u_width, uv_height);
            return 0;
        case libyuv::kRotate270:
            libyuv::RotatePlane270(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
            libyuv::RotatePlane270(src_v_u, src_stride_v, dst_u_v, dst_stride_u_v, u_width, uv_height);
            return 0;
        case libyuv::kRotate180:
            libyuv::RotatePlane180(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
            libyuv::RotatePlane180(src_v_u, src_stride_v, dst_u_v, dst_stride_u_v, u_width, uv_height);
            return 0;
        default:
            break;
    }
    return -1;
}

// 1. abgr -> i420
// 2. i420 -> rotate 270 r_nv12
bool RGBConvertNv12Rotate::ConvertImpl(uint8_t* src_abgr, int32_t width, int32_t height, int width_stride, uint8_t* dst_r_nv12) {
    int source_width = width_stride;
    int source_height = height;

    // argb-to-i420
    TestCostTime cost_all;
    TestCostTime cost_time;

    // convert rgb data to i420
    // store y in m_i420_buff
    // store u and v blocks in m_i420_uv_planes_buff
    cost_time.Reset();
    int32_t u_bytes = source_width * source_height / 4;
    int32_t src_half_w = source_width >> 1;
    int ret = libyuv::ABGRToI420((const uint8_t *)src_abgr, source_width * 4,
                                 m_i420_buff, source_width,
                                 m_i420_uv_planes_buff, src_half_w,
                                 m_i420_uv_planes_buff + u_bytes, src_half_w,
                                 source_width,source_height);
    cost_time.ShowCostTime("ABGRToNV12 convert");
    if (ret < 0) {
        LOGI("ABGRToNV12 failure");
        return false;
    }

    // copy v, u data to i420 uv area
    // yyyyyy
    // yyyyyy
    // yyyyyy
    // yyyyyy
    // yyyyyy
    // yyyyyy
    // vvv
    // uuu
    // vvv
    // uuu
    // vvv
    // uuu
    cost_time.Reset();
    int v_height = source_height / 2;
    int32_t y_bytes = source_width * source_height;
    uint8_t* ptr = m_i420_buff + y_bytes;  // u_v area
    uint8_t* src_u = m_i420_uv_planes_buff;
    uint8_t* src_v = m_i420_uv_planes_buff + u_bytes;
    for (int h = 0; h < v_height; ++h) {
        // copy v line
        memcpy(ptr, src_v, src_half_w);
        ptr += src_half_w;
        src_v += src_half_w;

        // copy u line
        memcpy(ptr, src_u, src_half_w);
        ptr += src_half_w;
        src_u += src_half_w;
    }
    cost_time.ShowCostTime("memcpy data");

    // i420 rotate 270
    cost_time.Reset();
    // y block + v,u lines interleaved block(v line, u line, vl, ul ...)
    uint8_t* src_i420_v_u = m_i420_buff + y_bytes;

    int32_t r_y_stride = source_height;
    int dst_image_width = width;
    uint8_t* r_uv = dst_r_nv12 + dst_image_width * source_height;
    int32_t r_uv_stride = source_height;
    ret = Y_U_V_2PlanesRotate(m_i420_buff, source_width,
                             src_i420_v_u, src_half_w,
                             dst_r_nv12, r_y_stride,
                             r_uv, r_uv_stride,
                             dst_image_width, source_height,
                             (libyuv::RotationMode)270);
    cost_time.ShowCostTime("y_uv_2planesRotate");
    if (ret < 0) {
        LOGI("y_uv_2planesRotate failure");
        return false;
    }

    cost_all.ShowCostTime("--- all cost time ---");
    return true;
}

// source image:
// w: width_stride, h: height
//
// destination image:
// w: width, h: height (before rotate)
bool RGBConvertNv12Rotate::AbgrConvertToNV12RotateImpl(int width, int height, int width_stride,
                                                       unsigned char* img_abgr_data, unsigned char* out_nv12_data) {
    return ConvertImpl(img_abgr_data, width, height, width_stride, out_nv12_data);
}
