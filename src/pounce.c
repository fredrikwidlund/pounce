#define _GNU_SOURCE

#include <stdio.h>
#include <err.h>
#include <sys/sysinfo.h>

#include <dynamic.h>
#include <reactor.h>

#include "pounce.h"
#include "worker.h"

static core_status pounce_timeout(core_event *event)
{
  pounce *pounce = event->state;
  worker *worker;

  if (event->type != TIMER_ALARM)
    err(1, "timer");

  timer_clear(&pounce->timer);

  list_foreach(&pounce->workers, worker)
    worker_destruct(worker);

  return CORE_ABORT;
}

void pounce_construct(pounce *pounce, core_callback *callback, void *state)
{
  (void) callback;
  (void) state;

  *pounce = (struct pounce) {0};
  list_construct(&pounce->workers);
  timer_construct(&pounce->timer, pounce_timeout, pounce);
}

void pounce_configure(pounce *pounce, int argc, char **argv)
{
  size_t i, remaining, share;
  worker *worker;

  (void) argc;
  (void) argv;

  // manual configuration for now
  pounce->host = "127.0.0.1";
  pounce->serv = "80";
  pounce->duration_warmup = 1;
  pounce->duration_measure = 1;
  pounce->connections = 2;
  pounce->workers_count = 2;

  net_resolve(pounce->host, pounce->serv, AF_INET, SOCK_STREAM, 0, &pounce->addrinfo);
  if (!pounce->addrinfo)
    return;

  remaining = pounce->connections;
  for (i = 0; i < pounce->workers_count; i++)
  {
    share = remaining / (pounce->workers_count - i);
    remaining -= share;
    worker = list_push_back(&pounce->workers, NULL, sizeof *worker);
    worker_construct(worker, NULL, NULL);
    worker_configure(worker, pounce, i, share);
  }
}

void pounce_start(pounce *pounce)
{
  timer_set(&pounce->timer, pounce->duration_measure * 1000000000, 0);
  pounce->time_start = core_now(NULL);
}

void pounce_stop(pounce *pounce)
{
  size_t requests_ok = 0, requests_fail = 0;
  double duration;
  worker *worker;

  pounce->time_stop = core_now(NULL);

  list_foreach(&pounce->workers, worker)
  {
    requests_ok += worker->requests_ok;
    requests_fail += worker->requests_fail;
  }

  duration = (double) (pounce->time_stop - pounce->time_start) / 1000000000.0;
  fprintf(stdout, "duration: %.02fs\n", duration);
  fprintf(stdout, "requests: %lu\n", requests_ok + requests_fail);
  fprintf(stdout, "success:  %.02f%%\n", 100.0 * (double) requests_ok / (double) (requests_ok + requests_fail));
  fprintf(stdout, "rate:     %.02f\n", (double) (requests_ok + requests_fail) / duration);
}

void pounce_destruct(pounce *pounce)
{
  worker *worker;

  list_foreach(&pounce->workers, worker)
    worker_destruct(worker);
  list_destruct(&pounce->workers, NULL);
  freeaddrinfo(pounce->addrinfo);
}
