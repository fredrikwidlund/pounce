#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <reactor.h>

#include "pounce.h"

int main(int argc, char **argv)
{
  pounce pounce;

  reactor_construct();
  pounce_construct(&pounce, NULL, NULL);
  pounce_configure(&pounce, argc, argv);

  reactor_loop();
  pounce_report(&pounce);

  pounce_destruct(&pounce);
  reactor_destruct();
}
