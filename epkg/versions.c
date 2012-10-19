/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  versions.c - version-aware code for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <epkg.h>

#include <stdio.h>
#include <dirent.h>
#include <sys/param.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif


/* ver_t matching function */
int
ver_match(char *v1, char *v2)
{
	return !encap_vercmp(v1, v2);
}


/* remove all listed versions of a package */
int
remove_package_versionlist(char *pkgname, encap_list_t *ver_l)
{
	char pkgspec[MAXPATHLEN];
	encap_listptr_t lp;
	int i = 0;

	if (ver_l == NULL)
		return i;

	encap_listptr_reset(&lp);
	while (encap_list_next(ver_l, &lp) != 0)
	{
		encap_pkgspec_join(pkgspec, sizeof(pkgspec), pkgname,
				   (char *)encap_listptr_data(&lp));
		i += remove_pkg(pkgspec);
	}

	return i;
}


/* select one version to be installed, and remove all others */
int
install_package_version(char *pkgname, char *inst_ver, encap_list_t *ver_l)
{
	char buf[MAXPATHLEN], *ver;
	encap_listptr_t lp;

#ifdef DEBUG
	printf("    install_package_version(): inst_ver = \"%s\"\n", inst_ver);
	printf("    install_package_version(): ver_l:");
	encap_listptr_reset(&lp);
	while (encap_list_next(ver_l, &lp) != 0)
	{
		printf(" %s", (char *)encap_listptr_data(&lp));
	}
	putchar('\n');
#endif

	encap_listptr_reset(&lp);
	if (inst_ver != NULL && inst_ver[0] != '\0')
	{
		/* find specified version */
		if (encap_list_search(ver_l, &lp, inst_ver,
				      (encap_matchfunc_t)ver_match) == 0)
		{
			encap_pkgspec_join(buf, sizeof(buf), pkgname, inst_ver);
			fprintf(stderr, "    ! package %s not found!\n", buf);
			return -1;
		}
	}
	else
	{
		/* no version specified - use latest version by default */
		encap_list_prev(ver_l, &lp);

		/* if EPKG_OPT_BACKOFF is set, use second-latest version */
		if (BIT_ISSET(epkg_opts, EPKG_OPT_BACKOFF)
		    && encap_list_prev(ver_l, &lp) == 0)
		{
			fprintf(stderr,
				"    ! previous version of package %s "
				"not found!\n", pkgname);
			return -1;
		}
	}

	/*
	** lp now points to the right version - save it and remove it
	** from the list
	*/
	ver = (char *)encap_listptr_data(&lp);
	encap_list_del(ver_l, &lp);

	/* remove remaining versions and install the selected version */
	remove_package_versionlist(pkgname, ver_l);

	encap_pkgspec_join(buf, sizeof(buf), pkgname, ver);

	return install_pkg(buf);
}


/* plugin for find_pkg_versions() */
int
version_list_add(void *state, char *pkgname, char *ver)
{
	char pkgstr[MAXPATHLEN];
	encap_list_t *ver_l = state;

#ifdef DEBUG
	printf("==> version_list_add(state=0x%lx, pkgname=\"%s\",ver=\"%s\")\n",
	       state, pkgname, ver);
#endif

	if (verbose > 1)
	{
		encap_pkgspec_join(pkgstr, sizeof(pkgstr), pkgname, ver);
		printf("\t%s\n", pkgstr);
	}

	if (encap_list_add(ver_l, strdup(ver)) == -1)
	{
		fprintf(stderr, "    ! encap_list_add(): %s\n",
			strerror(errno));
		return -1;
	}

	return 0;
}


/* inserts a specific pkgspec into the hash/list structure for update mode */
int
insert_pkg_in_hash(char *pkgspec, encap_hash_t *h)
{
	char name[MAXPATHLEN], ver[MAXPATHLEN];
	encap_hashptr_t hp;
	package_versions_t pvs, *pvp;

#ifdef DEBUG
	printf("==> insert_pkg_in_hash(pkgspec=\"%s\", h=0x%lx)\n", pkgspec, h);
#endif

	encap_pkgspec_parse(pkgspec, name, sizeof(name), ver, sizeof(ver),
			    NULL, 0, NULL, 0);
	if (name == NULL || ver == NULL)
	{
		fprintf(stderr, "    ! error parsing pkgspec\n");
		return -1;
	}

	strlcpy(pvs.pv_pkgname, name, sizeof(pvs.pv_pkgname));
	pvs.pv_ver_l = NULL;
	encap_hashptr_reset(&hp);
	if (encap_hash_getkey(h, &hp, &pvs, NULL) != 0)
	{
		pvp = (package_versions_t *)encap_hashptr_data(&hp);
#ifdef DEBUG
		printf("    insert_pkg_in_hash(): found entry for package %s\n",
		       name);
		printf("    insert_pkg_in_hash(): hp.bucket = %d\n",
		       hp.bucket);
#endif
	}
	else
	{
		pvs.pv_ver_l = encap_list_new(LIST_USERFUNC,
					   (encap_cmpfunc_t)encap_vercmp);
		if (pvs.pv_ver_l == NULL)
		{
			fprintf(stderr,
				"    ! malloc() failed in encap_list_new()\n");
			return -1;
		}
		pvp = (package_versions_t *)malloc(sizeof(package_versions_t));
		memcpy(pvp, &pvs, sizeof(package_versions_t));
		if (encap_hash_add(h, pvp) != 0)
		{
			fprintf(stderr,
				"    ! encap_hash_insert() failed!\n");
			return -1;
		}
	}

	/* now we know we have a list for this package, so insert the new ver */
	if (encap_list_add(pvp->pv_ver_l, strdup(ver)) != 0)
	{
		fprintf(stderr, "    ! encap_list_add() failed!\n");
		return -1;
	}

	return 0;
}


static int
read_source_exclude_file(encap_list_t *ex_l)
{
	FILE *f;
	char tmp1[MAXPATHLEN];

	snprintf(tmp1, sizeof(tmp1), "%s/encap.exclude", source);
	f = fopen(tmp1, "r");
	if (f == NULL)
	{
		if (errno != ENOENT)
		{
			fprintf(stderr, "     ! fopen(\"%s\"): %s\n", tmp1,
				strerror(errno));
			return -1;
		}
		return 0;
	}

	/* add the entries from the exclude file */
	while (fscanf(f, "%s", tmp1) == 1)
		encap_list_add(ex_l, (void *)strdup(tmp1));

	fclose(f);		/* dont bother checking for error... */
	return 1;
}


/* return a hash of all packages in the source directory */
encap_hash_t *
all_packages(void)
{
	DIR *d;
	struct dirent *di;
	char tmp[MAXPATHLEN];
	struct stat s;
	encap_hash_t *pkg_h;
	encap_listptr_t lp;
	encap_list_t *myex_l = NULL;

	if (verbose)
		printf("  > scanning all packages in source directory...\n");

	pkg_h = encap_hash_new(128, NULL);
	if (pkg_h == NULL)
	{
		fprintf(stderr, "   ! malloc() failed in encap_hash_new()\n");
		return NULL;
	}

	if ((d = opendir(source)) == NULL)
	{
		fprintf(stderr, "   ! opendir(\"%s\"): %s\n", source,
			strerror(errno));
		return NULL;
	}

	/* read source-level encap.exclude file if applicable */
	if (BIT_ISSET(epkg_opts, EPKG_OPT_OLDEXCLUDES))
	{
		if (verbose)
			printf("    > reading %s/encap.exclude\n", source);
		myex_l = encap_list_new(LIST_USERFUNC, NULL);
		read_source_exclude_file(myex_l);
	}

	while ((di = readdir(d)) != NULL)
	{
		if (strcmp(di->d_name, ".") == 0 ||
		    strcmp(di->d_name, "..") == 0)
			continue;

		snprintf(tmp, sizeof(tmp), "%s/%s", source, di->d_name);
		if (stat(tmp, &s) != 0)
		{
			fprintf(stderr, "    ! stat(\"%s\"): %s\n", tmp,
				strerror(errno));
			return NULL;
		}
		if (!S_ISDIR(s.st_mode))
			continue;

		/* if the filename matches an exclude entry, ignore it */
		encap_listptr_reset(&lp);
		if (BIT_ISSET(epkg_opts, EPKG_OPT_OLDEXCLUDES) &&
		    myex_l != NULL &&
		    encap_list_search(myex_l, &lp, di->d_name,
				      (encap_matchfunc_t)encap_str_match) != 0)
		{
			if (verbose > 1)
				printf("    * excluding package %s\n",
				       di->d_name);
			continue;
		}

		if (verbose > 1)
			printf("\t%s\n", di->d_name);

		if (insert_pkg_in_hash(di->d_name, pkg_h) != 0)
			return NULL;
	}

	closedir(d);
	if (BIT_ISSET(epkg_opts, EPKG_OPT_OLDEXCLUDES))
		encap_list_free(myex_l, free);

	return pkg_h;
}


/* free memory associated with a list of package_versions structures */
void
free_package_versions(package_versions_t *pkgvers)
{
	encap_list_free(pkgvers->pv_ver_l, (encap_freefunc_t)free);
	free(pkgvers);
}


