#include "modules/autoarmor.hpp"

using namespace sdk;

// Indices (1.8.9) :
//  ItemArmor.armorType      : 0 casque, 1 plastron, 2 jambières, 3 bottes
//  InventoryPlayer.armorInv : 0 bottes, 1 jambières, 2 plastron, 3 casque  -> index = 3 - type
//  ContainerPlayer (id 0)   : 5..8 armure (5 + type), 9..35 inventaire, 36..44 hotbar (inv 0..8)
namespace {
    constexpr int PROTECTION_ID = 0; // Enchantment.protection.effectId
    constexpr int CLICK_PICKUP = 0, CLICK_SHIFT = 1;

    int toContainerSlot(int invIndex) { return invIndex < 9 ? 36 + invIndex : invIndex; }
}

float AutoArmor::score(JNIEnv* env, jobject stack, int wantedType) {
    if (!stack) return -1.0f;
    jobject item = env->CallObjectMethod(stack, M.stack_getItem);
    if (clearEx(env) || !item) return -1.0f;

    float s = -1.0f;
    if (env->IsInstanceOf(item, C.ItemArmor) && env->GetIntField(item, F.armor_armorType) == wantedType) {
        const int reduce = env->GetIntField(item, F.armor_damageReduceAmount);
        const int prot = env->CallStaticIntMethod(C.EnchantmentHelper, M.ench_getEnchantmentLevel, PROTECTION_ID, stack);
        s = static_cast<float>(reduce) + (clearEx(env) ? 0.0f : static_cast<float>(prot));
    }
    env->DeleteLocalRef(item);
    return s;
}

void AutoArmor::onUpdate(Ctx& c) {
    JNIEnv* env = c.env;

    // Container 0 = inventaire du joueur : utilisable sans écran ouvert ou dans l'inventaire.
    if (c.screen && !env->IsInstanceOf(c.screen, C.GuiInventory)) return;

    const auto now = clock::now();
    if (now - last_ < std::chrono::milliseconds(delayMs_)) return;

    jobject inv = env->GetObjectField(c.player, F.player_inventory);
    jobject container = env->GetObjectField(c.player, F.player_inventoryContainer);
    if (!inv || !container) return;

    // Objet tenu au curseur (inventaire ouvert) : on ne touche à rien.
    jobject held = env->CallObjectMethod(inv, M.inv_getItemStack);
    if (clearEx(env) || held) return;

    const jint windowId = env->GetIntField(container, F.container_windowId);
    auto main = static_cast<jobjectArray>(env->GetObjectField(inv, F.inv_mainInventory));
    auto armor = static_cast<jobjectArray>(env->GetObjectField(inv, F.inv_armorInventory));
    if (!main || !armor) return;
    const jsize mainLen = env->GetArrayLength(main);

    auto clickSlot = [&](int slot, int mode) {
        jobject r = env->CallObjectMethod(c.controller, M.pc_windowClick, windowId, slot, 0, mode, c.player);
        clearEx(env);
        if (r) env->DeleteLocalRef(r);
    };

    for (int type = 0; type < 4; ++type) {
        jobject worn = env->GetObjectArrayElement(armor, 3 - type);
        const float wornScore = score(env, worn, type);

        int bestIdx = -1;
        float bestScore = wornScore;
        for (jsize i = 0; i < mainLen; ++i) {
            jobject s = env->GetObjectArrayElement(main, i);
            if (!s) continue;
            const float sc = score(env, s, type);
            if (sc > bestScore) {
                bestScore = sc;
                bestIdx = i;
            }
            env->DeleteLocalRef(s);
        }

        if (bestIdx >= 0) {
            const int from = toContainerSlot(bestIdx);
            if (!worn) {
                clickSlot(from, CLICK_SHIFT); // slot vide : shift-clic l'envoie directement dans l'armure
            } else {
                clickSlot(from, CLICK_PICKUP);     // prend la nouvelle pièce
                clickSlot(5 + type, CLICK_PICKUP); // échange avec la pièce portée
                clickSlot(from, CLICK_PICKUP);     // repose l'ancienne à la place
            }
            last_ = now;
            if (worn) env->DeleteLocalRef(worn);
            return; // une pièce par délai
        }
        if (worn) env->DeleteLocalRef(worn);
    }
}

void AutoArmor::drawSettings() {
    ImGui::SliderInt("Délai (ms)", &delayMs_, 0, 1000);
}
