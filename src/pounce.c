#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/sysinfo.h>

#include <reactor.h>

int net_client(struct addrinfo *addrinfo, int flags)
{
  int fd, e = 0;

  if (!addrinfo)
    return -1;

  flags = flags ? flags : NET_CLIENT_DEFAULT;
  fd = socket(addrinfo->ai_family, addrinfo->ai_socktype | (flags & NET_FLAG_NONBLOCK ? SOCK_NONBLOCK : 0),
              addrinfo->ai_protocol);
  if (fd == -1)
    return -1;

  if (e == 0 && flags & NET_FLAG_NODELAY)
    e = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (int[]){1}, sizeof(int));
  if (e == 0 && flags & NET_FLAG_QUICKACK)
    e = setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));
  if (e == 0)
    e = connect(fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
  if (e == 0 || (e == -1 && errno == EINPROGRESS))
    return fd;

  (void) close(fd);
  return -1;
}

enum
{
  HTTP_CLIENT_ERROR,
  HTTP_CLIENT_READY,
  HTTP_CLIENT_RESPONSE,
  HTTP_CLIENT_CLOSE
};

typedef struct http_client http_client;
struct http_client
{
  core_handler user;
  int fd;
  stream stream;
};

static core_status http_client_read(http_client *client)
{
  segment data;
  size_t offset;
  ssize_t n;
  http_response response = {0};
  core_status e;
  int close = 0;

  data = stream_read(&client->stream);
  for (offset = 0; offset < data.size; offset += n)
  {
    n = http_response_read(&response, segment_offset(data, offset));
    if (dynamic_unlikely(n == 0))
      break;
    if (dynamic_unlikely(n == -1))
      return core_dispatch(&client->user, HTTP_CLIENT_ERROR, 0);

    e = core_dispatch(&client->user, HTTP_CLIENT_RESPONSE, (uintptr_t) &response);
    if (dynamic_unlikely(e != CORE_OK))
      return e;

    if (segment_equal_case(http_headers_lookup(&response.headers, segment_string("Connection")),
                           segment_string("close")))
      close = 1;
  }

  stream_consume(&client->stream, offset);
  if (close)
    return core_dispatch(&client->user, HTTP_CLIENT_CLOSE, 0);

  if (offset)
  {
    e = core_dispatch(&client->user, HTTP_CLIENT_READY, 0);
    if (e != CORE_OK)
      return e;
    stream_flush(&client->stream);
  }

  return CORE_OK;
}

static core_status http_client_stream(core_event *event)
{
  http_client *client = event->state;
  core_status e;

  switch (event->type)
  {
  case STREAM_FLUSH:
    e = core_dispatch(&client->user, HTTP_CLIENT_READY, 0);
    if (e != CORE_OK)
      return e;
    stream_flush(&client->stream);
    return CORE_OK;
  case STREAM_READ:
    return http_client_read(client);
  case STREAM_CLOSE:
    return core_dispatch(&client->user, HTTP_CLIENT_CLOSE, 0);
  default:
    return core_dispatch(&client->user, HTTP_CLIENT_ERROR, 0);
  }
}

void http_client_construct(http_client *client, core_callback *callback, void *state)
{
  *client = (http_client) {.user = {.callback = callback, .state = state}, .fd = -1};
  stream_construct(&client->stream, http_client_stream, client);
}

void http_client_open(http_client *client, int fd)
{
  client->fd = fd;
  stream_open(&client->stream, client->fd);
  stream_notify(&client->stream);
}

void http_client_close(http_client *client)
{
  if (client->fd >= 0)
  {
    stream_close(&client->stream);
    client->fd = -1;
  }
}

void http_client_destruct(http_client *client)
{
  http_client_close(client);
  stream_destruct(&client->stream);
}

typedef struct session session;
typedef struct worker worker;

struct session
{
  worker *worker;
  http_client client;
};

struct worker
{
  size_t instance;
  pthread_t thread;
  struct addrinfo *addrinfo;
  timer timer;
  list sessions;
  size_t success;
  size_t failure;
};

static core_status request(core_event *event)
{
  session *session = event->state;
  http_response *response;

  switch (event->type)
  {
  case HTTP_CLIENT_READY:
    stream_write(&session->client.stream, segment_string("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"));
    break;
  case HTTP_CLIENT_RESPONSE:
    response = (http_response *) event->data;
    if (response->code == 200)
      session->worker->success++;
    else
      session->worker->failure++;
    break;
  case HTTP_CLIENT_CLOSE:
    http_client_close(&session->client);
    http_client_open(&session->client, net_client(session->worker->addrinfo, 0));
    break;
  }

  return CORE_OK;
}

void worker_construct(worker *worker, size_t instance)
{
  *worker = (struct worker) {.instance = instance};
}

void worker_open(worker *worker, struct addrinfo *addrinfo, size_t sessions)
{
  session *session;

  worker->addrinfo = addrinfo;

  while (sessions)
  {
    session = list_push_back(&worker->sessions, NULL, sizeof *session);
    session->worker = worker;
    http_client_construct(&session->client, request, session);
    http_client_open(&session->client, net_client(worker->addrinfo, 0));
    sessions--;
  }
}

static core_status worker_timeout(core_event *event)
{
  (void) event;
  core_abort(NULL);
  return CORE_ABORT;
}

void *worker_main(void *arg)
{
  worker *worker = arg;
  session *session;

  reactor_construct();
  reactor_affinity(worker->instance);
  timer_construct(&worker->timer, worker_timeout, worker);
  timer_set(&worker->timer, 10000000000, 0);
  list_construct(&worker->sessions);
  worker_open(worker, worker->addrinfo, 5);
  reactor_loop();
  list_foreach(&worker->sessions, session)
    http_client_destruct(&session->client);
  list_destruct(&worker->sessions, NULL);
  timer_destruct(&worker->timer);
  reactor_destruct();
  return NULL;
}

int main()
{
  struct addrinfo *addrinfo;
  list workers;
  worker *worker;
  size_t i, n;
  int e;

  list_construct(&workers);
  net_resolve("127.0.0.1", "80", AF_INET, SOCK_STREAM, AI_NUMERICHOST | AI_NUMERICSERV, &addrinfo);

  n = get_nprocs();
  for (i = 0; i < n; i++)
  {
    worker = list_push_back(&workers, NULL, sizeof *worker);
    worker->instance = i;
    worker->addrinfo = addrinfo;
    e = pthread_create(&worker->thread, NULL, worker_main, worker);
    if (e == -1)
      err(1, "pthread_create");
  }

  size_t total = 0;
  list_foreach(&workers, worker)
  {
    e = pthread_join(worker->thread, NULL);
    if (e == -1)
      err(1, "pthread_join");
    total += worker->success + worker->failure;
  }
  list_destruct(&workers, NULL);

  fprintf(stdout, "Requests/sec:  %.02f\n", (double) total / 10);
  freeaddrinfo(addrinfo);
}
