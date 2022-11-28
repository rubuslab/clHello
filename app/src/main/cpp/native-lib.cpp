#include <jni.h>
#include <string>

#include "Utility.h"
#include "SetImageData.h"
#include "YuvConvertOpenCl.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_ccsharehello_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    // std::string hello = "Hello from C++";
    // return env->NewStringUTF(hello.c_str());
    std::string hi = Hello();
    return env->NewStringUTF(hi.c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_ccsharehello_MainActivity_AddJNI(
        JNIEnv* env,
        jobject activity/* this */,
        jint a, jint b) {
    int count = a + b;
    return count;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_ccsharehello_MainActivity_SetImageDataJNI(
        JNIEnv* env,
        jobject activity/* this */,
        jint width,
        jint height,
        jintArray imgData) {
    // get data array, addref()
    jint* img_data = env->GetIntArrayElements(imgData, NULL);

    SetImageData(width, height, img_data);

    // release() to addref()
    env->ReleaseIntArrayElements(imgData, img_data, 0);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_ccsharehello_MainActivity_ConvertI420ToNV12JNI(JNIEnv *env, jobject thiz, jint width, jint height,
                                                                jbyteArray img_data) {
    uint64_t start = GetMilliseconds();
    // get data, addref()
    jbyte* img_bytes = env->GetByteArrayElements(img_data, NULL);
    bool success = YuvConvertHelper::getInstance().YuvI420ConvertToNV12(width, height, (unsigned char*)img_bytes);
    // release() to addref()
    env->ReleaseByteArrayElements(img_data, img_bytes, 0);
    uint64_t  end = GetMilliseconds();
    // cost time milliseconds
    uint32_t cost = (uint32_t)(end - start);
    return success;
}