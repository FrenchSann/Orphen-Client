#include "modules/reach.hpp"
#include "modules/autoclicker.hpp"
#include "modules/registry.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdint>

namespace modules {
Reach& reach() { static Reach value; return value; }
AutoClicker& autoClicker() { static AutoClicker value; return value; }
}
namespace {
using namespace sdk;
bool usingItem = false, blocked = false, pending = false;
bool failUsing = false, failSwing = false, failAttack = false;
int attacks = 0, swings = 0;
double distance = 3.5;
int objects[16]{};
jobject ref(int n) { return reinterpret_cast<jobject>(&objects[n]); }
void require(bool value, const char* text) {
    if (!value) { std::fprintf(stderr, "%s\n", text); std::exit(1); }
}
jmethodID method(int n) { return reinterpret_cast<jmethodID>(static_cast<uintptr_t>(n)); }
jfieldID field(int n) { return reinterpret_cast<jfieldID>(static_cast<uintptr_t>(n)); }
jboolean JNICALL exception(JNIEnv*) { return pending; }
void JNICALL clear(JNIEnv*) { pending = false; }
void JNICALL deleteRef(JNIEnv*, jobject) {}
jobject JNICALL copyRef(JNIEnv*, jobject obj) { return obj; }
jboolean JNICALL same(JNIEnv*, jobject a, jobject b) { return a == b; }
jboolean JNICALL booleanCall(JNIEnv*, jobject, jmethodID id, va_list) {
    if (id == M.player_isUsingItem) { pending = failUsing; return usingItem; }
    if (id == M.entity_canBeCollidedWith) return true;
    if (id == M.pc_extendedReach) return false;
    require(false, "Unexpected boolean call"); return false;
}
jobject JNICALL objectCall(JNIEnv*, jobject, jmethodID id, va_list) {
    if (id == M.entity_getPositionEyes) return ref(4);
    if (id == M.entity_getLook) return ref(5);
    if (id == M.entity_rayTrace) return blocked ? ref(9) : nullptr;
    if (id == M.entity_getEntityBoundingBox) return ref(7);
    if (id == M.list_get) return ref(6);
    require(false, "Unexpected object call"); return nullptr;
}
jint JNICALL intCall(JNIEnv*, jobject, jmethodID id, va_list) {
    require(id == M.list_size, "Unexpected integer call"); return 1;
}
jfloat JNICALL floatCall(JNIEnv*, jobject, jmethodID id, va_list) {
    require(id == M.entity_getCollisionBorderSize, "Unexpected float call"); return 0.0f;
}
void JNICALL voidCall(JNIEnv*, jobject, jmethodID id, va_list) {
    if (id == M.living_swingItem) { ++swings; pending = failSwing; return; }
    require(id == M.pc_attackEntity, "Unexpected void call");
    require(swings == 1, "Animation must precede the extended attack");
    ++attacks; pending = failAttack;
}
jobject JNICALL objectField(JNIEnv*, jobject, jfieldID id) {
    if (id == F.world_loadedEntityList) return ref(8);
    if (id == F.mop_typeOfHit) return BLOCK_TYPE;
    if (id == F.mop_hitVec) return ref(10);
    require(false, "Unexpected object field"); return nullptr;
}
jdouble JNICALL doubleField(JNIEnv*, jobject obj, jfieldID id) {
    if (obj == ref(4)) return id == F.vec_y ? 1.6 : 0.0;
    if (obj == ref(5)) return id == F.vec_x ? 1.0 : 0.0;
    if (obj == ref(10)) return id == F.vec_x ? 3.0 : id == F.vec_y ? 1.6 : 0.0;
    require(obj == ref(7), "Unexpected bounding box object");
    if (id == F.bb_minX) return distance;
    if (id == F.bb_maxX) return distance + 1.0;
    if (id == F.bb_minY) return 0;
    if (id == F.bb_maxY) return 2;
    return id == F.bb_minZ ? -0.5 : 0.5;
}
void reset() {
    attacks = swings = 0; pending = usingItem = blocked = false;
    failUsing = failSwing = failAttack = false; distance = 3.5;
}
}
int main() {
    using namespace sdk;
    M.entity_getPositionEyes = method(1); M.entity_getLook = method(2);
    M.entity_rayTrace = method(3); M.entity_getEntityBoundingBox = method(4);
    M.entity_getCollisionBorderSize = method(5); M.entity_canBeCollidedWith = method(6);
    M.player_isUsingItem = method(7); M.pc_extendedReach = method(8);
    M.living_swingItem = method(9); M.pc_attackEntity = method(10);
    M.list_size = method(11); M.list_get = method(12);
    F.vec_x = field(1); F.vec_y = field(2); F.vec_z = field(3);
    F.bb_minX = field(4); F.bb_minY = field(5); F.bb_minZ = field(6);
    F.bb_maxX = field(7); F.bb_maxY = field(8); F.bb_maxZ = field(9);
    F.world_loadedEntityList = field(10); F.mop_typeOfHit = field(11); F.mop_hitVec = field(12);
    BLOCK_TYPE = ref(11);
    JNINativeInterface_ functions{};
    functions.ExceptionCheck = exception; functions.ExceptionClear = clear;
    functions.DeleteLocalRef = deleteRef; functions.NewLocalRef = copyRef; functions.IsSameObject = same;
    functions.CallBooleanMethodV = booleanCall; functions.CallObjectMethodV = objectCall;
    functions.CallIntMethodV = intCall; functions.CallFloatMethodV = floatCall;
    functions.CallVoidMethodV = voidCall; functions.GetObjectField = objectField; functions.GetDoubleField = doubleField;
    JNIEnv env{&functions};
    Ctx c; c.env = &env; c.player = ref(1); c.world = ref(2); c.controller = ref(3);
    Reach& reach = modules::reach(); reach.setEnabled(true);
    usingItem = true;
    require(!reach.tryAttack(c, true) && attacks == 0, "Reach must pause while using an item");
    reset(); failUsing = true;
    require(!reach.tryAttack(c, true) && attacks == 0 && !pending, "Failed item-state query must stop the attack");
    reset();
    require(reach.tryAttack(c, true) && attacks == 1 && swings == 1, "Extended 3.5-block attack must remain available");
    reset(); distance = 4.5;
    require(!reach.tryAttack(c, true) && attacks == 0, "Do not attack outside configured range");
    reset(); distance = 2.5;
    require(!reach.tryAttack(c, true) && attacks == 0, "Normal-range attacks must be left to Minecraft");
    reset(); blocked = true;
    require(!reach.tryAttack(c, true) && attacks == 0, "Do not attack through a closer block");
    reset(); failSwing = true;
    require(!reach.tryAttack(c, true) && attacks == 0 && !pending, "Failed animation must prevent attack");
    reset(); failAttack = true;
    require(!reach.tryAttack(c, true) && !pending, "Java attack failure must not be reported as success");
    std::puts("Reach: item-state guards, extended range, occlusion and JNI failures passed");
}
