/*
 * $Id: getvar.c,v 1.1 1999/10/07 13:41:10 krueger Exp $
 */

#include "util.h"
#include "cmd.h"

void *read_variable();

main(argc, argv)
int argc;char *argv[];
{
  void *value;
  int type = VARTYPE_NIL;
  
  mmdf_init(argv[0]);
  read_variable(argv[1], &value, &type);
  
  switch(type)
  {
      case VARTYPE_NIL:
      default:
        break;
      
      case VARTYPE_CHAR:
        printf("%s\n", value);
        break;
      
      case VARTYPE_INT:
        printf("%d\n", *(int *)value);
        break;
      
      case VARTYPE_OCT:
        printf("%o\n", *(int *)value);
        break;
      
      case VARTYPE_LONG:
        printf("%ld\n", *(long *)value);
        break;
  }
  
  
}