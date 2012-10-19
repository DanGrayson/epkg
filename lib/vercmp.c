/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  vercmp.c - encap version comparison routines
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <ctype.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif


/*
** encap_vercmp() - compare ver1 and ver2
** returns:
**	-1	ver1 < ver2
**	0	ver1 == ver2
**	1	ver1 > ver2
*/
int
encap_vercmp(char *ver1, char *ver2)
{
	char buf1[MAXPATHLEN], buf2[MAXPATHLEN];
	char tmp1[MAXPATHLEN], tmp2[MAXPATHLEN];
	char *sub1, *sub2;
	char *next1 = buf1, *next2 = buf2;
	register char *cp1, *cp2;
	char *ep1, *ep2;
	char *pp1, *pp2;
	unsigned long ul1, ul2;
	size_t sz1, sz2;
	int i;

#ifdef DEBUG
	printf("==> encap_vercmp(\"%s\", \"%s\")\n", ver1, ver2);
#endif

	strlcpy(buf1, ver1, sizeof(buf1));
	if ((pp1 = strchr(buf1, '+')) != NULL)
		*pp1++ = '\0';
	strlcpy(buf2, ver2, sizeof(buf2));
	if ((pp2 = strchr(buf2, '+')) != NULL)
		*pp2++ = '\0';

	while (1)
	{
		sub1 = strsep(&next1, ".");
		sub2 = strsep(&next2, ".");
		if (sub1 == NULL || sub2 == NULL)
			break;

#ifdef DEBUG
		printf("    encap_vercmp(): substrings \"%s\" and \"%s\"\n",
		       sub1, sub2);
#endif

		cp1 = sub1;
		cp2 = sub2;

		while (*cp1 != '\0' && *cp2 != '\0')
		{
#ifdef DEBUG
			printf("    encap_vercmp(): cp1=\"%s\", cp2=\"%s\"\n",
				cp1 ? cp1 : "NULL", cp2 ? cp2 : "NULL");
#endif

			if (isdigit((int)*cp1))
			{
				if (! isdigit((int)*cp2))
					/*
					** this won't usually happen,
					** but we'll guess that the one
					** with the number is greater
					*/
					return 1;

#ifdef DEBUG
				puts("    encap_vercmp(): comparing numbers");
#endif

				/*
				** isolate and compare the numerical values
				*/
				ul1 = strtoul(cp1, &ep1, 0);
				ul2 = strtoul(cp2, &ep2, 0);
				if (ul1 < ul2)
					return -1;
				if (ul1 > ul2)
					return 1;

#ifdef DEBUG
				puts("    encap_vercmp(): ...numeric strings");
#endif

				/*
				** if they're equal numerically,
				** compare them in string form
				** (e.g., "06" is less than "6")
				*/
				strcpy(tmp1, cp1);
				sz1 = strspn(tmp1, "0123456789");
				tmp1[sz1] = '\0';
				strcpy(tmp2, cp2);
				sz2 = strspn(tmp2, "0123456789");
				tmp2[sz2] = '\0';

				i = strcmp(tmp1, tmp2);
				if (i != 0)
					return i;

				cp1 += sz1;
				cp2 += sz2;
			}
			else	/* the first character in cp1 isn't a digit */
			{
				if (isdigit((int)*cp2))
					/*
					** this won't usually happen,
					** but we'll guess that the one
					** with the number is greater
					*/
					return -1;

#ifdef DEBUG
				puts("    encap_vercmp(): comparing strings");
#endif

				/*
				** isolate and compare the non-numeric strings
				*/
				strcpy(tmp1, cp1);
				ep1 = strpbrk(tmp1, "0123456789");
				if (ep1 != NULL)
					*ep1 = '\0';
				cp1 += strlen(tmp1);
				strcpy(tmp2, cp2);
				ep2 = strpbrk(tmp2, "0123456789");
				if (ep2 != NULL)
					*ep2 = '\0';
				cp2 += strlen(tmp2);
				i = strcmp(tmp1, tmp2);
				if (i < 0)
					return -1;
				if (i > 0)
					return 1;
			}
		}

#ifdef DEBUG
		puts("    encap_vercmp(): at least one substring done");
#endif

		/*
		** if one of them has something left over in this subversion,
		** it's the bigger one, unless the other one has an additional
		** subversion and this one doesn't
		*/
		if (*cp1 == '\0' && *cp2 != '\0')
		{
			if (next1 != NULL && next2 == NULL)
				return 1;
			else
				return -1;
		}
		if (*cp1 != '\0' && *cp2 == '\0')
		{
			if (next1 == NULL && next2 != NULL)
				return -1;
			else
				return 1;
		}
	}

#ifdef DEBUG
	puts("    encap_vercmp(): all substrings done");
#endif

	if (sub1 == NULL && sub2 != NULL)
		return -1;
	if (sub1 != NULL && sub2 == NULL)
		return 1;

	/*
	** check pkg version
	*/
	if (pp1 != NULL && pp2 != NULL)
		return encap_vercmp(pp1, pp2);
	if (pp1 == NULL && pp2 != NULL)
		return -1;
	if (pp1 != NULL && pp2 == NULL)
		return 1;

	/*
	** both versions are the same
	*/
	return 0;
}


#ifdef TEST_VERCMP

struct test_case
{
	char *tc_ver1;
	char *tc_ver2;
	int tc_result;
};

static struct test_case tests[] = {
	{ "1.0",	"0.1",		1 },
	{ "1.0",	"1.0",		0 },
	{ "1.0",	"1.1",		-1 },
	{ "1.0",	"1.0.1",	-1 },
	{ "1.0",	"1.0a",		-1 },
	{ "1.0a2",	"1.0.1",	-1 },
	{ "1.0",	"1.a",		1 },
	{ "1..0",	"1.0",		1 },
	{ "1..0",	"1.0.0",	-1 },
	{ "",		"1",		-1 },
	{ "1.0",	"1.0+0",	-1 },
	{ "1.0+1",	"1.0+0",	1 },
	{ NULL,		NULL,		0 }
};


int
main(int argc, char *argv[])
{
	int i, j;

	for (i = 0; tests[i].tc_ver1 != NULL; i++)
	{
		printf("tests[%d]: encap_vercmp(\"%s\", \"%s\") == ",
		       i, tests[i].tc_ver1, tests[i].tc_ver2);
		j = encap_vercmp(tests[i].tc_ver1, tests[i].tc_ver2);
		printf("%d", j);
		if (j != tests[i].tc_result)
			printf("\t*** EXPECTING: %d", tests[i].tc_result);
		putchar('\n');
	}

	exit(0);
}

#endif /* TEST_VERCMP */

