/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  check.c - package check recursion plugins for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <epkg.h>

#include <stdio.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/* check whether or not a package is installed */
int
check_pkg(char *pkgname)
{
	ENCAP *encap;
	int status;

	if (verbose)
		printf("  > checking package %s\n", pkgname);

	if (encap_open(&encap, source, target, pkgname,
		       options, epkg_print) == -1)
	{
		printf("  ! cannot open package %s - check aborted\n", pkgname);
		return -1;
	}
	status = encap_check(encap, NULL);
	encap_close(encap);

	switch (status)
	{
	case ENCAP_STATUS_FAILED:
		if (verbose)
			printf("    ! check failed\n");
		break;
	case ENCAP_STATUS_SUCCESS:
		if (verbose)
			printf("    > check successful\n");
		break;
	case ENCAP_STATUS_PARTIAL:
		if (verbose)
			printf("    > check partially successful\n");
		break;
	case ENCAP_STATUS_NOOP:
		break;
	default:
		printf("    ! unknown status %d\n", status);
		return -1;
	}

	return (status == ENCAP_STATUS_FAILED);
}


/* check mode */
int
check_mode(char *pkgspec)
{
	encap_list_t *ver_l;
	char *checkver;
	char pkg[MAXPATHLEN], name[MAXPATHLEN], ver[MAXPATHLEN] = "";
	encap_listptr_t lp;
	int i;

	if (verbose)
		printf("epkg: checking package %s...\n", pkgspec);

	/* if versioning is off, simply install the specified package */
	if (!BIT_ISSET(epkg_opts, EPKG_OPT_VERSIONING))
		return check_pkg(pkgspec);

	if (verbose)
		printf("  > reading Encap source directory...\n");

	ver_l = encap_list_new(LIST_USERFUNC, (encap_cmpfunc_t)encap_vercmp);
	if (ver_l == NULL)
	{
		fprintf(stderr, "    ! encap_list_new(): %s\n",
			strerror(errno));
		return -1;
	}

	i = encap_find_versions(source, pkgspec, version_list_add, ver_l);
	if (i == 0)
	{
		encap_pkgspec_parse(pkgspec, name, sizeof(name),
				    ver, sizeof(ver), NULL, 0, NULL, 0);
		i = encap_find_versions(source, name,
					version_list_add, ver_l);
		if (i == 0)
		{
			fprintf(stderr, "    ! no versions of package %s "
				"found!\n", pkgspec);
			return -1;
		}
	}
	else
		strlcpy(name, pkgspec, sizeof(name));
	if (i == -2)
		return -1;
	if (i == -1)
	{
		fprintf(stderr, "    ! find_pkg_versions(): %s\n",
			strerror(errno));
		return -1;
	}

	if (verbose > 1)
		putchar('\n');

	encap_listptr_reset(&lp);
	while (encap_list_prev(ver_l, &lp) != 0)
	{
		checkver = (char *)encap_listptr_data(&lp);
		encap_pkgspec_join(pkg, sizeof(pkg), name, checkver);
		i = check_pkg(pkg);
	}

	encap_list_free(ver_l, (encap_freefunc_t)free);

	return i;
}


