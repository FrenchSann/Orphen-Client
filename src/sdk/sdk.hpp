#pragma once
#include <jni.h>

// Minecraft 1.8.9 vanilla — noms obfusqués (MCP 1.8.9 / stable_22, vérifiés contre 1.8.9.jar).
// Version figée : tout est codé en dur, aucune couche de compatibilité.
namespace sdk {

struct Classes {
    jclass Minecraft;          // ave  net.minecraft.client.Minecraft
    jclass MovingObjectPos;    // auh  util.MovingObjectPosition
    jclass MovingObjectType;   // auh$a
    jclass Vec3;               // aui
    jclass Entity;             // pk
    jclass EntityLivingBase;   // pr
    jclass EntityPlayer;       // wn
    jclass AxisAlignedBB;      // aug
    jclass World;              // adm
    jclass PlayerController;   // bda  client.multiplayer.PlayerControllerMP
    jclass InventoryPlayer;    // wm
    jclass Container;          // xi
    jclass ItemStack;          // zx
    jclass ItemArmor;          // yj
    jclass EnchantmentHelper;  // ack
    jclass TileEntity;         // akw
    jclass Vec3i;              // df   (BlockPos = cj extends df)
    jclass TileChest;          // aky
    jclass TileDispenser;      // alc
    jclass TileDropper;        // ald  (extends alc)
    jclass TileHopper;         // alj
    jclass GuiInventory;       // azc
    jclass ActiveRenderInfo;   // auz
    jclass RenderManager;      // biu
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
