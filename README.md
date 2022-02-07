Programme qui se chargera de récupérer et traiterdes requêtes. Un client pourra émettre une requête pour demander :  
* Les informations concernant un utilisateur à partir de son uid ou de son nom (login, nom réel, groupe,﻿répertoire dédié, shell utilisé, . . .). 
* Les informations sur un processus en fonction de sonpid(commande, propriétaire, état, . . .).
* Toute commande système usuelle. 
  
## Aide d'utitilisation des programmes implémenter:  
  
1. makefile  
* Usage:  
  * make : pour créer les exécutables (genere-texte genere-mots ac-matrice ac-hachage).  
  * make clean : pour supprimer les fichiers .o et les exécutables généré.  
  
2. Lancer le serveur.  
./serveur
  
3. Lancer le client.  
./client [commande à traiter]  
  
## Démo:  
  
<img src="https://github.com/Youva-git/demon-information-du-systeme/blob/master/Ecommandes.gif">