/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  util.c - common encap utility functions
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
# include <stdarg.h>
#else
# include <varargs.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/* glob matching function */
int
glob_match(char *filename, char *pattern)
{
	return !fnmatch(pattern, filename, FNM_PATHNAME | FNM_PERIOD);
}


/* subdir matching function */
int
partial_glob_match(char *dir, char *pattern)
{
	char buf1[MAXPATHLEN], buf2[MAXPATHLEN];
	char *tmp1, *tmp2;

#ifdef DEBUG
	printf("in partial_glob_match(\"%s\",\"%s\")\n", dir, pattern);
#endif

	strlcat(buf1, dir, sizeof(buf1));
	tmp1 = dirname(buf1);

	strlcat(buf2, pattern, sizeof(buf2));
	if ((tmp2 = strchr(&(buf2[strlen(tmp1) + 1]), '/')) != NULL)
		*tmp2 = '\0';

#ifdef DEBUG
	printf("returning glob_match(\"%s\",\"%s\")\n", dir, buf2);
#endif

	return glob_match(dir, buf2);
}


/*** FIXME: can this just be LHPKG_str_matchfunc() ??? ****/
/* pkgspec matching function */
int
pkg_match(char *specific_pkg, char *pkgspec)
{
	char name1[MAXPATHLEN], name2[MAXPATHLEN];
	char ver1[MAXPATHLEN], ver2[MAXPATHLEN];

	/* parse both pkgspecs */
	encap_pkgspec_parse(specific_pkg, name1, sizeof(name1),
			    ver1, sizeof(ver1), NULL, 0, NULL, 0);
	encap_pkgspec_parse(pkgspec, name2, sizeof(name2),
			    ver2, sizeof(ver2), NULL, 0, NULL, 0);

	/* no match if the names aren't the same */
	if (strcmp(name1, name2) != 0)
		return 0;

	/* if both versions match, it matches */
	if (encap_vercmp(ver1, ver2) == 0)
		return 1;

	/* otherwise, no match */
	return 0;
}


/*
** return the absolute path to what a link points to,
** based on the absolute path to the link in linkname.
** returns:
**	0				success
**	-1 (and sets errno)		failure
*/
int
get_link_dest(char *linkname, char *buf, size_t bufsize)
{
	char linkdest[MAXPATHLEN];
	char retval[MAXPATHLEN];
	int i;

#ifdef DEBUG
	printf("==> get_link_dest(\"%s\", 0x%lx, %d)\n", linkname, buf,
	       bufsize);
#endif

	i = readlink(linkname, linkdest, sizeof(linkdest));
	if (i == -1)
		return -1;
	linkdest[i] = '\0';

	if (linkdest[0] == '/')
		strcpy(retval, linkdest);
	else
		snprintf(retval, sizeof(retval), "%s/%s", dirname(linkname),
			 linkdest);

#ifdef DEBUG
	printf("    get_link_dest(): calling encap_cleanpath(\"%s\")\n",
	       linkname, retval);
#endif

	return encap_cleanpath(retval, buf, bufsize);
}


