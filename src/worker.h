#ifndef WORKER_H_INCLUDED
#define WORKER_H_INCLUDED

#include <pthread.h>

#include <dynamic.h>
#include <reactor.h>

#include "stats.h"
#include "pounce.h"

typedef struct worker worker;
struct worker
{
  pounce    *pounce;
  size_t     instance;
  size_t     connections_count;
  core_id    job;
  stats      stats;
  pthread_t  thread;
};

void worker_construct(worker *, core_callback *, void *);
void worker_configure(worker *, pounce *, size_t, size_t);
void worker_stop(worker *);
void worker_destruct(worker *);

#endif /* WORKER_H_INCLUDED */
