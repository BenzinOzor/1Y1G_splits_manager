# Présentation
Pour la deuxième année consécutive, je me suis lancé le défi de faire un jeu pour chaque année depuis mon année de naissance. Je suis né en 1991, donc je fais un jeu de 91, puis un de 92, etc ... J'essaie de me chronométrer sur chacun pour voir combien de temps je mets à les finir mais aussi à faire la liste complète.

Ce projet est parti d'un souhait de ma part de gérer un fichier [**LiveSplit**](https://livesplit.org/ "Logiciel de chronométrage utilisé par les speedruners"). Je voulais ajouter un split (segment de temps) par session de jeu, il fallait donc que je modifier le .lss pour ajouter un split, et un json que j'avais à côté pour une extension de Livesplit qui permet de sauvegarder sa progression et la reprendre.

J'ai fini par totalement dépasser cette idée et n'utiliser que le Splits Manager et implémenter directement le chronométrage dedans.

# Interface
La fenêtre se découpe en deux parties:

- **À gauche:** Les différents jeux de la liste avec leur temps de complétion.
- **À droite:** Les informations du jeu en cours avec le chronomètre de la session et des statistiques en dessous.
<img width="921" height="870" alt="image" src="https://github.com/user-attachments/assets/62268573-a5cd-4ab7-9681-326ba5a29f87" />

## Partie gauche
Chaque section peut être d'une couleur différente représentant l'état courant du jeu:

- **🟦 Bleu:** Aucun état particulier. Aucune session n'a été faite sur ce jeu.
- **🟩 Vert:** Terminé.
- **🟥 Rouge:** Abandonné. Le temps passé sur le jeu sera pris en compte mais le delta entre temps passé et estimation ne le sera que si le temps passé à dépassé l'estimation.
- **🟨 Jaune:** Courant. Le jeu sur lequel les sessions faites via l'application seront ajoutées.
- **🟪 Violet:** En cours. Il peut y avoir plusieurs jeux en cours à la fois, cet état représente les jeux commencés en parallèle du courant. Par défaut à la première ouverture de la liste, le premier jeu en cours trouvé sera le courant.

Survoler le titre d'un jeu fera apparaître un tooltip contenant la jaquette du jeu et quelques statistiques.
<p align="center"><img width="380" height="209" alt="image" src="https://github.com/user-attachments/assets/64fd2321-e98f-44fc-8efe-d745e7b9671d" /></p>

Effectuer un clic droit sur un nom donnera accès à un menu contextuel permettant de changer l'état d'un jeu ou de lui attribuer ou retirer une jaquette. Un jeu déjà terminé ou abandonné ne peut pas être sélectionné pour être le courant.
<p align="center"><img width="237" height="93" alt="image" src="https://github.com/user-attachments/assets/378fd659-b5a3-4d5d-81ac-daa43573568d" /></p>

Effectuer un clic gauche sur un nom ouvre la section correspondante à un jeu. On peut d'abord y voir l'estimation du temps qu'il prendra à finir (modifiable directement ici si besoin) et le delta à côté si le jeu est terminé ou que le temps passé à dépassé l'estimation.
En dessous, on trouve une liste de toutes les sessions effectuées dans cet ordre:

- **Numéro de la session:** Le premier nombre indique le numéro local au jeu puis, entre parenthèses, le numéro global à toute la liste. Sur la capture d'écran, la première session est la première du jeu mais la 200ème de la liste. Les numéros de session globaux ne prennent pas en compte la chronologie dans laquelle les sessions ont été faites (peut-être plus tard), elles sont déterminées à la lecture du json contenant les infos de la liste.
- **Date de la session:** En utilisant l'application pour se chronométrer et mettre à jour la liste, les dates des sessions seront automatiquement ajoutées. La date est optionnelle si la session est ajoutée d'une autre manière.
- **Durée de la session:** Le temps qu'à duré la session de jeu suivant le format **"HH:MM:SS"**.
- **Temps de jeu total au moment de la session:** Le cumul de temps de toutes les sessions jusqu'à celle-ci.

Survoler la colonne de la date ou de la durée de la session mettra le texte en surbrillance. Il est possible de cliquer dessus et les éditer, le reste de l'application (cumul de temps et statistiques) s'adaptera directement aux nouvelles données entrées. Le format de la date pendant cette édition suis le format choisi dans les options.

La dernière ligne n'est visible que pour les jeux qui n'ont pas encore été terminés ou abandonnés. Il s'agit de deux champs textes permettant d'indiquer la durée (format **"HH:MM:SS"**) et la date (optionnelle, format ISO **"YYYY-MM-DD"**) d'une session effectuée hors de l'application, et d'indiquer l'état (En cours, Terminé ou Abandonné) du jeu après l'ajout de cette session. Toute durée ou date invalide empêchera la validation de la nouvelle session.
<p align="center"><img width="444" height="272" alt="image" src="https://github.com/user-attachments/assets/ff37e85d-d8cf-4d6d-89e5-e90248f92890" /></p>

## Partie droite
On y trouve d'abord le titre de la liste, puis les informations sur le jeu courant. Le titre du jeu se trouve juste sous le titre de la liste, avec sa jaquette en dessous si il lui en a été attribuée une, et les chronomètres à côté. Le plus gros représente la durée de la session, puis sont indiqués dessous le temps total passé sur le jeu, l'estimation du temps nécessaire pour le finir, puis le delta entre le temps passé et l'estimation qui peut servir de temps restant jusqu'à la fin supposée du jeu.
Il y a ensuite plusieurs boutons permettant de gérer la session en cours, leur utilisation est détaillée [**plus bas**](#gérer-le-chronomètre).
<p align="center"><img width="456" height="278" alt="image" src="https://github.com/user-attachments/assets/89b425b2-efb5-4994-abd3-c5cfbfb15e70" /></p>

Enfin, il reste la partie statistiques, que je me suis beaucoup amusé à remplir.
<p align="center"><img width="456" height="540" alt="image" src="https://github.com/user-attachments/assets/989b96a8-d3cd-4f1d-8e48-71f6371971ce" /></p>


# Utilisation
## Installation
Télécharger le fichier zip contenant la dernière version de l'application. Il contient un fichier appelé **Setup.exe** et un autre, Splits Manager.msi. Le .msi installera l'application seule alors que l'exe installera aussi toutes les dépendances nécessaires pour faire tout faire marcher. **Il est recommandé d'utiliser Setup.exe!**
Le reste de l'installation est classique, choisir l'endroit où installer l'application et tout devrait être opérationnel. L'exe permettant de la lancer se trouvera dans le dossier *"/1A1J Splits Manager/Bin/Release"*.

## Première mise en place
Au premier lancement, rien ne sera affiché car il faut donner une liste au Splits Manager. Si une liste a déjà été créée, passer directement à [**Gérer le fichier contenant la liste**](#gérer-le-fichier-contenant-la-liste). Pour créer une liste, aller dans *Fichier* en haut de la fenêtre puis *Créer*.
<p align="center"><img width="500" height="231" alt="image" src="https://github.com/user-attachments/assets/0c7d2bf6-47fb-4cb3-a443-d18b6d7851e4" /></p>

Pour l'instant, le seul moyen pratique de créer une liste est de copier coller le contenu d'une liste de participant·e sur le Google Sheet dédié, l'application assume que les informations viennent de là et va donc s'attendre à la présence de certaines informations dans ce qui va être collé dans la fenêtre.
Une feuille typique de 1 Année 1 Jeu ressemble à ça (certaines colonnes sont optionnelles):

<p align="center"><img width="1096" height="417" alt="image" src="https://github.com/user-attachments/assets/4a1f0e91-cd6e-4549-82c7-37c2f0f98aff" /></p>

Les colonnes utiles pour le Splits Manager vont de l'état de complétion au temps passé sur le jeu. Une fois la séléction dans la feuille faite et le contenu copié dans la fenêtre de création, il est possible de séléctionner quelles colonnes sont présentes dans le texte copié collé.

Les colonnes *Genre, Plateforme et Version* ne servent pas à l'application en tant que telles mais il est important de les cocher si elles font partie du texte collé car elle va s'en servir pour compter les colonnes et arriver à celles qui sont importantes pour en récupérer les informations. Il ne faut décocher les colonnes QUE si elles n'ont pas été copiées et ne sont pas présentes sur la feuille source, ce n'est pas une sélection de ce qui va être utilisé ou non dans l'application.

Dans le Google Sheet, l'année et le titre des jeux sont séparés, la dernière option cochable permet de fusionner les deux pour créer un nom sous le format `<année> - <titre>` si l'utilisateur·ice le désire.
Il ne reste plus qu'à cliquer sur le bouton *"Générer Liste de Jeux"* pour avoir un aperçu de ce que sera la liste dans l'application. À noter que chaque jeu n'aura qu'une session, le Splits Manager n'étant pas en mesure de les déduire depuis les informations données.

<p align="center"><img width="524" height="687" alt="image" src="https://github.com/user-attachments/assets/88e0b4b1-03a3-4362-a330-709ebd8e1e86" /></p>

Une fois la liste générée, il est possible de vérifier si tout à été bien généré dans une section en bas de la fenêtre de création, puis il ne reste plus qu'à confirmer la création en cliquant sur le bouton *"Confirmer"*.
<p align="center"><img width="560" height="275" alt="image" src="https://github.com/user-attachments/assets/48aaa018-d921-4694-a432-c263cf0912e2" /></p>

## Gérer le fichier contenant la liste
L'application peut gérer un fichier de liste de différentes manières, toutes accessibles depuis le menu *Fichier* en haut de la fenêtre:

- **Ouvrir...:** Permet de séléctionner un fichier de liste .json créé auparavant.
- **Enregistrer:** Modifie le .json précédemment ouvert avec les modifications effectuées pendant la session. Le fichier concerné sera indiqué en survolant cette option.
- **Enregistrer Sous...:** Enregistre la liste actuelle dans un nouveau fichier .json donc l'emplacement est choisi par l'utilisateur·ice.
- **Rafraichir Json:** Permet de recharger l'état de la liste au moment de la dernière sauvegarde.

Si le Splits Manager a été précédemment fermée alors qu'une liste avait été ouverte, cette dernière sera automatiquement réouverte à la prochaine utilisation de l'application.

<p align="center"><img width="191" height="163" alt="image" src="https://github.com/user-attachments/assets/1a64b7c9-b9c5-41d1-a112-34bed4bd1a18" /></p>


## Gérer le chronomètre
Le chronomètre se gère par les boutons situés dans la partie droite ou par [**raccourcis clavier**](#options-et-raccourcis-clavier).
Les boutons s'activent ou non suivant la situation et peuvent aussi changer de texte:
- **Démarrer/Finir:** Cliquer sur démarrer lancera le chronomètre de la session. Si il est lancé, le bouton devient "Finir" et cliquer dessus termine non seulement la session mais aussi le jeu, changeant de fait son état et faisant apparaître une popup de fin de jeu avec des statistiques. Finir un jeu passe le prochain non terminé en "Courant".
- **Pause/Reprendre:** Mettre en pause le chronomètre ou reprendre la session.
- **Arrêter:** Remettre le chronomètre à zéro et annuler la session en cours.

Quand la session est en pause, le bouton "mise à jour" à droite devient actif, permettant d'ajouter la session actuelle au jeu courant et change son état (En cours, Terminé, Abandonné) si besoin.

Le texte des chronomètres change de couleur en fonction de l'état de la session en cours:

- **Blanc:** Il n'y a pas de session en cours ou elle vient d'être arrêtée.
- **Vert:** Une session est en cours et le chronomètre défile.
- **Gris:** Une session est en cours mais le chronomètre a été mis en pause.
<p align="center"><img width="453" height="216" alt="image" src="https://github.com/user-attachments/assets/bbf6121b-40f7-4516-9603-9db391f6a89d" /></p>

## Options et raccourcis clavier
Un menu d'option est disponible dans le menu *Fichier* en haut de la fenêtre. Il permet par exemple de changer la langue de l'application et le format d'affichage des dates.

⚠️ Une option à garder en tête est celle des **Raccourcis Globaux**, permettant de détecter les raccourcis effectués en dehors de la fenêtre de l'application, quand elle n'a pas le focus. Très utile pour démarrer et arrêter le chronomètre sans avoir à réduire son jeu.

Des raccourcis clavier sont disponibles pour chacune des actions des boutons de la partie droite de l'application et ont le même fonctionnement. Il en existe aussi un pour sauvegarder les modifications apportées à la liste.

<p align="center"><img width="406" height="333" alt="image" src="https://github.com/user-attachments/assets/5f99a8e1-ab7c-4108-a44b-49ba4cbe4959" /></p>

NB: À l'heure où ce ReadMe est rédigé, le menu des options n'a pas été complètement traduit, ce sera fait dans une future mise à jour.
