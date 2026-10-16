#include "modules/reach.hpp"
#include "modules/registry.hpp"
#include "modules/autoclicker.hpp"
#include "core/state.hpp"
#include <algorithm>
#include <cmath>

using namespace sdk;

namespace {
    // Intersection rayon / AABB (méthode des slabs). Retourne la distance d'entrée ou -1.
    double rayBox(const Vec3d& o, const Vec3d& d, const Vec3d& mn, const Vec3d& mx) {
        double tmin = 0.0, tmax = 1e9;
        const double os[3] = { o.x, o.y, o.z }, ds[3] = { d.x, d.y, d.z };
        const double lo[3] = { mn.x, mn.y, mn.z }, hi[3] = { mx.x, mx.y, mx.z };
        for (int i = 0; i < 3; ++i) {
            if (std::abs(ds[i]) < 1e-9) {
                if (os[i] < lo[i] || os[i] > hi[i]) return -1.0;
                continue;
            }
            double t1 = (lo[i] - os[i]) / ds[i];
            double t2 = (hi[i] - os[i]) / ds[i];
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return -1.0;
        }
        return tmin;
    }
}

jobject Reach::findTarget(Ctx& c, double& outDist) {
    JNIEnv* env = c.env;
    const double range = range_;

    jobject eyesObj = env->CallObjectMethod(c.player, M.entity_getPositionEyes, 1.0f);
    jobject lookObj = env->CallObjectMethod(c.player, M.entity_getLook, 1.0f);
    if (clearEx(env) || !eyesObj || !lookObj) return nullptr;
    const Vec3d eye = game::readVec3(env, eyesObj);
    const Vec3d look = game::readVec3(env, lookObj);
    env->DeleteLocalRef(eyesObj);
    env->DeleteLocalRef(lookObj);

    // Un bloc devant coupe la portée (même raytrace que vanilla : Entity.rayTrace).
    double limit = range;
    jobject mop = env->CallObjectMethod(c.player, M.entity_rayTrace, (jdouble)range, 1.0f);
    if (clearEx(env)) return nullptr;
    if (mop) {
        jobject type = env->GetObjectField(mop, F.mop_typeOfHit);
        if (type && env->IsSameObject(type, BLOCK_TYPE)) {
            jobject hv = env->GetObjectField(mop, F.mop_hitVec);
            if (hv) {
                Vec3d h = game::readVec3(env, hv);
                limit = std::sqrt((h.x - eye.x) * (h.x - eye.x) + (h.y - eye.y) * (h.y - eye.y) + (h.z - eye.z) * (h.z - eye.z));
                env->DeleteLocalRef(hv);
            }
        }
        if (type) env->DeleteLocalRef(type);
        env->DeleteLocalRef(mop);
    }

    jobject best = nullptr;
    double bestDist = limit;
    jobject list = env->GetObjectField(c.world, F.world_loadedEntityList);
    game::forEach(env, list, [&](jobject e) {
        if (env->IsSameObject(e, c.player)) return;
        if (!env->CallBooleanMethod(e, M.entity_canBeCollidedWith) || clearEx(env)) return;
        jobject bb = env->CallObjectMethod(e, M.entity_getEntityBoundingBox);
        float border = env->CallFloatMethod(e, M.entity_getCollisionBorderSize);
        if (clearEx(env) || !bb) return;
        Vec3d mn{ env->GetDoubleField(bb, F.bb_minX) - border, env->GetDoubleField(bb, F.bb_minY) - border, env->GetDoubleField(bb, F.bb_minZ) - border };
        Vec3d mx{ env->GetDoubleField(bb, F.bb_maxX) + border, env->GetDoubleField(bb, F.bb_maxY) + border, env->GetDoubleField(bb, F.bb_maxZ) + border };
        env->DeleteLocalRef(bb);

        double t = rayBox(eye, look, mn, mx);
        if (t >= 0.0 && t <= bestDist) {
            if (best) env->DeleteLocalRef(best);
            best = env->NewLocalRef(e);
            bestDist = t;
        }
    });
    if (list) env->DeleteLocalRef(list);

    outDist = bestDist;
    return best;
}

bool Reach::tryAttack(Ctx& c, bool swing) {
    if (!enabled() || !c.inWorld() || c.screen) return false;
    double dist = 0.0;
    jobject target = findTarget(c, dist);
    if (!target) return false;

    // Portée entité vanilla : 6 en créatif (extendedReach), 3 sinon. En dessous, vanilla attaque déjà.
    const bool extended = c.env->CallBooleanMethod(c.controller, M.pc_extendedReach);
    const double vanilla = (!clearEx(c.env) && extended) ? 6.0 : 3.0;

    bool attacked = false;
    if (dist > vanilla) {
        if (swing) c.env->CallVoidMethod(c.player, M.living_swingItem);
        c.env->CallVoidMethod(c.controller, M.pc_attackEntity, c.player, target);
        clearEx(c.env);
        attacked = true;
    }
    c.env->DeleteLocalRef(target);
    return attacked;
}

void Reach::onUpdate(Ctx& c) {
    // Clic manuel : vanilla a déjà balancé le bras (raté), on envoie juste l'attaque.
    // Si l'autoclicker tourne, c'est lui qui appelle tryAttack à chaque clic.
    const bool down = state::leftHeld() && !c.screen;
    if (down && !wasDown_ && !modules::autoClicker().enabled())
        tryAttack(c, false);
    wasDown_ = down;
}

void Reach::drawSettings() {
    ImGui::SliderFloat("Portée", &range_, 3.0f, 6.0f, "%.2f blocs");
}

