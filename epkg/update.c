/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  update.c - update mode functions for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <epkg.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <netdb.h>
#include <dirent.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif


/* update a package to the latest level available in update_dir */
int
update_pkg(char *pkgname, encap_list_t *installed_l,
	   encap_list_t *update_l, char *requested_ver)
{
	update_file_t *update_latest = NULL;
	char *installed_latest = NULL;
	char pkgstr[MAXPATHLEN];
	encap_listptr_t lp;

#ifdef DEBUG
	printf("==> update_pkg(pkgname=\"%s\", installed_l=0x%lx, "
	       "update_l=0x%lx, requested_ver=\"%s\")\n",
	       pkgname, installed_l, update_l, requested_ver);
#endif

	encap_listptr_reset(&lp);
	if (requested_ver != NULL && requested_ver[0] != '\0')
	{
		/* find specified version */
		if (encap_list_search(update_l, &lp, requested_ver,
				      (encap_matchfunc_t)uf_match) == 0)
		{
			encap_pkgspec_join(pkgstr, sizeof(pkgstr), pkgname,
					   requested_ver);
			fprintf(stderr, "    * update %s not found\n",
				pkgstr);
			return 1;
		}
		update_latest = (update_file_t *)encap_listptr_data(&lp);
		encap_list_del(update_l, &lp);
	}
	else
	{
		/* compare most recent versions */
		encap_list_prev(update_l, &lp);
		update_latest = (update_file_t *)encap_listptr_data(&lp);
		encap_list_del(update_l, &lp);

		encap_listptr_reset(&lp);
		if (installed_l != NULL
		    && encap_list_prev(installed_l, &lp) != 0)
		{
			installed_latest = (char *)encap_listptr_data(&lp);
#ifdef DEBUG
			printf("installed_latest = \"%s\", "
			       "update_latest->uf_ver = \"%s\"\n",
			       (installed_latest != NULL
				? installed_latest
				: "N/A"),
			       (update_latest->uf_ver != NULL
				? update_latest->uf_ver
				: "N/A"));
#endif
			if (encap_vercmp(installed_latest,
					 update_latest->uf_ver) >= 0)
			{
				if (verbose > 1)
					fprintf(stderr,
						"    * no new updates found "
						"for package %s\n",
						pkgname);
				return 0;
			}
		}

		if (verbose > 1)
		{
			printf("    * current version:\t\t");
			if (installed_latest != NULL)
			{
				encap_pkgspec_join(pkgstr, sizeof(pkgstr),
						   pkgname, installed_latest);
				printf("%s\n", pkgstr);
			}
			else
				printf("N/A\n");
		}
	}

	encap_pkgspec_join(pkgstr, sizeof(pkgstr), pkgname,
			   update_latest->uf_ver);
	if (verbose)
		printf("    * selected update:\t\t%s\n",
		       update_latest->uf_url);

	if (BIT_ISSET(options, OPT_SHOWONLY))
		return 1;

	if (archive_extract(update_latest->uf_url, source,
			    (BIT_ISSET(options, OPT_FORCE)
			     ? ARCHIVE_OPT_FORCE
			     : 0)) == -1)
		return -1;

	if (installed_l != NULL)
		remove_package_versionlist(pkgname, installed_l);

	return install_pkg(pkgstr);
}


static encap_list_t *
find_update_aux(char *pkgname)
{
	encap_listptr_t lp;
	encap_list_t *update_l;
	update_file_t *ufp;
	char pkgstr[MAXPATHLEN];

	update_l = find_update_versions(pkgname);

	if (update_l != NULL && verbose > 2)
	{
		encap_listptr_reset(&lp);
		while (encap_list_next(update_l, &lp))
		{
			ufp = (update_file_t *)encap_listptr_data(&lp);
			encap_pkgspec_join(pkgstr, sizeof(pkgstr), pkgname,
					   ufp->uf_ver);
			printf("\t%s\n", pkgstr);
		}
	}

	return update_l;
}


/* update all locally installed packages */
static int
update_all(void)
{
	encap_hash_t *pkg_h;
	encap_hashptr_t hp;
	package_versions_t *pvp;
	int i, retval = 0;
	encap_list_t *update_l;

	if (verbose)
		printf("epkg: updating all local packages...\n");

	pkg_h = all_packages();

	encap_hashptr_reset(&hp);
	while (encap_hash_next(pkg_h, &hp) != 0)
	{
		pvp = (package_versions_t *)encap_hashptr_data(&hp);
		if (verbose > 1)
			printf("  > checking for updates of package %s...\n",
			       pvp->pv_pkgname);
		update_l = find_update_aux(pvp->pv_pkgname);
		if (update_l == NULL)
		{
			if (verbose > 1)
				fprintf(stderr,
					"    * no updates found for package "
					"%s\n", pvp->pv_pkgname);
			continue;
		}
		i = update_pkg(pvp->pv_pkgname, pvp->pv_ver_l, update_l, NULL);
		encap_list_free(update_l, (encap_freefunc_t)uf_free);
		if (i == -1)
			break;
		if (i)
			retval++;
	}

	encap_hash_free(pkg_h, (encap_freefunc_t)free_package_versions);
	return retval;
}


/* update mode */
int
update_mode(char *pkgspec)
{
	encap_list_t *installed_l, *update_l;
	encap_listptr_t lp;
	char name[MAXPATHLEN], ver[MAXPATHLEN] = "";
	int i;

#ifdef DEBUG
	printf("==> update_mode(\"%s\")\n", pkgspec);
#endif

	if (pkgspec == NULL)
		return update_all();

	if (verbose)
		printf("epkg: updating package %s...\n", pkgspec);

	if (verbose > 1)
		printf("  > checking for updates of package %s...\n", pkgspec);

	encap_pkgspec_parse(pkgspec, name, sizeof(name),
			    ver, sizeof(ver), NULL, 0, NULL, 0);

	/*
	** find updates:
	**  - first, assume pkgspec is just the package name
	**  - if that fails and encap_pkgspec_parse() returned a
	**    different name, try that
	*/
	update_l = find_update_aux(pkgspec);
	if (update_l == NULL && strcmp(pkgspec, name) != 0)
		update_l = find_update_aux(name);

	if (update_l == NULL)
	{
		fprintf(stderr,
			"    * no updates found for package %s\n",
			pkgspec);
		return 1;
	}

	/* find versions currently installed */
	if (verbose > 1)
		printf("  > reading Encap source directory...\n");
	installed_l = encap_list_new(LIST_USERFUNC,
				     (encap_cmpfunc_t)encap_vercmp);
	if (installed_l == NULL)
	{
		fprintf(stderr, "  ! encap_list_new(): %s\n",
			strerror(errno));
		return -1;
	}

	i = encap_find_versions(source, name, version_list_add,
				installed_l);
	if (i == -2)
		return -1;
	if (i == -1)
	{
		fprintf(stderr, "  ! encap_find_versions(): %s\n",
			strerror(errno));
		return -1;
	}
	if (i == 0)
	{
		encap_list_free(installed_l, free);
		installed_l = NULL;
	}

	encap_listptr_reset(&lp);
	if (ver[0] != '\0'
	    && installed_l != NULL
	    && encap_list_search(installed_l, &lp, ver,
	    			 (encap_matchfunc_t)ver_match) != 0)
	{
		if (verbose > 1)
			fprintf(stderr,
				"  ! requested package %s is already "
				"installed\n", pkgspec);
		return 0;
	}

	i = update_pkg(name, installed_l, update_l, ver);

	encap_list_free(update_l, (encap_freefunc_t)uf_free);
	if (installed_l != NULL)
		encap_list_free(installed_l, free);

	return i;
}


