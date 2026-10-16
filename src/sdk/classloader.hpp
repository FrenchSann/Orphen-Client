#pragma once
#include <jni.h>
#include <algorithm>
#include <string>

namespace sdk {

// A native hook has no Java caller to supply Lunar's application class loader.
// Resolve through thread context loaders, then keep the defining loader of Minecraft.
inline jclass loadClass(JNIEnv* env, jobject loader, const char* name) {
    if (!loader) return env->FindClass(name);
    jclass type = env->GetObjectClass(loader);
    jmethodID load = env->GetMethodID(type, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    env->DeleteLocalRef(type);
    if (!load) return nullptr;
    std::string binary(name);
    std::replace(binary.begin(), binary.end(), '/', '.');
    jstring text = env->NewStringUTF(binary.c_str());
    if (!text) return nullptr;
    auto result = static_cast<jclass>(env->CallObjectMethod(loader, load, text));
    env->DeleteLocalRef(text);
    return result;
}

inline jobject findGameLoader(JNIEnv* env) {
    if (env->PushLocalFrame(32) != JNI_OK) { env->ExceptionClear(); return nullptr; }
    auto finish = [&](jobject loader) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        return env->PopLocalFrame(loader);
    };
    jclass thread = env->FindClass("java/lang/Thread");
    if (!thread) return finish(nullptr);
    jmethodID context = env->GetMethodID(thread, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
    if (!context) return finish(nullptr);
    jclass classType = env->FindClass("java/lang/Class");
    if (!classType) return finish(nullptr);
    jmethodID defining = env->GetMethodID(classType, "getClassLoader", "()Ljava/lang/ClassLoader;");
    if (!defining) return finish(nullptr);
    auto tryThread = [&](jobject candidate) -> jobject {
        jobject loader = env->CallObjectMethod(candidate, context);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return nullptr; }
        if (!loader) return nullptr;
        jclass mc = loadClass(env, loader, "net/minecraft/client/Minecraft");
        env->DeleteLocalRef(loader);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return nullptr; }
        if (!mc) return nullptr;
        jobject result = env->CallObjectMethod(mc, defining);
        env->DeleteLocalRef(mc);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return nullptr; }
        return result;
    };
    jmethodID current = env->GetStaticMethodID(thread, "currentThread", "()Ljava/lang/Thread;");
    if (!current) return finish(nullptr);
    jobject self = env->CallStaticObjectMethod(thread, current);
    if (env->ExceptionCheck() || !self) return finish(nullptr);
    if (jobject loader = tryThread(self)) return finish(loader);

    jmethodID traces = env->GetStaticMethodID(thread, "getAllStackTraces", "()Ljava/util/Map;");
    if (!traces) return finish(nullptr);
    jobject map = env->CallStaticObjectMethod(thread, traces);
    if (env->ExceptionCheck() || !map) return finish(nullptr);
    jclass mapType = env->FindClass("java/util/Map");
    if (!mapType) return finish(nullptr);
    jmethodID keySet = env->GetMethodID(mapType, "keySet", "()Ljava/util/Set;");
    if (!keySet) return finish(nullptr);
    jobject set = env->CallObjectMethod(map, keySet);
    if (env->ExceptionCheck() || !set) return finish(nullptr);
    jclass setType = env->FindClass("java/util/Set");
    if (!setType) return finish(nullptr);
    jmethodID toArray = env->GetMethodID(setType, "toArray", "()[Ljava/lang/Object;");
    if (!toArray) return finish(nullptr);
    auto threads = static_cast<jobjectArray>(env->CallObjectMethod(set, toArray));
    if (env->ExceptionCheck() || !threads) return finish(nullptr);
    for (jsize i = 0; i < env->GetArrayLength(threads); ++i) {
        jobject candidate = env->GetObjectArrayElement(threads, i);
        if (env->ExceptionCheck() || !candidate) return finish(nullptr);
        jobject loader = tryThread(candidate);
        env->DeleteLocalRef(candidate);
        if (loader) return finish(loader);
    }
    return finish(nullptr);
}
} // namespace sdk
