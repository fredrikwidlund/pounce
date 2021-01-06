#ifndef NET_H_INCLUDED
#define NET_H_INCLUDED

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define NET_CLIENT_DEFAULT (NET_FLAG_NONBLOCK | NET_FLAG_NODELAY)

int net_client(struct addrinfo *, int);

#endif /* NET_H_INCLUDED */
