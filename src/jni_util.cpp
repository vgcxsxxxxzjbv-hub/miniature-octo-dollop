#include "spinox/jni_util.h"
#include "spinox/log.h"
#include <pthread.h>
#include <unistd.h>
namespace spinox::jni_util {
static JavaVM* g_vm = nullptr;
static jobject g_act = nullptr;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
void setVm(JavaVM* vm) { g_vm = vm; }
JNIEnv* env() {
    if (!g_vm) return nullptr;
    JNIEnv* e = nullptr;
    if (g_vm->GetEnv((void**)&e, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_vm->AttachCurrentThread(&e, nullptr) != JNI_OK) return nullptr;
    }
    return e;
}
void setActivity(jobject a) {
    JNIEnv* e = env(); if (!e) return;
    pthread_mutex_lock(&g_mu);
    if (g_act) e->DeleteGlobalRef(g_act);
    g_act = a ? e->NewGlobalRef(a) : nullptr;
    pthread_mutex_unlock(&g_mu);
}
jobject activity() {
    pthread_mutex_lock(&g_mu);
    jobject a = g_act;
    pthread_mutex_unlock(&g_mu);
    return a;
}
jobject waitActivity(int timeoutMs) {
    int elapsed = 0;
    while (elapsed < timeoutMs) {
        JNIEnv* e = env();
        if (e) {
            jclass at = e->FindClass("android/app/ActivityThread");
            if (at) {
                jmethodID cur = e->GetStaticMethodID(at, "currentActivityThread",
                    "()Landroid/app/ActivityThread;");
                jobject t = e->CallStaticObjectMethod(at, cur);
                if (t) {
                    jfieldID f = e->GetFieldID(at, "mActivities", "Landroid/util/ArrayMap;");
                    jobject map = e->GetObjectField(t, f);
                    if (map) {
                        jclass mc = e->GetObjectClass(map);
                        jmethodID sz = e->GetMethodID(mc, "size", "()I");
                        if (e->CallIntMethod(map, sz) > 0) {
                            jmethodID va = e->GetMethodID(mc, "valueAt", "(I)Ljava/lang/Object;");
                            jobject rec = e->CallObjectMethod(map, va, 0);
                            jclass rc = e->GetObjectClass(rec);
                            jfieldID af = e->GetFieldID(rc, "activity", "Landroid/app/Activity;");
                            jobject a = e->GetObjectField(rec, af);
                            if (a) { setActivity(a); return activity(); }
                        }
                    }
                }
            }
            if (e->ExceptionCheck()) e->ExceptionClear();
        }
        usleep(100000);
        elapsed += 100;
    }
    return nullptr;
}
}
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
    spinox::jni_util::setVm(vm);
    return JNI_VERSION_1_6;
}
