#include "sdk/sdk.hpp"
#include "core/log.hpp"

namespace sdk {

namespace {
    bool g_ok = true;

    jclass cls(JNIEnv* env, const char* name) {
        jclass local = env->FindClass(name);
        if (!local || clearEx(env)) {
            LOG("classe introuvable : %s", name);
            g_ok = false;
            return nullptr;
        }
        auto global = static_cast<jclass>(env->NewGlobalRef(local));
        env->DeleteLocalRef(local);
        return global;
    }

    jfieldID fld(JNIEnv* env, jclass c, const char* name, const char* sig, bool isStatic = false) {
        if (!c) return nullptr;
        jfieldID id = isStatic ? env->GetStaticFieldID(c, name, sig) : env->GetFieldID(c, name, sig);
        if (!id || clearEx(env)) {
            LOG("champ introuvable : %s %s", name, sig);
            g_ok = false;
            return nullptr;
        }
        return id;
    }

    jmethodID mth(JNIEnv* env, jclass c, const char* name, const char* sig, bool isStatic = false) {
        if (!c) return nullptr;
        jmethodID id = isStatic ? env->GetStaticMethodID(c, name, sig) : env->GetMethodID(c, name, sig);
        if (!id || clearEx(env)) {
            LOG("méthode introuvable : %s %s", name, sig);
            g_ok = false;
            return nullptr;
        }
        return id;
    }
}

bool init(JNIEnv* env) {
    g_ok = true;

    C.Minecraft         = cls(env, "ave");
    C.MovingObjectPos   = cls(env, "auh");
    C.MovingObjectType  = cls(env, "auh$a");
    C.Vec3              = cls(env, "aui");
    C.Entity            = cls(env, "pk");
    C.EntityLivingBase  = cls(env, "pr");
    C.EntityPlayer      = cls(env, "wn");
    C.AxisAlignedBB     = cls(env, "aug");
    C.World             = cls(env, "adm");
    C.PlayerController  = cls(env, "bda");
    C.InventoryPlayer   = cls(env, "wm");
    C.Container         = cls(env, "xi");
    C.ItemStack         = cls(env, "zx");
    C.ItemArmor         = cls(env, "yj");
    C.EnchantmentHelper = cls(env, "ack");
    C.TileEntity        = cls(env, "akw");
    C.Vec3i             = cls(env, "df");
    C.TileChest         = cls(env, "aky");
    C.TileDispenser     = cls(env, "alc");
    C.TileDropper       = cls(env, "ald");
    C.TileHopper        = cls(env, "alj");
    C.GuiInventory      = cls(env, "azc");
    C.ActiveRenderInfo  = cls(env, "auz");
    C.RenderManager     = cls(env, "biu");
    C.List              = cls(env, "java/util/List");

    // --- Minecraft ---
    F.mc_thePlayer        = fld(env, C.Minecraft, "h",  "Lbew;");
    F.mc_theWorld         = fld(env, C.Minecraft, "f",  "Lbdb;");
    F.mc_playerController = fld(env, C.Minecraft, "c",  "Lbda;");
    F.mc_objectMouseOver  = fld(env, C.Minecraft, "s",  "Lauh;");
    F.mc_currentScreen    = fld(env, C.Minecraft, "m",  "Laxu;");
    F.mc_leftClickCounter = fld(env, C.Minecraft, "ag", "I");
    F.mc_renderManager    = fld(env, C.Minecraft, "aa", "Lbiu;");
    F.mc_inGameHasFocus   = fld(env, C.Minecraft, "w",  "Z");
    M.mc_getMinecraft        = mth(env, C.Minecraft, "A",  "()Lave;", true);
    M.mc_clickMouse          = mth(env, C.Minecraft, "aw", "()V");
    M.mc_isSingleplayer      = mth(env, C.Minecraft, "F",  "()Z");
    M.mc_setIngameFocus      = mth(env, C.Minecraft, "n",  "()V");
    M.mc_setIngameNotInFocus = mth(env, C.Minecraft, "o",  "()V");

    // --- MovingObjectPosition ---
    F.mop_typeOfHit = fld(env, C.MovingObjectPos, "a", "Lauh$a;");
    F.mop_hitVec    = fld(env, C.MovingObjectPos, "c", "Laui;");
    F.mot_BLOCK     = fld(env, C.MovingObjectType, "b", "Lauh$a;", true);

    // --- Vec3 / AABB ---
    F.vec_x = fld(env, C.Vec3, "a", "D");
    F.vec_y = fld(env, C.Vec3, "b", "D");
    F.vec_z = fld(env, C.Vec3, "c", "D");
    F.bb_minX = fld(env, C.AxisAlignedBB, "a", "D");
    F.bb_minY = fld(env, C.AxisAlignedBB, "b", "D");
    F.bb_minZ = fld(env, C.AxisAlignedBB, "c", "D");
    F.bb_maxX = fld(env, C.AxisAlignedBB, "d", "D");
    F.bb_maxY = fld(env, C.AxisAlignedBB, "e", "D");
    F.bb_maxZ = fld(env, C.AxisAlignedBB, "f", "D");

    // --- Entity ---
    M.entity_getPositionEyes        = mth(env, C.Entity, "e",  "(F)Laui;");
    M.entity_getLook                = mth(env, C.Entity, "d",  "(F)Laui;");
    M.entity_rayTrace               = mth(env, C.Entity, "a",  "(DF)Lauh;");
    M.entity_getEntityBoundingBox   = mth(env, C.Entity, "aR", "()Laug;");
    M.entity_getCollisionBorderSize = mth(env, C.Entity, "ao", "()F");
    M.entity_canBeCollidedWith      = mth(env, C.Entity, "ad", "()Z");
    M.living_swingItem              = mth(env, C.EntityLivingBase, "bw", "()V");
    M.player_isUsingItem            = mth(env, C.EntityPlayer, "bS", "()Z");

    // --- World ---
    F.world_loadedEntityList     = fld(env, C.World, "f", "Ljava/util/List;");
    F.world_loadedTileEntityList = fld(env, C.World, "h", "Ljava/util/List;");

    // --- PlayerControllerMP ---
    M.pc_attackEntity = mth(env, C.PlayerController, "a", "(Lwn;Lpk;)V");
    M.pc_windowClick  = mth(env, C.PlayerController, "a", "(IIIILwn;)Lzx;");
    M.pc_extendedReach = mth(env, C.PlayerController, "i", "()Z");

    // --- Inventaire ---
    F.player_inventory          = fld(env, C.EntityPlayer, "bi", "Lwm;");
    F.player_inventoryContainer = fld(env, C.EntityPlayer, "bj", "Lxi;");
    F.inv_mainInventory         = fld(env, C.InventoryPlayer, "a", "[Lzx;");
    F.inv_armorInventory        = fld(env, C.InventoryPlayer, "b", "[Lzx;");
    M.inv_getItemStack          = mth(env, C.InventoryPlayer, "p", "()Lzx;");
    F.container_windowId        = fld(env, C.Container, "d", "I");
    M.stack_getItem             = mth(env, C.ItemStack, "b", "()Lzw;");
    F.armor_armorType           = fld(env, C.ItemArmor, "b", "I");
    F.armor_damageReduceAmount  = fld(env, C.ItemArmor, "c", "I");
    M.ench_getEnchantmentLevel  = mth(env, C.EnchantmentHelper, "a", "(ILzx;)I", true);

    // --- TileEntity ---
    M.te_getPos          = mth(env, C.TileEntity, "v", "()Lcj;");
    M.vec3i_getX         = mth(env, C.Vec3i, "n", "()I");
    M.vec3i_getY         = mth(env, C.Vec3i, "o", "()I");
    M.vec3i_getZ         = mth(env, C.Vec3i, "p", "()I");
    M.chest_getChestType = mth(env, C.TileChest, "n", "()I");

    // --- Rendu ---
    F.ari_MODELVIEW  = fld(env, C.ActiveRenderInfo, "b", "Ljava/nio/FloatBuffer;", true);
    F.ari_PROJECTION = fld(env, C.ActiveRenderInfo, "c", "Ljava/nio/FloatBuffer;", true);
    F.rm_renderPosX  = fld(env, C.RenderManager, "o", "D");
    F.rm_renderPosY  = fld(env, C.RenderManager, "p", "D");
    F.rm_renderPosZ  = fld(env, C.RenderManager, "q", "D");

    // --- java.util.List ---
    M.list_size = mth(env, C.List, "size", "()I");
    M.list_get  = mth(env, C.List, "get", "(I)Ljava/lang/Object;");

    if (C.MovingObjectType && F.mot_BLOCK) {
        jobject block = env->GetStaticObjectField(C.MovingObjectType, F.mot_BLOCK);
        BLOCK_TYPE = env->NewGlobalRef(block);
        env->DeleteLocalRef(block);
    }

    LOG(g_ok ? "SDK initialisé" : "SDK : des lookups ont échoué (voir ci-dessus)");
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
}

} // namespace sdk
