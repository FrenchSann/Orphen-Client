#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include "sdk/classloader.hpp"

static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}

// A real isolated Java loader exercises the native-hook lookup path without
// attaching to the user's game. The fixture contains an empty Minecraft class.
int main(int argc, char** argv) {
    require(argc == 3, "Usage: classloader_test <jvm.dll> <fixture-directory-URL>");
    HMODULE library = LoadLibraryA(argv[1]);
    require(library != nullptr, "Cannot load JVM");
    using CreateVM = jint(JNICALL*)(JavaVM**, void**, void*);
    auto create = reinterpret_cast<CreateVM>(GetProcAddress(library, "JNI_CreateJavaVM"));
    require(create != nullptr, "JNI_CreateJavaVM missing");
    JavaVMOption option{const_cast<char*>("-Xcheck:jni"), nullptr};
    JavaVMInitArgs args{JNI_VERSION_1_8, 1, &option, JNI_FALSE};
    JavaVM* vm = nullptr;
    JNIEnv* env = nullptr;
    require(create(&vm, reinterpret_cast<void**>(&env), &args) == JNI_OK, "Cannot create JVM");
    require(sdk::findGameLoader(env) == nullptr, "Must reject a JVM without Minecraft");
    require(!env->ExceptionCheck(), "Discovery must clear failed lookup exceptions");

    jclass urlClass = env->FindClass("java/net/URL");
    jmethodID urlCtor = env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
    jstring path = env->NewStringUTF(argv[2]);
    jobject url = env->NewObject(urlClass, urlCtor, path);
    require(!env->ExceptionCheck() && url, "Cannot create fixture URL");
    jobjectArray urls = env->NewObjectArray(1, urlClass, url);
    jclass loaderClass = env->FindClass("java/net/URLClassLoader");
    jmethodID loaderCtor = env->GetMethodID(loaderClass, "<init>", "([Ljava/net/URL;Ljava/lang/ClassLoader;)V");
    jobject loader = env->NewObject(loaderClass, loaderCtor, urls, nullptr);
    require(!env->ExceptionCheck() && loader, "Cannot create isolated loader");
    jclass threadClass = env->FindClass("java/lang/Thread");
    jmethodID current = env->GetStaticMethodID(threadClass, "currentThread", "()Ljava/lang/Thread;");
    jobject thread = env->CallStaticObjectMethod(threadClass, current);
    require(!env->ExceptionCheck() && thread, "Cannot find current thread");
    jmethodID setContext = env->GetMethodID(threadClass, "setContextClassLoader", "(Ljava/lang/ClassLoader;)V");
    env->CallVoidMethod(thread, setContext, loader);
    require(!env->ExceptionCheck(), "Cannot set context loader");

    jclass direct = env->FindClass("net/minecraft/client/Minecraft");
    require(!direct && env->ExceptionCheck(), "Fixture must be invisible to FindClass");
    env->ExceptionClear();
    jobject found = sdk::findGameLoader(env);
    require(found && !env->ExceptionCheck(), "Must discover Minecraft through the context loader");
    require(env->IsSameObject(found, loader), "Must return the defining loader");
    jclass mc = sdk::loadClass(env, found, "net/minecraft/client/Minecraft");
    require(mc && !env->ExceptionCheck(), "Slash-separated class names must load successfully");
    jclass list = sdk::loadClass(env, found, "java/util/List");
    require(list && !env->ExceptionCheck(), "Bootstrap classes must remain accessible");
    env->CallVoidMethod(thread, setContext, nullptr);
    require(!env->ExceptionCheck(), "Cannot restore context loader");
    require(vm->DestroyJavaVM() == JNI_OK, "Cannot destroy test JVM");
    std::puts("Class-loader integration checks passed");
}
