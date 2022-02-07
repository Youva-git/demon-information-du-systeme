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
#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include "fifo.h"

// Nom de segment de mÃ©moire partagÃ© avec id pour son unicitÃ©
#define NOM_SHM "/SHM_FIFO_39841259"

#define TAILLE_SHM sizeof(fifo) * 2048 
#define MAX_THREADS 8
//pthread_infos : structure contenant les informations necessaire pour le
//fonctionnement de chaque threads crée.
typedef struct pthread_infos {
  //is_taken : indique si le thread est ocuupé ou pas
  bool is_taken;
  //cmd : contient la commande qui sera executé par ce thread .elle est sous
  //sous forme de tableau de chaine de caractére. cmd[0] contient le nom tube
  //nommé qui sera utilisé, cmd[1] le nom de la commande,et ensuite les
  //arguments.
  char **cmd;
  sem_t mutex;
} pthread_infos;

//run : la fonction du lancement de nos threads.
void *run(void *arg);

//cmd_exe : prends notre commande sous forme de chaine de caratéres et renvoie
// un tableau de chaine de caractére. cmd[0] contient le nom tube
//nommé qui sera utilisé, cmd[1] le nom de la commande,et ensuite les
//arguments.
char **cmd_exe(char *src);

//affiche_cmd : affiche le contenu du tableau de chaines passé en paramaitre.
void affiche_cmd(char **cmd);

//find _free_thread : prend en paramaitre un tableau de structure du type
//pthread_infos est renvoie l'indice de la structure relié au premier thread
//disponible.
int find_free_thread(pthread_infos *t);

//init_create_threads : crée et inistialise un tableau contenant les structures
//concernant chaque thread .lance nos threads puis renvoie le tableau créer.
pthread_infos *init_create_threads();

//not_taken : le nombre de threads disponible.
int not_taken = MAX_THREADS;

//verrou : semaphore qui protége l'accés a la variable globale partagée
//not_taken.
sem_t verrou;

void gestionnaire(int signum);

pthread_infos *threads_infos;

int main(void) {
  fifo *shm = fifo_empty(NOM_SHM);
  fifo_init(shm);
  threads_infos = init_create_threads();
  //on initialise notre semaphore.
  if (sem_init(&verrou, 1, 1) == -1) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }
  struct sigaction action;
  action.sa_handler = gestionnaire;
  action.sa_flags = 0;
  if (sigfillset(&action.sa_mask) == -1) {
    perror("sigaction");
    exit(EXIT_SUCCESS);
  }
  if (sigaction(SIGINT, &action, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_SUCCESS);
  }
  while (1) {
    //on récupére notre commande sous forme de tableau de chaines . cmd[0]
    //contient le nom du tube nommé dans lequel on vas écrire le resultat de
    //l'exécution de la commande. A partir de cmd[1] notre commande ainsi que
    //ses argument.
    char **src = cmd_exe(fifo_pop(shm));
    //on vérifie si ils y'a des threads disponible.
    if (not_taken > 0) {
      if (sem_wait(&verrou) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
      }
      //on decrémente la variable pour indiquer qu'on occupe un thread de plus .
      --not_taken;
      if (sem_post(&verrou) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
      }
      //on cherche l'indice du premier thread disponible.
      int n = find_free_thread(threads_infos);
      //on lui donne la commande pour qu'il l'éxecute et on l'occupe.
      threads_infos[n].cmd = src;
      threads_infos[n].is_taken = true;
      if (sem_post(&threads_infos[n].mutex) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
      }
    } else {
      //si tous les thread sont occupé on envoie un signal au client qui a
      //demandé l'exécution de cette requétte . on l'identifie grace a son pid
      //qui le mémé avec le nom du tube nommé (c'est fait volentairement pour
      // ca)
      kill(atoi(src[0]), SIGUSR1);
    }
  }
  shm_unlink(NOM_SHM);
  exit(EXIT_SUCCESS);
}

pthread_infos *init_create_threads() {
  pthread_infos *threads_infos
    = malloc((unsigned int) MAX_THREADS * sizeof(struct pthread_infos));
  pthread_t thr[MAX_THREADS];
  for (int i = 0; i < MAX_THREADS; ++i) {
    threads_infos[i].is_taken = false;
    //initialisation des mutex de chaque threads pour qu'ils attendent qu'une
    //commande leur soit livrée.
    if (sem_init(&threads_infos[i].mutex, 1, 0) == -1) {
      perror("sem_init");
      exit(EXIT_FAILURE);
    }
    //on lance nos thread.
    if (pthread_create(&thr[i], NULL, run, &threads_infos[i]) != 0) {
      fprintf(stderr, "Error during pthread_create()\n");
      exit(EXIT_FAILURE);
    }
  }
  return threads_infos;
}

void affiche_cmd(char **cmd) {
  int i = 0;
  while (cmd[i] != NULL) {
    printf("%s ", cmd[i]);
    ++i;
  }
  printf("\n");
}

char **cmd_exe(char *src) {
  char ss[7];
  int i = 0;
  while (src[i] != ' ') {
    ss[i] = src[i];
    ++i;
  }
  int cmd_size = atoi(ss);
  char **cmd = malloc((size_t) (cmd_size + 1) * sizeof(cmd));
  int ii = 1;
  while (src[i] != ';') {
    if (src[i] == '&') {
      ii = 0;
    }
    ++i;
    char *s = malloc(strlen(src) * sizeof(s));
    int k = 0;
    while (src[i] != ' ' && src[i] != '&' && src[i] != ';') {
      s[k] = src[i];
      ++k;
      ++i;
    }
    cmd[ii] = s;
    ++ii;
  }
  cmd[cmd_size] = NULL;
  return cmd;
}

int find_free_thread(pthread_infos *t) {
  for (int i = 0; i < MAX_THREADS; ++i) {
    if (!t[i].is_taken) {
      return i;
    }
  }
  return -1;
}

void *run(void *arg) {
  pthread_infos *infos = arg;
  while (1) {
    //on attend qu'une commande soit livré au thread qui sera débloqué un peu
    //plus haut grace a un appel a post
    if (sem_wait(&infos->mutex) == -1) {
      perror("sem_init");
      exit(EXIT_FAILURE);
    }
    //on récupére la commande qu'on lui associé.
    char **cmd = infos->cmd;
    affiche_cmd(cmd);
    //on ouvre notre tube déja créer
    int fd = open(cmd[0], O_WRONLY);
    if (fd == -1) {
      perror("open");
      exit(EXIT_FAILURE);
    }
    //on se duplique.
    switch (fork()) {
      case -1:
        perror("fork");
        exit(EXIT_FAILURE);
      case 0:
        //on redirige la sortie standard vers le tube nommé associé a la
        //commande
        if (dup2(fd, STDOUT_FILENO) == -1) {
          perror("dup2");
          exit(EXIT_FAILURE);
        }
        if (close(fd) == -1) {
          perror("clos");
          exit(EXIT_FAILURE);
        }
        //on éxecute la commande.
        execvp(cmd[1], &cmd[1]);
        perror("execvp");
        exit(EXIT_FAILURE);
        break;
      default:
        if (close(fd) == -1) {
          perror("clos");
          exit(EXIT_FAILURE);
        }
        break;
    }
    //on indique que notre thread est libre pour une nouvelle commande.
    infos->is_taken = false;
    if (sem_wait(&verrou) == -1) {
      perror("sem_init");
      exit(EXIT_FAILURE);
    }
    ++not_taken;
    if (sem_post(&verrou) == -1) {
      perror("sem_init");
      exit(EXIT_FAILURE);
    }
    int i = 0;
    while (cmd[i] != NULL) {
      free(cmd[i]);
      ++i;
    }
    free(cmd);
  }
  return NULL;
}

void gestionnaire(int signum) {
  if (signum < 0) {
    fprintf(stderr, "wrong signal number\n");
    exit(EXIT_FAILURE);
  }
  shm_unlink(NOM_SHM);
  free(threads_infos);
  exit(EXIT_SUCCESS);
}
