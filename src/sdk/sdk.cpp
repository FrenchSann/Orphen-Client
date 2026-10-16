#include "sdk/sdk.hpp"
#include "sdk/classloader.hpp"
#include "core/log.hpp"

namespace sdk {

namespace {
    bool g_ok = true;
    jobject g_loader = nullptr;

    jclass cls(JNIEnv* env, const char* name) {
        jclass local = loadClass(env, g_loader, name);
        if (clearEx(env) || !local) {
            LOG("classe introuvable : %s", name);
            g_ok = false;
            return nullptr;
        }
        auto global = static_cast<jclass>(env->NewGlobalRef(local));
        env->DeleteLocalRef(local);
        if (clearEx(env) || !global) g_ok = false;
        return global;
    }

    jfieldID fld(JNIEnv* env, jclass c, const char* name, const char* sig, bool isStatic = false) {
        if (!c) return nullptr;
        jfieldID id = isStatic ? env->GetStaticFieldID(c, name, sig) : env->GetFieldID(c, name, sig);
        if (clearEx(env) || !id) {
            LOG("champ introuvable : %s %s", name, sig);
            g_ok = false;
            return nullptr;
        }
        return id;
    }

    jmethodID mth(JNIEnv* env, jclass c, const char* name, const char* sig, bool isStatic = false) {
        if (!c) return nullptr;
        jmethodID id = isStatic ? env->GetStaticMethodID(c, name, sig) : env->GetMethodID(c, name, sig);
        if (clearEx(env) || !id) {
            LOG("méthode introuvable : %s %s", name, sig);
            g_ok = false;
            return nullptr;
        }
        return id;
    }
}

bool init(JNIEnv* env) {
    g_ok = true;
    jobject loader = findGameLoader(env);
    if (!loader) {
        LOG("Chargeur Lunar Minecraft introuvable");
        return false;
    }
    g_loader = env->NewGlobalRef(loader);
    env->DeleteLocalRef(loader);
    if (clearEx(env) || !g_loader) return false;

    C.Minecraft         = cls(env, "net/minecraft/client/Minecraft");
    C.MovingObjectPos   = cls(env, "net/minecraft/util/MovingObjectPosition");
    C.MovingObjectType  = cls(env, "net/minecraft/util/MovingObjectPosition$MovingObjectType");
    C.Vec3              = cls(env, "net/minecraft/util/Vec3");
    C.Entity            = cls(env, "net/minecraft/entity/Entity");
    C.EntityLivingBase  = cls(env, "net/minecraft/entity/EntityLivingBase");
    C.EntityPlayer      = cls(env, "net/minecraft/entity/player/EntityPlayer");
    C.AxisAlignedBB     = cls(env, "net/minecraft/util/AxisAlignedBB");
    C.World             = cls(env, "net/minecraft/world/World");
    C.PlayerController  = cls(env, "net/minecraft/client/multiplayer/PlayerControllerMP");
    C.InventoryPlayer   = cls(env, "net/minecraft/entity/player/InventoryPlayer");
    C.Container         = cls(env, "net/minecraft/inventory/Container");
    C.ItemStack         = cls(env, "net/minecraft/item/ItemStack");
    C.ItemArmor         = cls(env, "net/minecraft/item/ItemArmor");
    C.EnchantmentHelper = cls(env, "net/minecraft/enchantment/EnchantmentHelper");
    C.TileEntity        = cls(env, "net/minecraft/tileentity/TileEntity");
    C.Vec3i             = cls(env, "net/minecraft/util/Vec3i");
    C.TileChest         = cls(env, "net/minecraft/tileentity/TileEntityChest");
    C.TileDispenser     = cls(env, "net/minecraft/tileentity/TileEntityDispenser");
    C.TileDropper       = cls(env, "net/minecraft/tileentity/TileEntityDropper");
    C.TileHopper        = cls(env, "net/minecraft/tileentity/TileEntityHopper");
    C.GuiInventory      = cls(env, "net/minecraft/client/gui/inventory/GuiInventory");
    C.ActiveRenderInfo  = cls(env, "net/minecraft/client/renderer/ActiveRenderInfo");
    C.RenderManager     = cls(env, "net/minecraft/client/renderer/entity/RenderManager");
    C.List              = cls(env, "java/util/List");

    // --- Minecraft ---
    F.mc_thePlayer        = fld(env, C.Minecraft, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;");
    F.mc_theWorld         = fld(env, C.Minecraft, "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;");
    F.mc_playerController = fld(env, C.Minecraft, "playerController", "Lnet/minecraft/client/multiplayer/PlayerControllerMP;");
    F.mc_objectMouseOver  = fld(env, C.Minecraft, "objectMouseOver", "Lnet/minecraft/util/MovingObjectPosition;");
    F.mc_currentScreen    = fld(env, C.Minecraft, "currentScreen", "Lnet/minecraft/client/gui/GuiScreen;");
    F.mc_leftClickCounter = fld(env, C.Minecraft, "leftClickCounter", "I");
    F.mc_renderManager    = fld(env, C.Minecraft, "renderManager", "Lnet/minecraft/client/renderer/entity/RenderManager;");
    F.mc_inGameHasFocus   = fld(env, C.Minecraft, "inGameHasFocus", "Z");
    M.mc_getMinecraft        = mth(env, C.Minecraft, "getMinecraft", "()Lnet/minecraft/client/Minecraft;", true);
    M.mc_clickMouse          = mth(env, C.Minecraft, "clickMouse", "()V");
    M.mc_runTick             = mth(env, C.Minecraft, "runTick", "()V");
    M.mc_isGamePaused        = mth(env, C.Minecraft, "isGamePaused", "()Z");
    M.mc_isSingleplayer      = mth(env, C.Minecraft, "isSingleplayer", "()Z");
    M.mc_setIngameFocus      = mth(env, C.Minecraft, "setIngameFocus", "()V");
    M.mc_setIngameNotInFocus = mth(env, C.Minecraft, "setIngameNotInFocus", "()V");

    // --- MovingObjectPosition ---
    F.mop_typeOfHit = fld(env, C.MovingObjectPos, "typeOfHit", "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;");
    F.mop_hitVec    = fld(env, C.MovingObjectPos, "hitVec", "Lnet/minecraft/util/Vec3;");
    F.mot_BLOCK     = fld(env, C.MovingObjectType, "BLOCK", "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;", true);

    // --- Vec3 / AABB ---
    F.vec_x = fld(env, C.Vec3, "xCoord", "D");
    F.vec_y = fld(env, C.Vec3, "yCoord", "D");
    F.vec_z = fld(env, C.Vec3, "zCoord", "D");
    F.bb_minX = fld(env, C.AxisAlignedBB, "minX", "D");
    F.bb_minY = fld(env, C.AxisAlignedBB, "minY", "D");
    F.bb_minZ = fld(env, C.AxisAlignedBB, "minZ", "D");
    F.bb_maxX = fld(env, C.AxisAlignedBB, "maxX", "D");
    F.bb_maxY = fld(env, C.AxisAlignedBB, "maxY", "D");
    F.bb_maxZ = fld(env, C.AxisAlignedBB, "maxZ", "D");

    // --- Entity ---
    M.entity_getPositionEyes        = mth(env, C.Entity, "getPositionEyes", "(F)Lnet/minecraft/util/Vec3;");
    M.entity_getLook                = mth(env, C.Entity, "getLook", "(F)Lnet/minecraft/util/Vec3;");
    M.entity_rayTrace               = mth(env, C.Entity, "rayTrace", "(DF)Lnet/minecraft/util/MovingObjectPosition;");
    M.entity_getEntityBoundingBox   = mth(env, C.Entity, "getEntityBoundingBox", "()Lnet/minecraft/util/AxisAlignedBB;");
    M.entity_getCollisionBorderSize = mth(env, C.Entity, "getCollisionBorderSize", "()F");
    M.entity_canBeCollidedWith      = mth(env, C.Entity, "canBeCollidedWith", "()Z");
    M.living_swingItem              = mth(env, C.EntityLivingBase, "swingItem", "()V");
    M.player_isUsingItem            = mth(env, C.EntityPlayer, "isUsingItem", "()Z");

    // --- World ---
    F.world_loadedEntityList     = fld(env, C.World, "loadedEntityList", "Ljava/util/List;");
    F.world_loadedTileEntityList = fld(env, C.World, "loadedTileEntityList", "Ljava/util/List;");

    // --- PlayerControllerMP ---
    M.pc_attackEntity = mth(env, C.PlayerController, "attackEntity", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/entity/Entity;)V");
    M.pc_windowClick  = mth(env, C.PlayerController, "windowClick", "(IIIILnet/minecraft/entity/player/EntityPlayer;)Lnet/minecraft/item/ItemStack;");
    M.pc_extendedReach = mth(env, C.PlayerController, "extendedReach", "()Z");

    // --- Inventaire ---
    F.player_inventory          = fld(env, C.EntityPlayer, "inventory", "Lnet/minecraft/entity/player/InventoryPlayer;");
    F.player_inventoryContainer = fld(env, C.EntityPlayer, "inventoryContainer", "Lnet/minecraft/inventory/Container;");
    F.inv_mainInventory         = fld(env, C.InventoryPlayer, "mainInventory", "[Lnet/minecraft/item/ItemStack;");
    F.inv_armorInventory        = fld(env, C.InventoryPlayer, "armorInventory", "[Lnet/minecraft/item/ItemStack;");
    M.inv_getItemStack          = mth(env, C.InventoryPlayer, "getItemStack", "()Lnet/minecraft/item/ItemStack;");
    F.container_windowId        = fld(env, C.Container, "windowId", "I");
    M.stack_getItem             = mth(env, C.ItemStack, "getItem", "()Lnet/minecraft/item/Item;");
    F.armor_armorType           = fld(env, C.ItemArmor, "armorType", "I");
    F.armor_damageReduceAmount  = fld(env, C.ItemArmor, "damageReduceAmount", "I");
    M.ench_getEnchantmentLevel  = mth(env, C.EnchantmentHelper, "getEnchantmentLevel", "(ILnet/minecraft/item/ItemStack;)I", true);

    // --- TileEntity ---
    M.te_getPos          = mth(env, C.TileEntity, "getPos", "()Lnet/minecraft/util/BlockPos;");
    M.vec3i_getX         = mth(env, C.Vec3i, "getX", "()I");
    M.vec3i_getY         = mth(env, C.Vec3i, "getY", "()I");
    M.vec3i_getZ         = mth(env, C.Vec3i, "getZ", "()I");
    M.chest_getChestType = mth(env, C.TileChest, "getChestType", "()I");

    // --- Rendu ---
    F.ari_MODELVIEW  = fld(env, C.ActiveRenderInfo, "MODELVIEW", "Ljava/nio/FloatBuffer;", true);
    F.ari_PROJECTION = fld(env, C.ActiveRenderInfo, "PROJECTION", "Ljava/nio/FloatBuffer;", true);
    F.rm_renderPosX  = fld(env, C.RenderManager, "renderPosX", "D");
    F.rm_renderPosY  = fld(env, C.RenderManager, "renderPosY", "D");
    F.rm_renderPosZ  = fld(env, C.RenderManager, "renderPosZ", "D");

    // --- java.util.List ---
    M.list_size = mth(env, C.List, "size", "()I");
    M.list_get  = mth(env, C.List, "get", "(I)Ljava/lang/Object;");

    if (g_ok && C.MovingObjectType && F.mot_BLOCK) {
        jobject block = env->GetStaticObjectField(C.MovingObjectType, F.mot_BLOCK);
        if (clearEx(env) || !block) g_ok = false;
        else {
            BLOCK_TYPE = env->NewGlobalRef(block);
            env->DeleteLocalRef(block);
            if (clearEx(env) || !BLOCK_TYPE) g_ok = false;
        }
    }

    LOG(g_ok ? "SDK initialisé" : "SDK : des lookups ont échoué (voir ci-dessus)");
    if (!g_ok) shutdown(env);
    return g_ok;
}

void shutdown(JNIEnv* env) {
    auto** begin = reinterpret_cast<jclass*>(&C);
    auto** end   = reinterpret_cast<jclass*>(&C + 1);
    for (auto** p = begin; p != end; ++p) {
        if (*p) env->DeleteGlobalRef(*p);
        *p = nullptr;
    }
    if (BLOCK_TYPE) env->DeleteGlobalRef(BLOCK_TYPE);
    BLOCK_TYPE = nullptr;
    if (g_loader) env->DeleteGlobalRef(g_loader);
    g_loader = nullptr;
    F = {};
    M = {};
}

} // namespace sdk
