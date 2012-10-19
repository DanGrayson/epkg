/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  log.c - transaction logging code for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <epkg.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif


/*
** write_encap_log() - write log entry
** returns:
**	0		success
**	-1		error
*/
int
write_encap_log(char *pkg, short mode, int status)
{
	char logfile[MAXPATHLEN];
	FILE *f;
	char *modestr, *returned_stat;
	char *state, *curtime;
	char *timep[5];
	time_t t;
	int i;

	/* check status */
	switch (status)
	{
	case ENCAP_STATUS_FAILED:
		returned_stat = "failed";
		break;
	case ENCAP_STATUS_SUCCESS:
		returned_stat = "success";
		break;
	case ENCAP_STATUS_PARTIAL:
		returned_stat = "partial";
		break;
	case ENCAP_STATUS_NOOP:
		return 0;
	default:
		fprintf(stderr,
			"epkg: cannot write log: unknown status %d\n", status);
		return -1;
	}

	/* check mode */
	switch (mode)
	{
	case MODE_INSTALL:
		modestr = "install";
		break;
	case MODE_REMOVE:
		modestr = "remove";
		break;
	default:
		fprintf(stderr,
			"epkg: cannot write log: unknown mode %d\n", mode);
		return -1;
	}

	/* FIXME: we should probably just use strftime() here... */
	time(&t);
	curtime = ctime(&t);
	curtime[strlen(curtime) - 1] = '\0';
	state = curtime;
	i = 0;
	while ((timep[i] = strsep(&state, " ")) != NULL && i < 5)
	{
		if (*(timep[i]) == '\0')
			continue;
		i++;
	}
	state = strrchr(timep[3], ':');
	*state = '\0';

	/* open logfile */
	snprintf(logfile, sizeof(logfile), "%s/epkg.log", source);
	f = fopen(logfile, "a");
	if (f == NULL)
	{
		fprintf(stderr, "epkg: cannot write log: fopen(\"%s\"): %s\n",
			logfile, strerror(errno));
		return -1;
	}

	/* write log entry */
	fprintf(f, "%s %2s %s %s %-20s %-24s %-7s %s\n",
		timep[1], timep[2], timep[4], timep[3], target, pkg,
		modestr, returned_stat);

	/* done */
	fclose(f);
	return 0;
}


