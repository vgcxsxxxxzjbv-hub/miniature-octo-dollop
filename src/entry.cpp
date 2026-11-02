#include "spinox/il2cpp.h"
#include "spinox/overlay.h"
#include "spinox/jni_util.h"
#include "spinox/log.h"
#include <pthread.h>
#include <unistd.h>
static void* boot(void*) {
    for (int i = 0; i < 30; i++) {
        if (spinox::il2cpp::init()) break;
        sleep(1);
    }
    spinox::jni_util::waitActivity(20000);
    spinox::overlay::init();
    LOGI("boot complete");
    return nullptr;
}
__attribute__((constructor))
static void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, boot, nullptr);
}
