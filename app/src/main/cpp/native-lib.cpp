#include <jni.h>
#include <string>

#include "hello.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_ccsharehello_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    // std::string hello = "Hello from C++";
    // return env->NewStringUTF(hello.c_str());
    std::string hi = Hello();
    return env->NewStringUTF(hi.c_str());
}