//
// Created by Rubus on 12/6/2022.
//

#include "Utility.h"
#include "CL/cl.hpp"

template<typename T>
class N2Vec {
public:
    N2Vec(T v0, T v1): x(v0), y(v1) {}
    N2Vec(const N2Vec<T>& f): x(f.x), y(f.y) {}
    T x;
    T y;
};

template<typename T>
class N4Vec {
public:
    N4Vec(T v0, T v1, T v2, T v3): s0(v0), s1(v1), s2(v2), s3(v3) {}
    N4Vec(const N4Vec<T>& f): s0(f.s0), s1(f.s1), s2(f.s2), s3(f.s3) {}
    T s0;
    T s1;
    T s2;
    T s3;
};

template<typename T>
class N8Vec {
public:
    N8Vec() {}
    N8Vec(T v0, T v1, T v2, T v3, T v4, T v5, T v6, T v7): s0(v0), s1(v1), s2(v2), s3(v3), s4(v4), s5(v5), s6(v6), s7(v7) {}
    N8Vec(const N8Vec<T>& f): s0(f.s0), s1(f.s1), s2(f.s2), s3(f.s3), s4(f.s4), s5(f.s5), s6(f.s6), s7(f.s7) {}
    T s0;
    T s1;
    T s2;
    T s3;
    T s4;
    T s5;
    T s6;
    T s7;
};

template<typename T>
N4Vec<T> operator*(const N4Vec<T>& l, int val) {
  N4Vec<T> c = l;
  c.s0 *= val;
  c.s1 *= val;
  c.s2 *= val;
  c.s3 *= val;
  return c;
}

template<typename T>
N4Vec<T> operator+=(N4Vec<T>& l, int val) {
    l.s0 += val;
    l.s1 += val;
    l.s2 += val;
    l.s3 += val;
    return l;
}

template<typename T>
N4Vec<T> operator+(const N4Vec<T>& l, int val) {
    N4Vec<T> c = l;
    c += val;
    return c;
}

template<typename T>
N8Vec<T> operator*(const N8Vec<T>& l, int val) {
  N8Vec<T> c = l;
  c.s0 *= val;
  c.s1 *= val;
  c.s2 *= val;
  c.s3 *= val;
  c.s4 *= val;
  c.s5 *= val;
  c.s6 *= val;
  c.s7 *= val;
  return c;
}

template<typename T>
N8Vec<T> operator+=(N8Vec<T>& l, int val) {
  l.s0 += val;
  l.s1 += val;
  l.s2 += val;
  l.s3 += val;
  l.s4 += val;
  l.s5 += val;
  l.s6 += val;
  l.s7 += val;
  return l;
}

#define int2_ N2Vec<int>
#define int4_ N4Vec<int>
#define int8_ N8Vec<int>

#define uchar4_ N4Vec<unsigned char>
#define uchar8_ N8Vec<unsigned char>

uchar4_ vload4_(int offset, const unsigned char* buff) {
    const unsigned char* b = buff + offset;
    uchar4_ c(b[0], b[1], b[2], b[3]);
    return c;
}

uchar8_ vload8_(int offset, const unsigned char* buff) {
    const unsigned char* b = buff + offset;
    uchar8_ c(b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
    return c;
}

void vstore8_(uchar8_ v, int offset, unsigned char* buff) {
    unsigned char* b = buff + offset;
    b[0] = v.s0;
    b[1] = v.s1;
    b[2] = v.s2;
    b[3] = v.s3;
    b[4] = v.s4;
    b[5] = v.s5;
    b[6] = v.s6;
    b[7] = v.s7;
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





void cc_kYuvI420ToNV12Rotate_HostDebug_Blocks(const unsigned char* in_buff_yuv,  // 0, m_input_buff_yuv
                                              unsigned char* out_buff_yuv,                    // 1, m_output_buff_yuv
                                              const int y_width,                              // 2, y_width
                                              const int y_height,                             // 3, y_height
                                              const int y_bytes_size,                         // 4, y_width * y_height
                                              const int u_width,                              // 5, u_width
                                              const int u_height,                             // 6, u_height
                                              const int u_bytes_size,                         // 7, u_width * u_height
                                              int uw_pos,
                                              int uh_pos) {
    //int ux = get_global_id(0);  // u-block index at x - axle.
    //int uy = get_global_id(1);  // u-block index at y - axle.
    int ux = uw_pos;
    int uy = uh_pos;

    int u_block_width = 4;
    int u_block_height = 4;
    int max_u_x_index = u_width / u_block_width - 1;
    int max_u_y_index = u_height / u_block_height - 1;

    // printf("ux = %d, uy = %d\n", ux, uy);

    // origin - y image block
    int lu_start = (uy * u_block_height * 2) * y_width + (ux * u_block_width * 2);
    int8_ indexs(0, 1, 2, 3, 4, 5, 6, 7);
    int8_ offsets = indexs * y_width;
    offsets += lu_start;
    // load y block data, 8 x 8 (width * height)
    uchar8_ yl0 = vload8_(0, in_buff_yuv + offsets.s0);
    uchar8_ yl1 = vload8_(0, in_buff_yuv + offsets.s1);
    uchar8_ yl2 = vload8_(0, in_buff_yuv + offsets.s2);
    uchar8_ yl3 = vload8_(0, in_buff_yuv + offsets.s3);
    uchar8_ yl4 = vload8_(0, in_buff_yuv + offsets.s4);
    uchar8_ yl5 = vload8_(0, in_buff_yuv + offsets.s5);
    uchar8_ yl6 = vload8_(0, in_buff_yuv + offsets.s6);
    uchar8_ yl7 = vload8_(0, in_buff_yuv + offsets.s7);

    // rotated - y image block
    uchar8_ rl0(yl7.s0, yl6.s0, yl5.s0, yl4.s0, yl3.s0, yl2.s0, yl1.s0, yl0.s0);
    uchar8_ rl1(yl7.s1, yl6.s1, yl5.s1, yl4.s1, yl3.s1, yl2.s1, yl1.s1, yl0.s1);
    uchar8_ rl2(yl7.s2, yl6.s2, yl5.s2, yl4.s2, yl3.s2, yl2.s2, yl1.s2, yl0.s2);
    uchar8_ rl3(yl7.s3, yl6.s3, yl5.s3, yl4.s3, yl3.s3, yl2.s3, yl1.s3, yl0.s3);
    uchar8_ rl4(yl7.s4, yl6.s4, yl5.s4, yl4.s4, yl3.s4, yl2.s4, yl1.s4, yl0.s4);
    uchar8_ rl5(yl7.s5, yl6.s5, yl5.s5, yl4.s5, yl3.s5, yl2.s5, yl1.s5, yl0.s5);
    uchar8_ rl6(yl7.s6, yl6.s6, yl5.s6, yl4.s6, yl3.s6, yl2.s6, yl1.s6, yl0.s6);
    uchar8_ rl7(yl7.s7, yl6.s7, yl5.s7, yl4.s7, yl3.s7, yl2.s7, yl1.s7, yl0.s7);
    // rotated u-block index
    // rotated u block height equal to u-block-width, rotated y width equal to y-height.
    int2_ ru_pos (max_u_y_index - uy, ux);
    int rlu_start = (ru_pos.y * u_block_width * 2) * y_height + (ru_pos.x * u_block_height * 2);
    int8_ r_offsets = indexs * y_height;
    r_offsets += rlu_start;
    // write rotated y image data
    vstore8_(rl0, 0, out_buff_yuv + r_offsets.s0);
    vstore8_(rl1, 0, out_buff_yuv + r_offsets.s1);
    vstore8_(rl2, 0, out_buff_yuv + r_offsets.s2);
    vstore8_(rl3, 0, out_buff_yuv + r_offsets.s3);
    vstore8_(rl4, 0, out_buff_yuv + r_offsets.s4);
    vstore8_(rl5, 0, out_buff_yuv + r_offsets.s5);
    vstore8_(rl6, 0, out_buff_yuv + r_offsets.s6);
    vstore8_(rl7, 0, out_buff_yuv + r_offsets.s7);

    // u - v data
    // load u data
    const unsigned char* in_u_buff = in_buff_yuv + y_bytes_size;
    int u_start = (uy * u_block_height) * u_width + (ux * u_block_width);
    int4_ u_indexs (0, 1, 2, 3);
    int4_ u_offsets = u_indexs * u_width;
    u_offsets += u_start;
    // load u block 4 lines
    uchar4_ ul0 = vload4_(0, in_u_buff + u_offsets.s0);
    uchar4_ ul1 = vload4_(0, in_u_buff + u_offsets.s1);
    uchar4_ ul2 = vload4_(0, in_u_buff + u_offsets.s2);
    uchar4_ ul3 = vload4_(0, in_u_buff + u_offsets.s3);

    // load v block 4 lines
    const unsigned char* in_v_buff = in_u_buff + u_bytes_size;
    int4_ v_offsets = u_offsets;
    uchar4_ vl0 = vload4_(0, in_v_buff + v_offsets.s0);
    uchar4_ vl1 = vload4_(0, in_v_buff + v_offsets.s1);
    uchar4_ vl2 = vload4_(0, in_v_buff + v_offsets.s2);
    uchar4_ vl3 = vload4_(0, in_v_buff + v_offsets.s3);

    // uv lines
    uchar8_ uvl0(ul3.s0, vl3.s0, ul2.s0, vl2.s0, ul1.s0, vl1.s0, ul0.s0, vl0.s0);
    uchar8_ uvl1(ul3.s1, vl3.s1, ul2.s1, vl2.s1, ul1.s1, vl1.s1, ul0.s1, vl0.s1);
    uchar8_ uvl2(ul3.s2, vl3.s2, ul2.s2, vl2.s2, ul1.s2, vl1.s2, ul0.s2, vl0.s2);
    uchar8_ uvl3(ul3.s3, vl3.s3, ul2.s3, vl2.s3, ul1.s3, vl1.s3, ul0.s3, vl0.s3);

    // rotated image r-width equal to y_height
    int uv_start = y_bytes_size + (ru_pos.y * u_block_height) * y_height + ru_pos.x * u_block_width * 2;
    int4_ uv_offsets = u_indexs * y_height;
    uv_offsets += uv_start;
    // write rotated uv data
    vstore8_(uvl0, 0, out_buff_yuv + uv_offsets.s0);
    vstore8_(uvl1, 0, out_buff_yuv + uv_offsets.s1);
    vstore8_(uvl2, 0, out_buff_yuv + uv_offsets.s2);
    vstore8_(uvl3, 0, out_buff_yuv + uv_offsets.s3);

    /*
    // origin image, luminance 4 pixels:
    // p0, p1,
    // p2, p3
    ushort2 lp0 = (ushort2)(ux << 1, uy << 1);  // ux * 2, uy * 2
    ushort2 lp1 = (ushort2)(lp0.x + 1, lp0.y);
    ushort2 lp2 = (ushort2)(lp0.x, lp0.y + 1);
    ushort2 lp3 = (ushort2)(lp1.x, lp2.y);

    // rotated image, luminance 4 pixels:
    // rp2, rp0,
    // rp3, rp1
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
    uchar2 lu2_in = vload2(0, in_buff_yuv + (lp0.y * y_width + lp0.x));  // lp0, lp1, read 2 pixels once
    //out_buff_yuv[rp0.y * y_height + rp0.x] = lu2_in.x;
    //out_buff_yuv[rp1.y * y_height + rp1.x] = lu2_in.y;

    uchar2 lu2_in1 = vload2(0, in_buff_yuv + (lp2.y * y_width + lp2.x));  // lp2, lp3, read 2 pixels once
    //out_buff_yuv[rp2.y * y_height + rp2.x] = lu2_in1.x;
    //out_buff_yuv[rp3.y * y_height + rp3.x] = lu2_in1.y;

    uchar4  mask_4 = (uchar4)(2,0,  3,1);
    uchar4 r_lu4 = shuffle2(lu2_in, lu2_in1, mask_4);
    vstore2(r_lu4.lo, 0, out_buff_yuv + (rp2.y * y_height + rp2.x));
    vstore2(r_lu4.hi, 0, out_buff_yuv + (rp3.y * y_height + rp3.x));

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
    */
}

bool ConvertToNV12RotateImpl_HostDebug(int width, int height, unsigned char* yuv_i420_img_data) {
    cl_int err = CL_SUCCESS;
    unsigned char* input_yuv = yuv_i420_img_data;
    const int u_width = width / 2;
    const int u_height = height / 2;

    // 4 x 4, u blocks
    int u_x_blocks = u_width / 4;
    int u_y_blocks = u_height / 4;

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

    // ----------------test---------------------
    unsigned char *out_yuv = new unsigned char[width * height * 1.5];
    for (int u_h = localy - 1; u_h >=0; --u_h) {
        for (int u_w = localx - 1; u_w >= 0; --u_w) {
            cc_kYuvI420ToNV12Rotate_HostDebug_Blocks(input_yuv,  // 0, m_input_buff_yuv
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