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

#include "proton/acceptor.hpp"
#include "proton/connection.hpp"
#include "proton/event.hpp"
#include "proton/container.hpp"
#include "proton/handler.hpp"
#include "proton/value.hpp"

#include <iostream>
#include <map>
#include <stdio.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

#include "resource_reporter.h"


class simple_send : public proton::handler {
  private:
    proton::url url;
    proton::acceptor acceptor;
    rr_t resource_reporter;
    int  report_frequency;
    FILE * output_fp;


  public:
    int sent;
    int confirmed;
    int requested;
    int on_sendable_entries;

    simple_send(const std::string &s, int c, FILE * fp, int freq) : 
      url(s), 
      report_frequency(freq),
      output_fp(fp),
      sent(0), 
      confirmed(0), 
      requested(c) 
      {
        on_sendable_entries = 0;
      }

    void 
    on_start ( proton::event &e ) 
    {
      acceptor = e.container().listen(url);
      std::cout << "direct_send listening on " << url << std::endl;
    }



    static
    int
    now_timestamp ( void )
    {
      struct timeval t;
      gettimeofday ( & t, 0 );
      return (t.tv_sec * 1000000) + t.tv_usec;
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
    on_sendable(proton::event &e) 
    {
      proton::sender sender = e.sender();
      while (sender.credit() && sent < requested) 
      {
        if ( ! sent )
          rr_init ( & resource_reporter );

        proton::message msg;
        msg.id(sent + 1);
        std::map<std::string, int> m;
        m["sequence"]  = sent + 1;
	m["timestamp"] = now_timestamp();
        msg.body(m);
        sender.send(msg);
        sent++;

        if ( ! (sent % report_frequency) )
        {
          report ( output_fp );
        }
      }
    }



    void on_accepted(proton::event &e) {
        confirmed++;
        if (confirmed == requested) {
            std::cout << "all messages confirmed" << std::endl;
            e.connection().close();
            acceptor.close();
        }
    }

    void on_disconnected(proton::event &e) {
        sent = confirmed;
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
  int pid = getpid();
  srand ( pid );

  // Command line options
  std::string address("127.0.0.1:5672/examples");
  std::string destination(".");
  int message_count = 100;
  int report_frequency = 100000;
  int addr_number  = 0;
  int max_delay = 0;


  options opts(argc, argv);
  opts.add_value(address, 'a', "address", "listen and send on URL", "URL");
  opts.add_value(message_count, 'm', "messages", "send COUNT messages", "COUNT");
  opts.add_value(addr_number, 'q', "addr", "number of address to recv from", "ADDR");
  opts.add_value(destination, 'd', "dest", "destination dir for reports", "DEST");
  opts.add_value(max_delay, 'D', "max_delay", "maximum delay, in seconds, before starting", "DELAY");
  opts.add_value(report_frequency, 'f', "freq", "report frequency", "FREQ");




  try 
  {
    opts.parse();

    std::stringstream output_file_name;
    output_file_name << destination << "/tp_report_" << getpid() << ".txt";

    FILE * output_fp = fopen ( output_file_name.str().c_str(), "w" );
    if ( ! output_fp )
    {
      fprintf ( stderr,
                "direct_send can't open output file |%s|\n",
                destination.c_str()
              );
      exit ( 1 );
    }

    std::stringstream addr;
    addr << address << '/' << addr_number;

    if ( max_delay > 0 )
    {
      int delay = mrand ( max_delay + 1 );
      fprintf ( stderr, "sender %d: delaying for %d seconds\n", pid, delay);
      sleep ( delay );
    }

    fprintf ( stderr, "sender %d: sending to |%s|\n", pid, addr.str().c_str() );
    simple_send sender ( addr.str(),
                         message_count,
                         output_fp,
                         report_frequency
                       );

    proton::container(sender).run();

    fprintf ( stderr, "sender %d: sent %d messages\n", pid, sender.sent );
    fclose ( output_fp );

    return 0;
  } 
  catch (const bad_option& e) 
  {
    std::cout << opts << std::endl << e.what() << std::endl;
  } 
  catch (const std::exception& e) 
  {
    std::cerr << e.what() << std::endl;
  }

  return 1;
}
