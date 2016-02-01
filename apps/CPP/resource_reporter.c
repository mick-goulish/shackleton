#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "resource_reporter.h"


double
rr_now ( void )
{
  struct timeval t;
  gettimeofday ( & t, 0 );
  return t.tv_sec + ((double) t.tv_usec) / 1000000.0;
}





void
rr_init ( rr_p rr )
{
  pid_t pid = getpid();

  sprintf ( rr->stat_filename,  "/proc/%d/stat",  pid );
  sprintf ( rr->statm_filename, "/proc/%d/statm", pid );

  rr->cpu_ticks_per_second = sysconf ( _SC_CLK_TCK );
  rr->previous_cpu_reading = rr_get_cpu_usage ( rr );
  rr->previous_report_timestamp = rr_now ( );
}





int
rr_get_rss ( rr_p rr )
{
  int rss = 0;

  FILE * fp = fopen ( rr->statm_filename, "r" );
  fscanf ( fp, "%*d%d", & rss );
  fclose ( fp );
  return 4 * rss;
}




uint64_t
rr_get_cpu_usage ( rr_p rr )
{
  FILE * fp = fopen ( rr->stat_filename, "r" );
  if(! fp )
  {
    fprintf ( stderr, "get_cpu_usage can't open |%s|\n", rr->stat_filename );
    return 0.0;
  }
  long unsigned utime,
                stime;
  fscanf ( fp,
           "%*d %*s %*c"
           " %*d %*d %*d %*d %*d"
           " %*u"
           " %*lu %*lu %*lu %*lu"
           " %lu %lu",            /* utime stime */
           & utime, & stime
         );
  fclose ( fp );

  return utime + stime;
}





void
rr_report ( rr_p rr, double * cpu, int * rss )
{
  double            this_report_timestamp        = rr_now ( );
  long unsigned int current_cpu_reading          = rr_get_cpu_usage ( rr );
  double            interval                     = this_report_timestamp - rr->previous_report_timestamp;
  long unsigned int cpu_usage_since_last_report  = current_cpu_reading - rr->previous_cpu_reading;
  double            process_cpu_ticks_per_second = (double)cpu_usage_since_last_report / interval;
  double            cpu_percent_use              = process_cpu_ticks_per_second / rr->cpu_ticks_per_second;

  // prepare for next report.
  rr->previous_cpu_reading      = current_cpu_reading;
  rr->previous_report_timestamp = this_report_timestamp;

  * cpu = 100.0 * cpu_percent_use;

  * rss = rr_get_rss ( rr );
}





double
rr_seconds_since_last_report ( rr_p rr )
{
  return rr_now() - rr->previous_report_timestamp;
}





