/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  writeinfo.c - code to generate an encapinfo file for mkencap
**
**  Mark D. Roth <roth@feep.net>
*/

#include <mkencap.h>

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <netdb.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/*
** mk_set_encapinfo_date() - set encapinfo date field
*/
static void
mk_set_encapinfo_date(encapinfo_t *pkginfo)
{
#ifdef ENCAPINFO_DATE_TIME_T
	pkginfo->ei_date = time(NULL);
#else /* ! ENCAPINFO_DATE_TIME_T */
	time_t now;
	char datebuf[1024];

	now = time(NULL);
	strftime(datebuf, sizeof(datebuf), "%a %b %d %H:%M:%S %Z %Y",
		 localtime(&now));
	pkginfo->ei_date = strdup(datebuf);
#endif /* ENCAPINFO_DATE_TIME_T */
}


/*
** mk_set_encapinfo_contact() - set encapinfo contact field
*/
void
mk_set_encapinfo_contact(encapinfo_t *pkginfo)
{
	char *cp;
	struct passwd *pw;
	char hostbuf[MAXHOSTNAMELEN];
	char buf[1024];

	cp = getenv("ENCAP_CONTACT");
	if (cp != NULL)
		pkginfo->ei_contact = strdup(cp);
	else
	{
		pw = getpwuid(getuid());
		cp = strchr(pw->pw_gecos, ',');
		if (cp != NULL)
			*cp = '\0';
		gethostname(hostbuf, sizeof(hostbuf));
		snprintf(buf, sizeof(buf), "\"%s\" <%s@%s>",
			 pw->pw_gecos, pw->pw_name, hostbuf);
		pkginfo->ei_contact = strdup(buf);
	}
}


int
mk_encapinfo(char *pkgdir)
{
	char infofile[MAXPATHLEN];
	struct stat s;
	int retval = 0;

#ifdef DEBUG
	printf("==> mk_encapinfo(\"%s\")\n", pkgdir);
#endif

	snprintf(infofile, sizeof(infofile), "%s/encapinfo", pkgdir);

	if (stat(infofile, &s) == 0)
	{
		if (BIT_ISSET(mkencap_opts, MKENCAP_OPT_OVERWRITE))
		{
			if (unlink(infofile) == -1)
				fprintf(stderr, "mkencap: unlink(\"%s\"): %s\n",
					infofile, strerror(errno));
		}
		else
		{
			if (verbose)
				puts("mkencap: not replacing existing "
				     "encapinfo file");
			return 1;
		}
	}
	else if (errno != ENOENT)
	{
		fprintf(stderr, "mkencap: stat(\"%s\"): %s\n",
			infofile, strerror(errno));
		return -1;
	}

	/* set date field */
	if (!BIT_ISSET(mkencap_opts, MKENCAP_OPT_EMPTYENCAPINFO))
		mk_set_encapinfo_date(&pkginfo);
	else if (verbose)
		puts("mkencap: warning: not adding default fields "
		     "to encapinfo file");

	/* now write the new encapinfo file */
	if (verbose)
		printf("mkencap: creating encapinfo file %s\n", infofile);
	if (encapinfo_write(infofile, &pkginfo) != 0)
	{
		printf("mkencap: encapinfo_write(): %s\n", strerror(errno));
		return -1;
	}

	/* display new encapinfo file */
	if (verbose > 1)
	{
		putchar('\n');
		retval = mkencap_copy_file_to_fd(infofile, fileno(stdout));
		putchar('\n');
	}

	return retval;
}


