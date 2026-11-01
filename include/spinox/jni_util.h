#pragma once
#include <jni.h>
namespace spinox::jni_util {
void setVm(JavaVM* vm);
JNIEnv* env();
jobject activity();
void setActivity(jobject a);
jobject waitActivity(int timeoutMs = 15000);
}
