#define _GNU_SOURCE

#include <stdio.h>
#include <signal.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <dynamic.h>
#include <reactor.h>

#include "pounce.h"
#include "worker.h"
#include "connection.h"

int realtime(void)
{
  struct sched_param param;
  struct rlimit rlim;
  int e;

  e = sched_getparam(0, &param);
  if (e == -1)
    return -1;

  param.sched_priority = sched_get_priority_max(SCHED_FIFO);
  rlim = (struct rlimit) {.rlim_cur = param.sched_priority, .rlim_max = param.sched_priority};
  e = prlimit(0, RLIMIT_RTPRIO, &rlim, NULL);
  if (e == -1)
    return -1;

  e = sched_setscheduler(0, SCHED_FIFO, &param);
  if (e == -1)
    return -1;

  return 0;
}

static void worker_main(worker *worker)
{
  list connections;
  connection *connection;
  size_t i;
  core_counters *counters;

  worker->thread = pthread_self();
  reactor_construct();
  if (worker->pounce->affinity)
    reactor_affinity(worker->instance);
  if (worker->pounce->realtime)
    realtime();

  list_construct(&connections);
  for (i = 0; i < worker->connections_count; i++)
  {
    connection = list_push_back(&connections, NULL, sizeof *connection);
    connection_construct(connection, NULL, NULL);
    connection_start(connection, worker);
  }

  reactor_loop();
  counters = core_get_counters(NULL);
  stats_counters(&worker->stats, counters->awake, counters->sleep);

  list_foreach(&connections, connection) connection_destruct(connection);
  list_destruct(&connections, NULL);

  reactor_destruct();
  worker->thread = 0;
}

static core_status worker_job(core_event *event)
{
  worker *worker = event->state;

  switch (event->type)
  {
  case POOL_REQUEST:
    worker_main(worker);
    break;
  case POOL_REPLY:
    worker->job = 0;
    worker->thread = 0;
    break;
  }
  return CORE_OK;
}

void worker_construct(worker *worker, core_callback *callback, void *state)
{
  (void) callback;
  (void) state;

  *worker = (struct worker) {0};
  stats_construct(&worker->stats);
}

void worker_configure(worker *worker, pounce *pounce, size_t instance, size_t connections)
{
  worker->pounce = pounce;
  worker->instance = instance;
  worker->connections_count = connections;
  worker->job = pool_enqueue(NULL, worker_job, worker);
}

void worker_stop(worker *worker)
{
  if (worker->thread)
  {
    pthread_kill(worker->thread, SIGTERM);
    worker->thread = 0;
  }

  if (worker->job)
  {
    pool_cancel(NULL, worker->job);
    worker->job = 0;
  }
}

void worker_destruct(worker *worker)
{
  worker_stop(worker);
  stats_destruct(&worker->stats);
}
