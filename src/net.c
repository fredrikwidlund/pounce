#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include <reactor.h>

#include "net.h"

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
    e = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (int[]) {1}, sizeof(int));
  if (e == 0 && flags & NET_FLAG_QUICKACK)
    e = setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, (int[]) {1}, sizeof(int));
  if (e == 0)
    e = connect(fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
  if (e == 0 || (e == -1 && errno == EINPROGRESS))
    return fd;

  (void) close(fd);
  return -1;
}
