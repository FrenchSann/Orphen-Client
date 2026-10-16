# Orphen Client

DLL C++ injectée dans Minecraft **1.8.9 vanilla**, avec un menu ImGui, des hooks MinHook et les modules en JNI.
La version est figée : les noms obfusqués sont codés en dur dans `src/sdk/sdk.cpp`
(mappings MCP 1.8.9 / stable_22, vérifiés contre `1.8.9.jar`).

Les modules ne tournent qu'en **solo** (`Minecraft.isSingleplayer()`). En multijoueur ils restent inactifs.

> **Vanilla uniquement.** Lunar Client, Badlion, Forge et OptiFine renomment ou modifient les classes
> du jeu : les mappings ne correspondent plus et l'injecteur refuse de s'y attacher.
> Lance la 1.8.9 depuis le launcher Minecraft officiel.

## Build

Il faut Visual Studio 2022 (C++), CMake ≥ 3.20, et une build en x64.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Ça produit `build/bin/orphen.dll` et `build/bin/OrphenClient.exe`.

## Utilisation

1. Lance Minecraft 1.8.9 (profil vanilla) et ouvre un monde solo.
2. Lance `build/bin/OrphenClient.exe`. Il cherche la fenêtre « Minecraft 1.8.9 » et charge `orphen.dll`.
3. En jeu :
   - **Inser** : ouvre ou ferme le menu (la souris est libérée pendant que le menu est ouvert).
   - **Fin** avec le menu ouvert, ou le bouton « Décharger » : retire la DLL proprement.
   - Chaque module peut avoir une touche (bouton « Touche » dans le menu, Échap pour aucune).

Les erreurs éventuelles (mapping introuvable, etc.) sont écrites dans `%TEMP%\orphen.log`.

## Modules

| Module | Ce qu'il fait |
|---|---|
| **AutoClicker** | Tant que le clic gauche est maintenu, appelle `Minecraft.clickMouse()` au CPS réglé. Quand le viseur est sur un bloc, il ne clique pas et laisse le minage vanilla se faire, donc on peut toujours casser des blocs. Il se met en pause quand on utilise un objet (manger, bloquer, arc). |
| **Reach** | Raycast sur les entités jusqu'à la portée réglée (3 à 6 blocs), coupé par les blocs comme en vanilla. Si la cible est au-delà de la portée vanilla (3, ou 6 en créatif), il appelle `PlayerControllerMP.attackEntity`. Ça marche au clic manuel et avec l'AutoClicker. Au-delà de 6 blocs, le serveur intégré refuse l'attaque, d'où le plafond. |
| **AutoArmor** | Équipe la meilleure pièce de l'inventaire. Score = points d'armure + niveau de Protection. Il agit sans écran ouvert ou dans l'inventaire, une pièce par délai. |
| **StorageESP** | Boîtes 3D et tracers pour les coffres, coffres piégés, droppers, dispensers et hoppers. Couleur et activation se règlent par type, avec une distance max. La projection utilise les matrices de `ActiveRenderInfo`. |

## Structure

```
src/
  dllmain.cpp          thread d'init : récupère la JavaVM, installe le hook
  core/hooks.cpp       hook wglSwapBuffers + WndProc : tout tourne sur le thread du jeu
  core/state.hpp       état partagé menu / entrées
  sdk/sdk.cpp          classes, champs et méthodes obfusqués (global refs + IDs)
  sdk/game.hpp         Ctx par frame + helpers (Vec3, List)
  gui/menu.cpp         menu ImGui
  modules/             un fichier par module, interface commune dans module.hpp
injector/main.cpp      injecteur LoadLibrary
external/              imgui v1.91.9, minhook, jni.h (jdk8u)
```

### Ajouter un module

1. Hérite de `Module` (`modules/module.hpp`) et implémente `onUpdate`, `onRender` et/ou `drawSettings`.
2. Ajoute l'instance dans `modules/registry.cpp`.
3. Si tu as besoin de nouveaux champs ou méthodes, ajoute-les dans `sdk.hpp` / `sdk.cpp`.
   Pour retrouver un nom obfusqué : `joined.srg` dans `mcp-1.8.9-srg.zip` donne notch → srg,
   puis `fields.csv` / `methods.csv` de `mcp_stable-22-1.8.9` donnent srg → nom MCP.

Toutes les références JNI locales d'une frame sont libérées par `PushLocalFrame` / `PopLocalFrame`
dans le hook, et chaque module est suivi d'un `ExceptionClear`.
