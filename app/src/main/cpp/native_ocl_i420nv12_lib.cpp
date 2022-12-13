#include <jni.h>
#include <string>

#include "Utility.h"
#include "YuvI420ToNV12Rotate.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_rubus_clhello_MainActivity_getDevicesNameFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string name = GetDevicesName();
    return env->NewStringUTF(name.c_str());
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_rubus_clhello_MainActivity_ConvertI420ToNV12JNI(JNIEnv *env, jobject thiz,
                                  jint width, jint height,
                                  jbyteArray j_yuv_i420_data,
                                  jbyteArray j_out_nv12_data) {
    // width and height must equal to 8x
    // check this in java side
    if (width % 8 != 0 || height % 8 != 0) return false;

    // uint64_t start = GetMilliseconds();
    // get data, addref()
    jbyte* img_i420_bytes = env->GetByteArrayElements(j_yuv_i420_data, NULL);
    jbyte* out_nv12_data = env->GetByteArrayElements(j_out_nv12_data, NULL);

    // bool success = YuvConvertHelper::getInstance().YuvI420ConvertToNV12(width, height, (unsigned char*)img_bytes);
    bool success = YuvConvertRotateHelper::getInstance().YuvI420ConvertToNV12Rotate(width,
                                                           height,
                                                           (unsigned char*)img_i420_bytes,
                                                           (unsigned char*)out_nv12_data);

    // release() to addref()
    env->ReleaseByteArrayElements(j_yuv_i420_data, img_i420_bytes, 0);
    env->ReleaseByteArrayElements(j_out_nv12_data, out_nv12_data, 0);
    // uint64_t  end = GetMilliseconds();
    // cost time milliseconds
    // uint32_t cost = (uint32_t)(end - start);
    return success;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_rubus_clhello_MainActivity_ConvertAbgrToNV12JNI(JNIEnv *env, jobject thiz,
                                                         jint width, jint height,
                                                         jbyteArray j_abgr_data,
                                                         jbyteArray j_out_nv12_data) {
    // width and height must equal to 8x
    // check this in java side
    if (width % 8 != 0 || height % 8 != 0) return false;

    // uint64_t start = GetMilliseconds();
    // get data, addref()
    jbyte* img_abgr_bytes = env->GetByteArrayElements(j_abgr_data, NULL);
    jbyte* out_nv12_data = env->GetByteArrayElements(j_out_nv12_data, NULL);

    // bool success = YuvConvertHelper::getInstance().YuvI420ConvertToNV12(width, height, (unsigned char*)img_bytes);
    bool success = YuvConvertRotateHelper::getInstance().AbgrConvertToNV12Rotate(width, height,
                                                (unsigned char*)img_abgr_bytes,
                                                (unsigned char*)out_nv12_data);

    // release() to addref()
    env->ReleaseByteArrayElements(j_abgr_data, img_abgr_bytes, 0);
    env->ReleaseByteArrayElements(j_out_nv12_data, out_nv12_data, 0);
    // uint64_t  end = GetMilliseconds();
    // cost time milliseconds
    // uint32_t cost = (uint32_t)(end - start);
    return success;
}
