#pragma once
#include <jni.h>

namespace tick_hook {
using Callback = void (*)(JNIEnv*);
bool install(JavaVM* vm, JNIEnv* env, jclass target, const wchar_t* bridgeJar, Callback callback);
// Must run on the installing/game thread, outside the tick callback.
// Failure means the DLL must remain loaded until restoration can succeed.
bool remove(JNIEnv* env);
bool ready();
}
