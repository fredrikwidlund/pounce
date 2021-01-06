#ifndef HTTP_CLIENT_H_INCLUDED
#define HTTP_CLIENT_H_INCLUDED

#include <dynamic.h>
#include <reactor.h>

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
  stream       stream;
};

void http_client_construct(http_client *, core_callback *, void *);
void http_client_destruct(http_client *);
void http_client_open(http_client *, int);
void http_client_close(http_client *);

#endif /* HTTP_CLIENT_H_INCLUDED */
