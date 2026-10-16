#pragma once
#include "sdk/sdk.hpp"

// État du jeu capturé sur le thread principal, séparément pour chaque frame ou tick.
// Toutes les jobject sont des références locales, libérées par le PopLocalFrame correspondant.
struct Ctx {
    JNIEnv* env = nullptr;
    jobject mc = nullptr;
    jobject player = nullptr;      // EntityPlayerSP (null hors partie)
    jobject world = nullptr;       // WorldClient
    jobject controller = nullptr;  // PlayerControllerMP
    jobject screen = nullptr;      // GuiScreen ouvert (null = en jeu)
    bool singleplayer = false;

    bool inWorld() const { return player && world && controller; }
};

struct Vec3d { double x, y, z; };

namespace game {

inline Vec3d readVec3(JNIEnv* env, jobject v) {
    return { env->GetDoubleField(v, sdk::F.vec_x),
             env->GetDoubleField(v, sdk::F.vec_y),
             env->GetDoubleField(v, sdk::F.vec_z) };
}

// Parcourt une java.util.List ; fn(jobject elem) avec ref locale supprimée après chaque appel.
template <class Fn>
void forEach(JNIEnv* env, jobject list, Fn&& fn) {
    if (!list) return;
    jint n = env->CallIntMethod(list, sdk::M.list_size);
    if (sdk::clearEx(env)) return;
    for (jint i = 0; i < n; ++i) {
        jobject e = env->CallObjectMethod(list, sdk::M.list_get, i);
        if (sdk::clearEx(env)) return;
        if (!e) continue;
        fn(e);
        env->DeleteLocalRef(e);
    }
}

} // namespace game
