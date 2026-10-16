#pragma once
#include <jni.h>

// Lunar Client 1.8.9 — noms vérifiés contre lunar_named_b5_1.8.9.kin.
// Les classes sont résolues par le chargeur de Minecraft dans la JVM de Lunar.
namespace sdk {

struct Classes {
    jclass Minecraft;
    jclass MovingObjectPos;
    jclass MovingObjectType;
    jclass Vec3;
    jclass Entity;
    jclass EntityLivingBase;
    jclass EntityPlayer;
    jclass AxisAlignedBB;
    jclass World;
    jclass PlayerController;
    jclass InventoryPlayer;
    jclass Container;
    jclass ItemStack;
    jclass ItemArmor;
    jclass EnchantmentHelper;
    jclass TileEntity;
    jclass Vec3i;
    jclass TileChest;
    jclass TileDispenser;
    jclass TileDropper;
    jclass TileHopper;
    jclass GuiInventory;
    jclass ActiveRenderInfo;
    jclass RenderManager;
    jclass List;               // java/util/List
};

struct Fields {
    // Minecraft
    jfieldID mc_thePlayer, mc_theWorld, mc_playerController, mc_objectMouseOver,
             mc_currentScreen, mc_leftClickCounter, mc_renderManager, mc_inGameHasFocus;
    // MovingObjectPosition
    jfieldID mop_typeOfHit, mop_hitVec;
    jfieldID mot_BLOCK; // static
    // Vec3
    jfieldID vec_x, vec_y, vec_z;
    // AxisAlignedBB
    jfieldID bb_minX, bb_minY, bb_minZ, bb_maxX, bb_maxY, bb_maxZ;
    // World
    jfieldID world_loadedEntityList, world_loadedTileEntityList;
    // EntityPlayer / Inventory / Container
    jfieldID player_inventory, player_inventoryContainer;
    jfieldID inv_mainInventory, inv_armorInventory;
    jfieldID container_windowId;
    // ItemArmor
    jfieldID armor_armorType, armor_damageReduceAmount;
    // ActiveRenderInfo (static)
    jfieldID ari_MODELVIEW, ari_PROJECTION;
    // RenderManager
    jfieldID rm_renderPosX, rm_renderPosY, rm_renderPosZ;
};

struct Methods {
    jmethodID mc_getMinecraft;     // static
    jmethodID mc_clickMouse, mc_isSingleplayer, mc_setIngameFocus, mc_setIngameNotInFocus;
    jmethodID mc_runTick, mc_isGamePaused;
    jmethodID entity_getPositionEyes, entity_getLook, entity_rayTrace,
              entity_getEntityBoundingBox, entity_getCollisionBorderSize, entity_canBeCollidedWith;
    jmethodID living_swingItem;
    jmethodID player_isUsingItem;
    jmethodID pc_attackEntity, pc_windowClick, pc_extendedReach;
    jmethodID inv_getItemStack;
    jmethodID stack_getItem;
    jmethodID ench_getEnchantmentLevel; // static
    jmethodID te_getPos;
    jmethodID vec3i_getX, vec3i_getY, vec3i_getZ;
    jmethodID chest_getChestType;
    jmethodID list_size, list_get;
};

inline Classes C{};
inline Fields  F{};
inline Methods M{};
inline jobject BLOCK_TYPE = nullptr; // global ref : MovingObjectType.BLOCK

// À appeler depuis le thread principal du jeu (JNIEnv valide). Retourne false si un lookup échoue.
bool init(JNIEnv* env);
void shutdown(JNIEnv* env);

// Efface une exception Java en attente. Retourne true s'il y en avait une.
inline bool clearEx(JNIEnv* env) {
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return true;
    }
    return false;
}

} // namespace sdk
