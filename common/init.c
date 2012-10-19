/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  init.c - epkg initialization code
**
**  Mark D. Roth <roth@feep.net>
*/

#include <init.h>

#include <stdio.h>
#include <sys/param.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/* global variables */
char source[MAXPATHLEN] = DEFAULT_SOURCE;
char target[MAXPATHLEN] = DEFAULT_TARGET;


/* initialize source and target */
void
init_encap_paths(char *cwd, char *opt_src, char *opt_tgt)
{
	char *srcenv, *tgtenv;
	char buf[MAXPATHLEN];

	/*
	** if neither is set from the commandline, try the environment
	** variables
	*/
	if (opt_src == NULL && opt_tgt == NULL)
	{
		opt_src = getenv("ENCAP_SOURCE");
		if (opt_src != NULL && opt_src[0] != '/')
			opt_src = NULL;
		opt_tgt = getenv("ENCAP_TARGET");
		if (opt_tgt != NULL && opt_tgt[0] != '/')
			opt_tgt = NULL;
	}

	if (opt_src != NULL)
	{
		if (opt_src[0] != '/')
			snprintf(buf, sizeof(buf), "%s/%s", cwd, opt_src);
		else
			strlcpy(buf, opt_src, sizeof(buf));
		encap_cleanpath(buf, source, sizeof(source));
	}

	if (opt_tgt != NULL)
	{
		if (opt_tgt[0] != '/')
			snprintf(buf, sizeof(buf), "%s/%s", cwd, opt_tgt);
		else
			strlcpy(buf, opt_tgt, sizeof(buf));
		encap_cleanpath(buf, target, sizeof(target));
	}

	if (opt_src != NULL && opt_tgt == NULL)
	{
		snprintf(buf, sizeof(buf), "%s/..", source);
		encap_cleanpath(buf, target, sizeof(target));
	}

	if (opt_tgt != NULL && opt_src == NULL)
	{
		snprintf(buf, sizeof(buf), "%s/encap", target);
		encap_cleanpath(buf, source, sizeof(source));
	}

	/* now set environment variables for package scripts */
	snprintf(buf, sizeof(buf), "ENCAP_SOURCE=%s", source);
	srcenv = strdup(buf);
	if (putenv(srcenv) != 0)
	{
		perror("putenv()");
		exit(1);
	}
	snprintf(buf, sizeof(buf), "ENCAP_TARGET=%s", target);
	tgtenv = strdup(buf);
	if (putenv(tgtenv) != 0)
	{
		perror("putenv()");
		exit(1);
	}
}


