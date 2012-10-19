/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  platform.c - encap platform name code
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <errno.h>
#include <fcntl.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
# include <ctype.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_ODMI_H
# include <odmi.h>
#endif

#ifdef HAVE_SYS_SYSTEMCFG_H
# include <sys/systemcfg.h>
#endif

#ifdef HAVE_SYS_SYSTEMINFO_H
# include <sys/systeminfo.h>
#endif


#define BUFSIZE 1024


int
encap_platform_compat(char *pkg_platform, char *host_platform,
		      encap_list_t *suffix_l)
{
	char base[MAXPATHLEN], suffix[MAXPATHLEN];
	encap_listptr_t lp;
	char *cp;

	if (encap_platform_split(pkg_platform, base, sizeof(base),
				 suffix, sizeof(suffix)) == -1)
		return -1;

	if (strcmp(base, "share") == 0
	    && suffix[0] == '\0')
		return 1;

	/* FIXME: backward-compatibility hack for HP-UX */
	if (strncmp(host_platform, "pa-hpux", 7) == 0)
	{
		if (strncmp(base, "pa1.1-hpux", 10) == 0
		    || strncmp(base, "pa2.0-hpux", 10) == 0)
			memmove(base + 2, base + 5, strlen(base + 5) + 1);
	}

	if (strcmp(base, host_platform) != 0)
		return 0;

	if (suffix[0] == '\0')
		return 1;

	if (suffix_l != NULL)
	{
		encap_listptr_reset(&lp);
		while (encap_list_next(suffix_l, &lp) != 0)
		{
			cp = (char *)encap_listptr_data(&lp);
			if (strcmp(cp, suffix) == 0)
				return 1;
		}
	}

	return 0;
}


int
encap_platform_split(char *platform, char *base, size_t baselen,
		     char *other, size_t otherlen)
{
	char *cp;

	base[0] = '\0';
	other[0] = '\0';

	if (strcmp(platform, "share") == 0)
	{
		strlcpy(base, platform, baselen);
		return 0;
	}

	cp = strchr(platform, '-');
	if (cp == NULL)
		return -1;

	cp = strchr(cp + 1, '-');
	if (cp != NULL)
		*cp++ = '\0';

	strlcpy(base, platform, baselen);
	if (cp != NULL)
		strlcpy(other, cp, otherlen);

	return 0;
}


#ifdef HAVE_ODMI_H

static int
odm_offset(CLASS_SYMBOL lpp_class, char *field)
{
	int i;

	for (i = 0; i < lpp_class->nelem; i++)
		if (strcmp(lpp_class->elem[i].elemname, field) == 0)
			return lpp_class->elem[i].offset;

	return -1;
}


static int
aix_oslevel(char *buf, size_t bufsize)
{
	char *cnp;
	CLASS_SYMBOL lpp_class;
	int voff, roff, moff, doff;
	short v, r, m;
	char *cp;

	if (odm_initialize() == -1)
	{
		fprintf(stderr, "odm_initialize(): error %d\n", odmerrno);
		exit(1);
	}

	lpp_class = odm_mount_class("lpp");
	if (lpp_class == (CLASS_SYMBOL) - 1)
	{
		fprintf(stderr, "odm_mount_class(): error %d\n", odmerrno);
		return -1;
	}

	voff = odm_offset(lpp_class, "ver");
	roff = odm_offset(lpp_class, "rel");
	moff = odm_offset(lpp_class, "mod");
	if (voff == -1 || roff == -1 || moff == -1)
	{
		fprintf(stderr, "aix_oslevel(): cannot find VRM fields\n");
		return -1;
	}

	doff = odm_offset(lpp_class, "description");
	if (doff == -1)
	{
		fprintf(stderr,
			"aix_oslevel(): cannot find description field\n");
		return -1;
	}

	cnp = odm_get_obj(lpp_class, "name='bos.rte'", NULL, ODM_FIRST);
	if (cnp == (char *)-1)
	{
		fprintf(stderr, "odm_get_obj(): error %d\n", odmerrno);
		return -1;
	}

	memcpy(&v, cnp + voff, sizeof(short));
	memcpy(&r, cnp + roff, sizeof(short));
	memcpy(&m, cnp + moff, sizeof(short));

	snprintf(buf, bufsize, "%hd.%hd.%hd", v, r, m);

	memcpy(&cp, cnp + doff, sizeof(char *));
	free(cp);
	free(cnp);

	odm_terminate();
	return 0;
}

#endif /* HAVE_ODMI_H */


char *
encap_platform_name(char *buf, size_t bufsize)
{
	struct utsname ut;
	char archbuf[BUFSIZE] = "";
	char verbuf[BUFSIZE] = "";
	char osbuf[BUFSIZE] = "";
	char *cp;
	int i, j;

	uname(&ut);

	/*
	** AIX
	*/
	if (strcmp(ut.sysname, "AIX") == 0)
	{
#ifdef __ia64
		if (__ia64())
			strlcpy(archbuf, "ia64", sizeof(archbuf));
		else /* if (__power_pc() || __power_rs()) */
#endif /* __ia64 */
			strlcpy(archbuf, "rs6000", sizeof(archbuf));

#ifdef HAVE_ODMI_H
		if (aix_oslevel(verbuf, sizeof(verbuf)) == -1)
#endif /* HAVE_ODMI_H */
			snprintf(verbuf, sizeof(verbuf), "%s.%s", ut.version,
				 ut.release);
	}
	else

	/*
	** HP-UX
	*/
	if (strcmp(ut.sysname, "HP-UX") == 0)
	{
#ifdef _SC_CPU_VERSION
		i = sysconf(_SC_CPU_VERSION);
		if (CPU_IS_HP_MC68K(i))
			strlcpy(archbuf, "m68k", sizeof(archbuf));
		else
#endif /* _SC_CPU_VERSION */
			/* assume PA-RISC */
			strlcpy(archbuf, "pa", sizeof(archbuf));

		/* skip leading "B." */
		strlcpy(verbuf, ut.release + 2, sizeof(verbuf));
	}
	else

#ifdef HAVE_SYS_SYSTEMINFO_H
	/*
	** IRIX
	*/
	if (strncmp(ut.sysname, "IRIX", 4) == 0)
		sysinfo(SI_ARCHITECTURE, archbuf, sizeof(archbuf));
	else
#endif /* HAVE_SYS_SYSTEMINFO_H */

	/*
	** Linux
	*/
	if (strcmp(ut.sysname, "Linux") == 0)
	{
		/* only save the first two digits of the kernel version */
		strlcpy(verbuf, ut.release, sizeof(verbuf));
		if ((cp = strchr(verbuf, '.')) != NULL
		    && (cp = strchr(cp + 1, '.')) != NULL)
			*cp = '\0';
	}
	else

	/*
	** Solaris
	*/
	if (strcmp(ut.sysname, "SunOS") == 0
	    && strncmp(ut.release, "5.", 2) == 0)
	{
#ifdef HAVE_SYS_SYSTEMINFO_H
		sysinfo(SI_ARCHITECTURE, archbuf, sizeof(archbuf));
		if (strcmp(archbuf, "i86pc") == 0)
			strlcpy(archbuf, "ix86", sizeof(archbuf));
#endif /* HAVE_SYS_SYSTEMINFO_H */

		strlcpy(osbuf, "solaris", sizeof(osbuf));
		snprintf(verbuf, sizeof(verbuf), "%s%s",
			 (atoi(ut.release + 2) < 7 ? "2." : ""),
			 ut.release + 2);
	}
	else

	/*
	** OS X
	*/
	if (strcmp(ut.sysname, "Darwin") == 0)
	{
		/*
		** using the __ppc__ preprocessor definition is pretty
		** messy, but it's how Darwin's own uname command works...
		*/
#ifdef __ppc__
		strlcpy(archbuf, "powerpc", sizeof(archbuf));
#else /* ! __ppc__ */
		/*
		** here's a fallback in case __ppc__ doesn't work:
		** we copy ut.machine as lower case and without spaces
		** to handle something like "Power Macintosh"
		*/
		for (i = 0, j = 0; ut.machine[i] != '\0'; i++)
		{
			if (isalpha((int)ut.machine[i]))
				archbuf[j++] = tolower(ut.machine[i]);
		}
		archbuf[j] = '\0';
#endif /* __ppc__ */
	}

	/*
	** catch-all for "i.86.*"
	*/
	if (archbuf[0] == '\0'
	    && ut.machine[0] == 'i'
	    && strcmp(ut.machine + 2, "86") == 0)
		strlcpy(archbuf, "ix86", sizeof(archbuf));

	/*
	** set osbuf to lower-case version of ut.sysname
	*/
	if (osbuf[0] == '\0')
	{
		for (i = 0, j = 0; ut.sysname[i] != '\0'; i++)
		{
			if (isalpha((int)ut.sysname[i]))
				osbuf[j++] = tolower(ut.sysname[i]);
		}
		osbuf[j] = '\0';
	}

	/*
	** put it all together
	*/
	snprintf(buf, bufsize, "%s-%s%s",
		 (archbuf[0] ? archbuf : ut.machine), osbuf,
		 (verbuf[0] ? verbuf : ut.release));

	return buf;
}


char *
encap20_platform_name(char *buf, size_t bufsize)
{
	struct utsname ut;

	uname(&ut);
	if (strcmp(ut.sysname, "AIX") == 0)
		snprintf(buf, bufsize, "%s-%s.%s", ut.sysname, ut.version,
			 ut.release);
	else if (strcmp(ut.sysname, "HP-UX") == 0)
		snprintf(buf, bufsize, "%s-%s", ut.sysname, &(ut.release[2]));
	else
		snprintf(buf, bufsize, "%s-%s", ut.sysname, ut.release);

	return buf;
}


#ifdef TEST_PLATFORM_NAME

int
main(int argc, char *argv[])
{
	char buf[BUFSIZE];

	printf("%s\n", encap_platform_name(buf, sizeof(buf)));

	if (argc == 2)
	{
		printf("encap_platform_compat(\"%s\", \"%s\", NULL) = %d\n",
		       argv[1], buf,
		       encap_platform_compat(argv[1], buf, NULL));
	}

	exit(0);
}

#endif /* TEST_PLATFORM_NAME */

