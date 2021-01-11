#define _GNU_SOURCE

#include <stdio.h>
#include <sched.h>
#include <err.h>
#include <sys/sysinfo.h>

#include "reactor.h"

typedef struct hello hello;
struct hello
{
  int instance;
  server server;
  timer timer;
  size_t requests;
};

static core_status request(core_event *event)
{
  hello *hello = event->state;

  if (event->type != SERVER_REQUEST)
    err(1, "server");

  server_ok((server_context *) event->data, segment_string("text/plain"), segment_string("Hello, World!"));
  hello->requests++;
  return CORE_OK;
}

static core_status timeout(core_event *event)
{
  hello *hello = event->state;
  core_counters *counters;

  if (event->type != TIMER_ALARM)
    err(1, "timer");

  counters = core_get_counters(NULL);
  (void) fprintf(stderr, "[hello %d/%d] requests %lu, usage %.02f%%, frequency %.02fGHz\n", hello->instance, sched_getcpu(),
                 hello->requests, 100. * (double) counters->awake / (double) (counters->awake + counters->sleep),
                 (double) (counters->awake + counters->sleep) / 1000000000.0);
  core_clear_counters(NULL);
  hello->requests = 0;
  return CORE_OK;
}

int main()
{
  hello hello = {0};
  struct addrinfo *ai;
  int i, n = get_nprocs(), fd[n];

  net_resolve(NULL, "80", AF_INET, SOCK_STREAM, AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE, &ai);

  for (i = 0; i < n; i++)
    fd[i] = net_server(ai, 0);
  net_server_filter(fd[0], n);

  hello.instance = reactor_clone(n);
  reactor_affinity(hello.instance);
  reactor_construct();

  server_construct(&hello.server, request, &hello);
  server_open(&hello.server, fd[hello.instance]);
  timer_construct(&hello.timer, timeout, &hello);
  timer_set(&hello.timer, 1000000000, 1000000000);

  reactor_loop();

  timer_destruct(&hello.timer);
  server_destruct(&hello.server);
  reactor_destruct();
  freeaddrinfo(ai);
}
