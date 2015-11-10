#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mjson.h"


/*###########################################################
  Local function declarations
*###########################################################*/

static
mjson_node_p
_new_node ( mjson_node_type_e type, int n_kids );


static
void
_indent ( FILE * fp, int indent );


static
void
_check_type ( mjson_node_p node, mjson_node_type_e type );


static
void
_expand_child_space ( mjson_node_p node );


static
void
_add_child ( mjson_node_p node, mjson_node_p child_1 );





/*###########################################################
  External function definitions
*###########################################################*/


char const *
mjson_node_type_2_str ( mjson_node_type_e type )
{
  switch ( type )
  {
    case MJSON_NODE_TYPE_OBJECT:
      return "MJSON_NODE_TYPE_OBJECT";

    case MJSON_NODE_TYPE_ARRAY:
      return "MJSON_NODE_TYPE_ARRAY";

    case MJSON_NODE_TYPE_STRING:
      return "MJSON_NODE_TYPE_STRING";

    case MJSON_NODE_TYPE_VALUE:
      return "MJSON_NODE_TYPE_VALUE";

    case MJSON_NODE_TYPE_NUMBER:
      return "MJSON_NODE_TYPE_NUMBER";

    case MJSON_NODE_TYPE_PAIR:
      return "MJSON_NODE_TYPE_PAIR";

    default:
      return "MJSON_NODE_TYPE_NONE";
  }
}





mjson_node_p
mjson_string_node ( char const * value )
{
  mjson_node_p node = _new_node ( MJSON_NODE_TYPE_STRING, 0 );
  node->data = strdup ( value );
  return node;
}





mjson_node_p
mjson_number_node ( double value )
{
  mjson_node_p node = _new_node ( MJSON_NODE_TYPE_NUMBER, 0 );

  node->data = (char *) malloc ( sizeof ( double ) );
  double * p = (double *)(node->data);
  *p = value;
  return node;
}





mjson_node_p
mjson_pair_node ( mjson_node_p child_1,
                  mjson_node_p child_2
                )
{
  if ( child_1->type != MJSON_NODE_TYPE_STRING )
  {
    fprintf ( stderr, "mjson error: mjson_pair_node: first child must be string.\n" );
    exit ( 1 );
  }

  mjson_node_p node = _new_node ( MJSON_NODE_TYPE_PAIR, 2 );
  _add_child ( node, child_1 );
  _add_child ( node, child_2 );
  return node;
}





mjson_node_p
mjson_object_node ( int n_children )
{
  mjson_node_p node = _new_node ( MJSON_NODE_TYPE_OBJECT, 2 );

  node->max_children = n_children;
  node->children     = (char **) malloc ( node->max_children * sizeof(char*) );
  node->n_children   = 0;

  return node;
}





mjson_node_p
mjson_array_node ( int n_children )
{
  mjson_node_p node = _new_node ( MJSON_NODE_TYPE_ARRAY, 2 );

  node->max_children = n_children;
  node->children     = (char **) malloc ( node->max_children * sizeof(char*) );
  node->n_children   = 0;

  return node;
}





mjson_node_p
mjson_add_labeled_object ( mjson_node_p old_object, char const * label )
{
  mjson_node_p label_string       = mjson_string_node ( label );
  mjson_node_p new_object         = mjson_object_node ( 1 );
  mjson_node_p string_object_pair = mjson_pair_node ( label_string, new_object );
  mjson_add_child ( old_object, string_object_pair );

  return new_object;
}





mjson_node_p
mjson_add_labeled_object ( mjson_node_p old_object,
                           char const * label,
                           mjson_node_p new_object
                         )
{
  mjson_node_p label_string       = mjson_string_node ( label );
  mjson_node_p string_object_pair = mjson_pair_node ( label_string, new_object );
  mjson_add_child ( old_object, string_object_pair );

  return new_object;
}





mjson_node_p
mjson_add_labeled_array ( mjson_node_p old_object, char const * label )
{
  mjson_node_p label_string       = mjson_string_node ( label );
  mjson_node_p array              = mjson_array_node ( 4 );
  mjson_node_p string_array_pair = mjson_pair_node ( label_string, array );
  mjson_add_child ( old_object, string_array_pair );

  return array;
}





void
mjson_add_child ( mjson_node_p node, char const * str )
{
  mjson_node_p str_node = mjson_string_node ( str );
  mjson_add_child ( node, str_node );
}





void
mjson_add_child ( mjson_node_p node, mjson_node_p child )
{
  switch ( node->type )
  {
    case MJSON_NODE_TYPE_OBJECT:
    {
      if ( child->type != MJSON_NODE_TYPE_PAIR )
      {
        fprintf ( stderr, "mjson_error: mjson_add_child: object child must be pair.\n" );
        exit ( 1 );
      }

      _add_child ( node, child );
    }
    break;



    case MJSON_NODE_TYPE_ARRAY:
    {
      if ( !
           (
             (child->type == MJSON_NODE_TYPE_STRING ) ||
             (child->type == MJSON_NODE_TYPE_NUMBER ) ||
             (child->type == MJSON_NODE_TYPE_OBJECT ) ||
             (child->type == MJSON_NODE_TYPE_ARRAY  )
           )
         )
      {
        fprintf ( stderr, "mjson_error: mjson_add_child: bad child type for array.\n" );
        exit ( 1 );
      }
      
      _add_child ( node, child );
    }
    break;



    default:
      fprintf ( stderr, 
                "mjson_error: mjson_add_child: can't add child to node type |%s|\n",
                mjson_node_type_2_str ( node->type )
              );
      exit ( 1 );
    break;
  }
}






// Sorts only this node -- not recursive.
void
mjson_sort_children ( mjson_node_p node )
{
  switch ( node->type )
  {
    // If this is anobject, then it will have a list of pairs.
    case MJSON_NODE_TYPE_OBJECT:
    if ( node->n_children > 1 )
    {
      bool disordered = true;
      while ( disordered )
      {
        disordered = false;
        for ( int i = 1; i < node->n_children; ++ i )
        {
          mjson_node_p pair_node_0 = (mjson_node_p) (node->children[i - 1]);
          mjson_node_p pair_node_1 = (mjson_node_p) (node->children[i]);

          mjson_node_p string_node_0 = (mjson_node_p) (pair_node_0->children[0]);
          mjson_node_p string_node_1 = (mjson_node_p) (pair_node_1->children[0]);

          char * str_0 = string_node_0->data;
          char * str_1 = string_node_1->data;

          if ( 0 < strcmp ( str_0, str_1 ) )
          {
            char * temp = node->children[i - 1];
            node->children [ i - 1 ] = node->children [ i ];
            node->children [ i ] = temp;
            disordered = true;
          }
        }
      }
    }
    break;

    default:
    break;
  }
}






void
mjson_print ( mjson_node_p node,
              FILE * fp,
              int indent,
              bool sort
            )
{
  switch ( node->type )
  {
    case MJSON_NODE_TYPE_NUMBER:
    {
      _indent ( fp, indent );
      double * p = (double *)(node->data);
      fprintf ( fp, "%.3lf", *p );
    }
    break;


    case MJSON_NODE_TYPE_STRING:
    {
      _indent ( fp, indent );
      fprintf ( fp, "\"%s\"", node->data );
    }
    break;


    case MJSON_NODE_TYPE_PAIR:
    {
      _indent ( fp, indent );
      mjson_node_p child_0 = (mjson_node_p) (node->children[0]),
                   child_1 = (mjson_node_p) (node->children[1]);

      fprintf ( fp, "\"%s\" : ", child_0->data );

      if ( 
           (child_1->type != MJSON_NODE_TYPE_OBJECT)
           &&
           (child_1->type != MJSON_NODE_TYPE_ARRAY)
         )
      {
        // scalar: print on same line as string key.
        mjson_print ( child_1, fp, 0, false );
      }
      else
      {
        fputs ( "\n", fp );
        mjson_print ( child_1, fp, indent, sort );
      }
    }
    break;


    case MJSON_NODE_TYPE_OBJECT:
    {
      if ( sort )
        mjson_sort_children ( node );

      _indent ( fp, indent );
      fprintf ( fp, "{\n" );

      for ( int i = 0; i < node->n_children; ++ i )
      {
        mjson_print ( (mjson_node_p)(node->children[i]), fp, indent + 2, sort );
        if ( i < node->n_children - 1 )
          fputs ( ",\n", fp );
        else
          fputs ( "\n", fp );
      }
      
      _indent ( fp, indent );
      fprintf ( fp, "}" );
    }
    break;


    case MJSON_NODE_TYPE_ARRAY:
    {
      _indent ( fp, indent );
      fprintf ( fp, "[\n" );

      for ( int i = 0; i < node->n_children; ++ i )
      {
        mjson_print ( (mjson_node_p)(node->children[i]), fp, indent + 2, sort );
        if ( i < node->n_children - 1 )
          fputs ( ",\n", fp );
        else
          fputs ( "\n", fp );
      }
      
      _indent ( fp, indent );
      fprintf ( fp, "]" );
    }
    break;


    default:
      fprintf ( stderr, 
                "mjson error: printing node with unknown type |%s|.\n",
                mjson_node_type_2_str ( node->type )
              );
      exit ( 1 );
  }
}





/*###########################################################
  Local function definitions
*###########################################################*/


mjson_node_p
_new_node ( mjson_node_type_e type, int n_kids )
{ 
  mjson_node_p node = 0;

  node = (mjson_node_p) malloc ( sizeof ( mjson_node_t ) );

  node->type         = type;

  node->data         = 0;
  node->max_children = 0;
  node->n_children   = 0;
  node->children     = 0;

  if ( n_kids > 0 )
  {
    node->max_children = n_kids;
    node->n_children   = 0;
    node->children     = (char **) malloc ( node->max_children * sizeof(char*) );
  }

  return node;
}





static
void
_indent ( FILE * fp, int indent )
{
  for ( int i = 0; i < indent; ++ i )
    fputc ( ' ', fp );
}





static
void
_check_type ( mjson_node_p node, mjson_node_type_e type )
{
  if ( node->type != type )
  {
    fprintf ( stderr, 
              "mjson_node_string error: node type is |%s|, but should be |%s|.\n", 
              mjson_node_type_2_str ( node->type ),
              mjson_node_type_2_str ( type )
            );
    exit ( 1 );
  }
}





static
void
_expand_child_space ( mjson_node_p node )
{
  if ( node->max_children == 0 )
    node->max_children = 1;
  else
    node->max_children *= 2;

  char ** new_space = (char **) malloc ( node->max_children * sizeof(char*) );
  for ( int i = 0; i < node->n_children; ++ i )
  {
    new_space[i] = node->children[i];
  }
  free ( node->children );
  node->children = new_space;
}





static
void
_add_child ( mjson_node_p node, mjson_node_p new_child )
{
  if ( node->n_children >= node->max_children )
    _expand_child_space ( node );
  
  node->children [ node->n_children ] = (char *) new_child;

  ++ node->n_children;
}





