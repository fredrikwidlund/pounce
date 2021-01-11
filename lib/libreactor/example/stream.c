#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>

#include <reactor.h>

void process(stream *stream)
{
  segment line;

  while (1)
  {
    line = stream_read_line(stream);
    if (!line.size)
      break;
    (void) printf("%.*s", (int) line.size, (char *) line.base);
    stream_consume(stream, line.size);
  }
}

static core_status callback(core_event *event)
{
  switch (event->type)
  {
  case STREAM_READ:
    process(event->state);
    return CORE_OK;
  default:
    process(event->state);
    stream_destruct(event->state);
    return CORE_ABORT;
  }
}

int main()
{
  stream stream;

  (void) fcntl(0, F_SETFL, O_NONBLOCK);

  reactor_construct();
  stream_construct(&stream, callback, &stream);
  stream_open(&stream, 0);

  reactor_loop();

  stream_destruct(&stream);
  reactor_destruct();
  fflush(stdout);
}
