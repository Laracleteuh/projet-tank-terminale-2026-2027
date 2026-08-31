# projet-tank-terminale-2026-2027 ###

Repo GitHub pour le projet de Tank RC des Terminales spécialités sciences de l'ingénieur. Contenu Open Source, et maintenu jusqu'en juillet 2027

### Configuration Requise ###

Ce projet est basé autour de l'Arduino Mega 2560 programmé via VS Code et son extension PlatformIO.
Sur chaque release un fichier .zip a build et un .hex pouvant être uploadé directement sera disponible.
Le fichier d'environnement platformio.ini est publié dans son dossier /PlatformIO/

### PCB Custom ###

Ce projet contient quelques cartes électroniques faites sur mesure. Les fichiers de celles-ci seront disponibles plus tard dans l'année, quand leur design sera validé et fonctionnel.
Ces designs ont étés fait sur KiCAD.

### Nom des versions

Deux types de firmware sont actuellement utilisés : 
  - Les _PUBLIC_PREVIEW (PR) -> firmware non-testés sur du materiel réel, soumis à des erreurs
  - Les _PUBLIC_RELEASE (REL) -> firmware testé sur du materiel et certifié sans erreur, soumis à des ajouts de fonctionnalité dans le futur

Chaque mise à jour est présentée sous la forme suivante : 20XX.Y_TYPE
20XX -> corresponds to the year
.Y -> corresponds to the version of the software released
_TYPE -> either _PUBLIC_PREVIEW or _PUBLIC_RELEASE

---
### Licence ###

Ce projet est distribué sous licence MIT pour le code. Les schémas électroniques et fichiers PCB sont mis à disposition sous licence CERN-OHL-P v2.
