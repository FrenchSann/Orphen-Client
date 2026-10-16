# Orphen Client

DLL C++ pour **Lunar Client, Minecraft 1.8.9**, avec un menu ImGui, des hooks MinHook et des modules JNI.

Les classes, champs et méthodes du SDK utilisent les noms fournis par
`lunar/lunar_named_b5_1.8.9.kin` dans le fichier de mappings de Lunar installé.
Le SDK recherche le chargeur de Minecraft parmi les chargeurs de contexte des threads
Java, puis l'utilise pour charger les classes du jeu. Il n'utilise plus les noms vanilla
obfusqués (`ave`, `bew`, etc.). Si un mapping manque, les modules restent désactivés
et les références JNI sont libérées ; le détail est écrit dans `%TEMP%\orphen.log`.

Les modules sont disponibles **en solo et en multijoueur**, dès qu'un monde,
un joueur et un contrôleur sont présents. Aucun filtrage de serveur n'est appliqué.
Les contrôles du serveur sur les attaques et les inventaires continuent à s'appliquer.

L'injecteur cible uniquement une fenêtre de jeu **Lunar Client 1.8.9** (LWJGL).
Le launcher Lunar seul et Minecraft vanilla ne sont pas des cibles compatibles.
Les autres versions de Minecraft ne sont pas prises en charge.

## Build

Il faut Visual Studio 2022 (C++), CMake ≥ 3.20, un **JDK 11 ou supérieur**
(`JAVA_HOME` doit pointer vers le JDK), et une build en x64.

```
git submodule update --init --recursive
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Ça produit `build/bin/orphen.dll`, `build/bin/OrphenClient.exe` et
`build/bin/orphen-bridge.jar`. Conserve ces trois fichiers ensemble.

## Utilisation

1. Lance Minecraft 1.8.9 dans Lunar Client et ouvre un monde ou rejoins un serveur.
2. Lance `build/bin/OrphenClient.exe`. Il cherche la fenêtre « Lunar Client 1.8.9 » et charge `orphen.dll`.
3. En jeu :
   - **Inser** : ouvre ou ferme le menu (la souris est libérée pendant que le menu est ouvert).
   - **Fin** avec le menu ouvert, ou le bouton « Décharger » : retire la DLL proprement.
   - Chaque module peut avoir une touche (bouton « Touche » dans le menu, Échap pour aucune).

Les erreurs éventuelles (mapping introuvable, etc.) sont écrites dans `%TEMP%\orphen.log`.

## Modules

| Module | Ce qu'il fait |
|---|---|
| **AutoClicker** | Évalué à l'entrée de `Minecraft.runTick()`, jamais depuis le rendu. Tant que le clic gauche est maintenu, appelle `Minecraft.clickMouse()` au CPS réglé, au maximum une fois par tick et sans rafale de rattrapage. Laisse le minage normal et propose une pause pendant l'utilisation d'un objet. |
| **Reach** | Raycast sur les entités jusqu'à la portée réglée (3 à 6 blocs), coupé par les blocs comme en vanilla. Si la cible est au-delà de la portée vanilla (3, ou 6 en créatif), il appelle `PlayerControllerMP.attackEntity`. Ça marche au clic manuel et avec l'AutoClicker. Au-delà de 6 blocs, le serveur intégré refuse l'attaque, d'où le plafond. |
| **AutoArmor** | Équipe la meilleure pièce de l'inventaire. Score = points d'armure + niveau de Protection. Il agit sans écran ouvert ou dans l'inventaire, une pièce par délai. |
| **StorageESP** | Boîtes 3D et tracers pour les coffres, coffres piégés, droppers, dispensers et hoppers. Couleur et activation se règlent par type, avec une distance max. La projection utilise les matrices de `ActiveRenderInfo`. |

## Structure

```
src/
  dllmain.cpp          thread d'init : récupère la JavaVM, installe le hook
  core/hooks.cpp       hook wglSwapBuffers + WndProc : tout tourne sur le thread du jeu
  core/tick_hook.cpp   instrumentation JVMTI de runTick, restauration au déchargement
  core/state.hpp       état partagé menu / entrées
  sdk/sdk.cpp          classes, champs et méthodes Lunar (global refs + IDs)
  sdk/game.hpp         Ctx par frame + helpers (Vec3, List)
  gui/menu.cpp         menu ImGui
  modules/             un fichier par module, interface commune dans module.hpp
injector/main.cpp      injecteur LoadLibrary
bridge/                callback Java et transformateur ASM isolé
external/              imgui, minhook, JNI/JVMTI, ASM 9.8
```

### Ajouter un module

1. Hérite de `Module` (`modules/module.hpp`) et implémente `onUpdate`, `onRender` et/ou `drawSettings`.
2. Ajoute l'instance dans `modules/registry.cpp`.
3. Si tu as besoin de nouveaux champs ou méthodes, ajoute-les dans `sdk.hpp` / `sdk.cpp`.
   Vérifie les noms et les signatures dans les mappings de la version Lunar ciblée.

Toutes les références JNI locales d'une frame sont libérées par `PushLocalFrame` / `PopLocalFrame`
dans le hook, et chaque module est suivi d'un `ExceptionClear`.

## Vérification

Validation des références du SDK contre les mappings Lunar installés (Python 3) :

```powershell
python tests/check_lunar_mappings.py
```

Un chemin de fichier JAR peut être fourni en argument. Aucun fichier Lunar n'est redistribué.
Pour tester le chargement JNI dans une JVM isolée, sans toucher au jeu :

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DORPHEN_BUILD_TESTS=ON
cmake --build build --config Release
python tests/run_classloader_test.py build/Release/classloader_test.exe "C:/chemin/vers/bin/server/jvm.dll"
```

Ce test génère une classe vide dans un répertoire temporaire, inaccessible à `FindClass`,
puis vérifie sa résolution par un chargeur Java distinct avec `-Xcheck:jni`.
Il ne remplace pas une validation des modules dans une session Lunar réelle.

Test du callback de tick dans une JVM isolée (utilise le même `jvm.dll`) :

```powershell
build/Release/tick_hook_test.exe "C:/chemin/vers/bin/server/jvm.dll" "$PWD/build/test-fixture" "$PWD/build/bin/orphen-bridge.jar"
```

Il vérifie l'absence de callback pendant le rendu ou depuis un autre thread, un callback
par tick avant le corps original, la restauration, la réinstallation sans doublon et
le nettoyage après un JAR absent ou une méthode incompatible.

## AutoClicker et ticks

Le callback est inséré au début de `runTick()V` par retransformation JVMTI. Le transformateur
ASM est chargé séparément des bibliothèques de Lunar ; le code original du tick est conservé.
Le SDK et les contrôles d'état restent sur le thread principal. Le menu, une perte de focus,
l'absence de monde, un écran ouvert ou une pause du jeu suspendent l'AutoClicker.
Les autres modules conservent leur fonctionnement précédent.

Si le JAR manque ou si la JVM refuse la retransformation, l'AutoClicker reste indisponible
et le menu l'indique : aucun repli vers le rendu. Au déchargement, une retransformation
sans notre callback restaure la classe en préservant les transformations des autres agents.
Si cette restauration échoue, la DLL reste chargée et l'erreur est journalisée.

La compilation et les tests isolés ne prouvent pas la disparition d'un flag sur un serveur.
Cette modification concerne le déclenchement de l'AutoClicker ; elle ne change pas la portée.
