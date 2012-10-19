/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  encapinfo.c - encapinfo file parsing code
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <sys/param.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif


/*
** read encapinfo file and store data in pkginfo.
** returns:
**	0				success
**	-1				error
*/
static int
encapinfo_read(char *filename, ENCAP *encap)
{
	FILE *f;
	char line[1024], buf[1024];
	char *cp;

#ifdef DEBUG
	printf("==> encapinfo_read(filename=\"%s\", encap=0x%lx)\n",
	       filename, encap);
#endif

	f = fopen(filename, "r");
	if (f == NULL)
	{
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_ERROR,
				       "cannot open encapinfo file");
		return -1;
	}

	/*
	** the first line identifies what version of the package format
	** it is.
	*/
	if (fgets(line, sizeof(line), f) == NULL)
	{
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_ERROR,
				       "cannot read encapinfo file");
		fclose(f);
		return -1;
	}

	/* strip newlines */
	if (strlen(line) > 0 && line[strlen(line) - 1] == '\n')
		line[strlen(line) - 1] = '\0';

	if (sscanf(line, "encap %s", buf) != 1)
	{
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_FAIL,
				       "parse error in encapinfo file: \"%s\"",
				       line);
		fclose(f);
		return -1;
	}

	/* we only grok package formats 2.1 and earlier */
	encap->e_pkginfo.ei_pkgfmt = strdup(buf);
	if (encap_vercmp(encap->e_pkginfo.ei_pkgfmt, "2.1") > 0)
	{
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_FAIL,
				       "unsupported Encap package format "
				       "version \"%s\" - you may need to "
				       "upgrade epkg",
				       encap->e_pkginfo.ei_pkgfmt);
		fclose(f);
		return -1;
	}

	/* read each line */
	while (fgets(line, sizeof(line), f) != NULL)
	{
		/* strip newlines */
		if (strlen(line) > 0 && line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';

#ifdef DEBUG
		printf("read: \"%s\"\n", line);
#endif

		/* if it's a comment, skip it */
		cp = line;
		while ((cp = strchr(line, '#')) != NULL)
		{
			if (cp == line || cp[-1] != '\\')
			{
				*cp = '\0';
				break;
			}
			cp++;
		}
#ifdef DEBUG
		printf("after stripping comments: \"%s\"\n", line);
#endif

		/* skip blank lines */
		if (strspn(line, " \t") == strlen(line))
			continue;

		/* parse directive and handle errors */
		if (encapinfo_parse_directive(line, &(encap->e_pkginfo),
					      buf, sizeof(buf)) == -1)
		{
			(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_FAIL,
					       "%s: %s", buf, line);
			fclose(f);
			return -1;
		}
	}

	if (! feof(f))
	{
#ifdef DEBUG
		printf("<== encapinfo_read(): fgets() failed!\n");
#endif
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_ERROR,
				       "error reading encapinfo file");
		fclose(f);
		return -1;
	}

#ifdef DEBUG
	printf("<== encapinfo_read(): success\n");
#endif

	fclose(f);
	return 0;
}



/*
** encapinfo_parse_directive() - parse encapinfo directive into pkginfo
** returns:
**	0			success
**	-1 (and sets errbuf)	failure
*/
int
encapinfo_parse_directive(char *text, encapinfo_t *pkginfo,
			  char *errbuf, size_t errsz)
{
	char buf[1024];
	char *tok, *nextp;
	linkname_t lns, *lnp;

	/* make a copy so we don't modify the original line */
	strlcpy(buf, text, sizeof(buf));

	/* parse the line */
	nextp = buf;
	do
		tok = strsep(&nextp, " \t");
	while (tok != NULL && *tok == '\0');

	/* skip leading whitespace */
	if (nextp != NULL)
		nextp += strspn(nextp, " \t");

	/* fail if no arguments */
	if (nextp == NULL || *nextp == '\0')
	{
		errno = EINVAL;
		return -1;
	}

	/* platform name */
	if (strcmp(tok, "platform") == 0)
	{
		if (pkginfo->ei_platform)
		{
			snprintf(errbuf, errsz,
				 "unique field \"%s\" specified twice", tok);
			errno = EINVAL;
			return -1;
		}
		pkginfo->ei_platform = strdup(nextp);
		return 0;
	}

	/* one-line package description */
	if (strcmp(tok, "description") == 0)
	{
		if (pkginfo->ei_description)
		{
			snprintf(errbuf, errsz,
				 "unique field \"%s\" specified twice", tok);
			errno = EINVAL;
			return -1;
		}
		pkginfo->ei_description = strdup(nextp);
		return 0;
	}

	/* creation date */
	if (strcmp(tok, "date") == 0)
	{
#ifdef ENCAPINFO_DATE_TIME_T
		/*
		** Disabled 1/12/01 - strptime() doesn't
		** grok "%Z" on all platforms
		*/

		struct tm stm;

		if (pkginfo->ei_date)
		{
			snprintf(errbuf, errsz,
				 "unique field \"%s\" specified twice", tok);
			return -1;
		}

		if (strptime(nextp, "%a %b %d %H:%M:%S %Z %Y",
			     &stm) == NULL)
		{
			snprintf(errbuf, errsz, "invalid date");
			return -1;
		}

		pkginfo->ei_date = mktime(&stm);
#else /* ! ENCAPINFO_DATE_TIME_T */
		if (pkginfo->ei_date)
		{
			snprintf(errbuf, errsz,
				 "unique field \"%s\" specified twice", tok);
			errno = EINVAL;
			return -1;
		}
		pkginfo->ei_date = strdup(nextp);
#endif /* ENCAPINFO_DATE_TIME_T */
		return 0;
	}

	/* maintainer contact information */
	if (strcmp(tok, "contact") == 0)
	{
		if (pkginfo->ei_contact)
		{
			snprintf(errbuf, errsz,
				 "unique field \"%s\" specified twice", tok);
			errno = EINVAL;
			return -1;
		}
		pkginfo->ei_contact = strdup(nextp);
		return 0;
	}

	/* prerequisite checking */
	if (strcmp(tok, "prereq") == 0)
	{
		if (encap_prereq_parse(pkginfo, nextp) == -1)
		{
			snprintf(errbuf, errsz,
				 "parse error in encapinfo file");
			return -1;
		}
		return 0;
	}

	/* links with special names */
	if (strcmp(tok, "linkname") == 0)
	{
#ifdef DEBUG
		printf("libencap: adding %s to linkname list\n",
		       nextp);
#endif
		do
			tok = strsep(&nextp, " ");
		while (tok != NULL && *tok == '\0');

		if (tok == NULL || nextp == NULL || *nextp == '\0')
		{
			snprintf(errbuf, errsz,
				 "parse error in encapinfo file");
			errno = EINVAL;
			return -1;
		}

		nextp += strspn(nextp, " \t");

		strlcpy(lns.ln_pkgdir_path, tok,
			sizeof(lns.ln_pkgdir_path));
		strlcpy(lns.ln_newname, nextp, sizeof(lns.ln_newname));

		lnp = (linkname_t *)malloc(sizeof(linkname_t));
		memcpy(lnp, &lns, sizeof(linkname_t));
		encap_hash_add(pkginfo->ei_ln_h, (void *)lnp);

		return 0;
	}

	/*
	** directories which should be linked directly
	** instead of recursed into
	*/
	if (strcmp(tok, "linkdir") == 0)
	{
#ifdef DEBUG
		printf("libencap: adding %s to linkdir list\n",
		       nextp);
#endif
		encap_list_add(pkginfo->ei_ld_l, strdup(nextp));
		return 0;
	}

	/*
	** files which are required to be linked in for the
	** install to succeed
	*/
	if (strcmp(tok, "require") == 0)
	{
#ifdef DEBUG
		printf("libencap: adding %s to require list\n",
		       nextp);
#endif
		encap_list_add(pkginfo->ei_rl_l, strdup(nextp));
		return 0;
	}

	/* files to exclude from linking/removing */
	if (strcmp(tok, "exclude") == 0)
	{
#ifdef DEBUG
		printf("libencap: adding %s to exclude list\n",
		       nextp);
#endif
		encap_list_add(pkginfo->ei_ex_l, strdup(nextp));
		return 0;
	}

	snprintf(errbuf, errsz, "unknown encapinfo directive");
	errno = EINVAL;
	return -1;
}


int
encap_get_info(ENCAP *encap)
{
	struct stat s;
	char buf[MAXPATHLEN];
	int i;
	const char *pkg_scripts[] = {
		"preinstall", "postinstall", "preremove", "postremove", NULL
	};

#ifdef DEBUG
	printf("==> encap_get_info(encap=0x%lx)\n", encap);
#endif

	encapinfo_init(&(encap->e_pkginfo));

	/* check for encapinfo file */
	snprintf(buf, sizeof(buf), "%s/%s/encapinfo",
		 encap->e_source, encap->e_pkgname);
	if (stat(buf, &s) == 0)
		return encapinfo_read(buf, encap);
	if (errno != ENOENT)
		return -1;

	/* if the encapinfo file doesn't exist, it's a 1.x package */

	for (i = 0; pkg_scripts[i] != NULL; i++)
	{
		snprintf(buf, sizeof(buf), "%s/%s/%s",
			 encap->e_source, encap->e_pkgname, pkg_scripts[i]);
		if (stat(buf, &s) == 0)
		{
			/* Encap 1.1 is classic plus package scripts */
			encap->e_pkginfo.ei_pkgfmt = strdup("1.1");
			return 0;
		}
		if (errno != ENOENT)
			return -1;
	}

	/* Encap 1.0 is the classic format with no extensions */
	encap->e_pkginfo.ei_pkgfmt = strdup("1.0");
	return 0;
}


/*
** creates an encapinfo file.
** returns:
**	0				success
**	-1 (and sets errno)		failure
*/
int
encapinfo_write(char *outfile, encapinfo_t *pkginfo)
{
	FILE *f;
	struct linkname *lnp;
	encap_hashptr_t hp;
	encap_listptr_t lp;

#ifdef DEBUG
	printf("==> encapinfo_write(\"%s\", 0x%lx)\n", outfile, pkginfo);
#endif

	encap_listptr_reset(&lp);

	f = fopen(outfile, "w");
	if (f == NULL)
		return -1;

	fprintf(f, "encap %s\t# libencap-%s\n",
		pkginfo->ei_pkgfmt, libencap_version);

	if (pkginfo->ei_platform != NULL)
		fprintf(f, "platform %s\n", pkginfo->ei_platform);

#ifdef ENCAPINFO_DATE_TIME_T
	if (pkginfo->ei_date != 0)
	{
		char buf[1024];

		strftime(buf, 1024, "%a %b %d %H:%M:%S %Z %Y",
			 localtime(&pkginfo->ei_date));

		fprintf(f, "date %s\n", buf);
	}
#else /* ! ENCAPINFO_DATE_TIME_T */
	if (pkginfo->ei_date != NULL)
		fprintf(f, "date %s\n", pkginfo->ei_date);
#endif /* ENCAPINFO_DATE_TIME_T */

	if (pkginfo->ei_contact != NULL)
		fprintf(f, "contact %s\n", pkginfo->ei_contact);

	if (pkginfo->ei_description != NULL)
		fprintf(f, "description %s\n", pkginfo->ei_description);

	if (pkginfo->ei_ex_l != NULL)
	{
		encap_listptr_reset(&lp);
		while (encap_list_next(pkginfo->ei_ex_l, &lp))
			fprintf(f, "exclude %s\n",
				(char *)encap_listptr_data(&lp));
	}

	if (pkginfo->ei_rl_l != NULL)
	{
		encap_listptr_reset(&lp);
		while (encap_list_next(pkginfo->ei_rl_l, &lp))
			fprintf(f, "require %s\n",
				(char *)encap_listptr_data(&lp));
	}

	if (pkginfo->ei_ld_l != NULL)
	{
		encap_listptr_reset(&lp);
		while (encap_list_next(pkginfo->ei_ld_l, &lp))
			fprintf(f, "linkdir %s\n",
				(char *)encap_listptr_data(&lp));
	}

	if (pkginfo->ei_pr_l != NULL)
	{
		encap_listptr_reset(&lp);
		while (encap_list_next(pkginfo->ei_pr_l, &lp))
		{
			char buf[MAXPATHLEN];
			ENCAP_PREREQ *epp;

			epp = (ENCAP_PREREQ *)encap_listptr_data(&lp);

			encap_prereq_print(epp, buf, sizeof(buf));
			fprintf(f, "prereq %s\n", buf);
		}
	}

	if (pkginfo->ei_ln_h != NULL)
	{
		encap_hashptr_reset(&hp);
		while (encap_hash_next(pkginfo->ei_ln_h, &hp))
		{
			lnp = (linkname_t *)encap_hashptr_data(&hp);
			fprintf(f, "linkname %s %s\n", lnp->ln_pkgdir_path,
				lnp->ln_newname);
		}
	}

	fclose(f);
	return 0;
}


int
encapinfo_init(encapinfo_t *pkginfo)
{
	memset(pkginfo, 0, sizeof(encapinfo_t));
	pkginfo->ei_rl_l = encap_list_new(LIST_USERFUNC, NULL);
	if (pkginfo->ei_rl_l == NULL)
		return -1;
	pkginfo->ei_ld_l = encap_list_new(LIST_USERFUNC, NULL);
	if (pkginfo->ei_ld_l == NULL)
		return -1;
	pkginfo->ei_ex_l = encap_list_new(LIST_USERFUNC, NULL);
	if (pkginfo->ei_ex_l == NULL)
		return -1;
	pkginfo->ei_pr_l = encap_list_new(LIST_USERFUNC, NULL);
	if (pkginfo->ei_pr_l == NULL)
		return -1;
	pkginfo->ei_ln_h = encap_hash_new(128, NULL);
	if (pkginfo->ei_ln_h == NULL)
		return -1;
	return 0;
}


void
encapinfo_free(encapinfo_t *pkginfo)
{
	if (pkginfo->ei_rl_l != NULL)
		encap_list_free(pkginfo->ei_rl_l, free);
	if (pkginfo->ei_ld_l != NULL)
		encap_list_free(pkginfo->ei_ld_l, free);
	if (pkginfo->ei_ex_l != NULL)
		encap_list_free(pkginfo->ei_ex_l, free);
	if (pkginfo->ei_pr_l != NULL)
		encap_list_free(pkginfo->ei_pr_l, free);
	if (pkginfo->ei_ln_h != NULL)
		encap_hash_free(pkginfo->ei_ln_h, free);
	if (pkginfo->ei_pkgfmt != NULL)
		free(pkginfo->ei_pkgfmt);
	if (pkginfo->ei_platform != NULL)
		free(pkginfo->ei_platform);
	if (pkginfo->ei_contact != NULL)
		free(pkginfo->ei_contact);
	if (pkginfo->ei_description != NULL)
		free(pkginfo->ei_description);
#ifndef ENCAPINFO_DATE_TIME_T
	if (pkginfo->ei_date != NULL)
		free(pkginfo->ei_date);
#endif /* ! ENCAPINFO_DATE_TIME_T */
}


/*
** reads the encap.exclude file in directory dir and adds entries to list
** ex_l.  if prefix is non-NULL, it and '/' are prepended to each entry read.
** returns:
**	1				success
**	0				encap.exclude file doesn't exist
**	-1 (and sets errno)		error
*/
int
encap_read_exclude_file(char *dir, char *prefix, encap_list_t *ex_l)
{
	FILE *f;
	char tmp1[MAXPATHLEN];
	char tmp2[MAXPATHLEN];

	snprintf(tmp1, sizeof(tmp1), "%s/encap.exclude", dir);
	f = fopen(tmp1, "r");
	if (f == NULL)
	{
		if (errno != ENOENT)
			return -1;
		return 0;
	}

	/* add the entries from the exclude file */
	while (fscanf(f, "%s", tmp1) == 1)
		if (prefix != NULL && *prefix != '\0')
		{
			snprintf(tmp2, sizeof(tmp2), "%s/%s", prefix, tmp1);
			encap_list_add(ex_l, strdup(tmp2));
		}
		else
			encap_list_add(ex_l, strdup(tmp1));

	fclose(f);		/* dont bother checking for error... */

	return 1;
}


