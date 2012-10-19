/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  prereq.c - prerequisite checking code
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif


static int parse_pkgspec(char *line, ENCAP_PREREQ *ep);
static int check_pkgspec(ENCAP *encap, ENCAP_PREREQ *ep);
static size_t print_pkgspec(ENCAP_PREREQ *ep, char *buf, size_t bufsize);

static int parse_path(char *line, ENCAP_PREREQ *ep);
static int check_path(ENCAP *encap, ENCAP_PREREQ *ep);
static size_t print_path(ENCAP_PREREQ *ep, char *buf, size_t bufsize);


typedef int (*ep_parsefunc_t)(char *, ENCAP_PREREQ *);
typedef int (*ep_checkfunc_t)(ENCAP *, ENCAP_PREREQ *);
typedef size_t (*ep_printfunc_t)(ENCAP_PREREQ *, char *, size_t);


struct prereq_type
{
	unsigned short pt_type;		/* prerequisite type */
	char *pt_name;			/* name in encapinfo file */
	ep_parsefunc_t pt_parsefunc;	/* parsing function */
	ep_checkfunc_t pt_checkfunc;	/* checking function */
	ep_printfunc_t pt_printfunc;	/* printing function */
};
typedef struct prereq_type PREREQ_TYPE;

static PREREQ_TYPE prereq_types[] = {
	{ ENCAP_PREREQ_PKGSPEC,		"pkgspec",
		parse_pkgspec,	check_pkgspec,	print_pkgspec },
	{ ENCAP_PREREQ_DIRECTORY,	"directory",
		parse_path,	check_path,	print_path },
	{ ENCAP_PREREQ_REGFILE,		"regfile",
		parse_path,	check_path,	print_path },
	{ 0,				NULL,
		NULL,		NULL,		NULL }
};


size_t
encap_prereq_print(ENCAP_PREREQ *ep, char *buf, size_t bufsize)
{
	int i;

#ifdef DEBUG
	printf("==> encap_prereq_print(ep=0x%lx, buf=0x%lx, bufsize=%d)\n",
	       ep, buf, bufsize);
#endif

	for (i = 0; prereq_types[i].pt_type != 0; i++)
		if (prereq_types[i].pt_type ==
		    (ep->ep_type & ENCAP_PREREQ_TYPEMASK))
			break;
	if (prereq_types[i].pt_type == 0)
	{
		errno = EINVAL;
		return -1;
	}

	if (strlcpy(buf, prereq_types[i].pt_name, bufsize) > bufsize
	    || strlcat(buf, " ", bufsize) > bufsize)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	return (*prereq_types[i].pt_printfunc)(ep, buf, bufsize);
}


/*
** encap_prereq_parse() - calls the appropriate parsing function for the
** given prereq type.
** returns:
**	0	success
**	-1	failure (and sets errno)
*/
int
encap_prereq_parse(encapinfo_t *pkginfo, char *line)
{
	ENCAP_PREREQ *ep;
	char *tok, *nextp;
	int i;

#ifdef DEBUG
	printf("==> encap_prereq_parse(0x%lx, \"%s\")\n", pkginfo, line);
#endif

	nextp = line;
	do
		tok = strsep(&nextp, " \t");
	while (tok != NULL && *tok == '\0');
	if (tok == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	nextp += strspn(nextp, " \t");

#ifdef DEBUG
	printf("    encap_prereq_parse(): type=\"%s\"\n", tok);
#endif

	for (i = 0; prereq_types[i].pt_type != 0; i++)
		if (strcmp(tok, prereq_types[i].pt_name) == 0)
			break;
	if (prereq_types[i].pt_type == 0)
	{
		errno = EINVAL;
		return -1;
	}

	ep = (ENCAP_PREREQ *)calloc(1, sizeof(ENCAP_PREREQ));
	if (ep == NULL)
		return -1;
	ep->ep_type = prereq_types[i].pt_type;

#ifdef DEBUG
	printf("    encap_prereq_parse(): calling parsefunc() plugin...\n");
#endif

	if ((*prereq_types[i].pt_parsefunc)(nextp, ep) == -1)
		return -1;

#ifdef DEBUG
	printf("    encap_prereq_parse(): ep->ep_type = %d\n", ep->ep_type);
	if ((ep->ep_type & ENCAP_PREREQ_TYPEMASK) == ENCAP_PREREQ_PKGSPEC)
	{
		char buf[MAXPATHLEN];

		pkgspectostr(ep->ep_un.ep_pkgspec, buf, sizeof(buf));
		printf("    encap_prereq_parse(): ep->ep_un.ep_pkgspec = "
		       "\"%s\"\n", buf);
		printf("    encap_prereq_parse(): range = %d\n",
		       ep->ep_type & ENCAP_PREREQ_RANGEMASK);
	}
	else
		printf("    encap_prereq_parse(): ep->ep_un.ep_pathname = "
		       "\"%s\"\n", ep->ep_un.ep_pathname);
	printf("    encap_prereq_parse(): adding to prereq list\n");
#endif

	encap_list_add(pkginfo->ei_pr_l, ep);

#ifdef DEBUG
	printf("<== encap_prereq_parse(): success\n");
#endif
	return 0;
}


/*
** parse and check functions for pathnames
*/


static int
parse_path(char *line, ENCAP_PREREQ *ep)
{
	ep->ep_un.ep_pathname = strdup(line);
	return 0;
}


static int
check_path(ENCAP *encap, ENCAP_PREREQ *ep)
{
	struct stat s;

	if (stat(ep->ep_un.ep_pathname, &s) != 0)
	{
		if (errno != ENOENT)
		{
			(*encap->e_print_func)(encap, NULL, NULL,
					EPT_PKG_ERROR,
					"prerequisite error: stat(\"%s\")",
					ep->ep_un.ep_pathname);
			return -1;
		}
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_FAIL,
				       "prerequisite not met: \"regfile %s\"",
				       ep->ep_un.ep_pathname);
		return 1;
	}

	if ((ep->ep_type & ENCAP_PREREQ_TYPEMASK) == ENCAP_PREREQ_DIRECTORY)
	{
		if (S_ISDIR(s.st_mode))
		{
			(*encap->e_print_func)(encap, NULL, NULL,
					EPT_PKG_INFO,
					"prerequisite met: \"directory %s\"",
					ep->ep_un.ep_pathname);
			return 0;
		}
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_FAIL,
				"prerequisite not met: \"directory %s\"",
				ep->ep_un.ep_pathname);
		return 1;
	}

	/* ENCAP_PREREQ_REGFILE */
	if (S_ISDIR(s.st_mode))
	{
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_FAIL,
				"prerequisite not met: \"regfile %s\"",
				ep->ep_un.ep_pathname);
		return 1;
	}
	(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_INFO,
			       "prerequisite met: \"regfile %s\"",
			       ep->ep_un.ep_pathname);
	return 0;
}


static size_t
print_path(ENCAP_PREREQ *ep, char *buf, size_t bufsize)
{
	size_t len;

	len = strlcat(buf, ep->ep_un.ep_pathname, bufsize);
	if (len > bufsize)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	return len;
}


/*
** parse and check functions for pkgspecs
*/


static int
parse_pkgspec(char *line, ENCAP_PREREQ *ep)
{
	char *tok;

#ifdef DEBUG
	printf("==> parse_pkgspec(\"%s\", 0x%lx)\n", line, ep);
#endif

	do
		tok = strsep(&line, " \t");
	while (tok != NULL && *tok == '\0');
	if (tok == NULL || *line == '\0')
	{
		errno = EINVAL;
		return -1;
	}
	line += strspn(line, " \t");

	switch (tok[0])
	{
	case '=':
		BIT_SET(ep->ep_type, ENCAP_PREREQ_EXACT);
		break;
	case '*':
		BIT_SET(ep->ep_type, ENCAP_PREREQ_ANY);
		break;
	case '>':
		BIT_SET(ep->ep_type, ENCAP_PREREQ_NEWER);
		if (tok[1] == '=')
			BIT_SET(ep->ep_type, ENCAP_PREREQ_EXACT);
		break;
	case '<':
		BIT_SET(ep->ep_type, ENCAP_PREREQ_OLDER);
		if (tok[1] == '=')
			BIT_SET(ep->ep_type, ENCAP_PREREQ_EXACT);
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	ep->ep_un.ep_pkgspec = strdup(line);

	return 0;
}


struct ver_range
{
	char *vr_ver;
	unsigned short vr_range;
};


static int
prereq_pkg_plugin(void *state, char *pkgname, char *ver)
{
	int i;
	struct ver_range *vr = state;

#ifdef DEBUG
	printf("==> prereq_pkg_plugin(vr={ver=\"%s\", range=%d}, "
	       "pkgname=\"%s\", ver=\"%s\")\n",
	       vr->vr_ver, vr->vr_range, pkgname, ver);
#endif

	if (BIT_ISSET(vr->vr_range, ENCAP_PREREQ_ANY))
		return -1;

	i = encap_vercmp(ver, vr->vr_ver);
	if ((BIT_ISSET(vr->vr_range, ENCAP_PREREQ_OLDER) && i < 0) ||
	    (BIT_ISSET(vr->vr_range, ENCAP_PREREQ_EXACT) && i == 0) ||
	    (BIT_ISSET(vr->vr_range, ENCAP_PREREQ_NEWER) && i > 0))
	{

		/*****************************************
		*** FIXME: Check to see if package is
		***        actually installed, not just
		***        sitting in the source dir.
		*****************************************/

#ifdef DEBUG
		printf("<== prereq_pkg_plugin(): match found!\n");
#endif
		return -1;
	}

	return 0;
}


static char *
rangetostr(unsigned short range)
{
	switch (range)
	{
	case ENCAP_PREREQ_OLDER:
		return "<";
	case ENCAP_PREREQ_EXACT:
		return "=";
	case ENCAP_PREREQ_NEWER:
		return ">";
	case ENCAP_PREREQ_ANY:
		return "*";
	case ENCAP_PREREQ_OLDER | ENCAP_PREREQ_EXACT:
		return "<=";
	case ENCAP_PREREQ_NEWER | ENCAP_PREREQ_EXACT:
		return ">=";
	default:
		return "";
	}
}


static int
check_pkgspec(ENCAP *encap, ENCAP_PREREQ *ep)
{
	char name[MAXPATHLEN], ver[MAXPATHLEN] = "";
	struct ver_range vr;

	vr.vr_ver = ver;
	vr.vr_range = (ep->ep_type & ENCAP_PREREQ_RANGEMASK);

	if (vr.vr_range == ENCAP_PREREQ_ANY)
		strlcpy(name, ep->ep_un.ep_pkgspec, sizeof(name));
	else
		encap_pkgspec_parse(ep->ep_un.ep_pkgspec, name, sizeof(name),
				    ver, sizeof(ver), NULL, 0, NULL, 0);

	switch (encap_find_versions(encap->e_source, name,
				    prereq_pkg_plugin, &vr))
	{
	case -2:
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_INFO,
				"verified prerequisite: \"pkgspec %s %s\"",
				rangetostr(ep->ep_type &
					   ENCAP_PREREQ_RANGEMASK),
				ep->ep_un.ep_pkgspec);
		return 0;
	case -1:
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_ERROR,
				"error while verifying prerequisite: "
				"\"pkgspec %s %s\"",
				rangetostr(ep->ep_type &
					   ENCAP_PREREQ_RANGEMASK),
				ep->ep_un.ep_pkgspec);
		return -1;
	default:
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_FAIL,
				"prerequisite not met: \"pkgspec %s %s\"",
				rangetostr(ep->ep_type &
					   ENCAP_PREREQ_RANGEMASK),
				ep->ep_un.ep_pkgspec);
		return 1;
	}
}


static size_t
print_pkgspec(ENCAP_PREREQ *ep, char *buf, size_t bufsize)
{
#ifdef DEBUG
	printf("==> print_pkgspec()\n");
#endif

	if (strlcat(buf, rangetostr(ep->ep_type & ENCAP_PREREQ_RANGEMASK),
		    bufsize) > bufsize ||
	    strlcat(buf, " ", bufsize) > bufsize ||
	    strlcat(buf, ep->ep_un.ep_pkgspec, bufsize) > bufsize)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	return strlen(buf);
}


/*
 * check prerequisites from given list.
 * returns 0 on success, -1 on failure, 1 if prereqs not satisfied.
 */
int
encap_check_prereqs(ENCAP *encap)
{
	encap_listptr_t lp;
	ENCAP_PREREQ *ep;
	int i;

	encap_listptr_reset(&lp);
	while (encap_list_next(encap->e_pkginfo.ei_pr_l, &lp) != 0)
	{
		ep = (ENCAP_PREREQ *)encap_listptr_data(&lp);

#ifdef DEBUG
		printf("    encap_check_prereqs(): ep->ep_type == %d\n",
		       ep->ep_type);
#endif

		for (i = 0; prereq_types[i].pt_type != 0; i++)
			if (prereq_types[i].pt_type ==
			    (ep->ep_type & ENCAP_PREREQ_TYPEMASK))
				break;
		if (prereq_types[i].pt_type == 0)
		{
			(*encap->e_print_func)(encap, NULL, NULL,
					       EPT_PKG_FAIL,
					       "internal error: unknown "
					       "prerequisite type");
			errno = EINVAL;
			return -1;
		}

		switch ((*prereq_types[i].pt_checkfunc)(encap, ep))
		{
		case 1:
			return 1;
		case -1:
			return -1;
		case 0:
			break;
		}
	}

	return 0;
}


