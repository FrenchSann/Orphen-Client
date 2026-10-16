#include "modules/autoclicker.hpp"
#include "modules/registry.hpp"
#include "modules/reach.hpp"
#include "core/state.hpp"

using namespace sdk;

void AutoClicker::onTick(Ctx& c) {
    if (c.screen || !state::leftHeld()) {
        next_ = {};
        return;
    }

    const auto now = clock::now();
    if (next_ != clock::time_point{} && now < next_) return;

    if (pauseWhileUsing_) {
        const bool using_ = c.env->CallBooleanMethod(c.player, M.player_isUsingItem);
        if (!clearEx(c.env) && using_) return;
    }

    // Évalué uniquement au début d'un tick : au maximum un clic par tick,
    // sans rattrapage en rafale si le jeu a pris du retard.
    const auto interval = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(1.0 / cps_));
    next_ = (next_ == clock::time_point{} || now - next_ > interval) ? now + interval : next_ + interval;

    click(c);
}

void AutoClicker::click(Ctx& c) {
    JNIEnv* env = c.env;

    // Entité au-delà de la portée vanilla : le Reach envoie l'attaque lui-même.
    if (modules::reach().tryAttack(c, true)) return;

    if (breakBlocks_) {
        jobject mop = env->GetObjectField(c.mc, F.mc_objectMouseOver);
        bool onBlock = false;
        if (mop) {
            jobject type = env->GetObjectField(mop, F.mop_typeOfHit);
            onBlock = type && env->IsSameObject(type, BLOCK_TYPE);
            if (type) env->DeleteLocalRef(type);
            env->DeleteLocalRef(mop);
        }
        // Le clic maintenu est géré par vanilla (sendClickBlockToController) : on ne casse pas le minage.
        if (onBlock) return;
    }

    // clickMouse() ignore le clic si leftClickCounter > 0 (cooldown de 10 ticks après un clic dans le vide).
    env->SetIntField(c.mc, F.mc_leftClickCounter, 0);
    env->CallVoidMethod(c.mc, M.mc_clickMouse);
    clearEx(env);
}

void AutoClicker::drawSettings() {
    ImGui::SliderFloat("CPS", &cps_, 1.0f, 20.0f, "%.1f");
    ImGui::Checkbox("Laisser casser les blocs", &breakBlocks_);
    ImGui::Checkbox("Pause en utilisant un objet", &pauseWhileUsing_);
}

