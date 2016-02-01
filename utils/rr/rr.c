#include <stdio.h>
#include <stdlib.h>

#include "resource_reporter.h"



int
main ( int argc, char ** argv )
{
  if ( argc != 3 )
  {
    fprintf ( stderr, "usage: rr PID pause\n" );
    exit ( 1 );
  }

  int pid   = atoi ( argv[1] );
  int pause = atoi ( argv[2] );
  int total_time = 0;

  rr_t rr;

  rr_init ( & rr, pid );

  double cpu;
  int    rss;


  int first_time = 1;

  while ( 1 )
  {
    sleep ( pause );
    total_time += pause;
    rr_report ( & rr, & cpu, & rss );
    if ( first_time )
    {
      fprintf ( stdout, "time\tcpu\trss\n" );
      first_time = 0;
    }

    fprintf ( stdout, 
              "%d\t%lf\t%d\n", 
              total_time,
              cpu, 
              rss 
            );
    fflush ( stdout );
  }
}





