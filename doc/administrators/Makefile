SOURCE	= title overview building runtime tables tuning

troff:	$(SOURCE)
	tbl $(SOURCE) | troff -ms > admin.troff

nroff:	$(SOURCE)
	tbl $(SOURCE) | nroff -ms > admin.nroff
