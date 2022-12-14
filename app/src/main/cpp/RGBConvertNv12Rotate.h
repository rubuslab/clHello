//
// Created by Rubus on 12/14/2022.
//

#ifndef CLHELLO_RGBCONVERTNV12ROTATE_H
#define CLHELLO_RGBCONVERTNV12ROTATE_H

#include <stdint.h>

#include "libyuv.h"
#include "Singleton.h"

class RGBConvertNv12Rotate {
private:
    // origin image w x h
    int m_width_stride = 0;
    int m_height = 0;

    // destination image w x h
    int m_width = 0;
    // int m_height = 0;

    unsigned char* m_i420_buff = nullptr;
    unsigned char* m_i420_uv_planes_buff = nullptr;

    // width x height
    // 1088 * 1920
    // 720 * 1280
    bool IsSupportedWidth() { return (m_width_stride == 1088 || m_width_stride == 720 || m_width_stride == 1920 || m_width_stride == 1280); }

    // source image:
    // w: width_stride, h: height
    //
    // destination image:
    // w: width, h: height (before rotate)
    bool ConvertImpl(uint8_t* src_abgr, int32_t width, int32_t height, int width_stride, uint8_t* dst_r_nv12);
    int Y_U_V_2PlanesRotate(const uint8_t* src_y, int src_stride_y,
                           const uint8_t* src_uv, int src_stride_uv,
                           uint8_t* dst_y, int dst_stride_y,
                           uint8_t* dst_uv, int dst_stride_uv,
                           int width,
                           int height,
                           enum libyuv::RotationMode mode);

    void ReleaseBuffer();

public:
    RGBConvertNv12Rotate(int width, int height, int width_stride): m_width(width), m_height(height), m_width_stride(width_stride) {}
    ~RGBConvertNv12Rotate() { ReleaseBuffer(); }

    bool Init();
    bool IsSameSize(int w, int h) { return (m_width == w && m_height == h); }

    // source image:
    // w: width_stride, h: height
    //
    // destination image:
    // w: width, h: height (before rotate)
    bool AbgrConvertToNV12RotateImpl(int width, int height, int width_stride, unsigned char* img_abgr_data, unsigned char* out_nv12_data);
};


class RGBConvertNv12RotateHelper:public Singleton<RGBConvertNv12RotateHelper> {
private:
    RGBConvertNv12Rotate* m_rgb_to_nv12_obj = nullptr;

    void InitRgbToNV12(int w, int h, int w_stride) {
        if (m_rgb_to_nv12_obj != nullptr && !m_rgb_to_nv12_obj->IsSameSize(w, h)) { ReleaseObject(); }
        if (m_rgb_to_nv12_obj == nullptr) {
            m_rgb_to_nv12_obj = new RGBConvertNv12Rotate(w, h, w_stride);
            if (!m_rgb_to_nv12_obj->Init()) { ReleaseObject(); }
        }
    }
    void ReleaseObject() { delete m_rgb_to_nv12_obj; m_rgb_to_nv12_obj = nullptr; }

public:
    // source image:
    // w: width_stride, h: height
    //
    // destination image:
    // w: width, h: height (before rotate)
    bool RgbConvertToNV12RotateImpl(int width, int height, int width_stride, unsigned char* img_abgr_data, unsigned char* out_nv12_data) {
        // width and height must equal to 8x
        // check image width and height is 8x at invoker side.
        // if (width % 8 != 0 || height % 8 != 0) return false;

        InitRgbToNV12(width, height, width_stride);
        bool ok = false;
        if (m_rgb_to_nv12_obj) { ok = m_rgb_to_nv12_obj->AbgrConvertToNV12RotateImpl(width, height, width_stride, img_abgr_data, out_nv12_data); }
        if (!ok) { ReleaseObject(); }
        return ok;
    }
};

#endif //CLHELLO_RGBCONVERTNV12ROTATE_H
