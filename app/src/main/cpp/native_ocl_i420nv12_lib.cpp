#include <jni.h>
#include <string>

#include "Utility.h"
#include "YuvI420ToNV12Rotate.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_ccsharehello_MainActivity_getDevicesNameFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string name = GetDevicesName();
    return env->NewStringUTF(name.c_str());
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_ccsharehello_MainActivity_ConvertI420ToNV12JNI(JNIEnv *env, jobject thiz, jint width, jint height,
                                                                jbyteArray img_data) {
    // width and height must equal to 8x
    // check this in java side
    if (width % 8 != 0 || height % 8 != 0) return false;

    // uint64_t start = GetMilliseconds();
    // get data, addref()
    jbyte* img_bytes = env->GetByteArrayElements(img_data, NULL);
    // bool success = YuvConvertHelper::getInstance().YuvI420ConvertToNV12(width, height, (unsigned char*)img_bytes);
    bool success = YuvConvertRotateHelper::getInstance().YuvI420ConvertToNV12Rotate(width, height, (unsigned char*)img_bytes);
    // release() to addref()
    env->ReleaseByteArrayElements(img_data, img_bytes, 0);
    // uint64_t  end = GetMilliseconds();
    // cost time milliseconds
    // uint32_t cost = (uint32_t)(end - start);
    return success;
}