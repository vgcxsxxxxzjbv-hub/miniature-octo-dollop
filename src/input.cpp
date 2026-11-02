#include "spinox/overlay.h"
#include <jni.h>
extern "C" JNIEXPORT jboolean JNICALL
Java_com_spinox_NativeBridge_onTouch(JNIEnv* e, jclass, jobject ev) {
    if (!ev) return JNI_FALSE;
    jclass mc = e->GetObjectClass(ev);
    jmethodID ga = e->GetMethodID(mc, "getAction", "()I");
    jmethodID gx = e->GetMethodID(mc, "getX", "()F");
    jmethodID gy = e->GetMethodID(mc, "getY", "()F");
    if (!ga || !gx || !gy) return JNI_FALSE;
    int a = (int)e->CallIntMethod(ev, ga) & 0xFF;
    float x = e->CallFloatMethod(ev, gx);
    float y = e->CallFloatMethod(ev, gy);
    spinox::overlay::setTouch(a, x, y);
    return spinox::overlay::isOpen() ? JNI_TRUE : JNI_FALSE;
}
