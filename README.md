# Présentation
Pour la deuxième année consécutive, je me suis lancé le défi de faire un jeu pour chaque année depuis mon année de naissance. Je suis né en 1991, donc je fais un jeu de 91, puis un de 92, etc ... J'essaie de me chronométrer sur chacun pour voir combien de temps je mets à les finir mais aussi à faire la liste complète.
Ce projet est parti d'un souhait de ma part de gérer un fichier [LiveSplit](https://livesplit.org/ "Logiciel de chronométrage utilisé par les speedruners"). Je voulais ajouter un split (segment de temps) par session de jeu, il fallait donc que je modifier le .lss pour ajouter un split, et un json que j'avais à côté pour une extension de Livesplit qui permet de sauvegarder sa progression et la reprendre.
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
Il y a ensuite plusieurs boutons permettant de gérer la session en cours, ils s'activent ou non suivant la situation et peuvent aussi changer de texte:
- **Démarrer/Finir:** Cliquer sur démarrer lancera le chronomètre de la session. Si il est lancé, le bouton devient "Finir" et cliquer dessus termine non seulement la session mais aussi le jeu, changeant de fait son état et faisant apparaître une popup de fin de jeu dont on reparlera [plus tard](#terminer-un-jeu).
<p align="center"><img width="456" height="278" alt="image" src="https://github.com/user-attachments/assets/89b425b2-efb5-4994-abd3-c5cfbfb15e70" /></p>

# Utilisation
## Terminer un jeu
