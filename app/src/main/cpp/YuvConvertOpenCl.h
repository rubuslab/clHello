#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "Singleton.h"
#include "CL/cl.hpp"

class YuvI420ToNV12OpenCc {
private:
    int m_width = 0;
    int m_height = 0;
    int m_max_device_workgroups = 0;
    int m_workgroups = 1;
    int m_u_lines_per_group = 1;

    std::vector<cl::Device> m_devices;
    cl::Context* m_context = nullptr;
    cl::Program* m_program = nullptr;
    cl::CommandQueue* m_queue = nullptr;
    cl::Kernel* m_kernel_yuvi420_to_nv12 = nullptr;
    cl::Buffer* m_input_buff_uv = nullptr;
    cl::Buffer* m_output_buff_uv = nullptr;

    void Release();
    void Log(std::string message);
public:
    YuvI420ToNV12OpenCc(int width, int height): m_width(width), m_height(height) {}
    ~YuvI420ToNV12OpenCc() { Release(); }
    bool IsSameSize(int w, int h) { return (m_width == w && m_height == h); }

    bool Init();
    bool ConvertToNV12Impl(int width, int height, unsigned char* img_yuv_data);
};

class YuvConvertHelper:public Singleton<YuvConvertHelper> {
private:
    YuvI420ToNV12OpenCc* m_yuvi420_to_nv12_obj = nullptr;

    void InitI420ToNV32(int w, int h) {
        if (m_yuvi420_to_nv12_obj != nullptr && !m_yuvi420_to_nv12_obj->IsSameSize(w, h)) { ReleaseYuvI420ToNV12Obj(); }
        if (m_yuvi420_to_nv12_obj == nullptr) {
            m_yuvi420_to_nv12_obj = new YuvI420ToNV12OpenCc(w, h);
            if (!m_yuvi420_to_nv12_obj->Init()) { ReleaseYuvI420ToNV12Obj(); }
        }
    }
    void ReleaseYuvI420ToNV12Obj() { delete m_yuvi420_to_nv12_obj; m_yuvi420_to_nv12_obj = nullptr; }

public:
    bool YuvI420ConvertToNV12(int width, int height, unsigned char* img_yuv_data) {
        InitI420ToNV32(width, height);
        bool ok = false;
        if (m_yuvi420_to_nv12_obj) { ok = m_yuvi420_to_nv12_obj->ConvertToNV12Impl(width, height, img_yuv_data); }
        if (!ok) { ReleaseYuvI420ToNV12Obj(); }
        return ok;
    }
};