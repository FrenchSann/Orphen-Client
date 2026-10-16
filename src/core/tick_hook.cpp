#include "core/tick_hook.hpp"
#include "core/log.hpp"
#include "sdk/classloader.hpp"
#include <jvmti.h>
#include <atomic>
#include <cstring>

namespace {
    jvmtiEnv* g_ti = nullptr;
    jclass g_target = nullptr, g_bridge = nullptr, g_transformer = nullptr;
    jmethodID g_transform = nullptr;
    tick_hook::Callback g_callback = nullptr;
    DWORD g_thread = 0;
    std::atomic_bool g_enabled{false};
    bool g_transformed = false;

    bool checked(JNIEnv* env) {
        if (!env->ExceptionCheck()) return true;
        env->ExceptionClear();
        LOG("tick hook : exception Java (chargement ou transformation)");
        return false;
    }
    bool checked(jvmtiError error, const char* operation) {
        if (error == JVMTI_ERROR_NONE) return true;
        LOG("tick hook : %s, erreur JVMTI %d", operation, static_cast<int>(error));
        return false;
    }
    void JNICALL nativeTick(JNIEnv* env, jclass) {
        if (GetCurrentThreadId() != g_thread || !g_enabled.load() || !g_callback) return;
        static thread_local bool running = false;
        if (running || env->ExceptionCheck()) return;
        running = true;
        try { g_callback(env); }
        catch (...) { LOG("tick hook : exception native, callback desactive"); g_enabled = false; }
        checked(env);
        running = false;
    }
    void JNICALL transform(jvmtiEnv* ti, JNIEnv* env, jclass redefining, jobject,
                           const char*, jobject, jint size, const unsigned char* bytes,
                           jint* newSize, unsigned char** newBytes) {
        if (!redefining || !g_target || !env->IsSameObject(redefining, g_target)) return;
        if (env->PushLocalFrame(8) != JNI_OK) { checked(env); return; }
        jbyteArray input = env->NewByteArray(size);
        if (checked(env) && input) {
            env->SetByteArrayRegion(input, 0, size, reinterpret_cast<const jbyte*>(bytes));
            if (checked(env)) {
                auto output = static_cast<jbyteArray>(env->CallStaticObjectMethod(g_transformer, g_transform, input));
                if (checked(env) && output) {
                    jint length = env->GetArrayLength(output);
                    unsigned char* buffer = nullptr;
                    if (checked(ti->Allocate(length, &buffer), "Allocate")) {
                        env->GetByteArrayRegion(output, 0, length, reinterpret_cast<jbyte*>(buffer));
                        if (checked(env)) {
                            *newSize = length;
                            *newBytes = buffer;
                            g_transformed = true;
                        } else ti->Deallocate(buffer);
                    }
                }
            }
        }
        env->PopLocalFrame(nullptr);
    }

    // Separate loader with bootstrap parent: never resolve Lunar's own ASM version.
    jclass transformerClass(JNIEnv* env, const wchar_t* path) {
        jclass fileClass = env->FindClass("java/io/File");
        if (!checked(env) || !fileClass) return nullptr;
        jmethodID ctor = env->GetMethodID(fileClass, "<init>", "(Ljava/lang/String;)V");
        if (!checked(env) || !ctor) return nullptr;
        jstring text = env->NewString(reinterpret_cast<const jchar*>(path), static_cast<jsize>(wcslen(path)));
        if (!checked(env) || !text) return nullptr;
        jobject file = env->NewObject(fileClass, ctor, text);
        if (!checked(env) || !file) return nullptr;
        jmethodID toURI = env->GetMethodID(fileClass, "toURI", "()Ljava/net/URI;");
        if (!checked(env) || !toURI) return nullptr;
        jobject uri = env->CallObjectMethod(file, toURI);
        if (!checked(env) || !uri) return nullptr;
        jclass uriClass = env->GetObjectClass(uri);
        if (!checked(env) || !uriClass) return nullptr;
        jmethodID toURL = env->GetMethodID(uriClass, "toURL", "()Ljava/net/URL;");
        if (!checked(env) || !toURL) return nullptr;
        jobject url = env->CallObjectMethod(uri, toURL);
        if (!checked(env) || !url) return nullptr;
        jclass urlClass = env->FindClass("java/net/URL");
        if (!checked(env) || !urlClass) return nullptr;
        jobjectArray urls = env->NewObjectArray(1, urlClass, url);
        if (!checked(env) || !urls) return nullptr;
        jclass loaderClass = env->FindClass("java/net/URLClassLoader");
        if (!checked(env) || !loaderClass) return nullptr;
        jmethodID loaderCtor = env->GetMethodID(loaderClass, "<init>", "([Ljava/net/URL;Ljava/lang/ClassLoader;)V");
        if (!checked(env) || !loaderCtor) return nullptr;
        jobject loader = env->NewObject(loaderClass, loaderCtor, urls, nullptr);
        if (!checked(env) || !loader) return nullptr;
        jclass result = sdk::loadClass(env, loader, "orphen/bridge/Transformer");
        return checked(env) ? result : nullptr;
    }
}

namespace tick_hook {
bool install(JavaVM* vm, JNIEnv* env, jclass target, const wchar_t* bridgeJar, Callback callback) {
    if (g_ti || !target || !callback) return false;
    if (env->PushLocalFrame(48) != JNI_OK) { checked(env); return false; }
    auto fail = [&] { remove(env); env->PopLocalFrame(nullptr); return false; };
    g_thread = GetCurrentThreadId();
    if (vm->GetEnv(reinterpret_cast<void**>(&g_ti), JVMTI_VERSION_1_2) != JNI_OK || !g_ti) {
        LOG("tick hook : JVMTI indisponible");
        return fail();
    }
    jvmtiCapabilities capabilities{};
    capabilities.can_retransform_classes = 1;
    if (!checked(g_ti->AddCapabilities(&capabilities), "AddCapabilities")) return fail();
    jboolean modifiable = JNI_FALSE;
    if (!checked(g_ti->IsModifiableClass(target, &modifiable), "IsModifiableClass") || !modifiable) return fail();
    jclass transformer = transformerClass(env, bridgeJar);
    if (!transformer) return fail();
    g_transformer = static_cast<jclass>(env->NewGlobalRef(transformer));
    if (!checked(env) || !g_transformer) return fail();
    g_transform = env->GetStaticMethodID(transformer, "instrument", "([B)[B");
    if (!checked(env) || !g_transform) return fail();

    jclass classClass = env->FindClass("java/lang/Class");
    if (!checked(env) || !classClass) return fail();
    jmethodID getLoader = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    if (!checked(env) || !getLoader) return fail();
    jobject gameLoader = env->CallObjectMethod(target, getLoader);
    if (!checked(env) || !gameLoader) return fail();
    jclass bridge = sdk::loadClass(env, gameLoader, "orphen/bridge/TickBridge");
    if (env->ExceptionCheck()) env->ExceptionClear(); // absent on first install
    if (!bridge) {
        jmethodID getBytes = env->GetStaticMethodID(transformer, "bridgeBytes", "()[B");
        if (!checked(env) || !getBytes) return fail();
        auto bytes = static_cast<jbyteArray>(env->CallStaticObjectMethod(transformer, getBytes));
        if (!checked(env) || !bytes) return fail();
        jsize length = env->GetArrayLength(bytes);
        jbyte* data = env->GetByteArrayElements(bytes, nullptr);
        if (!checked(env) || !data) return fail();
        bridge = env->DefineClass("orphen/bridge/TickBridge", gameLoader, data, length);
        env->ReleaseByteArrayElements(bytes, data, JNI_ABORT);
        if (!checked(env) || !bridge) return fail();
    }
    g_bridge = static_cast<jclass>(env->NewGlobalRef(bridge));
    if (!checked(env) || !g_bridge) return fail();
    JNINativeMethod native{const_cast<char*>("nativeTick"), const_cast<char*>("()V"), reinterpret_cast<void*>(nativeTick)};
    jint registered = env->RegisterNatives(bridge, &native, 1);
    if (!checked(env) || registered != JNI_OK) return fail();
    g_target = static_cast<jclass>(env->NewGlobalRef(target));
    if (!checked(env) || !g_target) return fail();
    g_callback = callback;
    jvmtiEventCallbacks callbacks{};
    callbacks.ClassFileLoadHook = transform;
    if (!checked(g_ti->SetEventCallbacks(&callbacks, sizeof(callbacks)), "SetEventCallbacks")) return fail();
    if (!checked(g_ti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr), "Enable transform")) return fail();
    if (!checked(g_ti->RetransformClasses(1, &g_target), "Install runTick") || !g_transformed) return fail();
    g_enabled = true;
    LOG("AutoClicker : callback installe a l'entree de Minecraft.runTick()");
    env->PopLocalFrame(nullptr);
    return true;
}

bool remove(JNIEnv* env) {
    g_enabled = false;
    if (g_ti) {
        if (!checked(g_ti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr), "Disable transform")) return false;
        // Retransform without our transformer restores this environment's input bytecode,
        // preserving the other agents' transformations (no stale bytecode snapshot).
        if (g_transformed && g_target && !checked(g_ti->RetransformClasses(1, &g_target), "Restore runTick")) return false;
        g_transformed = false;
        if (!checked(g_ti->DisposeEnvironment(), "DisposeEnvironment")) return false;
        g_ti = nullptr;
    }
    if (g_bridge) {
        jint result = env->UnregisterNatives(g_bridge);
        if (!checked(env) || result != JNI_OK) return false;
    }
    for (jclass* ref : {&g_target, &g_bridge, &g_transformer}) {
        if (*ref) env->DeleteGlobalRef(*ref);
        *ref = nullptr;
    }
    g_transform = nullptr;
    g_callback = nullptr;
    return true;
}
bool ready() { return g_enabled.load(); }
}
