/*      structure for holding command table info        */

struct cmdstruct
{
    char *cmdname;      /* string reference */
    int  cmdtoken;      /* internal token  */
    int  cmdnargs;      /* minimum argument count */
};

typedef struct cmdstruct Cmd;

#define VARTYPE_NIL    0
#define VARTYPE_CHAR   1
#define VARTYPE_INT    2
#define VARTYPE_LONG   3
#define VARTYPE_OCT    4
