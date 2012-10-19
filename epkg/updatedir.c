/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  updatedir.c - update directory access functions for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <epkg.h>

#include <stdio.h>
#include <sys/param.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif


/*************************************************************************
 ***  type functions for update path list
 *************************************************************************/

static encap_list_t *update_path_l = NULL;

/*
** FIXME: ud_pkgs_l should be a hash, but I'm not sure what
**        hash function to use...
*/
struct update_dir
{
	char *ud_url;			/* URL of update directory */
	encap_list_t *ud_pkgs_l;	/* package list */
};
typedef struct update_dir update_dir_t;


/*
** FIXME: setting update_path_l->flags is blatant cheating!
**        (the list API needs to be fixed to provide
**        a clean way to prepend to the list)
*/
int
update_path_add(char *url, int prepend)
{
	update_dir_t *udp;

	if (prepend)
		update_path_l->flags = LIST_STACK;

	udp = (update_dir_t *)calloc(1, sizeof(update_dir_t));
	if (udp == NULL)
		return -1;

	udp->ud_url = strdup(url);
	if (udp->ud_url == NULL)
		return -1;

	if (encap_list_add(update_path_l, udp) != 0)
		return -1;

	if (prepend)
		update_path_l->flags = LIST_QUEUE;

	if (update_path_l->first == NULL)
		printf("N/A");

	return 0;
}


int
update_path_isset(void)
{
	return ((encap_list_nents(update_path_l) > 0) ? 1 : 0);
}


void
update_path_print(void)
{
	encap_listptr_t lp;
	update_dir_t *udp;

	encap_listptr_reset(&lp);

	while (encap_list_next(update_path_l, &lp) != 0)
	{
		udp = (update_dir_t *)encap_listptr_data(&lp);

		printf("%s%s", (update_path_l->first == lp ? "" : ":"),
		       udp->ud_url);
	}
}


int
update_path_init(void)
{
	char buf[10240];
	char *cp, *thisp, *nextp;

	update_path_l = encap_list_new(LIST_QUEUE, NULL);
	if (update_path_l == NULL)
		return -1;

	cp = getenv("ENCAP_UPDATE_PATH");
	if (cp != NULL)
	{
		strlcpy(buf, cp, sizeof(buf));

		nextp = buf;
		while ((thisp = strsep(&nextp, " \t")) != NULL)
		{
			if (*thisp == '\0')
				continue;

			if (update_path_add(thisp, 0) != 0)
				return -1;
		}

		return 0;
	}

	cp = getenv("ENCAP_UPDATEDIR");
	if (cp != NULL
	    && update_path_add(cp, 0) != 0)
		return -1;

	return 0;
}


static void
ud_free(update_dir_t *udp)
{
	if (udp->ud_url != NULL)
		free(udp->ud_url);
	if (udp->ud_pkgs_l != NULL)
		encap_list_free(udp->ud_pkgs_l, free);
	free(udp);
}


void
update_path_free(void)
{
	encap_list_free(update_path_l, (encap_freefunc_t)ud_free);
}


/* plugin function for download code */
static int
add_update(char *url, encap_list_t *pkg_l)
{
#ifdef DEBUG
	printf("==> add_update(url=\"%s\", pkg_l=0x%lx)\n", url, pkg_l);
#endif

	encap_list_add(pkg_l, strdup(url));
	return 0;
}


/*************************************************************************
 ***  type functions for update hash
 *************************************************************************/

int
uf_match(char *ver, update_file_t *uf)
{
	return ver_match(uf->uf_ver, ver);
}


int
uf_cmp(update_file_t *uf1, update_file_t *uf2)
{
	return encap_vercmp(uf1->uf_ver, uf2->uf_ver);
}


/* free memory associated with an update_file structure */
void
uf_free(update_file_t *uf)
{
	free(uf->uf_url);
	free(uf->uf_ver);
	free(uf);
}


/*************************************************************************
 ***  code to check each update file for a match
 *************************************************************************/

struct update_state
{
	char *us_pkg;
	encap_list_t *us_uv_l;
};
typedef struct update_state update_state_t;

static int
update_check(char *url, update_state_t *usp)
{
	char pkgname[MAXPATHLEN], pkgver[MAXPATHLEN], pkgplatform[MAXPATHLEN];
	update_file_t *uf;
	encap_listptr_t lp;
	char *file;

#ifdef DEBUG
	printf("==> update_check(url=\"%s\", usp={\"%s\", 0x%lx})\n",
	       url, usp->us_pkg, usp->us_uv_l);
#endif

	file = basename(url);
	encap_pkgspec_parse(file, pkgname, sizeof(pkgname),
			    pkgver, sizeof(pkgver),
			    pkgplatform, sizeof(pkgplatform),
			    NULL, 0);

#ifdef DEBUG
	printf("=== update_check(): file=\"%s\", pkgname=\"%s\", "
	       "pkgver=\"%s\"\n", file, pkgname, pkgver);
#endif

	/* if platform is specified, make sure platform matches */
	if (pkgplatform[0] != '\0')
	{
		if (!encap_platform_compat(pkgplatform, platform,
					   platform_suffix_l))
			return 0;
	}

	encap_listptr_reset(&lp);
	if (strcmp(pkgname, usp->us_pkg) == 0
	    && encap_list_search(usp->us_uv_l, &lp, pkgver,
	    			 (encap_matchfunc_t)uf_match) == 0)
	{
		uf = (update_file_t *)malloc(sizeof(update_file_t));
		if (uf == NULL)
			return -1;

		uf->uf_url = strdup(url);
		uf->uf_ver = strdup(pkgver);

		encap_list_add(usp->us_uv_l, uf);
	}

	return 0;
}


/*************************************************************************
 ***  public access function
 *************************************************************************/

/*
** find_update_versions() - return a list of update_file structs which
**                          point to the versions of pkgname which are
**                          found in the update directories
*/
encap_list_t *
find_update_versions(char *pkgname)
{
	encap_listptr_t lp;
	char *cp;
	char update_dir[MAXPATHLEN];
	update_dir_t *udp;
	update_state_t us;

#ifdef DEBUG
	printf("==> find_update_versions(pkgname=\"%s\")\n", pkgname);
#endif

	us.us_pkg = pkgname;
	us.us_uv_l = encap_list_new(LIST_USERFUNC, (encap_cmpfunc_t)uf_cmp);
	if (us.us_uv_l == NULL)
	{
		fprintf(stderr, "  ! encap_list_new() failed\n");
		return NULL;
	}

	encap_listptr_reset(&lp);
	while (encap_list_next(update_path_l, &lp) != 0)
	{
		udp = (update_dir_t *)encap_listptr_data(&lp);

		/* replace "%p" token with platform name */
		encap_gsub(udp->ud_url, "%p", platform,
			   update_dir, sizeof(update_dir));

#if 0
		/* strip trailing '/' for cosmetic purposes */
		sz = strlen(update_dir) - 1;
		if (update_dir[sz] == '/')
			update_dir[sz] = '\0';
#endif

		if (udp->ud_pkgs_l == NULL)
		{
			udp->ud_pkgs_l = encap_list_new(LIST_QUEUE, NULL);
			if (udp->ud_pkgs_l == NULL)
				return NULL;

			if (download_dir(update_dir,
					 (dir_entry_func_t)add_update,
					 udp->ud_pkgs_l) == -1)
			{
				encap_list_del(update_path_l, &lp);
				encap_list_prev(update_path_l, &lp);
			}
		}

		if (encap_list_iterate(udp->ud_pkgs_l,
				       (encap_iterate_func_t)update_check,
				       &us) == -1)
			return NULL;

		/*
		** if EPKG_OPT_UPDATEALLDIRS isn't set and we found a
		** matching package in this directory, stop here
		*/
		if (!BIT_ISSET(epkg_opts, EPKG_OPT_UPDATEALLDIRS)
		    && encap_list_nents(us.us_uv_l) > 0)
			break;
	}

	if (encap_list_nents(us.us_uv_l) == 0)
	{
		encap_list_free(us.us_uv_l, NULL);
		us.us_uv_l = NULL;
	}

	return us.us_uv_l;
}


