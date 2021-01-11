#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <sys/sysinfo.h>

#include <dynamic.h>
#include <reactor.h>

#include "url.h"
#include "pounce.h"
#include "worker.h"

static core_status pounce_timeout(core_event *event)
{
  pounce *pounce = event->state;

  if (event->type != TIMER_ALARM)
    err(1, "timer");
  timer_clear(&pounce->timer);

  pounce_stop(pounce);
  return CORE_ABORT;
}

void pounce_construct(pounce *pounce, core_callback *callback, void *state)
{
  (void) callback;
  (void) state;

  *pounce = (struct pounce) {0};
  list_construct(&pounce->workers);
  timer_construct(&pounce->timer, pounce_timeout, pounce);
  string_construct(&pounce->request);
}

static void pounce_usage(void)
{
  extern char *__progname;

  (void) fprintf(stderr, "Usage: %s [OPTION]... URL\n", __progname);
  (void) fprintf(stderr, "HTTP load generator.\n");
  (void) fprintf(stderr, "\n");
  (void) fprintf(stderr, "Options:\n");
  (void) fprintf(stderr, "    -c NUMBER       set number of connections (defaults to number of cpu cores * 4)\n");
  (void) fprintf(stderr, "    -t NUMBER       set number of threads (defaults to number of cpu cores)\n");
  (void) fprintf(stderr, "    -d SECONDS      set duration of benchmark (defaults to 10 seconds)\n");
  (void) fprintf(stderr, "    -H HEADER       add custom header to request\n");
  (void) fprintf(stderr, "    -p NUMBER       pipeline a number of requests (defaults to off)\n");
  (void) fprintf(stderr,
                 "    -a              set thread affinity (automatic when threads equal number of cpu cores)\n");
  (void) fprintf(stderr, "    -r              enable realtime scheduler (defaults to off)\n");
  (void) fprintf(stderr, "    -v              increase verbosity\n");
  (void) fprintf(stderr, "    -h              display this help\n");
}

void pounce_configure(pounce *pounce, int argc, char **argv)
{
  size_t i, remaining, share;
  worker *worker;
  int c;
  char *s;

  while (1)
  {
    c = getopt(argc, argv, "ac:d:H:p:rt:vh");
    if (c == -1)
      break;
    switch (c)
    {
    case 'a':
      pounce->affinity = 1;
      break;
    case 'c':
      pounce->connections = strtoul(optarg, NULL, 0);
      break;
    case 'd':
      pounce->duration = strtod(optarg, NULL);
      break;
    case 'H':
      string_append(&pounce->request, optarg);
      string_append(&pounce->request, "\r\n");
      break;
    case 'p':
      pounce->pipeline = strtoul(optarg, NULL, 0);
      break;
    case 'r':
      pounce->realtime = 1;
      break;
    case 't':
      pounce->workers_count = strtoul(optarg, NULL, 0);
      break;
    case 'v':
      pounce->verbosity++;
      break;
    case 'h':
    default:
      pounce_usage();
      return;
    }
  }

  argc -= optind;
  argv += optind;
  if (argc != 1)
  {
    pounce_usage();
    return;
  }

  if (pounce->workers_count == 0)
    pounce->workers_count = get_nprocs();
  if (pounce->workers_count == (size_t) get_nprocs())
    pounce->affinity = 1;
  if (pounce->connections == 0)
    pounce->connections = pounce->workers_count * 4;
  if (pounce->duration == 0)
    pounce->duration = 10.0;

  pounce->host = url_host(argv[0]);
  pounce->serv = url_port(argv[0]);
  pounce->target = url_target(argv[0]);
  (void) asprintf(&s, "GET %s HTTP/1.1\r\nHost: %s\r\n", pounce->target, pounce->host);
  string_prepend(&pounce->request, s);
  free(s);
  string_append(&pounce->request, "\r\n");

  net_resolve(pounce->host, pounce->serv, AF_INET, SOCK_STREAM, 0, &pounce->addrinfo);
  if (!pounce->addrinfo)
  {
    warnx("unable to resolve url http://%s:%s", pounce->host, pounce->serv);
    return;
  }

  pool_limits(NULL, 0, pounce->workers_count + 8);
  remaining = pounce->connections;
  for (i = 0; i < pounce->workers_count; i++)
  {
    share = remaining / (pounce->workers_count - i);
    remaining -= share;
    worker = list_push_back(&pounce->workers, NULL, sizeof *worker);
    worker_construct(worker, NULL, NULL);
    worker_configure(worker, pounce, i, share);
  }

  if (pounce->verbosity >= 1)
    (void) fprintf(stderr, "[pounce] running benchmark for %.02fs on %lu threads with %lu connections\n",
                   pounce->duration, pounce->workers_count, pounce->connections);

  timer_set(&pounce->timer, pounce->duration * 1000000000, 0);
}

void pounce_stop(pounce *pounce)
{
  worker *worker;

  list_foreach(&pounce->workers, worker) worker_stop(worker);
}

void pounce_report(pounce *pounce)
{
  worker *worker;
  char prefix[32];
  stats stats;

  if (list_empty(&pounce->workers))
    return;

  if (pounce->verbosity >= 1)
    list_foreach(&pounce->workers, worker)
    {
      (void) snprintf(prefix, sizeof prefix, "[worker %lu]", worker->instance);
      stats_report(&worker->stats, prefix, stderr);
    }

  stats_construct(&stats);
  list_foreach(&pounce->workers, worker) stats_aggregate(&stats, &worker->stats);
  stats_report(&stats, NULL, stdout);
  stats_destruct(&stats);
}

void pounce_destruct(pounce *pounce)
{
  worker *worker;

  list_foreach(&pounce->workers, worker) worker_destruct(worker);
  list_destruct(&pounce->workers, NULL);
  free(pounce->host);
  free(pounce->serv);
  free(pounce->target);
  freeaddrinfo(pounce->addrinfo);
  string_destruct(&pounce->request);
  *pounce = (struct pounce) {0};
}
