/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  remove_pkg.c - removal recursion plugins for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/* remove a specific package directory */
int
remove_dir(ENCAP *encap, encap_source_info_t *srcinfo,
	   encap_target_info_t *tgtinfo)
{
	if (BIT_ISSET(encap->e_options, OPT_NUKETARGETDIRS))
	{

		/* attempt to remove target directory */
		if (!BIT_ISSET(encap->e_options, OPT_SHOWONLY) &&
		    rmdir(srcinfo->src_target_path) != 0)
		{

			/*
			 * failure ok if directory isn't empty, doesn't exist,
			 * or is a mount point
			 */
			if (errno != EEXIST && errno != EBUSY &&
			    errno != ENOTEMPTY && errno != ENOENT)
			{
				(*encap->e_print_func)(encap, srcinfo, tgtinfo,
						       EPT_REM_ERROR, "rmdir");
				return R_ERR;
			}
		}
		else
			(*encap->e_print_func)(encap, srcinfo, tgtinfo,
					       EPT_REM_OK, NULL);
	}

	return R_FILEOK;
}


/* remove a specific package link */
int
remove_link(ENCAP *encap, encap_source_info_t *srcinfo,
	    encap_target_info_t *tgtinfo)
{
	/* link doesn't exist */
	if (!BIT_ISSET(tgtinfo->tgt_flags, TGT_EXISTS))
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_REM_NOOP,
				       NULL);
		return R_SKIP;
	}

	/* make sure it's a symlink */
	if (!BIT_ISSET(tgtinfo->tgt_flags, TGT_ISLNK))
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_REM_FAIL,
				       "not a symlink");
		return R_SKIP;
	}

	/* if it's not an encap link, don't touch it */
	if (!BIT_ISSET(tgtinfo->tgt_flags, TGT_DEST_ENCAP_SRC))
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_REM_FAIL,
				       "not an Encap link");
		return R_SKIP;
	}

	/* if it's a link to a non-existing package, issue a warning
	   and dont touch it */
	if (!BIT_ISSET(tgtinfo->tgt_flags, TGT_DEST_PKGDIR_EXISTS))
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_REM_FAIL,
				       "link to non-existant package");
		return R_SKIP;
	}

	/* make sure it points into this package. */
	if (strcmp(encap->e_pkgname, tgtinfo->tgt_link_existing_pkg) != 0)
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_REM_FAIL,
				       "link to package %s",
				       tgtinfo->tgt_link_existing_pkg);
		return R_SKIP;
	}

	/* now we can safely blow it away */
	if (!BIT_ISSET(encap->e_options, OPT_SHOWONLY) &&
	    remove(srcinfo->src_target_path) != 0)
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo,
				       EPT_REM_ERROR, "remove");
		return R_ERR;
	}

	/* success */
	(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_REM_OK, NULL);
	return R_FILEOK;
}


