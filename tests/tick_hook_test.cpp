#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "core/tick_hook.hpp"

static int callbacks = 0;
static jclass fixture = nullptr;
static jfieldID ticks = nullptr;
static void require(bool value, const char* text) {
    if (!value) { std::fprintf(stderr, "%s\n", text); std::exit(1); }
}
static void tick(JNIEnv* env) {
    require(env->GetStaticIntField(fixture, ticks) == callbacks, "Callback must precede tick body");
    ++callbacks;
}
int wmain(int argc, wchar_t** argv) {
    require(argc == 4, "Usage: tick_hook_test jvm.dll fixture-directory bridge.jar");
    auto library = LoadLibraryW(argv[1]);
    require(library != nullptr, "JVM missing");
    using CreateVM = jint(JNICALL*)(JavaVM**, void**, void*);
    auto create = reinterpret_cast<CreateVM>(GetProcAddress(library, "JNI_CreateJavaVM"));
    require(create != nullptr, "CreateJavaVM missing");
    std::wstring widePath = argv[2];
    int length = WideCharToMultiByte(CP_ACP, 0, widePath.data(), static_cast<int>(widePath.size()), nullptr, 0, nullptr, nullptr);
    std::string narrowPath(length, '\0');
    WideCharToMultiByte(CP_ACP, 0, widePath.data(), static_cast<int>(widePath.size()), narrowPath.data(), length, nullptr, nullptr);
    std::string classPath = "-Djava.class.path=" + narrowPath;
    JavaVMOption options[]{{classPath.data(), nullptr}, {const_cast<char*>("-Xcheck:jni"), nullptr}};
    JavaVMInitArgs args{JNI_VERSION_1_8, 2, options, JNI_FALSE};
    JavaVM* vm = nullptr; JNIEnv* env = nullptr;
    require(create(&vm, reinterpret_cast<void**>(&env), &args) == JNI_OK, "Cannot create VM");
    jclass localFixture = env->FindClass("TickFixture");
    require(localFixture && !env->ExceptionCheck(), "Fixture missing");
    fixture = static_cast<jclass>(env->NewGlobalRef(localFixture));
    env->DeleteLocalRef(localFixture);
    ticks = env->GetStaticFieldID(fixture, "ticks", "I");
    jmethodID ctor = env->GetMethodID(fixture, "<init>", "()V");
    jmethodID runTick = env->GetMethodID(fixture, "runTick", "()V");
    jmethodID render = env->GetMethodID(fixture, "render", "()V");
    jobject game = env->NewObject(fixture, ctor);
    require(game && !env->ExceptionCheck(), "Fixture init failed");
    auto call = [&](jmethodID method) {
        env->CallVoidMethod(game, method);
        require(!env->ExceptionCheck(), "Java method failed");
    };
    require(!tick_hook::install(vm, env, fixture, L"Z:/orphen-test-missing/bridge.jar", tick), "Missing bridge must fail closed");
    require(!tick_hook::ready() && !env->ExceptionCheck(), "Failed installation must leave clean JNI state");
    jclass noTick = env->FindClass("TickFixture$WithoutTick");
    require(noTick && !env->ExceptionCheck(), "No-tick fixture missing");
    require(!tick_hook::install(vm, env, noTick, argv[3], tick), "Missing runTick must fail closed");
    require(!tick_hook::ready() && !env->ExceptionCheck(), "Transformation failure must leave clean JNI state");
    require(tick_hook::install(vm, env, fixture, argv[3], tick), "Tick hook installation failed");
    require(!tick_hook::install(vm, env, fixture, argv[3], tick), "Duplicate install must be rejected");
    for (int i = 0; i < 100; ++i) call(render);
    require(callbacks == 0, "Rendering must never trigger a click update");
    for (int i = 0; i < 5; ++i) { call(runTick); call(render); }
    require(callbacks == 5, "Exactly one callback required per tick");
    require(env->GetStaticIntField(fixture, ticks) == 5, "Original tick body must be preserved");
    jmethodID worker = env->GetStaticMethodID(fixture, "workerTick", "(LTickFixture;)V");
    env->CallStaticVoidMethod(fixture, worker, game);
    require(!env->ExceptionCheck(), "Worker call failed");
    require(callbacks == 5 && env->GetStaticIntField(fixture, ticks) == 6, "Other threads must never run the callback");
    require(tick_hook::remove(env), "Restoration failed");
    call(runTick);
    require(callbacks == 5, "Callback must stop after unloading");
    require(env->GetStaticIntField(fixture, ticks) == 7, "Restored tick must still execute");
    callbacks = 7;
    require(tick_hook::install(vm, env, fixture, argv[3], tick), "Reinstallation failed");
    call(runTick);
    require(callbacks == 8, "Reinstallation must not duplicate callbacks");
    require(tick_hook::remove(env), "Second restoration failed");
    env->DeleteGlobalRef(fixture);
    require(vm->DestroyJavaVM() == JNI_OK, "VM shutdown failed");
    std::puts("Tick hook: failure cleanup, render/thread isolation, tick entry, restoration and reinstall passed");
}
