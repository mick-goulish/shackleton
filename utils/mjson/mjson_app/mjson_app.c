
#include <stdio.h>

#include "mjson.h"







int
main ( )
{ 
  mjson_node_p top_level      = mjson_object_node ( 1 );
  mjson_node_p results_object = mjson_add_labeled_pair ( top_level,      "results" );
  mjson_node_p tests_object   = mjson_add_labeled_pair ( results_object, "tests" );
  mjson_node_p year_object    = mjson_add_labeled_pair ( tests_object,   "2015" );
  mjson_node_p month_object   = mjson_add_labeled_pair ( year_object,    "10" );
  mjson_node_p day_object     = mjson_add_labeled_pair ( month_object,   "27" );



  // message_size ----------------------------------------
  mjson_node_p message_size   = mjson_add_labeled_pair ( day_object,   "message_size" );
  mjson_node_p ms_C           = mjson_add_labeled_pair ( message_size, "C" );


  mjson_node_p receiver_1     = mjson_string_node ( "receiver" );
  mjson_node_p array_1        = mjson_array_node ( 3 );
  mjson_add_child ( array_1, "recv_bps_vs_message_size.jpg" );
  mjson_add_child ( array_1, "recv_message_size_vs_cpu.jpg" );
  mjson_add_child ( array_1, "recv_mps_vs_message_size.jpg" );
  mjson_node_p pair_1 = mjson_pair_node ( receiver_1, array_1 );
  mjson_add_child ( ms_C, pair_1 );


  mjson_node_p sender_1     = mjson_string_node ( "sender" );
  mjson_node_p array_2      = mjson_array_node ( 1 );
  mjson_add_child ( array_2, "send_message_size_vs_cpu.jpg" );
  mjson_node_p pair_2 = mjson_pair_node ( sender_1, array_2 );
  mjson_add_child ( ms_C, pair_2 );



  // p2p_soak ----------------------------------------
  mjson_node_p p2p_soak   = mjson_add_labeled_pair ( day_object,   "p2p_soak" );
  mjson_node_p pp_C       = mjson_add_labeled_pair ( p2p_soak, "C" );


  mjson_node_p receiver_2     = mjson_string_node ( "receiver" );
  mjson_node_p array_3        = mjson_array_node ( 4 );
  mjson_add_child ( array_3, "cpu.jpg" );
  mjson_add_child ( array_3, "memory.jpg" );
  mjson_add_child ( array_3, "throughput.jpg" );
  mjson_add_child ( array_3, "latency.jpg" );
  mjson_node_p pair_3 = mjson_pair_node ( receiver_2, array_3 );
  mjson_add_child ( pp_C, pair_3 );


  mjson_node_p sender_2     = mjson_string_node ( "sender" );
  mjson_node_p array_4      = mjson_array_node ( 3 );
  mjson_add_child ( array_4, "cpu.jpg" );
  mjson_add_child ( array_4, "memory.jpg" );
  mjson_add_child ( array_4, "throughput.jpg" );
  mjson_node_p pair_4 = mjson_pair_node ( sender_2, array_4 );
  mjson_add_child ( pp_C, pair_4 );




  mjson_print ( top_level, stderr, 2 );
}





