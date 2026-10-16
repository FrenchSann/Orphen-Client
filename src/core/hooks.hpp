#pragma once
#include <jni.h>
#include <Windows.h>

namespace hooks {
    // Installe le hook wglSwapBuffers. vm = JavaVM du jeu.
    bool install(JavaVM* vm, HANDLE unloadDoneEvent);
    void remove();
}
