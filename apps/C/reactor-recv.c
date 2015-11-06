/*
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


/*#######################################################
  This program is Cliff Jansen's reactor-recv.c , with
  some hacking and slashing that I thought was probably
  only useful in this context.
*#######################################################*/


/*
 * Implements a subset of msgr-recv.c using reactor events.
 */

#include "proton/message.h"
#include "proton/error.h"
#include "proton/types.h"
#include "proton/reactor.h"
#include "proton/handlers.h"
#include "proton/engine.h"
#include "proton/url.h"
#include "msgr-common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "resource_reporter.h"




typedef struct 
{
  Addresses_t subscriptions;
  uint64_t    msg_count;
  int         recv_count;
  int         incoming_window;
  int         reply;
  const char  * name;
  bool        timestamping;
  int         report_frequency;
  char        results_dir[1000];
} 
Options_t;





// Global context for this process
typedef struct 
{
  Options_t *opts;
  Statistics_t *stats;
  uint64_t sent;
  uint64_t received;
  pn_message_t *message;
  pn_acceptor_t * acceptor;
  char * encoded_data;
  size_t encoded_data_size;
  int connections;
  pn_list_t * active_connections;
  bool shutting_down;
  pn_handler_t *listener_handler;
  int quiesce_count;
  double total_latency;
  rr_t resource_reporter;
  FILE * report_fp;
} 
global_context_t;




// Per connection context
typedef struct 
{
  global_context_t * global;
  int                connection_id;
  pn_link_t *        recv_link;
} 
connection_context_t;





static
double
now_timestamp ( void )
{
  struct timeval t;
  gettimeofday ( & t, 0 );
  return t.tv_sec + ((double) t.tv_usec) / 1000000.0;
}





static char *
ensure_buffer(char *buf, size_t needed, size_t *actual)
{
  char* new_buf;
  // Make room for the largest message seen so far, plus extra for slight changes in metadata content
  if (needed + 1024 <= *actual)
    return buf;
  needed += 2048;
  new_buf = (char *) realloc(buf, needed);
  if (new_buf != NULL) {
    buf = new_buf;
    *actual = buf ? needed : 0;
  }
  return buf;
}





void 
global_shutdown ( global_context_t * gc )
{
  if (gc->shutting_down) return;
  gc->shutting_down = true;
  pn_acceptor_close(gc->acceptor);
  size_t n = pn_list_size(gc->active_connections);
  for (size_t i = 0; i < n; i++) {
    pn_connection_t *conn = (pn_connection_t *) pn_list_get(gc->active_connections, i);
    if (!(pn_connection_state(conn) & PN_LOCAL_CLOSED)) {
      pn_connection_close(conn);
    }
  }
}





connection_context_t *
connection_context ( pn_handler_t * h )
{
  connection_context_t *p = (connection_context_t *) pn_handler_mem(h);
  return p;
}





void 
connection_context_init ( connection_context_t *cc, global_context_t * gc )
{
  cc->global = gc;
  pn_incref(gc->listener_handler);
  cc->connection_id = gc->connections++;
  cc->recv_link = 0;
}







void 
connection_cleanup ( pn_handler_t *h )
{
  connection_context_t *cc = connection_context(h);
  // Undo pn_incref() from connection_context_init()
  pn_decref(cc->global->listener_handler);
}





bool
get_message_timestamp ( pn_message_t * msg, double * timestamp )
{
  pn_data_t * annotations = pn_message_annotations ( msg );
  pn_data_rewind ( annotations );
  pn_data_next   ( annotations );
  if ( PN_MAP == pn_data_type ( annotations ) )
  {
    int len = pn_data_get_map ( annotations );
    pn_data_enter ( annotations );
    for (int i = 0; i < len; i += 2 ) 
    {
      pn_data_next ( annotations );
      pn_type_t type = pn_data_type ( annotations );
      if ( PN_STRING == type ) 
      {
        pn_bytes_t string = pn_data_get_symbol ( annotations );
        if ( ! strncmp ( "timestamp", string.start, string.size ) )
        {
          pn_data_next ( annotations );
          type = pn_data_type ( annotations );
          if ( type == PN_DOUBLE )
          {
            * timestamp = pn_data_get_double ( annotations );
            return true;
          }
        }
      }
    }
  }

  return false;
}





void 
connection_dispatch ( pn_handler_t *h, pn_event_t *event, pn_event_type_t type )
{
  connection_context_t *cc = connection_context(h);

  switch ( type ) 
  {
    case PN_LINK_REMOTE_OPEN:
      {
        pn_link_t *link = pn_event_link(event);
        if (pn_link_is_receiver(link)) {
          check(cc->recv_link == NULL, "Multiple incomming links on one connection");
          cc->recv_link = link;
          pn_connection_t *conn = pn_event_connection(event);
          pn_list_add(cc->global->active_connections, conn);
          if (cc->global->shutting_down) {
            pn_connection_close(conn);
            break;
          }

          pn_flowcontroller_t *fc = pn_flowcontroller(1024);
          pn_handler_add(h, fc);
          pn_decref(fc);
        }
      }
      break;

    case PN_DELIVERY:
      {
        pn_link_t *recv_link = pn_event_link(event);
        pn_delivery_t *dlv = pn_event_delivery(event);
        if (pn_link_is_receiver(recv_link) && !pn_delivery_partial(dlv)) {
          if (cc->global->received == 0) statistics_start(cc->global->stats);

          size_t encoded_size = pn_delivery_pending(dlv);
          cc->global->encoded_data = ensure_buffer(cc->global->encoded_data, encoded_size,
                                                   &cc->global->encoded_data_size);
          check(cc->global->encoded_data, "decoding buffer realloc failure");

          /*
            If this was the first message received, 
            initialize our reporting.
          */
          if ( ! cc->global->received )
            rr_init ( & cc->global->resource_reporter );

          ssize_t n = pn_link_recv(recv_link, cc->global->encoded_data, encoded_size);
          check(n == (ssize_t) encoded_size, "message data read fail");
          //fprintf ( stderr, "MDEBUG encoded_size == %d\n", encoded_size );
          pn_message_t *msg = cc->global->message;

          int err = pn_message_decode ( msg, cc->global->encoded_data, n );
          check ( err == 0, "message decode error" );

          /* MICK -- annotate! ================================  */
           if ( cc->global->opts->timestamping )
           {
             double message_timestamp;
             if ( get_message_timestamp ( msg, & message_timestamp ) )
             {
               double now = now_timestamp ( );
               cc->global->total_latency += (now - message_timestamp);
             }
             else
             {
               fprintf ( stderr, 
                         "receiver: no timestamp at msg count %d.\n", 
                         cc->global->received 
                       );
               exit ( 1 );
             }
           }
          /* MICK -- end annotate! =============================  */


          cc->global->received++;

          /*---------------------------------------
            Do a report
          ---------------------------------------*/
          if ( ! ( cc->global->received % cc->global->opts->report_frequency ) )
          {
            double cpu_percentage;
            int    rss;
            double sslr = rr_seconds_since_last_report ( & cc->global->resource_reporter );
            rr_report ( & cc->global->resource_reporter, & cpu_percentage, & rss );
            double throughput = (double)(cc->global->opts->report_frequency) / sslr;

            if ( cc->global->opts->timestamping )
            {
              double average_latency = cc->global->total_latency / 
                                       cc->global->opts->report_frequency;
              average_latency *= 1000.0;  // in msec.
              cc->global->total_latency = 0;

              fprintf ( cc->global->report_fp, 
                        "recv_msgs: %10d   cpu: %5.1lf   rss: %6d   throughput: %8.0lf   latency: %.3lf\n", 
                        cc->global->received, 
                        cpu_percentage,
                        rss,
                        throughput,
                        average_latency
                      );
            }
            else
            {
              fprintf ( cc->global->report_fp, 
                        "recv_msgs: %10d   cpu: %5.1lf   rss: %6d   throughput: %8.0lf\n", 
                        cc->global->received, 
                        cpu_percentage,
                        rss,
                        throughput
                      );
            }

          }
          pn_delivery_settle(dlv); // move this up

          statistics_msg_received(cc->global->stats, msg);
        }
        if (cc->global->received >= cc->global->opts->msg_count) {
          global_shutdown(cc->global);
        }
      }
      break;

    case PN_CONNECTION_UNBOUND:
      {
        pn_connection_t *conn = pn_event_connection(event);
        pn_list_remove(cc->global->active_connections, conn);
        pn_connection_release(conn);
      }
      break;

    default:
      break;
  }
}





pn_handler_t *
connection_handler ( global_context_t *gc )
{
  pn_handler_t *h = pn_handler_new(connection_dispatch, sizeof(connection_context_t), connection_cleanup);
  connection_context_t *cc = connection_context(h);
  connection_context_init(cc, gc);
  return h;
}





void 
start_listener ( global_context_t *gc, pn_reactor_t *reactor )
{
  check(gc->opts->subscriptions.count > 0, "no listening address");
  pn_url_t *listen_url = pn_url_parse(gc->opts->subscriptions.addresses[0]);
  const char *host = pn_url_get_host(listen_url);
  const char *port = pn_url_get_port(listen_url);
  if (port == 0 || strlen(port) == 0)
    port = "5672";
  if (host == 0 || strlen(host) == 0)
    host = "0.0.0.0";
  if (*host == '~') host++;
  gc->acceptor = pn_reactor_acceptor(reactor, host, port, NULL);
  check(gc->acceptor, "acceptor creation failed");
  pn_url_free(listen_url);
}





void 
global_context_init ( global_context_t *gc, Options_t *o, Statistics_t *s )
{
  gc->opts = o;
  gc->stats = s;
  gc->sent = 0;
  gc->received = 0;
  gc->encoded_data_size = 0;
  gc->encoded_data = 0;
  gc->message = pn_message();
  check(gc->message, "failed to allocate a message");
  gc->connections = 0;
  gc->active_connections = pn_list(PN_OBJECT, 0);
  gc->acceptor = 0;
  gc->shutting_down = false;
  gc->listener_handler = 0;
  gc->quiesce_count = 0;
  gc->total_latency = 0;

  char report_filename[1000];
  sprintf ( report_filename, "%s/report.txt", gc->opts->results_dir );
  if ( ! (gc->report_fp = fopen ( report_filename, "w" )) )
  {
    fprintf ( stderr, 
              "reactor-recv error: can't open report file |%s|\n", 
              report_filename
            );
    exit ( 1 );
  }
}





global_context_t *
global_context ( pn_handler_t * h )
{
  return (global_context_t *) pn_handler_mem ( h );
}





void 
listener_cleanup ( pn_handler_t * h )
{
  global_context_t *gc = global_context(h);
  pn_message_free(gc->message);
  free(gc->encoded_data);
  pn_free(gc->active_connections);
}





void 
listener_dispatch ( pn_handler_t *h, pn_event_t * event, pn_event_type_t type )
{
  global_context_t * gc = global_context ( h );
  if ( type == PN_REACTOR_QUIESCED )
    gc->quiesce_count++;
  else
    gc->quiesce_count = 0;

  switch (type) 
  {
    case PN_CONNECTION_INIT:
      {
        pn_connection_t * connection = pn_event_connection ( event );

        // New incoming connection on listener socket.  Give each a separate handler.
        pn_handler_t *ch = connection_handler(gc);
        pn_handshaker_t *handshaker = pn_handshaker();
        pn_handler_add(ch, handshaker);
        pn_decref(handshaker);
        pn_record_t *record = pn_connection_attachments(connection);
        pn_record_set_handler(record, ch);
        pn_decref(ch);
      }
      break;

    case PN_REACTOR_INIT:
      {
        pn_reactor_t *reactor = pn_event_reactor(event);
        start_listener(gc, reactor);
      }
      break;

    case PN_REACTOR_FINAL:
      {
        if (gc->received == 0) statistics_start(gc->stats);
        //statistics_report(gc->stats, gc->sent, gc->received);
        fclose ( gc->report_fp );
      }
      break;

    default:
      break;
  }
}





pn_handler_t *
listener_handler ( Options_t *opts, Statistics_t *stats )
{
  pn_handler_t *h = pn_handler_new(listener_dispatch, sizeof(global_context_t), listener_cleanup);
  global_context_t *gc = global_context ( h );
  global_context_init ( gc, opts, stats );
  gc->listener_handler = h;
  return h;
}





static void 
parse_options ( int argc, char **argv, Options_t *opts )
{
  opterr = 0;

  memset( opts, 0, sizeof(*opts) );
  opts->recv_count = -1;
  opts->report_frequency = 0;
  opts->timestamping = false;
  opts->msg_count = 0;
  strcpy ( opts->results_dir, "none" );
  addresses_init( &opts->subscriptions);

  for ( int i = 1; i < argc; ++ i )
  {
    char * arg = argv[i];

    if ( ! strcmp ( "-c", arg ) )
    {
      opts->msg_count = atoi(argv[i+1]);
      ++ i;
    }
    else
    if ( ! strcmp ( "-t", arg ) )
    {
      opts->timestamping = true;
    }
    else
    if ( ! strcmp ( "-r", arg ) )
    {
      opts->report_frequency = atoi(argv[i+1]);
      ++ i;
    }
    else
    if ( ! strcmp ( "-d", arg ) )
    {
      strcpy ( opts->results_dir, argv[i+1]);
      ++ i;
    }
    else
    {
      fprintf ( stderr, "reactor-recv unknown option: |%s|\n", arg );
      exit ( 1 );
    }
  }

  if (opts->subscriptions.count == 0) 
    addresses_add( &opts->subscriptions, "amqp://~0.0.0.0" );
}





int 
main ( int argc, char** argv )
{
  Options_t opts;
  Statistics_t stats;
  parse_options ( argc, argv, & opts );
  pn_reactor_t *reactor = pn_reactor();

  // set up default handlers for our reactor
  pn_handler_t *root = pn_reactor_get_handler(reactor);
  pn_handler_t *lh = listener_handler ( & opts, & stats );
  pn_handler_add(root, lh);
  pn_handshaker_t *handshaker = pn_handshaker();
  pn_handler_add(root, handshaker);

  // Omit decrefs else segfault.  Not sure why they are necessary
  // to keep valgrind happy for the connection_handler, but not here.
  // pn_decref(handshaker);
  // pn_decref(lh);

  pn_reactor_run ( reactor );
  pn_reactor_free ( reactor );

  addresses_free( & opts.subscriptions );

  return 0;
}





