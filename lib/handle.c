/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  handle.c - libencap code for initializing an ENCAP handle
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif


const char libencap_version[] = PACKAGE_VERSION;


/* open an Encap package */
int
encap_open(ENCAP **encap, char *source, char *target,
	   char *pkgname, unsigned long options, encap_print_func_t print_func)
{
	char pkgdir[MAXPATHLEN];
	struct stat s;

#ifdef DEBUG
	printf("==> encap_open(encap=0x%lx, source=\"%s\", target=\"%s\", "
	       "pkgname=\"%s\", options=%04o, print_func=0x%lx)\n",
	       encap, source, target, pkgname, options, print_func);
#endif

	snprintf(pkgdir, sizeof(pkgdir), "%s/%s", source, pkgname);
	if (stat(pkgdir, &s) == -1)
		return -1;
	if (!S_ISDIR(s.st_mode))
	{
		errno = ENOTDIR;
		return -1;
	}

	*encap = calloc(1, sizeof(ENCAP));
	if (*encap == NULL)
		return -1;

	strlcpy((*encap)->e_pkgname, pkgname, sizeof((*encap)->e_pkgname));
	strlcpy((*encap)->e_source, source, sizeof((*encap)->e_source));
	strlcpy((*encap)->e_target, target, sizeof((*encap)->e_target));
	(*encap)->e_options = options;
	(*encap)->e_print_func = print_func;

#ifdef DEBUG
	printf("<== encap_open(): calling encap_get_info() and returning\n");
#endif
	if (encap_get_info(*encap) < 0)
	{
		errno = EINVAL;
		return -1;
	}

	return 0;
}


/* close an Encap package */
void
encap_close(ENCAP *encap)
{
	encapinfo_free(&(encap->e_pkginfo));
	free(encap);
}


