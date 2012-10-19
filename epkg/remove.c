/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  remove.c - package removal mode for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <epkg.h>

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
# include <stdarg.h>
#else
# include <varargs.h>
#endif


/* remove package */
int
remove_pkg(char *pkg)
{
	ENCAP *encap;
	int status;

	if (verbose)
		printf("  > removing package %s\n", pkg);

	if (encap_open(&encap, source, target, pkg, options,
		       epkg_print) == -1)
	{
		printf("  ! cannot open package %s - removal aborted\n", pkg);
		return -1;
	}
	status = encap_remove(encap, exclude_decision);
	encap_close(encap);

	/* write logfile */
	if (status != ENCAP_STATUS_NOOP &&
	    !BIT_ISSET(options, OPT_SHOWONLY) &&
	    BIT_ISSET(epkg_opts, EPKG_OPT_WRITELOG))
		write_encap_log(pkg, MODE_REMOVE, status);

	switch (status)
	{
	case ENCAP_STATUS_FAILED:
		if (verbose)
			printf("    ! removal failed\n");
		break;
	case ENCAP_STATUS_SUCCESS:
		if (verbose)
			printf("    > removal successful\n");
		break;
	case ENCAP_STATUS_PARTIAL:
		if (verbose)
			printf("    > removal partially successful\n");
		break;
	case ENCAP_STATUS_NOOP:
		break;
	default:
		printf("    ! unknown status %d\n", status);
		return -1;
	}

	if (status == ENCAP_STATUS_FAILED ||
	    ((status != ENCAP_STATUS_NOOP) && BIT_ISSET(options, OPT_SHOWONLY)))
		return 1;

	return 0;
}


/* remove mode */
int
remove_mode(char *pkgspec)
{
	encap_list_t *ver_l;
	char buf[MAXPATHLEN], name[MAXPATHLEN], ver[MAXPATHLEN] = "";
	encap_listptr_t lp;
	int i;

	if (verbose)
		printf("epkg: removing package %s...\n", pkgspec);

	/* if versioning is off, simply remove the specified package */
	if (!BIT_ISSET(epkg_opts, EPKG_OPT_VERSIONING))
		return remove_pkg(pkgspec);

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
		fprintf(stderr, "    ! encap_find_versions(): %s\n",
			strerror(errno));
		return -1;
	}

	if (ver[0] == '\0')
		i = remove_package_versionlist(name, ver_l);
	else
	{
		encap_listptr_reset(&lp);
		if (encap_list_search(ver_l, &lp, ver,
				      (encap_matchfunc_t)ver_match) == 0)
		{
			encap_pkgspec_join(buf, sizeof(buf), name, ver);
			fprintf(stderr, "    ! package %s not found!\n", buf);
			return -1;
		}
		encap_list_del(ver_l, &lp);
		i = remove_pkg(pkgspec);
	}

	encap_list_free(ver_l, (encap_freefunc_t)free);
	return i;
}


