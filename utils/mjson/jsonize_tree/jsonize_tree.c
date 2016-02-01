#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <string.h>



#include "mjson.h"


mjson_node_p
jsonize_dir ( mjson_node_p parent_node, char const * dir_name );

bool
leaf_node ( char const * dir_name );

mjson_node_p
jsonize_leaf ( mjson_node_p parent_node, char const * dir_name );

mjson_node_p
jsonize_nonleaf ( mjson_node_p parent_node, char const * dir_name );

mjson_node_p
jsonize_dir ( mjson_node_p parent_node, char const * dir_name );






bool
leaf_node ( char const * dir_name )
{
  bool its_a_leaf = false;

  DIR * dir = opendir ( dir_name );
  struct dirent * file;
  while ( file = readdir ( dir ) )
  {
    if ( file->d_name[0] == '.')
      continue;

    char fullname[2000];
    sprintf ( fullname, "%s/%s", dir_name, file->d_name );

    struct stat sb;
    stat ( fullname, & sb );

    if ((sb.st_mode & S_IFMT) == S_IFREG) 
    {
      its_a_leaf = true;
      break;
    }
  }

  closedir ( dir );

  return its_a_leaf;
}





mjson_node_p
jsonize_leaf ( mjson_node_p parent_node, char const * dir_name )
{
  DIR * dir = opendir ( dir_name );
  struct dirent * file;
  char * temp = strdup ( dir_name );
  mjson_node_p dir_contents_array = mjson_add_labeled_array ( parent_node, basename(temp) );

  while ( file = readdir ( dir ) )
  {
    if ( file->d_name[0] == '.' )
      continue;

    char fullname[2000];
    sprintf ( fullname, "%s/%s", dir_name, file->d_name );

    struct stat sb;
    stat ( fullname, & sb );

    if ((sb.st_mode & S_IFMT) == S_IFREG) 
    {
      mjson_add_child ( dir_contents_array, file->d_name );
    }
    else
    if ((sb.st_mode & S_IFMT) == S_IFDIR)
    {
      fprintf ( stderr, "mjson warning: ignoring dir |%s| in leaf node.\n", fullname );
    }
  }

  closedir ( dir );

  return parent_node;
}





mjson_node_p
jsonize_nonleaf ( mjson_node_p parent_node, char const * dir_name )
{
  DIR * dir = opendir ( dir_name );
  struct dirent * file;
  char * temp = strdup ( dir_name );
  mjson_node_p dir_contents_node = mjson_add_labeled_object ( parent_node, basename(temp) );

  while ( file = readdir ( dir ) )
  {
    if ( file->d_name[0] == '.' )
      continue;

    char fullname[2000];
    sprintf ( fullname, "%s/%s", dir_name, file->d_name );

    struct stat sb;
    stat ( fullname, & sb );

    if ((sb.st_mode & S_IFMT) == S_IFREG)
    {
      fprintf ( stderr, 
                "mjson error: Mick promised this would never happen. |%s|\n",
                fullname
              );
      exit ( 1 );
    }
    else
    if ((sb.st_mode & S_IFMT) == S_IFDIR)
    {
      jsonize_dir ( dir_contents_node, fullname );
    }
  }

  closedir ( dir );

  return parent_node;
}





mjson_node_p
jsonize_dir ( mjson_node_p parent_node, char const * dir_name )
{
  if ( leaf_node ( dir_name ) )
    return jsonize_leaf ( parent_node, dir_name );
  else
    return jsonize_nonleaf ( parent_node, dir_name );
}





/*
  /var/www/html/perf/results/tests
    will contain 2015, 2016, etc.

*/
int
main ( int argc, char ** argv )
{ 
  if ( argc != 2 )
  {
    fprintf ( stderr, "jsonize_tree error: I need target dir on command line.\n" );
    exit ( 1 );
  }

  char * target_dir = argv[1];
  mjson_node_p base_node = mjson_object_node ( 1 );

  jsonize_dir ( base_node, target_dir );
  mjson_print ( base_node, stdout, 2, true );
  fputc ( '\n', stdout );
}





