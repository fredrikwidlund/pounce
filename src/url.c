#include <string.h>

#include "url.h"

char *url_host(char *arg)
{
  char *b = arg, *e, *d;

  d = strstr(b, "://");
  if (d)
    b = d + 3;
  e = strchr(b, ':');
  if (!e)
    e = strchr(b, '/');
  if (!e)
    e = b + strlen(b);
  return strndup(b, e - b);
}

char *url_port(char *arg)
{
  char *b = arg, *e, *d;

  d = strstr(b, "://");
  if (d)
    b = d + 3;
  d = strchr(b, ':');
  if (!d)
    return strdup("80");
  b = d + 1;
  e = strchr(b, '/');
  if (!e)
    e = b + strlen(b);
  return strndup(b, e - b);
}

char *url_target(char *arg)
{
  char *b = arg, *e, *d;

  d = strstr(b, "://");
  if (d)
    b = d + 3;
  d = strchr(b, '/');
  if (!d)
    return strdup("/");
  b = d + 1;
  return strdup(b);
}
