/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  version.c - package version routines
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <ctype.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/*
** calls verfunc() for all versions of pkgname in the source directory.
** returns:
**	number of versions found	success
**	-1 (and sets errno)		failure
**	-2				verfunc() returned -1
*/
int
encap_find_versions(char *source, char *pkgname,
		    verfunc_t verfunc, void *state)
{
	DIR *dirp;
	struct dirent *dep;
	struct stat s;
	int numvers = 0;
	char path[MAXPATHLEN], name[MAXPATHLEN], ver[MAXPATHLEN];

#ifdef DEBUG
	printf("==> encap_find_versions(\"%s\")\n", pkgname);
#endif

	dirp = opendir(source);
	if (dirp == NULL)
		return -1;

	while ((dep = readdir(dirp)) != NULL)
	{
		if (strcmp(dep->d_name, ".") == 0
		    || strcmp(dep->d_name, "..") == 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s", source, dep->d_name);
		if (stat(path, &s) == -1)
		{
			closedir(dirp);
			return -1;
		}
		if (!S_ISDIR(s.st_mode))
			continue;

		encap_pkgspec_parse(dep->d_name, name, sizeof(name),
				    ver, sizeof(ver), NULL, 0, NULL, 0);
		if (strcmp(pkgname, name) != 0)
			continue;

		if (verfunc != NULL
		    && (*verfunc)(state, pkgname, ver) == -1)
			return -2;

		numvers++;
	}

#ifdef DEBUG
	printf("<== encap_find_versions(): found %d versions\n", numvers);
#endif
	return numvers;
}


