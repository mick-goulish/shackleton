/*
 * Implements a subset of msgr-send.c using reactor events.
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






static
double
now_timestamp ( void )
{
  struct timeval t;
  gettimeofday ( & t, 0 );
  return t.tv_sec + ((double) t.tv_usec) / 1000000.0;
}





typedef struct {
    Addresses_t targets;
    uint64_t msg_count;
    uint32_t msg_size;  // of body
    int   outgoing_window;
    unsigned int report_interval;      // in seconds
    //Addresses_t subscriptions;
    //Addresses_t reply_tos;
    int   get_replies;
    int   unique_message;  // 1 -> create and free a pn_message_t for each send/recv
    int   timeout;      // in seconds
    int   incoming_window;
    int   recv_count;
    const char *name;
    char *certificate;
    char *privatekey;   // used to sign certificate
    char *password;     // for private key file
    char *ca_db;        // trusted CA database
    bool timestamping;
    int  report_frequency;
    char results_dir[1000];
    bool print_message_size;
} Options_t;


static void usage(int rc)
{
    printf("Usage: reactor-send [OPTIONS] \n"
           " -a <addr> \tThe target address [amqp[s]://domain[/name]]\n"
           " -c # \tNumber of messages to send before exiting [0=forever]\n"
           " -b # \tSize of message body in bytes [1024]\n"
           " -R \tWait for a reply to each sent message\n"
           " -V \tEnable debug logging\n"
           );
    exit(rc);
}


typedef struct {
  Options_t *opts;
  Statistics_t *stats;
  uint64_t sent;
  uint64_t received;
  pn_message_t *message;
  pn_message_t *reply_message;
  pn_atom_t id;
  char *encoded_data;
  size_t encoded_data_size;
  pn_url_t *send_url;
  pn_string_t *hostname;
  pn_string_t *container_id;
  pn_string_t *reply_to;
  rr_t resource_reporter;
  FILE * report_fp;
  bool done;
} sender_context_t;





void 
sender_context_init ( sender_context_t * sc, Options_t * opts, Statistics_t * stats )
{
  sc->done = false;
  sc->opts = opts;
  sc->stats = stats;
  sc->sent = 0;
  sc->received = 0;
  sc->id.type = PN_ULONG;
  // 4096 extra bytes should easily cover the message metadata
  sc->encoded_data_size = sc->opts->msg_size + 4096;
  sc->encoded_data = (char *)calloc(1, sc->encoded_data_size);
  check(sc->encoded_data, "failed to allocate encoding buffer");
  sc->container_id = pn_string("reactor-send"); // prefer uuid-like name

  sc->reply_message = (sc->opts->get_replies) ? pn_message() : 0;
  sc->message = pn_message();
  check(sc->message, "failed to allocate a message");
  sc->reply_to = pn_string("amqp://");
  pn_string_addf(sc->reply_to, "%s", pn_string_get(sc->container_id));
  pn_message_set_reply_to(sc->message, pn_string_get(sc->reply_to));
  pn_data_t *body = pn_message_body(sc->message);
  // borrow the encoding buffer this one time
  char *data = sc->encoded_data;
  pn_data_put_binary(body, pn_bytes(sc->opts->msg_size, data));

  check(sc->opts->targets.count > 0, "no specified address");
  sc->send_url = pn_url_parse(sc->opts->targets.addresses[0]);
  const char *host = pn_url_get_host(sc->send_url);
  const char *port = pn_url_get_port(sc->send_url);
  sc->hostname = pn_string(host);
  if (port && strlen(port))
    pn_string_addf(sc->hostname, ":%s", port);
  
  char report_filename[1000];
  sprintf ( report_filename, "%s/report.txt", opts->results_dir );

  if ( ! ( sc->report_fp = fopen ( report_filename, "w" ) ) )
  {
    fprintf ( stderr, 
              "reactor-send can't open report file |%s|\n", 
              report_filename 
            );
    exit ( 1 );
  }
}




sender_context_t *
sender_context(pn_handler_t *h)
{
  return (sender_context_t *) pn_handler_mem(h);
}




void 
sender_cleanup ( pn_handler_t * h )
{
  sender_context_t *sc = sender_context(h);
  pn_message_free(sc->message);
  pn_message_free(sc->reply_message);
  pn_url_free(sc->send_url);
  pn_free(sc->hostname);
  pn_free(sc->container_id);
  pn_free(sc->reply_to);
  free(sc->encoded_data);
}

pn_handler_t *replyto_handler(sender_context_t *sc);

pn_message_t* get_message(sender_context_t *sc, bool sending) {
  if (sc->opts->unique_message) {
    pn_message_t *m = pn_message();
    check(m, "failed to allocate a message");
    if (sending) {
      pn_message_set_reply_to(m, pn_string_get(sc->reply_to));
      // copy the data
      pn_data_t *body = pn_message_body(m);
      pn_data_t *template_body = pn_message_body(sc->message);
      pn_data_put_binary(body, pn_data_get_binary(template_body));
    }
    return m;
  }
  else
    return sending ? sc->message : sc->reply_message;  // our simplified "message pool"
}

void return_message(sender_context_t *sc, pn_message_t *m) {
  if (sc->opts->unique_message)
    pn_message_free(m);
}

void sender_dispatch(pn_handler_t *h, pn_event_t *event, pn_event_type_t type)
{
  sender_context_t *sc = sender_context(h);

  switch ( type ) 
  {
  case PN_CONNECTION_INIT:

    {
      pn_connection_t *conn = pn_event_connection(event);
      pn_connection_set_container(conn, pn_string_get(sc->container_id));
      pn_connection_set_hostname(conn, pn_string_get(sc->hostname));
      pn_connection_open(conn);
      pn_session_t *ssn = pn_session(conn);
      pn_session_open(ssn);
      pn_link_t *snd = pn_sender(ssn, "sender");
      const char *path = pn_url_get_path(sc->send_url);
      if (path && strlen(path)) {
        pn_terminus_set_address(pn_link_target(snd), path);
        pn_terminus_set_address(pn_link_source(snd), path);
      }
      pn_link_open(snd);
    }
    break;

  case PN_LINK_FLOW:
    {
      pn_link_t *snd = pn_event_link(event);
      while (pn_link_credit(snd) > 0 && sc->sent < sc->opts->msg_count) {

        if (sc->sent == 0)
          statistics_start(sc->stats);

        char tag[8];
        void *ptr = &tag;
        *((uint64_t *) ptr) = sc->sent;
        pn_delivery_t *dlv = pn_delivery(snd, pn_dtag(tag, 8));

        // setup the message to send
        pn_message_t *msg = get_message(sc, true);;
        pn_message_set_address(msg, sc->opts->targets.addresses[0]);
        sc->id.u.as_ulong = sc->sent;
        pn_message_set_correlation_id(msg, sc->id);
        pn_message_set_creation_time(msg, msgr_now());

        /* MICK -- annotate! */
        if ( sc->opts->timestamping )
        {
          double val = now_timestamp();
          pn_data_t * annotations = pn_message_annotations ( msg );
          pn_data_clear ( annotations );
          pn_data_put_map(annotations);
          pn_data_enter(annotations);
          pn_data_put_string ( annotations, pn_bytes(9, "timestamp") );
          pn_data_put_double ( annotations, val );
          pn_data_exit(annotations);
        }
        /* MICK -- end annotate! */


        size_t size = sc->encoded_data_size;
        int err = pn_message_encode(msg, sc->encoded_data, &size);
        check(err == 0, "message encoding error");
        pn_link_send(snd, sc->encoded_data, size);
        pn_delivery_settle(dlv);
        if ( ! sc->sent )
        {
          rr_init ( & sc->resource_reporter );
        }

        sc->sent++;

        /*---------------------------------------
          Do a report
        ---------------------------------------*/
        if ( ! (sc->sent % sc->opts->report_frequency ))
        {
          double cpu_percentage;
          int    rss;
          double sslr = rr_seconds_since_last_report ( & sc->resource_reporter );
          rr_report ( & sc->resource_reporter, & cpu_percentage, & rss );
          double throughput = (double)(sc->opts->report_frequency) / sslr;
          
	  static bool first_time = true;

	  if ( first_time )
	  {
	    if ( sc->opts->print_message_size )
	    {
              fprintf(sc->report_fp, "msg_size\tsend_msgs\tcpu\trss\tthroughput\n");
	    }
	    else
	    {
              fprintf(sc->report_fp, "send_msgs\tcpu\trss\tthroughput\n");
	    }
	    first_time = false;
	  }

          // was:
          // "send_msgs: %10d   cpu: %5.1lf   rss: %6d   throughput: %8.0lf\n"
	  if ( sc->opts->print_message_size )
	  {
	    fprintf ( sc->report_fp,
		      "%d\t%d\t%lf\t%d\t%lf\n",
		      sc->opts->msg_size,
		      sc->sent,
		      cpu_percentage,
		      rss,
		      throughput
		    );
	  }
	  else
	  {
	    fprintf ( sc->report_fp,
		      "%d\t%lf\t%d\t%lf\n",
		      sc->sent,
		      cpu_percentage,
		      rss,
		      throughput
		    );
	  }
       }

        return_message(sc, msg);
      }
      if (sc->done != true && 
          sc->sent == sc->opts->msg_count && 
          !sc->opts->get_replies 
         ) 
      {
        sc->done = true;
        pn_link_close(snd);
        pn_connection_t *conn = pn_event_connection(event);
        pn_connection_close(conn);
        fclose ( sc->report_fp );
      }
    }
    break;

  case PN_LINK_INIT:
    {
      pn_link_t *link = pn_event_link(event);
      if (pn_link_is_receiver(link)) {
        // Response messages link.  Could manage credit and deliveries in this handler but
        // a dedicated handler also works.
        pn_handler_t *replyto = replyto_handler(sc);
        pn_flowcontroller_t *fc = pn_flowcontroller(1024);
        pn_handler_add(replyto, fc);
        pn_decref(fc);
        pn_handshaker_t *handshaker = pn_handshaker();
        pn_handler_add(replyto, handshaker);
        pn_decref(handshaker);
        pn_record_t *record = pn_link_attachments(link);
        pn_record_set_handler(record, replyto);
        pn_decref(replyto);
      }
    }
    break;

  case PN_CONNECTION_LOCAL_CLOSE:
    {
      // statistics_report(sc->stats, sc->sent, sc->received);
    }
    break;

  default:
    break;
  }
}





pn_handler_t *
sender_handler ( Options_t *opts, Statistics_t * stats )
{
  pn_handler_t *h = pn_handler_new(sender_dispatch, sizeof(sender_context_t), sender_cleanup);
  sender_context_t *sc = sender_context(h);
  sender_context_init(sc, opts, stats);
  return h;
}





sender_context_t *
replyto_sender_context ( pn_handler_t * h )
{
  sender_context_t **p = (sender_context_t **) pn_handler_mem(h);
  return *p;
}





void 
replyto_cleanup ( pn_handler_t * h )
{
}





void 
replyto_dispatch ( pn_handler_t *h, pn_event_t *event, pn_event_type_t type ) 
{
  sender_context_t *sc = replyto_sender_context(h);

  switch (type) {
  case PN_DELIVERY:
    {
      check(sc->opts->get_replies, "Unexpected reply message");
      pn_link_t *recv_link = pn_event_link(event);
      pn_delivery_t *dlv = pn_event_delivery(event);
      if (pn_link_is_receiver(recv_link) && !pn_delivery_partial(dlv)) {
        size_t encoded_size = pn_delivery_pending(dlv);
        check(encoded_size <= sc->encoded_data_size, "decoding buffer too small");
        ssize_t n = pn_link_recv(recv_link, sc->encoded_data, encoded_size);
        check(n == (ssize_t)encoded_size, "read fail on reply link");
        pn_message_t *msg = get_message(sc, false);
        int err = pn_message_decode(msg, sc->encoded_data, n);
        check(err == 0, "message decode error");
        statistics_msg_received(sc->stats, msg);
        sc->received++;
        pn_delivery_settle(dlv);
        return_message(sc, msg);
      }
      if (sc->received == sc->opts->msg_count) {
        pn_link_close(recv_link);
        pn_connection_t *conn = pn_event_connection(event);
        pn_connection_close(conn);
      }
    }
    break;
  default:
    break;
  }
}





pn_handler_t *
replyto_handler(sender_context_t *sc)
{
  pn_handler_t *h = pn_handler_new(replyto_dispatch, sizeof(sender_context_t *), replyto_cleanup);
  sender_context_t **p = (sender_context_t **) pn_handler_mem(h);
  *p = sc;
  return h;
}




static 
void 
parse_options ( int argc, char **argv, Options_t *opts )
{
    opterr = 0;

    memset( opts, 0, sizeof(*opts) );
    opts->msg_size  = 100;
    opts->timestamping = false;
    opts->timeout = -1;
    opts->report_frequency = 0;
    opts->recv_count = -1;
    opts->unique_message = 0;
    addresses_init(&opts->targets);

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
        strcpy ( opts->results_dir, argv[i+1] );
        ++ i;
      }
      else
      if ( ! strcmp ( "-s", arg ) )
      {
        opts->msg_size = atoi(argv[i+1]);
        fprintf ( stderr, "reactor-send: msg_size %d\n", opts->msg_size );
        ++ i;
      }
      else
      if ( ! strcmp ( "-print_message_size", arg ) )
      {
        opts->print_message_size = true;
      }
      else
      {
        fprintf ( stderr, "reactor-send unknown option: |%s|\n", arg );
        exit ( 1 );
      }
    }

    // default target if none specified
    if (opts->targets.count == 0) addresses_add( &opts->targets, "amqp://0.0.0.0" );
}





int 
main ( int argc, char ** argv )
{
  Options_t opts;
  Statistics_t stats;
  parse_options( argc, argv, &opts );

  pn_reactor_t *reactor = pn_reactor();
  pn_handler_t *sh = sender_handler(&opts, &stats);
  pn_handler_add(sh, pn_handshaker());
  pn_reactor_connection(reactor, sh);
  pn_reactor_run(reactor);
  pn_reactor_free(reactor);

  pn_handler_free(sh);
  addresses_free(&opts.targets);

  return 0;
}




