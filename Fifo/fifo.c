#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "fifo.h"

#define TAILLE_SHM sizeof(fifo) * 2048
#define FIFO_MAX 2048

static size_t size_libre(fifo *s) {
  return (size_t) (FIFO_MAX - s->push + s->pop);
}

fifo *fifo_empty(const char *shm_name) {
  int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (shm_fd == -1) {
    perror("shm_open");
    exit(EXIT_FAILURE);
  }
  if (ftruncate(shm_fd, TAILLE_SHM) == -1) {
    perror("ftruncate");
    exit(EXIT_FAILURE);
  }
  fifo *shm = mmap(NULL, TAILLE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd,
          0);
  if (shm == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }
  return shm;
}

void fifo_push(fifo *s, char *src) {
  if (sem_wait(&s->mutex2) == -1) {
    perror("sem_wait");
    exit(EXIT_FAILURE);
  }
  if ((size_libre(s) <= strlen(src))) {
    if (FIFO_MAX < strlen(src)) {
      fprintf(stderr,
          "la taille de votre commande ne peut pas entre supporter\n");
      if (sem_post(&s->mutex2) == -1) {
        perror("sem_wait");
        exit(EXIT_FAILURE);
      }
      exit(EXIT_FAILURE);
    }
    s->taille_depasser = true;
    s->size_cmd = strlen(src);
    if (sem_wait(&s->mutex3) == -1) {
      perror("sem_wait");
      exit(EXIT_FAILURE);
    }
  }
  if (sem_wait(&s->mutex1) == -1) {
    perror("sem_wait");
    exit(EXIT_FAILURE);
  }
  if (sem_post(&s->mutex2) == -1) {
    perror("sem_wait");
    exit(EXIT_FAILURE);
  }
  int i = 0;
  while (src[i] != '\0') {
    s->buffer[s->push] = src[i];
    s->push = (s->push + 1) % FIFO_MAX;
    ++i;
  }
  s->buffer[s->push] = '\0';
  s->push = (s->push + 1) % FIFO_MAX;
  if (sem_post(&s->mutex1) == -1) {
    perror("sem_post");
    exit(EXIT_FAILURE);
  }
  if (sem_post(&s->plein) == -1) {
    perror("sem_post");
    exit(EXIT_FAILURE);
  }
}

char *fifo_pop(fifo *s) {
  if (sem_wait(&s->plein) == -1) {
    perror("sem_post");
    exit(EXIT_FAILURE);
  }
  if (sem_wait(&s->mutex1) == -1) {
    perror("sem_post");
    exit(EXIT_FAILURE);
  }
  char *dest = malloc(FIFO_MAX * sizeof(dest));
  int i = 0;
  while (s->buffer[s->pop] != '\0') {
    dest[i] = s->buffer[s->pop];
    s->pop = (s->pop + 1) % FIFO_MAX;
    ++i;
  }
  s->pop = (s->pop + 1) % FIFO_MAX;
  if (s->taille_depasser) {
    if (size_libre(s) >= s->size_cmd) {
      if (sem_post(&s->mutex3) == -1) {
        perror("sem_post");
        exit(EXIT_FAILURE);
      }
    }
  }
  if (sem_post(&s->mutex1) == -1) {
    perror("sem_post");
    exit(EXIT_FAILURE);
  }
  return dest;
}

void fifo_init(fifo *s) {
  if (sem_init(&s->mutex1, 1, 1) == -1) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }
  if (sem_init(&s->mutex2, 1, 1) == -1) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }
  if (sem_init(&s->mutex3, 1, 0) == -1) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }
  if (sem_init(&s->plein, 1, 0) == -1) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }
  s->taille_depasser = false;
  s->size_cmd = 0;
}
