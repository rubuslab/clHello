//
// Created by Rubus on 12/3/2022.
//

#ifndef CCSHAREHELLO_YUVI420TONV12ROTATE_H
#define CCSHAREHELLO_YUVI420TONV12ROTATE_H


#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "Singleton.h"
#include "CL/cl.hpp"

#ifndef R_CODE
#define R_CODE(...) std::string(" "#__VA_ARGS__" ") // evil stringification macro, similar syntax to raw string R"(...)"
#endif

class YuvI420ToNV12Rotate {
private:
    const int kDefaultLocalGroupSize = 64;  // default 64

    int m_width = 0;
    int m_height = 0;
    int m_local_group_size = kDefaultLocalGroupSize;
    int m_global_work_items = 0;

    std::vector<cl::Device> m_devices;
    cl::Context* m_context = nullptr;
    cl::Program* m_program = nullptr;
    cl::CommandQueue* m_queue = nullptr;
    cl::Kernel* m_kernel_yuvi420_to_nv12 = nullptr;
    cl::Buffer* m_input_buff_yuv = nullptr;
    cl::Buffer* m_output_buff_yuv = nullptr;

    void Release();
    void Log(std::string message);
public:
    YuvI420ToNV12Rotate(int width, int height): m_width(width), m_height(height) {}
    ~YuvI420ToNV12Rotate() { Release(); }
    bool IsSameSize(int w, int h) { return (m_width == w && m_height == h); }

    bool Init();
    bool ConvertToNV12RotateImpl(int width, int height, unsigned char* img_yuv_data);

    bool ConvertToNV12RotateImpl_HostDebug(int width, int height, unsigned char* yuv_i420_img_data);
};

class YuvConvertRotateHelper:public Singleton<YuvConvertRotateHelper> {
private:
    YuvI420ToNV12Rotate* m_yuvi420_to_nv12_obj = nullptr;

    void InitI420ToNV32(int w, int h) {
        if (m_yuvi420_to_nv12_obj != nullptr && !m_yuvi420_to_nv12_obj->IsSameSize(w, h)) { ReleaseYuvI420ToNV12Obj(); }
        if (m_yuvi420_to_nv12_obj == nullptr) {
            m_yuvi420_to_nv12_obj = new YuvI420ToNV12Rotate(w, h);
            if (!m_yuvi420_to_nv12_obj->Init()) { ReleaseYuvI420ToNV12Obj(); }
        }
    }
    void ReleaseYuvI420ToNV12Obj() { delete m_yuvi420_to_nv12_obj; m_yuvi420_to_nv12_obj = nullptr; }

public:
    bool YuvI420ConvertToNV12Rotate(int width, int height, unsigned char* img_yuv_data) {
        InitI420ToNV32(width, height);
        bool ok = false;
        if (m_yuvi420_to_nv12_obj) { ok = m_yuvi420_to_nv12_obj->ConvertToNV12RotateImpl(width, height, img_yuv_data); }
        if (!ok) { ReleaseYuvI420ToNV12Obj(); }
        return ok;
    }
};

#endif //CCSHAREHELLO_YUVI420TONV12ROTATE_H
