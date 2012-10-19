/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  batch.c - batch mode for epkg
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
#endif


/* batch mode */
int
batch_mode(void)
{
	char tmp[MAXPATHLEN];
	encap_hash_t *pkg_h;
	char *ver;
	package_versions_t *pvp;
	encap_hashptr_t hp;
	encap_listptr_t lp;

	pkg_h = all_packages();

	if (verbose)
		printf("epkg: installing latest version of all packages...\n");

#ifdef DEBUG
	encap_hashptr_reset(&hp);
	while (encap_hash_next(pkg_h, &hp))
	{
		pvp = (package_versions_t *)encap_hashptr_data(&hp);
		printf("%s:", pvp->pv_pkgname);
		encap_listptr_reset(&lp);
		while (encap_list_next(pvp->pv_ver_l, &lp) != 0)
			printf(" v%s", (char *)encap_listptr_data(&lp));
		printf(" done\n");
	}
#endif

	encap_hashptr_reset(&hp);
	while (encap_hash_next(pkg_h, &hp) != 0)
	{
		pvp = (package_versions_t *)encap_hashptr_data(&hp);

		if (!BIT_ISSET(epkg_opts, EPKG_OPT_VERSIONING))
		{
			encap_listptr_reset(&lp);
			while (encap_list_next(pvp->pv_ver_l, &lp) != 0)
			{
				ver = (char *)encap_listptr_data(&lp);
				encap_pkgspec_join(tmp, sizeof(tmp),
						   pvp->pv_pkgname, ver);
				install_pkg(tmp);
			}
			continue;
		}

		install_package_version(pvp->pv_pkgname, NULL, pvp->pv_ver_l);
	}

	/* clean up */
	encap_hash_free(pkg_h, (encap_freefunc_t)free_package_versions);

	return 0;
}


