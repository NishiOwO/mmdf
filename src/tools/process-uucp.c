/*
 * process-uucp.c -- Make mmdf tables from uucp project derived information.
 *
 * USAGE: process-uucp [ -d domain ... ] [ -r relay-host ] 
 *			[ -c host ... ] file ...
 *
 * -d domain		generate a domain table for the domain.
 *			The file name is "wrkdmn.uucp-dom.ain", and
 *			the entries are like:
 *
 *	dom: dom.ain
 *
 * -r relay-host 	have the entries in the domain file be
 *			routed through "relay-host". i.e.
 *
 *	dom: dom.ain, relay-host
 *
 * -c host		This causes the wrkchn.uucp-uucp file to be split into
 *			multiple files.  If a path has its' first component
 *			as the named host, then the entry will be put
 *			into a seperate file named "wrkchn.uucp-host".
 *
 * The input file is the output of pathalias.  The format is:
 *
 * {host,domain}	a!b!c!...%s...
 *
 * From this we make both domain tables and channel tables.
 *
 * There are a couple of files which are automatically created:
 *
 * wrkchn.uucp-uucp	This contains domain:path entries for any paths
 *			which did not match hosts given in the -c list.
 * wrkdmn.uucp-top	This domains dom:dom.ain entries for domains
 *			which did not match domains given in -d's.
 *
 * A word about the file naming scheme.  The general format of file
 * names which I use on tables is "<type>.<source>-<subtype>".
 * "<type>" is something like domain, chan, or alias.  "<source>"
 * is some identifier saying where the information came from, like
 * uucp or bitnet.  "<subtype>" is something like domain for domain
 * tables, channel name for channel tables, or some other sort of
 * identifier depending on what the table is for.  (For instance,
 * I also have alias-global, alias-forward, and alias-local tables).
 *
 * Suggested use is something like:
 *
 * process-uucp -d uucp -d edu -d arpa pathalias-output-file
 *
 * AUTHOR:
 * 
 * David Herron, University of Kentucky Mathematical Sciences
 * Mon Nov 16 17:39:18 EST 1987
 */

#include <stdio.h>
#include <ctype.h>

struct namelist {
	char *name;
	char *fname;
	/* FILE *ofile; */
};

/* File name prefixes */
#define WRK_CHAN	"wrkchn.uucp"
#define WRK_DMN		"wrkdmn.uucp"

void process();
char buf[BUFSIZ];


void addfile();
char *files[BUFSIZ];
int numfiles = 0;


void addomain();
struct namelist domains[BUFSIZ];
int numdomains = 0;


void addhost();
struct namelist hosts[BUFSIZ];
int numhosts = 0;


char *lowerify();
char *RelayHost = (char *)0;

main(argc, argv)
int argc;
char **argv;
{
	int i;

	if (argc <= 1) {
		fprintf(stderr, "Usage: %s file ...\n", argv[0]);
		exit(1);
	}
	sprintf(buf, "%s-uucp", WRK_CHAN);
	close(creat(buf, 0644));
	for (i=1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch(argv[i][1]) {
			case 'd':
				i++;
				printf("addomain(%s);\n", argv[i]);
				addomain(argv[i]);
				break;
			case 'c':
				i++;
				printf("addhost(%s);\n", argv[i]);
				addhost(argv[i]);
				break;
			case 'r':
				i++;
				RelayHost = lowerify(argv[i]);
				printf("relay-host: %s\n", RelayHost);
				break;
			default:
				break;
			}
		}
		else {
			addfile(argv[i]);
		}
	}
	for (i=0; i<numfiles; i++) {
		/* printf("process(%s);\n", files[i]); */
		process(files[i]);
	}
	exit(0);
}

void
addomain(s)
char *s;
{
	int slen;

	domains[numdomains].name = s;
	slen = strlen(WRK_DMN) + 1 + strlen(s) + 1;
	domains[numdomains].fname = malloc(slen);
	sprintf(domains[numdomains].fname, "%s-%s", WRK_DMN, s);
	numdomains++;
}

void
addhost(s)
char *s;
{
	int slen;

	hosts[numhosts].name = s;
	slen = strlen(WRK_DMN) + 1 + strlen(s) + 1;
	hosts[numhosts].fname = malloc(slen);
	sprintf(hosts[numhosts].fname, "%s-%s", WRK_CHAN, s);
	numhosts++;
}

void
addfile(s)
char *s;
{
	files[numfiles++] = s;
}

char *
lowerify(s)
char *s;
{
	register char *sp = s;

	while (*sp != '\0') {
		if (isupper(*sp))
			*sp = tolower(*sp);
		sp++;
	}
	return(s);
}

/*
 * indomain(name, domain) -- Test to see if the given name is 
 * within a domain ... we do this as a sort of reverse-strcmp().
 *
 * The return value is NULL if it is not, and a pointer within the
 * tested name if it is.
 */
char *
indomain(name, domain)
char *name, *domain;
{
	register char *namp, *domp;

	namp = name; domp = domain;
	if (!namp || !domp)
		return(NULL);
	/* printf("indomain(%s, %s); -- ", name, domain); */
	/* Find the end of the strings */
	namp = &(name[strlen(name)-1]);
	domp = &(domain[strlen(domain)-1]);
	/* printf("start at namp=<%s> domp=<%s> -- ", namp, domp); */
	for (; namp>=name && domp>=domain; namp--, domp--)
		if (namp[0] != domp[0])
			break;
	namp++; domp++;
	/* printf("match at namp=<%s> domp=<%s>\n", namp, domp); */
	if (
	       (namp>name && domp==domain && namp[-1]=='.') 
	    || namp == name
	   )
		return(namp);
	else
		return(NULL);
}

/* These have to be here because the compiler chokes if they're
 * inside process().
 */
char pathbuf[BUFSIZ], dmnbuf[BUFSIZ], bf2[BUFSIZ];

/*
 * process(file) -- Process the file making all the tables which 
 * which we were told to make on the command line.
 *
 * A note about the algorithm.  If you read the code below you will
 * notice that I am constantly open()ing and close()ing files to
 * append records to the end of the files.  This is because, in
 * theory at least, someone could have 50 domain files that they
 * generate from this program.  
 *
 * Maybe in practice one will never have more than 10 (to pick a number 
 * out of the air).  If you really want to "fix" this, be my guest.  It 
 * should be a fairly trivial re-write.  The "fix" I have in mind is to
 * change "domains[]" into an array of structures remembering the domain
 * name, the file name for the domain, and the FILE * pointing to that
 * file.  Then there are a few places below where one looks up the
 * file name, fopen()s the file in "a" mode, write the record, and close
 * the file.  One would need to change those places to merely write the 
 * record after looking up the FILE * in the struct namelist.
 *
 * NOTE NOTE NOTE ... The procedure as it is opens files in append mode
 * and there's no automatic truncation of any files.  You will have to
 * do this by hand.  (Well, engrave it in your shell script which processes
 * the pathalias output).
 */
void
process(file)
char *file;
{
	register char *sp, *sp2;
	register int i;
	FILE *ifile, *cfile, *tcfile, *dfile, *topfile;
	int hlen;
	char *index(), *strcpy(), *strcat();

	ifile = fopen(file, "r");
	if (ifile == NULL) {
		/* printf("Open of %s failed with errno=%d\n", file, errno); */
		perror(file);
		return;
	}
	sprintf(buf, "%s-uucp", WRK_CHAN);
	cfile = fopen(buf, "a");
	if (cfile == NULL) {
		/* printf("Open of %s failed with errno=%d\n", buf, errno); */
		perror(buf);
		fclose(ifile);
		return;
	}
	sprintf(buf, "%s-top", WRK_DMN);
	topfile = fopen(buf, "a");
	if (topfile == NULL) {
		/* printf("Open of %s failed with errno=%d\n", buf, errno); */
		perror(buf);
		fclose(ifile);
		fclose(cfile);
		return;
	}
	while (fgets(buf, BUFSIZ, ifile) != NULL) {
		if ((sp = index(buf, '\n')) != NULL)
			*sp = '\0';
		/* copy the domain part */
		for (sp=buf, sp2=dmnbuf;
		     *sp!='\0' && *sp!=' ' && *sp!='\t';
		     sp++, sp2++)
			*sp2 = *sp;
		*sp2 = '\0';
		/* Then the path part */
		if (*sp != '\0') {
			while (*sp==' ' || *sp=='\t')
			     sp++;
			for (sp2=pathbuf;
			     *sp!='\0' && *sp!=' ' && *sp!='\t';
			     sp++, sp2++)
				*sp2 = *sp;
			*sp2 = '\0';
		}
		if (pathbuf[0] == '\0')
			strcpy(pathbuf, "NO-PATH-GIVEN!%s");
		/* printf("before = %s\n", dmnbuf); */
		(void) lowerify(dmnbuf);
		/* printf("after = %s\n", dmnbuf); */
		if ((sp = index(dmnbuf, '.')) == NULL)
			strcat(dmnbuf, ".uucp");
		if (dmnbuf[0] == '.')
			for (sp = dmnbuf; *sp != '\0'; sp++)
				*sp = *(sp+1);
		/*
		 * If the given domain name is within one of those for which
		 * we have a domain table, output it into the domain table
		 * and into the channel table.
		 */
		for (i=0; i<numdomains; i++) {
			if ((sp2=indomain(dmnbuf, domains[i].name)) != NULL) {
				if (sp2 > dmnbuf) {
					dfile = fopen(domains[i].fname, "a");
					/* write out first part of dom.ain */
					for (sp=dmnbuf; sp < (sp2-1); sp++)
						fputc(*sp, dfile);
					if (RelayHost)
						fprintf(dfile, ":\t%s, %s\n", 
							dmnbuf, RelayHost);
					else
						fprintf(dfile, ":\t%s\n", dmnbuf);
					fclose(dfile);
					break;
				}
				else {
					/*
					 * This case is for when the name we're
					 * testing fully matches the domain name
					 * for one of the tables.  We aren't 
					 * supposed to make an entry for him
					 * in this table, but in the table for
					 * the parent domain.  But we don't
					 * know here whether we are keeping
					 * a table for that domain ... HOWEVER,
					 * if we keep looping through the rest of
					 * the table names we will eventually 
					 * find out.
					 */
					continue;
				}
			}
		}
		/*
		 * This case is for when we've run through the entire list 
		 * of domains for which we have tables and the name we
		 * have does not fit with any of those.  So, we insert
		 * it into the top-level table.
		 */
		if (i == numdomains) {
			if (RelayHost)
				fprintf(topfile, "%s:\t%s, %s\n", 
					dmnbuf, dmnbuf, RelayHost);
			else
				fprintf(topfile, "%s:\t%s\n", dmnbuf, dmnbuf);
		}
		/*
		 * This section handles making entries in the channel table.
		 */
		if (numhosts > 0) {
			for (i=0; i<numhosts; i++) {
				hlen = strlen(hosts[i].name);
				/* This shortcuts if we already know that
				 * the names CANNOT match. i.e. if the next
				 * character is not '!', then the lengths
				 * of the two names are not the same.
				 */
				if (pathbuf[hlen] != '!')
					continue;
				if (strncmp(pathbuf, hosts[i].name, hlen) != 0)
					continue;
				tcfile = fopen(hosts[i].fname, "a");
				fprintf(tcfile, "%s:\t%s\n", dmnbuf, pathbuf);
				fclose(tcfile);
				break;
			}
			/* If we make it all the way through the list, then
			 * we didn't write out the record to a file.  Put
			 * the record out into the default file.
			 */
			if (i == numhosts)
				fprintf(cfile, "%s:\t%s\n", dmnbuf, pathbuf);
		}
		else
			fprintf(cfile, "%s:\t%s\n", dmnbuf, pathbuf);
	}
	/* printf("done with %s\n", file); */
	fclose(cfile);
	fclose(ifile);
	fclose(topfile);
}
