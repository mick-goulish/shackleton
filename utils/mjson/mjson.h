#ifndef MJSON_H
#define MJSON_H

typedef
enum
{
  MJSON_NODE_TYPE_NONE,
  MJSON_NODE_TYPE_OBJECT,
  MJSON_NODE_TYPE_ARRAY,
  MJSON_NODE_TYPE_STRING,
  MJSON_NODE_TYPE_VALUE,
  MJSON_NODE_TYPE_NUMBER,
  MJSON_NODE_TYPE_PAIR
}
mjson_node_type_e;



typedef
struct mjson_node_s
{
  mjson_node_type_e type;
  char * data;
  int n_children,
      max_children;
  char ** children;
}
mjson_node_t,
* mjson_node_p;



char const *
mjson_node_type_2_str ( mjson_node_type_e );



// Make a new string node.
mjson_node_p
mjson_string_node ( char const * value );



// Make a new number node.
mjson_node_p
mjson_number_node ( double value );



// Make a new pair node.
mjson_node_p
mjson_pair_node ( mjson_node_p child_1,
                  mjson_node_p child_2
                );



// Make a new object node.
mjson_node_p
mjson_object_node ( int n_children );



// Make a new array node.
mjson_node_p
mjson_array_node ( int n_children );



/*-------------------------------------------------------
  Add a child to a node.  
  The parent node must be of type object or array.
-------------------------------------------------------*/
void
mjson_add_child ( mjson_node_p node, mjson_node_p child );

void
mjson_add_child ( mjson_node_p node, char const * str );



/*--------------------------------------------------------------
  This is a convenience function for a common special case.
  You want to add to an object a pair that looks like this:
     "label" : { }  
  And, since you are going to do a big chain of these, you
  want it to return the { } -- so you can do it again to that 
  object.
  The first form is for when you do not already have the
  child object created, and you just want to make an empty one.
--------------------------------------------------------------*/
mjson_node_p
mjson_add_labeled_object ( mjson_node_p parent_object, 
                           char const * label 
                         );


mjson_node_p
mjson_add_labeled_object ( mjson_node_p old_object, 
                           char const * label,
                           mjson_node_p child_object
                         );



mjson_node_p
mjson_add_labeled_array ( mjson_node_p old_object, char const * label );


// Sorts only this node -- not recursive.
void
mjson_sort_children ( mjson_node_p node );


void
mjson_print ( mjson_node_p node,
              FILE * fp,
              int indent,
              bool sort
            );



#endif // MJSON_H
