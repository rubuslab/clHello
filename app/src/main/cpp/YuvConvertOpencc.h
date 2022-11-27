#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "Singleton.h"
#include "CL/cl.hpp"

class YuvI420ToNV12Opencc {
private:
    int m_Width = 0;
    int m_Height = 0;

    std::vector<cl::Device> m_Devices;
    cl::Context* m_Context = nullptr;
    cl::Program* m_Program = nullptr;
    // cl::Buffer* m_inputBuffer = nullptr;
    cl::CommandQueue* m_Queue = nullptr;
    cl::Kernel* m_Kernel = nullptr;

    void Release();
    void Log(std::string message);
public:
    YuvI420ToNV12Opencc(int width, int height): m_Width(width), m_Height(height) {}
    ~YuvI420ToNV12Opencc() { Release(); }
    bool IsSameSize(int w, int h) { return (m_Width == w && m_Height == h); }

    bool Init();
    bool ConvertToNV12Impl(int width, int height, unsigned char* img_data);
};

class YuvConvertHelper:public Singleton<YuvConvertHelper> {
private:
    YuvI420ToNV12Opencc* m_YuvI420ToNV21 = nullptr;

    void InitI420ToNV32(int w, int h) {
        if (m_YuvI420ToNV21 != nullptr && !m_YuvI420ToNV21->IsSameSize(w, h)) { ReleaseYuvI420ToNV12Obj(); }
        if (m_YuvI420ToNV21 == nullptr) {
            m_YuvI420ToNV21 = new YuvI420ToNV12Opencc(w, h);
            if (!m_YuvI420ToNV21->Init()) { ReleaseYuvI420ToNV12Obj(); }
        }
    }
    void ReleaseYuvI420ToNV12Obj() { delete m_YuvI420ToNV21; m_YuvI420ToNV21 = nullptr; }

public:
    bool YuvI420ConvertToNV12(int width, int height, unsigned char* img_data) {
        InitI420ToNV32(width, height);
        bool ok = false;
        if (m_YuvI420ToNV21) { ok = m_YuvI420ToNV21->ConvertToNV12Impl(width, height, img_data); }
        if (!ok) { ReleaseYuvI420ToNV12Obj(); }
        return ok;
    }
};