/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  install_pkg.c - install recursion plugins for epkg
**
**  Mark D. Roth <roth@feep.net>
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


int
install_dir(ENCAP *encap, encap_source_info_t *srcinfo,
	    encap_target_info_t *tgtinfo)
{
	char *msg = NULL;
	int type = EPT_INST_OK;

#ifdef DEBUG
	printf("==> install_dir(encap=\"%s\", srcinfo=\"%s\")\n",
	       encap->e_pkgname, srcinfo->src_pkgdir_relative);
#endif

	if (BIT_ISSET(tgtinfo->tgt_flags, TGT_EXISTS))
	{

		if (BIT_ISSET(tgtinfo->tgt_flags, TGT_ISDIR))
		{
			(*encap->e_print_func)(encap, srcinfo, tgtinfo,
					       EPT_INST_NOOP, NULL);
			return R_SKIP;
		}

		if (!BIT_ISSET(encap->e_options, OPT_FORCE))
		{

			if (!BIT_ISSET(tgtinfo->tgt_flags, TGT_DEST_ENCAP_SRC))
			{
				(*encap->e_print_func)(encap, srcinfo,
						       tgtinfo,
						       EPT_INST_FAIL,
						       "not an Encap link");
				return R_ERR;
			}

			if (BIT_ISSET(tgtinfo->tgt_flags, TGT_DEST_EXISTS))
			{
				(*encap->e_print_func)(encap, srcinfo,
						tgtinfo, EPT_INST_FAIL,
						"target directory conflicts "
						    "with symlink: %s -> %s",
						srcinfo->src_target_path,
						tgtinfo->tgt_link_existing);
				return R_ERR;
			}

		}
		else
			msg = "forced replacement";

		if (!BIT_ISSET(encap->e_options, OPT_SHOWONLY) &&
		    remove(srcinfo->src_target_path) != 0)
		{
			(*encap->e_print_func)(encap, srcinfo, tgtinfo,
					       EPT_INST_ERROR, "remove");
			return R_ERR;
		}

		type = EPT_INST_REPL;
	}

	if (!BIT_ISSET(encap->e_options, OPT_SHOWONLY) &&
	    mkdir(srcinfo->src_target_path, 0755) != 0)
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo,
				       EPT_INST_ERROR, "mkdir");
		return R_ERR;
	}
	else
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, type, msg);

	/* read encap.exclude file where applicable */
	if (BIT_ISSET(encap->e_options, OPT_EXCLUDES) &&
	    (encap_vercmp(encap->e_pkginfo.ei_pkgfmt, "2.0") < 0) &&
	    encap_read_exclude_file(srcinfo->src_path,
				    srcinfo->src_pkgdir_relative,
				    encap->e_pkginfo.ei_ex_l))
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_PKG_INFO,
				       "read encap.exclude file");

	return R_FILEOK;
}


/* create a specific package link */
int
install_link(ENCAP *encap, encap_source_info_t *srcinfo,
	     encap_target_info_t *tgtinfo)
{
	int type = EPT_INST_OK;
	char *msg = NULL;

#ifdef DEBUG
	printf("==> install_link(encap=\"%s\", srcinfo=\"%s\")\n",
	       encap->e_pkgname, srcinfo->src_pkgdir_relative);
#endif

	/* conflict resolution */
	if (BIT_ISSET(tgtinfo->tgt_flags, TGT_EXISTS))
	{
		/*
		 * if the desired link already exists, return without
		 * doing anything
		 */
		if (BIT_ISSET(tgtinfo->tgt_flags, TGT_DEST_ENCAP_SRC) &&
		    BIT_ISSET(tgtinfo->tgt_flags, TGT_DEST_PKGDIR_EXISTS) &&
		    strcmp(tgtinfo->tgt_link_existing_pkg,
			   encap->e_pkgname) == 0 &&
		    strcmp(tgtinfo->tgt_link_existing_pkgdir_relative,
			   srcinfo->src_link_expecting +
			     strlen(srcinfo->src_link_expecting) -
			     strlen(tgtinfo->tgt_link_existing_pkgdir_relative))
									== 0)
		{
			(*encap->e_print_func)(encap, srcinfo, tgtinfo,
					       EPT_INST_NOOP, NULL);
			return R_SKIP;
		}

		/*
		 * unless we're in force mode, detect non-resolvable
		 * conflicts and fail
		 */
		if (!BIT_ISSET(encap->e_options, OPT_FORCE))
		{
			/* make sure it's a symlink */
			if (!BIT_ISSET(tgtinfo->tgt_flags, TGT_ISLNK))
			{
				(*encap->e_print_func)(encap, srcinfo,
						       tgtinfo,
						       EPT_INST_FAIL,
						       "not a symlink");
				return R_ERR;
			}

			/* make sure it's an encap link */
			if (!BIT_ISSET(tgtinfo->tgt_flags, TGT_DEST_ENCAP_SRC))
			{
				(*encap->e_print_func)(encap, srcinfo,
						       tgtinfo,
						       EPT_INST_FAIL,
						       "not an Encap link");
				return R_ERR;
			}

			/*
			 * if the existing link points to an existing file
			 * in an existing package other than the one we're
			 * installing, it's a conflict
			 */
			if (BIT_ISSET(tgtinfo->tgt_flags, TGT_DEST_EXISTS) &&
			    BIT_ISSET(tgtinfo->tgt_flags,
				      TGT_DEST_PKGDIR_EXISTS) &&
			    strcmp(tgtinfo->tgt_link_existing_pkg,
				   encap->e_pkgname) != 0)
			{
				(*encap->e_print_func)(encap, srcinfo,
						tgtinfo, EPT_INST_FAIL,
						"conflicting link to "
						    "package %s",
						tgtinfo->tgt_link_existing_pkg);
				return R_ERR;
			}

		}
		else
			msg = "forced replacement";

		if (!BIT_ISSET(encap->e_options, OPT_SHOWONLY) &&
		    remove(srcinfo->src_target_path) != 0)
		{
			(*encap->e_print_func)(encap, srcinfo, tgtinfo,
					       EPT_INST_ERROR, "remove");
			return R_ERR;
		}

		type = EPT_INST_REPL;
	}

	if (!BIT_ISSET(encap->e_options, OPT_SHOWONLY) &&
	    symlink(srcinfo->src_link_expecting, srcinfo->src_target_path) != 0)
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo,
				       EPT_INST_ERROR, "symlink");
		return R_ERR;
	}
	else
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, type, msg);

	return R_FILEOK;
}


