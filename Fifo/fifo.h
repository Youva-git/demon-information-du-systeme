#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

//ce module manipule une file synchronisé circulaire partagée en mémoire .on a
//deux operation la lecture(le retrait) et l'écriture (l'insérsion).l'accés a
//cette section est donné au premier processus arrivéé entre le lecteur et les
//écrivain.
//pour l'écriture : un écrivain reste bloqué tant qu'il ne y'a pas assez
//d'espace libre dans la memoire partagée ou s'il y'a deja un autre écrivain.
//met fin a l'éxécution si la taille de ce qu'on veux écrire est plus grande que
// la taille du segment de mémoire partagée.
//pour la lecture: un lecteur reste bloqué si y'a rien n'est déja ecris sur la
//file ou si il y'a deja un ecrivain dans la file.

//fifo : structure capable de gérer une file synchronisé et qui contient nos
//variables dans la mémoire partagée.
typedef struct fifo {
  sem_t mutex1;
  sem_t mutex2;
  sem_t mutex3;
  sem_t plein;
  //indique si la taille de la commande est supérieur a la taille de la partie
  //libre du segment de mémoire partagée et inferieur a la taille maximal de
  //ce dernier.
  bool taille_depasser;
  //contient la taille de la chaine qui doit étre ajouté au tampon.
  size_t size_cmd;
  //Position d'ajout dans le tampon
  int push;
  //Position de suppression dans le tampon
  int pop;
  //Le tampon contenant les données
  char buffer[];
} fifo;

//fifo_empty : crée un segment de mémoire partagée et renvoie un pointeur vers
//vers ce segment.
fifo *fifo_empty(const char *shm_name);

//fifo_push : prend en paramaitre le segment de mémoire partagé
//pointé par s et une chaine de caractére pointé par src et ajoute cette chaine
//au tompon a partir de l'indice s->push.
//fifo_push : reste bloqué tant qu'il ne y'a pas assez d'espace libre dans la
//memoire partagée ou s'il y'a deja un autre écrivain.
//met fin a l'éxécution si la taille de src est plus grande que la taille du
//segment de mémoire partagée .
void fifo_push(fifo *s, char *src);

//fifo_pop : renvoie et retire une chaine de caractére qui contient une commande
//fifo_pop: reste bloqué si rien n'est déja ecris sur la file ou si il y'a
//deja un ecrivain dans la file.
char *fifo_pop(fifo *s);

//fifo_init : inistialise les variables de notre structure qui controle la
//mémoire partagée.
void fifo_init(fifo *s);
