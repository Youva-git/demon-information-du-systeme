#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include "fifo.h"

#define NOM_SHM "/SHM_FIFO_39841259"
#define TAILLE_SHM sizeof(fifo) * 2048

//nb_digits : prend comme paramaitre un entier et renvoie le nombre de chiffres
//de cette entier.
int nb_digit(int n);

//mon_tube : renvoie le nom du tube qui sera utilisé pour recevoir le resultat
//de l'execution de commande envoyé par le client qui est le pid du processus
//courant et crée un tube nommé avec ce nom.
char *mon_tube(void);

//cmd_exe : prend en paramaitre un entier argc un tableau de chaines argv une
//une chaine de caractére mon_tube et renvoie une chaine qui contient le nombre
//d'element de la chaine, le contenu de argv séparé par des espaces et
//mon _tube separé par -
char *cmd_exe(int argc, char *argv[], char *nom_tube);

//affiche_res : prend en paramaitre le nom d'un tube ,lit et affiche son contenu
//sur la sortie standard.
void affiche_res(char *nom_fifo);

void gestionnaire(int signum);

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    exit(EXIT_FAILURE);
  }
  //on ouvre le segment de mémoire partagée (la file synchronisé) et on
  //connecte le client.
  fifo *shm = fifo_empty(NOM_SHM);
  //on obtient le nom du tube nommé
  char *nom_tube = mon_tube();
  //on forme notre commande et les informations concernant sous forme d'une
  //seule chaine de caractére.
  char *cmd_exes = cmd_exe(argc, argv, nom_tube);
  //on ajoute notre commande a la file (mémoire partagée).
  fifo_push(shm, cmd_exes);
  //on affiche le resultat de l'execution de la commande celui qui est transmis
  //par le serveur sur l'entrée standard.
  affiche_res(nom_tube);
  free(nom_tube);
  free(cmd_exes);
  //on capture le signale envoyé par le serveur et on affiche un message de
  //deconnection.
  struct sigaction action;
  action.sa_handler = gestionnaire;
  action.sa_flags = 0;
  if (sigaction(SIGUSR1, &action, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

int nb_digit(int n) {
  int i = 1;
  while (n >= 10) {
    n /= 10;
    ++i;
  }
  return i;
}

char *mon_tube(void) {
  int nb_c_tube = (nb_digit((int) getpid()));
  char *nom_tube = malloc((size_t) nb_c_tube * sizeof(nom_tube));
  sprintf(nom_tube, "%d", getpid());
  if (mkfifo(nom_tube, S_IWUSR | S_IRUSR) == -1) {
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }
  return nom_tube;
}

char *cmd_exe(int argc, char **argv, char *nom_tube) {
  int i = 0;
  for (int j = 1; j < argc; ++j) {
    i += (int) strlen(argv[j]);
  }
  char *cmd = malloc((i + (int) strlen(nom_tube)) * sizeof(cmd));
  int e = 1;
  int l = 0;
  while (e < argc) {
    for (size_t j = 0; j < strlen(argv[e]); ++j) {
      cmd[l] = argv[e][j];
      ++l;
    }
    ++e;
    if (e == argc) {
      cmd[l] = '&';
      ++l;
      for (size_t j = 0; j < strlen(nom_tube); ++j) {
        cmd[l] = nom_tube[j];
        ++l;
      }
      cmd[l] = ';';
      ++l;
      cmd[l] = '\0';
      ++l;
    } else {
      cmd[l] = ' ';
      ++l;
    }
  }

  int cmd_size = nb_digit(argc);
  char *cmd_exe = malloc((size_t) (l + cmd_size) * sizeof(cmd_exe));
  sprintf(cmd_exe, "%d %s", argc, cmd);
  free(cmd);
  return cmd_exe;
}

void affiche_res(char *nom_fifo) {
  int fd = open(nom_fifo, O_RDONLY);
  if (fd == -1) {
    perror("open");
    close(fd);
    unlink(nom_fifo);
    exit(EXIT_FAILURE);
  }
  char c;
  ssize_t n;
  while ((n = read(fd, &c, sizeof(c))) > 0) {
    if (write(STDIN_FILENO, &c, sizeof(c)) == -1) {
      close(fd);
      unlink(nom_fifo);
      perror("write");
      exit(EXIT_FAILURE);
    }
  }
  if (close(fd) == -1) {
    unlink(nom_fifo);
    perror("clos");
    exit(EXIT_FAILURE);
  }
  if (unlink(nom_fifo)) {
    perror("unlink");
    exit(EXIT_FAILURE);
  }
}

void gestionnaire(int signum) {
  if (signum < 0) {
    fprintf(stderr, "Wrong signal number\n");
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}
