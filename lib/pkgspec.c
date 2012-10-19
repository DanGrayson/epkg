/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  pkgspec.c - encap pkgspec parsing routines
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>

#include <string.h>


int
encap_pkgspec_parse(char *pkgspec,
		    char *name, size_t nlen,
		    char *ver, size_t vlen,
		    char *platform, size_t plen,
		    char *extension, size_t elen)
{
	char buf[MAXPATHLEN];
	char *cp;

	if (name != NULL)
		name[0] = '\0';
	if (ver != NULL)
		ver[0] = '\0';
	if (platform != NULL)
		platform[0] = '\0';
	if (extension != NULL)
		extension[0] = '\0';

	if (strlcpy(buf, pkgspec, sizeof(buf)) > sizeof(buf))
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	/*
	** find the extension
	*/
	cp = strrstr(buf, ".tar");
	if (cp == NULL)
		cp = strrstr(buf, ".tgz");
	if (cp != NULL)
	{
		*cp++ = '\0';
		if (extension != NULL
		    && strlcpy(extension, cp, elen) > elen)
		{
			errno = ENAMETOOLONG;
			return -1;
		}
	}

	/*
	** find the archive suffix
	*/
	cp = strrstr(buf, "-encap-");
	if (cp != NULL)
	{
		*cp = '\0';
		cp += 7;
		if (platform != NULL
		    && strlcpy(platform, cp, plen) > plen)
		{
			errno = ENAMETOOLONG;
			return -1;
		}
	}

	/*
	** find the version
	*/
	cp = strrchr(buf, '-');
	if (cp != NULL)
	{
		*cp++ = '\0';
		if (*cp == '\0')
			cp = "-";
		if (ver != NULL
		    && strlcpy(ver, cp, vlen) > vlen)
		{
			errno = ENAMETOOLONG;
			return -1;
		}
	}

	/*
	** whatever's left is the package name
	*/
	if (name != NULL
	    && strlcpy(name, buf, nlen) > nlen)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	return 0;
}


int
encap_pkgspec_join(char *buf, size_t buflen, char *name, char *ver)
{
	if (strlcpy(buf, name, buflen) >= buflen)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	if (ver == NULL || ver[0] == '\0')
		return 0;

	if (strlcat(buf, "-", buflen) >= buflen)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	if (ver[0] == '-')
		return 0;

	if (strlcat(buf, ver, buflen) >= buflen)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	return 0;
}



#ifdef TEST_PKGSPEC_PARSE

static char *tests[] = {
	"grep",
	"qpopper.withpamid",
	"epkg-3.0+0",
	"sh-utils-1.0",
	"qpopper.withpamid-1.0",
	"sh-encap-1.0",
	"epkg.tar.gz",
	"epkg-3.0+0.tar.gz",
	"qpopper.withpamid-1.0.tar.gz",
	"epkg-3.0+0-encap-rs6000-aix4.3.3.tar.gz",
	"sh-utils-1.0-encap-sparc-solaris8.tgz",
	"sh-encap-1.0-encap-sparc-solaris8.tgz",
	"-encap-",
	NULL
};


int
main(int argc, char *argv[])
{
	int i;
	char name[MAXPATHLEN], ver[MAXPATHLEN];
	char platform[MAXPATHLEN], extension[MAXPATHLEN];

	for (i = 0; tests[i] != NULL; i++)
	{
		name[0] = ver[0] = platform[0] = extension[0] = '\0';

		if (encap_pkgspec_parse(tests[i],
					name, sizeof(name),
					ver, sizeof(ver),
					platform, sizeof(platform),
					extension, sizeof(extension)) == -1)
		{
			fprintf(stderr, "encap_pkgspec_parse(\"%s\"): %s\n\n",
				tests[i], strerror(errno));
			continue;
		}

		printf("tests[%d]\t: %s\n", i, tests[i]);
		printf("name\t\t: %s\n", name);
		if (ver[0] != '\0')
			printf("ver\t\t: %s\n", ver);
		if (platform[0] != '\0')
			printf("platform\t: %s\n", platform);
		if (extension[0] != '\0')
			printf("extension\t: %s\n", extension);
		putchar('\n');
	}

	exit(0);
}

#endif /* TEST_PKGSPEC_PARSE */

