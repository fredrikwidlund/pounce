#ifndef WORKER_H_INCLUDED
#define WORKER_H_INCLUDED

#include <pthread.h>

#include <dynamic.h>
#include <reactor.h>

#include "pounce.h"

typedef struct worker worker;
struct worker
{
  pounce    *pounce;
  size_t     instance;
  size_t     connections_count;
  core_id    job;
  size_t     requests_ok;
  size_t     requests_fail;
  pthread_t  thread;
};

void worker_construct(worker *, core_callback *, void *);
void worker_configure(worker *, pounce *, size_t, size_t);
void worker_destruct(worker *);

#endif /* WORKER_H_INCLUDED */
