# projet-tank-terminale-2026-2027 ###

Repo GitHub pour le projet de Tank RC des Terminales spécialités sciences de l'ingénieur. Contenu Open Source, et maintenu jusqu'en juillet 2027

⚠️ **Migration du firmware écrit en C++ avec les timings gérés à la main vers TankOS, un système basé sur FreeRTOS pour micro contrôleurs AVR** ⚠️

Le contenu du dossier /firmware/ ne sera pas supprimé mais ne sera plus mis à jour.
La raison de cette mise a jour est la facilitation de la gestion des timing sur la carte.
La dernière version du firmware sera la 2026.6_PUBLIC_PREVIEW et laissera place à TankOS 1.
La première version de TankOS sera publiée le 15 septembre au plus tard.

### Configuration Requise ###

Ce projet est basé autour de l'Arduino Mega 2560 programmé via VS Code et son extension PlatformIO.
Sur chaque release un fichier .zip a build et un .hex pouvant être uploadé directement sera disponible.
Le fichier d'environnement platformio.ini est publié dans son dossier /PlatformIO/

### PCB Custom ###

Ce projet contient quelques cartes électroniques faites sur mesure. Les fichiers de celles-ci seront disponibles plus tard dans l'année, quand leur design sera validé et fonctionnel.
Ces designs ont étés fait sur KiCAD.

### Python Scripts ###

Ce projet contient un dossier réservé à des scripts python. Ils sont fait maison et permettent de calculer des caractéristiques en rapport avec les moteurs plus facilement, n'hesitez pas à vous en servir si besoin ;). Pour moins se casser la tête pour des valeurs par défaut, dans la première input, tapez "def" pour charger les valeurs par défaut du programme. Elles seront mises à jour si jamais elles changent au cours du projet.

### MatLab ###

Ce projet contient un dossier réservé aux modèles MatLab et aux calculs. Dans celui-ci vous pourrez trouver le(s) modèle(s) de simulation des moteurs, et la fiche de calcul qui justifie CHAQUE valeur de MatLab (autre que celle qu'on peut trouver dans les docs techniques, vous pouvez chercher quand même, là c'est que les calculs)

### User Manuals ###

Ce projet contient un dossier réservé aux manuels utilisateurs. Ces manuels seront répartis dans des dossiers selon le type d'élément qu'ils présentent (PCB par exemple)

### Feuilles de Calcul ###

Ce projet contient un dossier réservé aux feuilles de calculs pour le tank. Utile si besoin. (Feuilles manuscrites)

### Nom des versions ###

Deux types de firmware sont actuellement utilisés : 
  - Les _PUBLIC_PREVIEW (PR) -> firmware non-testés sur du materiel réel, soumis à des erreurs
  - Les _PUBLIC_RELEASE (REL) -> firmware testé sur du materiel et certifié sans erreur, soumis à des ajouts de fonctionnalité dans le futur

Chaque mise à jour est présentée sous la forme suivante : 20XX.Y_TYPE

20XX -> correspond à l'année d'écriture du firmware

.Y -> correspond à la version du firmware

_TYPE -> soit _PUBLIC_PREVIEW ou _PUBLIC_RELEASE

---
### Licence ###

Ce projet est distribué sous licence MIT pour le code. Les schémas électroniques et fichiers PCB sont mis à disposition sous licence CERN-OHL-P v2.
