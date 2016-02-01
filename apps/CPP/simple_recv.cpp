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
#include "proton/link.hpp"
#include "proton/value.hpp"
#include "proton/message_id.hpp"
#include "proton/task.hpp"


#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <map>

#include "resource_reporter.h"

#include <time.h>
#include <sys/time.h>
#include <stdarg.h>



double
get_timestamp ( void )
{
  struct timeval t;
  gettimeofday ( & t, 0 );
  return t.tv_sec + ((double) t.tv_usec) / 1000000.0;
}






class 
simple_recv : public proton::handler 
{
  private:

    proton::url url;
    proton::receiver receiver;
    rr_t resource_reporter;
    int  report_frequency;
    FILE * output_fp;
    int pid;


  public:

    int64_t  expected;
    int64_t  received;

    simple_recv(const std::string &s, int c, FILE * fp, int freq ) : 
      url(s), 
      report_frequency(freq), 
      output_fp(fp),
      expected(c), 
      received(0)
    {
      if ( expected == -1 )
      {
        fprintf ( stderr, "receiver %d receiving forever.\n", getpid() );
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



    void 
    on_start(proton::event &e) 
    {
      log ( "on_start listening on |%s|", url.str().c_str() );
      receiver = e.container().open_receiver(url);
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
	fprintf ( output_fp, "recv_msgs\tcpu\trss\tthroughput\n");
        first_time = false;
      }

      fprintf ( output_fp,
		"%d\t%lf\t%d\t%lf\n",
		received,
		cpu,
		rss,
		throughput
	      );
    }



    void 
    on_message ( proton::event &e ) 
    {
      log ( "on_message" );

      double receive_timestamp = get_timestamp();

      proton::message& msg = e.message();

      double send_timestamp = msg.body().get<double>();
      double latency = receive_timestamp - send_timestamp;

      fprintf ( output_fp, "latency %.6lf\n", latency );

      if ( ! received )
      {
        rr_init ( & resource_reporter );
      }

      if ( (expected == 0)
           || 
           (expected == -1)
           || 
           (received < expected)
         ) 
      {
        received++;
        if ( ! ( received % report_frequency ) )
        {
          report ( output_fp );
        }

        if (received == expected) 
        {
          log ( "closing receiver and connection." );
          e.receiver().close();
          e.connection().close();
          char filename[1000];
          sprintf ( filename, "/tmp/simple_recv_%d_is_done", getpid() );
          FILE * fp = fopen ( filename, "w" );
          fprintf ( fp, ":-)\n" );
          fclose ( fp );
        }
      }
    }
    

    void
    on_unhandled ( proton::event &e )
    {
      log ( "on_unhandled |%s| ", e.name().c_str() );
    }



    void
    on_connection_bound ( proton::event & e )
    {
      log ( "on_connection_bound" );
    }


    void
    on_connection_close ( proton::event & e )
    {
      log ( "on_connection_close  received %d of %d messages.", received, expected );
    }


    void
    on_connection_error ( proton::event & e )
    {
      log ( "on_connection_error" );
    }


    void
    on_connection_final ( proton::event & e )
    {
      log ( "on_connection_final" );
    }


    void
    on_connection_init ( proton::event & e )
    {
      log ( "on_connection_init" );
    }


    void
    on_connection_local_close ( proton::event & e )
    {
      log ( "on_connection_local_close" );
    }


    void
    on_connection_local_open ( proton::event & e )
    {
      log ( "on_connection_local_open" );
    }


    void
    on_connection_open ( proton::event & e )
    {
      log ( "on_connection_open" );
    }


    void
    on_connection_remote_close ( proton::event & e )
    {
      log ( "on_connection_remote_close" );
    }


    void
    on_connection_remote_open ( proton::event & e )
    {
      log ( "on_connection_remote_open" );
    }


    void
    on_connection_unbound ( proton::event & e )
    {
      log ( "on_connection_unbound" );
    }


    void
    on_delivery ( proton::event & e )
    {
      log ( "on_delivery" );
    }


    void
    on_delivery_accept ( proton::event & e )
    {
      log ( "on_delivery_accept" );
    }


    void
    on_delivery_reject ( proton::event & e )
    {
      log ( "on_delivery_reject" );
    }


    void
    on_delivery_release ( proton::event & e )
    {
      log ( "on_delivery_release" );
    }


    void
    on_delivery_settle ( proton::event & e )
    {
      log ( "on_delivery_settle" );
    }


    void
    on_disconnect ( proton::event & e )
    {
      log ( "on_disconnect" );
    }


    void
    on_link_close ( proton::event & e )
    {
      log ( "on_link_close" );
    }


    void
    on_link_error ( proton::event & e )
    {
      log ( "on_link_error" );
    }


    void
    on_link_final ( proton::event & e )
    {
      log ( "on_link_final" );
    }


    void
    on_link_flow ( proton::event & e )
    {
      log ( "on_link_flow" );
    }


    void
    on_link_init ( proton::event & e )
    {
      log ( "on_link_init" );
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
    on_link_local_open ( proton::event & e )
    {
      log ( "on_link_local_open" );
    }


    void
    on_link_open ( proton::event & e )
    {
      log ( "on_link_open" );
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
    on_link_remote_open ( proton::event & e )
    {
      log ( "on_link_remote_open" );
    }


    #if 0
    void
    on_message ( proton::event & e )
    {
      fprintf ( output_fp, "MDEBUG %d on_message %.3lf\n", pid , get_timestamp());
      fflush  ( output_fp );
    }
    #endif


    void
    on_reactor_final ( proton::event & e )
    {
      log ( "on_reactor_final" );
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
    on_selectable_error ( proton::event & e )
    {
      log ( "on_selectable_error" );
    }


    void
    on_selectable_expired ( proton::event & e )
    {
      log ( "on_selectable_expired" );
    }


    void
    on_selectable_final ( proton::event & e )
    {
      log ( "on_selectable_final" );
    }


    void
    on_selectable_init ( proton::event & e )
    {
      log ( "on_selectable_init" );
    }


    void
    on_selectable_readable ( proton::event & e )
    {
      log ( "on_selectable_readable" );
    }


    void
    on_selectable_updated ( proton::event & e )
    {
      log ( "on_selectable_updated" );
    }


    void
    on_selectable_writable ( proton::event & e )
    {
      log ( "on_selectable_writable" );
    }


    void
    on_sendable ( proton::event & e )
    {
      log ( "on_sendable" );
    }


    void
    on_session_close ( proton::event & e )
    {
      log ( "on_session_close" );
    }


    void
    on_session_error ( proton::event & e )
    {
      log ( "on_session_error" );
    }


    void
    on_session_final ( proton::event & e )
    {
      log ( "on_session_final" );
    }


    void
    on_session_init ( proton::event & e )
    {
      log ( "on_session_init" );
    }


    void
    on_session_local_close ( proton::event & e )
    {
      log ( "on_session_local_close" );
    }


    void
    on_session_local_open ( proton::event & e )
    {
      log ( "on_session_local_open" );
    }


    void
    on_session_open ( proton::event & e )
    {
      log ( "on_session_open" );
    }


    void
    on_session_remote_close ( proton::event & e )
    {
      log ( "on_session_remote_close" );
    }


    void
    on_session_remote_open ( proton::event & e )
    {
      log ( "on_session_remote_open" );
    }


    #if 0
    void
    on_start ( proton::event & e )
    {
      log ( "on_start" );
    }
    #endif


    void
    on_timer ( proton::event & e )
    {
      log ( "on_timer" );
    }


    void
    on_timer_task ( proton::event & e )
    {
      log ( "on_timer_task" );
    }


    void
    on_transaction_abort ( proton::event & e )
    {
      log ( "on_transaction_abort" );
    }


    void
    on_transaction_commit ( proton::event & e )
    {
      log ( "on_transaction_commit" );
    }


    void
    on_transaction_declare ( proton::event & e )
    {
      log ( "on_transaction_declare" );
    }


    void
    on_transport ( proton::event & e )
    {
      log ( "on_transport" );
    }


    void
    on_transport_closed ( proton::event & e )
    {
      log ( "on_transport_closed" );
    }


    void
    on_transport_error ( proton::event & e )
    {
      log ( "on_transport_error" );
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
    }



};





int 
main(int argc, char **argv) 
{
    // Command line options
    std::string address("127.0.0.1:5672/examples");
    std::string destination(".");
    int message_count = 100;
    int report_frequency = 100000;
    int addr_number  = 0;
    options opts(argc, argv);
    opts.add_value(address, 'a', "address", "connect to and receive from URL", "URL");
    opts.add_value(message_count, 'm', "messages", "receive COUNT messages", "COUNT");
    opts.add_value(addr_number, 'q', "addr", "number of address to recv from", "ADDR");
    opts.add_value(destination, 'd', "dest", "destination dir for reports", "DEST");
    opts.add_value(report_frequency, 'f', "freq", "report frequency", "FREQ");

    try 
    {
      opts.parse();
      std::stringstream output_file_name;
      output_file_name << destination << "/tp_report_" << getpid() << ".txt";

      FILE * output_fp = fopen ( output_file_name.str().c_str(), "w" );
      if ( ! output_fp )
      {
        fprintf ( stderr, "simple_recv can't open output file |%s|\n", destination.c_str());
        exit ( 1 );
      }

      std::stringstream addr;
      addr << address << '/' << addr_number;

      fprintf ( stderr, "recv: receiving from |%s|\n", addr.str().c_str() );

      simple_recv recv ( addr.str(), message_count, output_fp, report_frequency );
      proton::container(recv).run();

      if ( recv.received < recv.expected )
      {
        fprintf ( output_fp, "MDEBUG exiting with failure %.3lf\n", get_timestamp());
        fclose ( output_fp );
        fprintf ( stderr, 
                  "receiver %d exiting with failure. %d of %d messages received.\n", 
                  getpid(),
                  recv.received,
                  recv.expected
                );
        return 1;
      }
      else
      {
        fprintf ( output_fp, "MDEBUG exiting with success %.3lf\n", get_timestamp());
        fclose ( output_fp );
        //fprintf ( stderr, "receiver %d exiting with success.\n", getpid() );
        return 0;
      }
    } 
    catch (const bad_option& e) 
    {
      std::cerr << opts << std::endl << e.what() << std::endl;
    } 
    catch (const std::exception& e) 
    {
      std::cerr << e.what() << std::endl;
    }

    fprintf ( stderr, "receiver exiting with error.\n" );
    return 2;
}





