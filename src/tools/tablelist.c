/*
 *      List tables referenced by the specified tailor file
 *
 *      Command line:
 *              tablelist [-t tailorfile]
 *
 */

#include "util.h"
#include "mmdf.h"
#include "ch.h"
#include "dm.h"
#include "chdbm.h"

extern Table **tb_list;  /* order tables searched         */
extern Domain **dm_list; /* ordr domain tables searched  */
extern Chan  **ch_tbsrch;  /* order chans searched         */

extern char *mmtailor;

/*
 * Main procedure.
 * process arguments, set flags and invoke file processing.
 * clean up and exit properly.
 */

main(argc, argv)
char **argv;
{
    int ind;
    char *p;
    register Table  *tblptr;
    char *prog;

    prog = argv[0];

    argv++; argc--;

    while(argc-- > 0)
    {
	p = *argv++;
	if(*p == '-')
	{
	    while(*++p)
		switch(*p)
		{
		    case 't':
			    mmtailor = *argv++;
			    break;

		    default:
			    fprintf (stderr,"Unknown flag %c\n",*p);
			    break;
		}
	}
    }

    mmdf_init(prog);

    /*
     *  Check for existence first
     */
	for (ind = 0; dm_list[ind] != (Domain *)0; ind++)
	    check(dm_list[ind] -> dm_table);

	for (ind = 0; ch_tbsrch[ind] != (Chan *) 0; ind++)
	{
	    check(ch_tbsrch[ind] -> ch_table);

	    if ((tblptr = ch_tbsrch[ind] -> ch_insource) != (Table *) 0)
		check(tblptr);
	    if ((tblptr = ch_tbsrch[ind] -> ch_outsource) != (Table *) 0)
		check(tblptr);
	    if ((tblptr = ch_tbsrch[ind] -> ch_indest) != (Table *) 0)
		check(tblptr);
	    if ((tblptr = ch_tbsrch[ind] -> ch_outdest) != (Table *) 0)
		check(tblptr);
	}

	for (ind = 0; (tblptr = tb_list[ind]) != (Table *) 0; ind++)
	    check (tblptr);
    } 

check (tblptr)
Table *tblptr;
{
#ifdef NAMESERVER
    if ((tblptr -> tb_flags & TB_SRC) == TB_NS)
	return;
#endif /* NAMESERVER */
    if (tblptr -> tb_fp == (FILE *)EOF)
	return;		     /* Already done */
    printf("%s\n", tblptr -> tb_file);
    tblptr -> tb_fp = (FILE *) EOF;
    return;
}
