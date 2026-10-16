# Versions compilées

Archives Windows x64 pour Lunar Client 1.8.9, suivies dans Git.

| Version source | Contenu | Téléchargement |
|---|---|---|
| `b422e1c` (dernière) | AutoClicker et Reach sur les ticks, contrôles d’état Reach | [Reach ticks](Orphen-Lunar-1.8.9-reach-ticks-b422e1c.zip) |
| `7c02b29` (précédente) | Support Lunar, multijoueur et AutoClicker sur les ticks | [AutoClicker ticks](Orphen-Lunar-1.8.9-autoclicker-ticks-7c02b29.zip) |

Extraire l’archive complète : **OrphenClient.exe**, **orphen.dll** et **orphen-bridge.jar**
doivent rester ensemble. Redémarrer Lunar avant de remplacer une version déjà injectée.

La version précédente provient de l’archive livrée au moment du commit ; ses sources
archivées ont été comparées au commit `7c02b29`. La dernière version a été recompilée
depuis `b422e1c` avant publication. Les tests Reach, mappings, chargeur JNI et ticks
ont réussi pour la dernière version. Aucune validation sur serveur ni garantie d’absence
de flags n’est annoncée.

Les [empreintes SHA-256](SHA256SUMS.txt) vérifient les archives. Chaque ZIP contient aussi
les empreintes de ses trois fichiers exécutables et les instructions de lancement.
