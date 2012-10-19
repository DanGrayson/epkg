/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  recursion.c - directory recursion code for epkg
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


/*
** core directory recursion.
*/
int
encap_recursion(ENCAP *encap, encap_source_info_t *dirinfo, int *status,
		int mode, encap_decision_func_t decision_func)
{
	DIR *d;
	struct dirent *di;
	encap_source_info_t srcinfo;
	encap_target_info_t tgtinfo;
	encap_action_func af;

#ifdef DEBUG
	printf("==> encap_recursion(encap=\"%s\", "
	       "dirinfo->src_pkgdir_relative=\"%s\", *status=%d, mode=0d)\n",
	       encap->e_pkgname, dirinfo->src_pkgdir_relative, *status, mode);
	printf("    encap_recursion(): dirinfo->src_path = \"%s\"\n",
	       dirinfo->src_path);
	printf("    encap_recursion(): dirinfo->src_target_path = \"%s\"\n",
	       dirinfo->src_target_path);
	printf("    encap_recursion(): dirinfo->src_target_relative = \"%s\"\n",
	       dirinfo->src_target_relative);
	printf("    encap_recursion(): dirinfo->src_link_expecting = \"%s\"\n",
	       dirinfo->src_link_expecting);
#endif

	if ((d = opendir(dirinfo->src_path)) == NULL)
	{
		(*encap->e_print_func)(encap, dirinfo, NULL, EPT_PKG_ERROR,
				       "opendir");
		BIT_SET(*status, ESTAT_ERR);
		return R_ERR;
	}

	/* iterate through each entry in the directory */
	while ((di = readdir(d)) != NULL)
	{
		if (strcmp(di->d_name, ".") == 0 ||
		    strcmp(di->d_name, "..") == 0)
			continue;

		if (dirinfo->src_pkgdir_relative[0] == '\0' &&
		    (strcmp(di->d_name, "preinstall") == 0 ||
		     strcmp(di->d_name, "postinstall") == 0 ||
		     strcmp(di->d_name, "preremove") == 0 ||
		     strcmp(di->d_name, "postremove") == 0))
			continue;

		if (encap_vercmp(encap->e_pkginfo.ei_pkgfmt, "2.0") < 0)
		{
			if (strcmp(di->d_name, "encap.exclude") == 0)
				continue;
		}
		else if (dirinfo->src_pkgdir_relative[0] == '\0' &&
			 strcmp(di->d_name, "encapinfo") == 0)
			continue;

		/* check Encap source file */
		if (encap_check_source(encap, dirinfo->src_pkgdir_relative,
				       di->d_name, &srcinfo) == -1)
		{
			(*encap->e_print_func)(encap, &srcinfo, &tgtinfo,
					       EPT_PKG_ERROR,
					       "encap_check_source");
			if (BIT_ISSET(srcinfo.src_flags, SRC_REQUIRED))
			{
				BIT_SET(*status, ESTAT_FATAL);
				closedir(d);
				return R_RETURN;
			}
			BIT_SET(*status, ESTAT_ERR);
			continue;
		}
		if (BIT_ISSET(srcinfo.src_flags, SRC_EXCLUDED))
		{
			(*encap->e_print_func)(encap, &srcinfo, &tgtinfo,
					       EPT_PKG_INFO, "excluding");
			continue;
		}

		/* don't process files in pkgdir unless specificly told to */
		if (!BIT_ISSET(encap->e_options, OPT_PKGDIRLINKS) &&
		    dirinfo->src_pkgdir_relative[0] == '\0' &&
		    !BIT_ISSET(srcinfo.src_flags, SRC_ISDIR))
		{
#if 0
			(*encap->e_print_func)(encap, &srcinfo, &tgtinfo,
					       EPT_PKG_INFO, "excluding");
#endif
			continue;
		}

		/* check Encap target link */
		if (encap_check_target(encap->e_source, srcinfo.src_target_path,
				       &tgtinfo) == -1)
		{
			(*encap->e_print_func)(encap, &srcinfo, &tgtinfo,
					       EPT_PKG_ERROR,
					       "encap_check_target");
			if (BIT_ISSET(srcinfo.src_flags, SRC_REQUIRED))
			{
				BIT_SET(*status, ESTAT_FATAL);
				closedir(d);
				return R_RETURN;
			}
			BIT_SET(*status, ESTAT_ERR);
			continue;
		}

		if (decision_func != NULL)
		{
			switch ((*decision_func)(encap, &srcinfo, &tgtinfo))
			{
			case R_ERR:
				if (BIT_ISSET(srcinfo.src_flags, SRC_REQUIRED))
				{
			case R_RETURN:
					BIT_SET(*status, ESTAT_FATAL);
					closedir(d);
					return R_RETURN;
				}
				BIT_SET(*status, ESTAT_ERR);
			case R_SKIP:
				continue;
			case R_FILEOK:
			default:
				break;
			}
		}

		/* normal links */
		if (!BIT_ISSET(srcinfo.src_flags, SRC_ISDIR) ||
		    BIT_ISSET(srcinfo.src_flags, SRC_LINKDIR))
		{
			if ((af = mode_info(mode)->file_func) != NULL)
			{
				switch ((*af)(encap, &srcinfo, &tgtinfo))
				{
				case R_ERR:
					/*
					 * if a linkdir failed, fall back to
					 * normal recursion
					 */
					if (BIT_ISSET(srcinfo.src_flags,
						      SRC_LINKDIR))
					{
						BIT_CLEAR(srcinfo.src_flags,
							  SRC_LINKDIR);
						(*encap->e_print_func)(encap,
						   NULL, NULL, EPT_PKG_INFO,
						   "%s: linkdir failed; "
						   "falling back to "
						   "recursion method",
						   srcinfo.src_target_relative);
						break;
					}
					if (BIT_ISSET(srcinfo.src_flags,
						      SRC_REQUIRED))
					{
				case R_RETURN:
						BIT_SET(*status, ESTAT_FATAL);
						closedir(d);
						return R_RETURN;
					}
					BIT_SET(*status, ESTAT_ERR);
					continue;
				case R_SKIP:
					BIT_SET(*status, ESTAT_NONEED);
					continue;
				case R_FILEOK:
					BIT_SET(*status, ESTAT_OK);
				default:
					continue;
				}
			}
		}

		/* directory recursion */
		if ((af = mode_info(mode)->predir_func) != NULL)
		{
			switch ((*af)(encap, &srcinfo, &tgtinfo))
			{
			case R_ERR:
				if (BIT_ISSET(srcinfo.src_flags, SRC_REQUIRED))
				{
			case R_RETURN:
					BIT_SET(*status, ESTAT_FATAL);
					closedir(d);
					return R_RETURN;
				}
				BIT_SET(*status, ESTAT_ERR);
				continue;
			case R_SKIP:
			case R_FILEOK:
			default:
				break;
			}
		}

		switch (encap_recursion(encap, &srcinfo, status,
					mode, decision_func))
		{
		case R_ERR:
			if (BIT_ISSET(srcinfo.src_flags, SRC_REQUIRED))
			{
				BIT_SET(*status, ESTAT_FATAL);
		case R_RETURN:
				closedir(d);
				return R_RETURN;
			}
		case R_SKIP:
			continue;
		case R_FILEOK:
		default:
			break;
		}

		if ((af = mode_info(mode)->postdir_func) != NULL)
		{
			switch ((*af)(encap, &srcinfo, &tgtinfo))
			{
			case R_ERR:
				if (BIT_ISSET(srcinfo.src_flags, SRC_REQUIRED))
				{
			case R_RETURN:
					BIT_SET(*status, ESTAT_FATAL);
					closedir(d);
					return R_RETURN;
				}
				BIT_SET(*status, ESTAT_ERR);
				break;
			case R_SKIP:
			case R_FILEOK:
			default:
				break;
			}
		}
	}

	closedir(d);
	return R_FILEOK;
}


