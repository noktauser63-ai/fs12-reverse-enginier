// Gecici: ses motorunun Android'de baglandigini kanitlayan kucuk giris noktasi.
#include <jni.h>

#include "engine.h"

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*) {
    static fs12::audio::Engine engine;
    static fs12::audio::Output* out = fs12::audio::createDefaultOutput();
    if (out) out->start(&engine);
    return JNI_VERSION_1_6;
}
