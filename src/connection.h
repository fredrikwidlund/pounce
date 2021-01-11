#ifndef CONNECTION_H_INCLUDED
#define CONNECTION_H_INCLUDED

#include <dynamic.h>

#include "worker.h"
#include "http_client.h"

typedef struct connection connection;
struct connection
{
  worker      *worker;
  http_client  client;
  uint64_t     request_start;
  uint64_t     request_stop;
  size_t       requests;
  size_t       responses;
};

void connection_construct(connection *, core_callback *, void *);
void connection_start(connection *, worker *);
void connection_destruct(connection *);

#endif /* CONNECTION_H_INCLUDED */
