/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  package.c - package processing code for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif


#define POST		0
#define PRE		1


const mode_info_t modemap[] = {
	{ ENCAP_INSTALL,	"install",	install_dir,
			NULL,		install_link },
	{ ENCAP_REMOVE,		"remove",	NULL,
			remove_dir,	remove_link },
	{ ENCAP_CHECK,		"check",	NULL,
			NULL,		check_link },
	{ 0,			NULL,		NULL,
			NULL,		NULL }
};

const mode_info_t *
mode_info(int mode)
{
	int i;

	for (i = 0; modemap[i].mode != 0; i++)
		if (modemap[i].mode == mode)
			return &(modemap[i]);

	return NULL;
}


/* run the appropriate (pre|post)(install|remove) script
   returns script exit value if successful, 0 if no script, -1 on error */
static int
run_script(ENCAP *encap, short pre, short mode)
{
	char script_name[MAXPATHLEN];
	char buf[MAXPATHLEN];
	struct stat s;
	FILE *fp;

	/* first, make sure the script exists */
	snprintf(script_name, sizeof(script_name), "%s/%s/%s%s",
		 encap->e_source, encap->e_pkgname,
		 (pre ? "pre" : "post"), mode_info(mode)->name);
	if (stat(script_name, &s) != 0)
	{
		if (errno == ENOENT)
			return 0;
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_ERROR,
				       "stat(\"%s\")", script_name);
		return -1;
	}

	/* set up the $ENCAP_PKGNAME environment variable */
	snprintf(buf, sizeof(buf), "ENCAP_PKGNAME=%s", encap->e_pkgname);
	if (putenv(buf) != 0)
	{
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_ERROR,
				       "putenv(\"%s\")", buf);
		return -1;
	}

	/* run it */
	(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_INFO,
			       "executing %s%s script",
			       (pre ? "pre" : "post"),
			       mode_info(mode)->name);
	if (BIT_ISSET(encap->e_options, OPT_SHOWONLY))
		return 0;
	fp = popen(script_name, "r");
	if (fp == NULL)
	{
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_ERROR,
				       "popen(\"%s\")", script_name);
		return -1;
	}

	while (fgets(buf, sizeof(buf), fp) != NULL)
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_RAW,
				       "%s", buf);

	return pclose(fp);
}


/* display README file.  returns 0 on success, -1 on error, or 1 if no README */
static int
display_readme(ENCAP *encap)
{
	char buf[MAXPATHLEN];
	FILE *fp;

	snprintf(buf, sizeof(buf), "%s/%s/README", encap->e_source,
		 encap->e_pkgname);
	fp = fopen(buf, "r");
	if (fp == NULL)
	{
		if (errno == ENOENT)
			return 1;
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_ERROR,
				       "fopen(\"%s\")", buf);
		return -1;
	}

	(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_INFO,
			       "Displaying README file...");
	while (fgets(buf, sizeof(buf), fp) != NULL)
		(*encap->e_print_func)(encap, NULL, NULL, EPT_PKG_RAW,
				       "%s", buf);

	fclose(fp);
	return 0;
}


static int
process_package(ENCAP *encap, int mode, encap_decision_func_t decision_func)
{
	int i;
	int status = 0;
	encap_source_info_t rootpath;

#ifdef DEBUG
	printf("==> process_package(encap=\"%s\", mode=%d)\n",
	       encap->e_pkgname, mode);
#endif

	if (encap_check_source(encap, NULL, NULL, &rootpath) == -1)
		return ESTAT_FATAL;

	/* run pre(install|remove) script */
	if (mode != ENCAP_CHECK &&
	    BIT_ISSET(encap->e_options, OPT_RUNSCRIPTS | OPT_RUNSCRIPTSONLY)
	    && encap_vercmp(encap->e_pkginfo.ei_pkgfmt, "1.0") > 0)
	{
		if ((i = run_script(encap, PRE, mode)) != 0)
		{
			if (i != -1)
				(*encap->e_print_func)(encap, NULL, NULL,
						EPT_PKG_FAIL,
						"pre%s script returned %d",
						mode_info(mode)->name, i);
			return ESTAT_FATAL;
		}
	}

	/* if not in client mode, do recursion */
	if (!BIT_ISSET(encap->e_options, OPT_RUNSCRIPTSONLY))
	{
		i = encap_recursion(encap, &rootpath, &status, mode,
				    decision_func);
#ifdef DEBUG
		printf("back from recursion(): status == %d\n", status);
#endif
		if (i == R_RETURN)
			return status;
	}

	/* run post(install|remove) script */
	if (BIT_ISSET(encap->e_options, OPT_RUNSCRIPTSONLY) ||
	    (BIT_ISSET(status, ESTAT_OK) &&
	     mode != ENCAP_CHECK &&
	     BIT_ISSET(encap->e_options, OPT_RUNSCRIPTS) &&
	     encap_vercmp(encap->e_pkginfo.ei_pkgfmt, "1.0") > 0))
	{
		if ((i = run_script(encap, POST, mode)) != 0)
		{
			if (i != -1)
				(*encap->e_print_func)(encap, NULL, NULL,
						EPT_PKG_FAIL,
						"post%s script returned %d",
						mode_info(mode)->name, i);
			BIT_SET(status, ESTAT_FATAL);
			return status;
		}
	}

	return status;
}


/* install a package */
int
encap_install(ENCAP *encap, encap_decision_func_t decision_func)
{
	int status;

	if (encap_vercmp(encap->e_pkginfo.ei_pkgfmt, "2.0") >= 0)
	{
		display_readme(encap);
		if (BIT_ISSET(encap->e_options, OPT_PREREQS) &&
		    encap_check_prereqs(encap) != 0)
			return -1;
	}

	status = process_package(encap, ENCAP_INSTALL, decision_func);

	if (BIT_ISSET(status, ESTAT_FATAL) || (status == ESTAT_ERR))
		/*
		 * Failure - two possibilities:
		 * 1. a required link failed
		 * 2. there were some failures, no successes, and no links were
		 *    already there
		 */
		return ENCAP_STATUS_FAILED;

	if (status == 0 ||
	    (BIT_ISSET(status, ESTAT_NONEED) &&
	     !BIT_ISSET(status, ESTAT_OK)))
		/*
		 * No-op - at least one link was already there,
		 * no links created
		 */
		return ENCAP_STATUS_NOOP;

	if (BIT_ISSET(status, ESTAT_ERR))
		/*
		 * Partial - if we get here, ESTAT_OK must be set, so
		 * ESTAT_ERR means some successes and some non-required
		 * failures.
		 */
		return ENCAP_STATUS_PARTIAL;

	/*
	 * Success - only ESTAT_OK is set, so it's a complete success
	 */
	return ENCAP_STATUS_SUCCESS;
}


/* remove a package */
int
encap_remove(ENCAP *encap, encap_decision_func_t decision_func)
{
	int status;

	status = process_package(encap, ENCAP_REMOVE, decision_func);

	if (BIT_ISSET(status, ESTAT_ERR))
		/*
		 * Failure - if we failed to remove any valid link,
		 * the removal fails
		 */
		return ENCAP_STATUS_FAILED;

	if (status == 0 || status == ESTAT_NONEED)
		/*
		 * No-op - all valid links were already removed
		 * (assume package was already removed)
		 */
		return ENCAP_STATUS_NOOP;

	if (BIT_ISSET(status, ESTAT_NONEED))
		/*
		 * Partial - if we get here, ESTAT_OK must be set,
		 * so ESTAT_NONEED means partial success
		 */
		return ENCAP_STATUS_PARTIAL;

	/*
	 * Success - only ESTAT_OK is set, so it's a success
	 */
	return ENCAP_STATUS_SUCCESS;
}


/* check a package */
int
encap_check(ENCAP *encap, encap_decision_func_t decision_func)
{
	int status;

	status = process_package(encap, ENCAP_CHECK, decision_func);

	if (BIT_ISSET(status, ESTAT_FATAL) || status == ESTAT_ERR)
		/*
		 * Failure - either a required link is not present,
		 * or no links are present
		 */
		return ENCAP_STATUS_FAILED;

	/*
	 * Success - at least one link is present, and all required links are
	 *           present
	 */
	return ENCAP_STATUS_SUCCESS;
}


