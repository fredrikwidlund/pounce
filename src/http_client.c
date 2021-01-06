#include <dynamic.h>
#include <reactor.h>

#include "net.h"
#include "http_client.h"

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
    close = segment_equal_case(http_headers_lookup(&response.headers, segment_string("Connection")),
                               segment_string("close"));
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
  *client = (http_client) {.user = {.callback = callback, .state = state}};
  stream_construct(&client->stream, http_client_stream, client);
}

void http_client_open(http_client *client, int fd)
{
  stream_open(&client->stream, fd);
  stream_notify(&client->stream);
}

void http_client_close(http_client *client)
{
  stream_close(&client->stream);
}

void http_client_destruct(http_client *client)
{
  stream_destruct(&client->stream);
}
