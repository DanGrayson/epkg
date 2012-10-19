/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  check_pkg.c - package check recursion plugins for epkg
**
**  Mark D. Roth <roth@uiuc.edu>
*/

#include <internal.h>

#include <stdio.h>
#include <dirent.h>
#include <sys/param.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/* check a specific package link */
int
check_link(ENCAP *encap, encap_source_info_t *srcinfo,
	   encap_target_info_t *tgtinfo)
{
	/* does the link exist */
	if (!BIT_ISSET(tgtinfo->tgt_flags, TGT_EXISTS))
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_CHK_FAIL,
				       "link does not exist");
		return R_ERR;
	}

	/* make sure it's a symlink */
	if (!BIT_ISSET(tgtinfo->tgt_flags, TGT_ISLNK))
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_CHK_FAIL,
				       "not a symlink");
		return R_ERR;
	}

	/* make sure it's an encap link */
	if (!BIT_ISSET(tgtinfo->tgt_flags, TGT_DEST_ENCAP_SRC))
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_CHK_FAIL,
				       "not an Encap link");
		return R_ERR;
	}

	/* check if the existing link points to a different package */
	if (strcmp(tgtinfo->tgt_link_existing_pkg, encap->e_pkgname) != 0)
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_CHK_FAIL,
				       "link to package %s",
				       tgtinfo->tgt_link_existing_pkg);
		return R_ERR;
	}

	/* If we get here, it's pointing to this package.  Now make
	   sure it's pointing to the right specific file. */
	if (strcmp(tgtinfo->tgt_link_existing_pkgdir_relative,
		   srcinfo->src_link_expecting +
		       strlen(srcinfo->src_link_expecting) -
		       strlen(tgtinfo->tgt_link_existing_pkgdir_relative)) != 0)
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_CHK_FAIL,
				    "link to incorrect file %s",
				    tgtinfo->tgt_link_existing_pkgdir_relative);
		return R_ERR;
	}

	/* if we fall through to here, the link is valid */
	(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_CHK_NOOP, NULL);
	return R_FILEOK;
}


