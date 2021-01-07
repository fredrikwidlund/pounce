#include <stdio.h>

#include "net.h"
#include "worker.h"
#include "connection.h"

static core_status connection_request(core_event *event)
{
  connection *connection = event->state;
  http_response *response;

  switch (event->type)
  {
  case HTTP_CLIENT_READY:
    stream_write(&connection->client.stream, segment_string("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"));
    connection->request_start = core_now(NULL);
    break;
  case HTTP_CLIENT_RESPONSE:
    connection->request_stop = core_now(NULL);
    response = (http_response *) event->data;
    stats_data(&connection->worker->stats, connection->request_stop - connection->request_start,
               response->code >= 200 && response->code < 300);
    break;
  case HTTP_CLIENT_CLOSE:
    http_client_close(&connection->client);
    http_client_open(&connection->client, net_client(connection->worker->pounce->addrinfo, 0));
    break;
  case HTTP_CLIENT_ERROR:
    http_client_close(&connection->client);
    return CORE_ABORT;
  }

  return CORE_OK;
}

void connection_construct(connection *connection, core_callback *callback, void *state)
{
  (void) callback;
  (void) state;
  http_client_construct(&connection->client, connection_request, connection);
}

void connection_start(connection *connection, worker *worker)
{
  connection->worker = worker;
  http_client_open(&connection->client, net_client(connection->worker->pounce->addrinfo, 0));
}

void connection_destruct(connection *connection)
{
  http_client_destruct(&connection->client);
}
