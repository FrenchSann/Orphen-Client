#include "modules/storageesp.hpp"
#include <cmath>

using namespace sdk;

namespace {
    // Projection monde -> écran avec les matrices capturées par ActiveRenderInfo pendant le rendu
    // du monde (column-major OpenGL). p est relatif à la caméra (monde - renderPos).
    // Retourne false si le point est derrière la caméra ; out contient alors la direction écran.
    bool project(const float* mv, const float* pr, double px, double py, double pz,
                 const ImVec2& size, ImVec2& out) {
        const float x = static_cast<float>(px), y = static_cast<float>(py), z = static_cast<float>(pz);
        const float ex = mv[0] * x + mv[4] * y + mv[8]  * z + mv[12];
        const float ey = mv[1] * x + mv[5] * y + mv[9]  * z + mv[13];
        const float ez = mv[2] * x + mv[6] * y + mv[10] * z + mv[14];
        const float ew = mv[3] * x + mv[7] * y + mv[11] * z + mv[15];
        const float cx = pr[0] * ex + pr[4] * ey + pr[8]  * ez + pr[12] * ew;
        const float cy = pr[1] * ex + pr[5] * ey + pr[9]  * ez + pr[13] * ew;
        const float cw = pr[3] * ex + pr[7] * ey + pr[11] * ez + pr[15] * ew;

        if (cw > 0.001f) {
            out = { (cx / cw + 1.0f) * 0.5f * size.x, (1.0f - cy / cw) * 0.5f * size.y };
            return true;
        }
        // Derrière : on garde la direction (cx, -cy) pour pousser le tracer vers le bord.
        out = { cx, -cy };
        return false;
    }
}

void StorageESP::onUpdate(Ctx& c) {
    const auto now = clock::now();
    if (now - lastScan_ < std::chrono::milliseconds(150)) return;
    lastScan_ = now;

    JNIEnv* env = c.env;
    cache_.clear();

    jobject list = env->GetObjectField(c.world, F.world_loadedTileEntityList);
    game::forEach(env, list, [&](jobject te) {
        Kind kind;
        if (env->IsInstanceOf(te, C.TileChest)) {
            const jint type = env->CallIntMethod(te, M.chest_getChestType); // 0 normal, 1 piégé
            if (clearEx(env)) return;
            kind = type == 1 ? TrappedChest : Chest;
        } else if (env->IsInstanceOf(te, C.TileDropper)) { // avant Dispenser : Dropper en hérite
            kind = Dropper;
        } else if (env->IsInstanceOf(te, C.TileDispenser)) {
            kind = Dispenser;
        } else if (env->IsInstanceOf(te, C.TileHopper)) {
            kind = Hopper;
        } else {
            return;
        }
        if (!cfg_[kind].show) return;

        jobject pos = env->CallObjectMethod(te, M.te_getPos);
        if (clearEx(env) || !pos) return;
        cache_.push_back({ env->CallIntMethod(pos, M.vec3i_getX),
                           env->CallIntMethod(pos, M.vec3i_getY),
                           env->CallIntMethod(pos, M.vec3i_getZ), kind });
        env->DeleteLocalRef(pos);
        clearEx(env);
    });
    if (list) env->DeleteLocalRef(list);
}

void StorageESP::onRender(Ctx& c, ImDrawList* dl) {
    if (cache_.empty() || c.screen) return;
    JNIEnv* env = c.env;

    jobject mvBuf = env->GetStaticObjectField(C.ActiveRenderInfo, F.ari_MODELVIEW);
    jobject prBuf = env->GetStaticObjectField(C.ActiveRenderInfo, F.ari_PROJECTION);
    jobject rm = env->GetObjectField(c.mc, F.mc_renderManager);
    if (!mvBuf || !prBuf || !rm) return;
    const auto* mv = static_cast<const float*>(env->GetDirectBufferAddress(mvBuf));
    const auto* pr = static_cast<const float*>(env->GetDirectBufferAddress(prBuf));
    if (!mv || !pr) return;

    const double camX = env->GetDoubleField(rm, F.rm_renderPosX);
    const double camY = env->GetDoubleField(rm, F.rm_renderPosY);
    const double camZ = env->GetDoubleField(rm, F.rm_renderPosZ);

    const ImVec2 size = ImGui::GetIO().DisplaySize;
    const ImVec2 center{ size.x * 0.5f, size.y * 0.5f };
    const double maxSq = static_cast<double>(maxDist_) * maxDist_;

    for (const Entry& e : cache_) {
        const KindCfg& k = cfg_[e.kind];
        if (!k.show) continue;

        // Les coffres sont plus petits qu'un bloc (1/16 de marge, 14/16 de haut).
        const bool chest = e.kind == Chest || e.kind == TrappedChest;
        const double in = chest ? 0.0625 : 0.0;
        const double x0 = e.x + in - camX, x1 = e.x + 1.0 - in - camX;
        const double y0 = e.y - camY,      y1 = e.y + (chest ? 0.875 : 1.0) - camY;
        const double z0 = e.z + in - camZ, z1 = e.z + 1.0 - in - camZ;

        const double mx = (x0 + x1) * 0.5, my = (y0 + y1) * 0.5, mz = (z0 + z1) * 0.5;
        if (mx * mx + my * my + mz * mz > maxSq) continue;

        const ImU32 col = ImGui::ColorConvertFloat4ToU32({ k.color[0], k.color[1], k.color[2], k.color[3] });

        if (boxes_) {
            ImVec2 p[8];
            bool visible = true;
            for (int i = 0; i < 8 && visible; ++i)
                visible = project(mv, pr, (i & 1) ? x1 : x0, (i & 2) ? y1 : y0, (i & 4) ? z1 : z0, size, p[i]);
            if (visible) {
                static constexpr int edges[12][2] = {
                    {0,1},{2,3},{4,5},{6,7}, {0,2},{1,3},{4,6},{5,7}, {0,4},{1,5},{2,6},{3,7} };
                for (auto& ed : edges) dl->AddLine(p[ed[0]], p[ed[1]], col, 1.5f);
                if (labels_) {
                    ImVec2 top;
                    if (project(mv, pr, mx, y1 + 0.25, mz, size, top)) {
                        const ImVec2 ts = ImGui::CalcTextSize(k.label);
                        dl->AddText({ top.x - ts.x * 0.5f, top.y - ts.y }, col, k.label);
                    }
                }
            }
        }

        if (tracers_) {
            ImVec2 t;
            if (!project(mv, pr, mx, my, mz, size, t)) {
                float len = std::sqrt(t.x * t.x + t.y * t.y);
                if (len < 1e-4f) { t = { 0.0f, 1.0f }; len = 1.0f; }
                const float far = size.x + size.y;
                t = { center.x + t.x / len * far, center.y + t.y / len * far };
            }
            dl->AddLine(center, t, col, 1.0f);
        }
    }
}

void StorageESP::drawSettings() {
    ImGui::Checkbox("Boîtes", &boxes_);
    ImGui::SameLine();
    ImGui::Checkbox("Tracers", &tracers_);
    ImGui::SameLine();
    ImGui::Checkbox("Noms", &labels_);
    ImGui::SliderFloat("Distance max", &maxDist_, 8.0f, 256.0f, "%.0f blocs");
    for (KindCfg& k : cfg_) {
        ImGui::PushID(k.label);
        ImGui::ColorEdit4("##col", k.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGui::SameLine();
        ImGui::Checkbox(k.label, &k.show);
        ImGui::PopID();
    }
}
