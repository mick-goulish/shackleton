/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include "options.hpp"


#include "proton/container.hpp"
#include "proton/event.hpp"
#include "proton/handler.hpp"
#include "proton/connection.hpp"
#include "proton/value.hpp"
#include "proton/task.hpp"


#include <iostream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <stdarg.h>


#include "resource_reporter.h"

#include <execinfo.h>






class 
simple_send : public proton::handler
{

  private:

    proton::url url;
    rr_t resource_reporter;
    int  report_frequency;
    FILE * output_fp;
    double done_time;

  public:

    int sent;
    int confirmed;
    int requested;
    proton::sender my_sender;
    int tick_ms;
    pid_t pid;



    void
    bt ( void )
    {
      void *array[10];
      size_t size;

      size = backtrace ( array, 10 );
      fprintf ( stderr, "backtrace -----------------------\n" );
      backtrace_symbols_fd ( array, size, 2 );
    }



    simple_send ( const std::string & s, 
                  int c, 
                  FILE * fp, 
                  int freq
                ) : 
      url(s), 
      report_frequency(freq), 
      output_fp(fp), 
      done_time(0),
      sent(0), 
      confirmed(0), 
      requested(c),
      my_sender(0)
    {
      tick_ms = 1000;

      if ( requested == -1 )
      {
        fprintf ( stderr, "simple_send %d: sending forever.\n", getpid() );
      }
      
      pid = getpid();
    }


    void
    log ( char const * format, ... )
    {
      FILE * fp = output_fp ? output_fp : stdout;

      va_list ap;
      fprintf ( fp, "%.4lf ", get_timestamp() );
      va_start(ap, format);
      vfprintf ( fp, format, ap );
      va_end(ap);
      fputc ( '\n', fp );
      fflush  ( fp );
    }



    double
    get_timestamp ( void )
    {
      struct timeval t;
      gettimeofday ( & t, 0 );
      return t.tv_sec + ((double) t.tv_usec) / 1000000.0;
    }



    void
    report ( FILE * output_fp )
    {
      double cpu;
      int    rss; 
      double sslr = rr_seconds_since_last_report ( & resource_reporter );
      double throughput = double(report_frequency) / sslr;
      rr_report ( & resource_reporter, & cpu, & rss );
      static bool first_time = true;

      if ( first_time )
      {
        first_time = false;
	fprintf ( output_fp, "send_msgs\tcpu\trss\tthroughput\n");
      }

      fprintf ( output_fp,
                "%d\t%lf\t%d\t%lf\n",
                sent,
                cpu,
                rss,
                throughput
              );
    }



    void
    send ( )
    {
      if ( ! my_sender )
      {
        fprintf ( stderr, "MDEBUG simple_send send called with no sender.\n" );
        return;
      }

      if (
           my_sender.credit() 
           && 
           (
             (requested == -1)
             ||
             (sent < requested) 
           )
         )
      {
        if ( ! sent )
          rr_init ( & resource_reporter );

        proton::message msg;
        msg.id(sent + 1);
        double ts = get_timestamp ( );
        msg.body(ts);
        // fprintf ( output_fp, "MDEBUG sender %d sending message at %.3lf\n", pid , get_timestamp() );
        my_sender.send ( msg );
        sent++;

        if ( ! (sent % report_frequency) )
        {
          report ( output_fp );
        }
      }
    }
    
    


//===============================================

    void
    on_start ( proton::event & e )
    {
      log ( "on_start sending to |%s|", url.str().c_str() );
      my_sender = e.container().open_sender(url);
      e.container().schedule ( tick_ms, this );
    }



    void
    on_connection_open ( proton::event & e )
    {
      log ( "on_connection_open" );
    }



    void
    on_session_open ( proton::event & e )
    {
      log ( "on_session_open" );
    }



    void
    on_link_open ( proton::event & e )
    {
      log ( "on_link_open" ); 
    }



    void
    on_sendable ( proton::event & e )
    {
      log ( "on_sendable" ); 
    }


    void
    on_delivery_accept ( proton::event & e )
    {
      log ( "on_delivery_accept" ); 
    }



    void
    on_delivery_settle ( proton::event & e )
    {
      log ( "on_delivery_settle  %d", sent ); 
      if ( sent >= requested )
      {
        log ( "closing now" );
        e.sender().close();
        e.connection().close();
        done_time = get_timestamp();
      }
    }



    void
    on_reactor_init ( proton::event & e )
    {
      log ( "on_reactor_init" ); 
    }


    void
    on_reactor_quiesced ( proton::event & e )
    {
      log ( "on_reactor_quiesced" ); 
    }


    void
    on_reactor_final ( proton::event & e )
    {
      log ( "on_reactor_final" ); 
    }


    void
    on_timer ( proton::event & e )
    {
      log ( "on_timer" );
      send ( );

      if ( done_time > 0 )
      {
        double now = get_timestamp();
        double done_duration = now - done_time;
        if ( done_duration >= 3.0 )
        {
          log ( "exiting -- my patience expired" );
          exit ( 0 );
        }
      }

      e.container().schedule ( tick_ms, this );
    }


    void
    on_connection_init ( proton::event & e )
    {
      log ( "on_connection_init" ); 
    }


    void
    on_connection_bound ( proton::event & e )
    {
      log ( "on_connection_bound" ); 
    }


    void
    on_connection_unbound ( proton::event & e )
    {
      log ( "on_connection_unbound" ); 
    }


    void
    on_connection_local_open ( proton::event & e )
    {
      log ( "on_connection_local_open" ); 
    }


    void
    on_connection_local_close ( proton::event & e )
    {
      log ( "on_connection_local_close" ); 
    }


    void
    on_connection_close ( proton::event & e )
    {
      log ( "on_connection_close" ); 
      log ( "exiting now" );
      exit ( 0 );
    }


    void
    on_connection_remote_open ( proton::event & e )
    {
      log ( "on_connection_remote_open" ); 
    }


    void
    on_connection_remote_close ( proton::event & e )
    {
      log ( "on_connection_remote_close" ); 
    }


    void
    on_connection_final ( proton::event & e )
    {
      log ( "on_connection_final" ); 
    }


    void
    on_session_init ( proton::event & e )
    {
      log ( "on_session_init" ); 
    }


    void
    on_session_local_open ( proton::event & e )
    {
      log ( "on_session_local_open" ); 
    }


    void
    on_session_local_close ( proton::event & e )
    {
      log ( "on_session_local_close" ); 
    }


    void
    on_session_remote_open ( proton::event & e )
    {
      log ( "on_session_remote_open" ); 
    }


    void
    on_session_remote_close ( proton::event & e )
    {
      log ( "on_session_remote_close" ); 
    }


    void
    on_session_final ( proton::event & e )
    {
      log ( "on_session_final" ); 
    }


    void
    on_link_init ( proton::event & e )
    {
      log ( "on_link_init" ); 
    }


    void
    on_link_local_open ( proton::event & e )
    {
      log ( "on_link_local_open" ); 
    }


    void
    on_link_local_close ( proton::event & e )
    {
      log ( "on_link_local_close" ); 
    }


    void
    on_link_local_detach ( proton::event & e )
    {
      log ( "on_link_local_detach" ); 
    }


    void
    on_link_remote_open ( proton::event & e )
    {
      log ( "on_link_remote_open" ); 
    }


    void
    on_link_remote_close ( proton::event & e )
    {
      log ( "on_link_remote_close" ); 
    }


    void
    on_link_remote_detach ( proton::event & e )
    {
      log ( "on_link_remote_detach" ); 
    }


    void
    on_link_flow ( proton::event & e )
    {
      log ( "on_link_flow" ); 
    }


    void
    on_link_final ( proton::event & e )
    {
      log ( "on_link_final" ); 
    }


    void
    on_delivery ( proton::event & e )
    {
      log ( "on_delivery" ); 

      confirmed++;
      // requested == -1 means "forever"
      if (confirmed == requested) 
      {
        log ( "all messages confirmed. closing connection" );
        fflush  ( output_fp );
        e.connection().close();
        exit ( 0 );
      }
    }


    void
    on_transport ( proton::event & e )
    {
      log ( "on_transport" ); 
    }


    void
    on_transport_error ( proton::event & e )
    {
      log ( "on_transport_error" ); 

      std::string error("transport error!");
      throw std::runtime_error ( error );
    }


    void
    on_transport_head_closed ( proton::event & e )
    {
      log ( "on_transport_head_closed" );
    }


    void
    on_transport_tail_closed ( proton::event & e )
    {
      log ( "on_transport_tail_closed" ); 

      if ( sent < requested ) 
      {
        std::string error("I wasn't done!");
        throw std::runtime_error ( error );
      }
    }


    void
    on_transport_closed ( proton::event & e )
    {
      log ( "on_transport_closed" ); 
    }


    void
    on_selectable_init ( proton::event & e )
    {
      log ( "on_selectable_init" ); 
    }


    void
    on_selectable_updated ( proton::event & e )
    {
      log ( "on_selectable_updated" ); 
    }


    void
    on_selectable_readable ( proton::event & e )
    {
      log ( "on_selectable_readable" ); 
    }


    void
    on_selectable_writable ( proton::event & e )
    {
      log ( "on_selectable_writable" ); 
    }


    void
    on_selectable_expired ( proton::event & e )
    {
      log ( "on_selectable_expired" ); 
    }


    void
    on_selectable_error ( proton::event & e )
    {
      log ( "on_selectable_error" ); 
    }


    void
    on_selectable_final ( proton::event & e )
    {
      log ( "on_selectable_final" ); 
    }


    void
    on_unhandled ( proton::event & e )
    {
      log ( "on_unhandled |%s| ", e.name().c_str() );
    }


};





int
mrand ( int one_past_max )
{
  double zero_to_one = (double) rand() / (double) RAND_MAX;
  return (int) (zero_to_one * (double) one_past_max);
}





int 
main ( int argc, char **argv ) 
{
  srand ( getpid() );

  // Command line options
  std::string address("127.0.0.1:5672/examples");
  std::string destination(".");
  int message_count = 20;
  int report_frequency = 100000;
  int addr_number  = 0;
  std::string start_signal_file;

  
  options opts(argc, argv);
  opts.add_value ( address, 'a', "address", "connect and send to URL", "URL");
  opts.add_value ( message_count, 'm', "messages", "send COUNT messages", "COUNT");
  opts.add_value ( addr_number, 'q', "addr", "number of address to recv from", "ADDR");
  opts.add_value ( destination, 'd', "dest", "destination dir for reports", "DEST");
  opts.add_value ( report_frequency, 'f', "freq", "report frequency", "FREQ");
  opts.add_value ( start_signal_file, 'F', "file", "start signal file", "FILE");
  
  int pid = getpid();

  FILE * output_fp = 0;


  for ( int tries = 0; tries < 3; ++ tries )
  {
  try 
  {
    opts.parse();
    std::stringstream output_file_name;
    output_file_name << destination << "/tp_report_" << getpid() << ".txt";

    std::stringstream addr;
    addr << address << '/' << addr_number;

    output_fp = fopen ( output_file_name.str().c_str(), "a" );
    if ( ! output_fp )
    {
      fprintf ( stderr, 
                "simple_recv can't open output file |%s|\n", 
                destination.c_str()
              );
      exit ( 1 );
    }


    fprintf ( stderr, "sender %d: sending to |%s|  try %d\n", pid, addr.str().c_str(), tries );
    fprintf ( output_fp, "sender %d: sending to |%s|  : try %d\n", pid, addr.str().c_str(), tries );
    simple_send sender ( addr.str(), 
                         message_count, 
                         output_fp, 
                         report_frequency
                       );


    while ( 5 )
    {
      //fprintf ( stderr, "sender %d waiting for file |%s|\n", getpid(), start_signal_file.c_str() );
      if ( -1 !=  access ( start_signal_file.c_str(), F_OK ) )
      {
        //fprintf ( stderr, "sender %d found start signal file.  Starting.\n", getpid() );
        break;
      }
      else
      {
        sleep ( 13 );
      }
    }

    proton::container(sender).run();

    fprintf ( output_fp, "sender %d: sent %d messages\n", pid, sender.sent );
    fclose  ( output_fp );
    return 0;
  } 
  catch (const bad_option& e) 
  {
    std::cout << opts << std::endl << e.what() << std::endl;
  } 
  catch (const std::exception& e) 
  {
    if ( output_fp )
    {
      fprintf ( output_fp, "MDEBUG --- CAUGHT AN ERROR!!!!!\n" );
      fclose  ( output_fp );
    }
    std::cerr << e.what() << std::endl;
  }
  }

  return 1;
}





