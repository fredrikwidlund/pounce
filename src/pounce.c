#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
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

static core_status timeout(core_event *event)
{
  worker *worker  = event->state;

  (void) fprintf(stdout, "[%lu] %lu/%lu\n", worker->instance, worker->success, worker->failure);
  worker->success = 0;
  worker->failure = 0;
  return CORE_OK;
}

void worker_construct(worker *worker)
{
  *worker = (struct worker) {0};
  timer_construct(&worker->timer, timeout, worker);
  list_construct(&worker->sessions);
}

void worker_open(worker *worker, struct addrinfo *addrinfo, size_t sessions)
{
  session *session;

  worker->addrinfo = addrinfo;
  timer_set(&worker->timer, 1000000000, 1000000000);

  while (sessions)
  {
    session = list_push_back(&worker->sessions, NULL, sizeof *session);
    session->worker = worker;
    http_client_construct(&session->client, request, session);
    http_client_open(&session->client, net_client(worker->addrinfo, 0));
    sessions--;
  }
}

int main()
{
  struct addrinfo *addrinfo;
  worker worker;
  size_t n = get_nprocs();

  net_resolve("127.0.0.1", "80", AF_INET, SOCK_STREAM, AI_NUMERICHOST | AI_NUMERICSERV, &addrinfo);

  worker.instance = reactor_clone(n);
  reactor_affinity(worker.instance);
  reactor_construct();
  worker_construct(&worker);
  worker_open(&worker, addrinfo, 8);

  reactor_loop();
  reactor_destruct();
}
