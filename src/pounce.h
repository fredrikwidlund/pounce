#ifndef POUNCE_H_INCLUDED
#define POUNCE_H_INCLUDED

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <dynamic.h>

typedef struct pounce pounce;
struct pounce
{
  char            *host;
  char            *serv;
  char            *target;
  int              verbosity;
  int              affinity;
  int              realtime;
  double           duration;
  size_t           connections;
  size_t           workers_count;
  struct addrinfo *addrinfo;
  list             workers;
  timer            timer;
};

void pounce_construct(pounce *, core_callback *, void *);
void pounce_configure(pounce *, int, char **);
void pounce_stop(pounce *);
void pounce_report(pounce *);
void pounce_destruct(pounce *);

#endif /* POUNCE_H_INCLUDED */

