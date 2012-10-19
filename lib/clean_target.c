/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  clean_target.c - target-cleaning functions for libencap
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/* this does all the work for cleaning the target
   returns 0 on success, -1 on error */
static int
recursive_clean(char *source, unsigned int options,
		encap_print_func_t print_func,
		encap_decision_func_t decision_func,
		size_t targetlen,
		encap_source_info_t *dirsrcinfo,
		encap_target_info_t *dirtgtinfo, encap_list_t *ex_l)
{
	DIR *d;
	struct dirent *di;
	encap_listptr_t lp;
	int retval = 0;
	encap_target_info_t tgtinfo;
	encap_source_info_t srcinfo;
	ENCAP encap;

#ifdef DEBUG
	printf("==> recursive_clean(source=\"%s\", options=%d, "
	       "print_func=0x%lx, targetlen=%lu, dirsrcinfo=\"%s\", "
	       "dirtgtinfo=\"%s\", ex_l=0x%lx)\n",
	       source, options, print_func, (unsigned long)targetlen,
	       dirsrcinfo->src_target_path, dirtgtinfo->tgt_link_existing,
	       ex_l);
#endif

	/* initialize for call to decision_func */
	memset(&encap, 0, sizeof(encap));
	encap.e_print_func = print_func;

	/* read encap.exclude file where applicable */
	if (BIT_ISSET(options, OPT_TARGETEXCLUDES) &&
	    encap_read_exclude_file(dirsrcinfo->src_target_path,
				    dirsrcinfo->src_target_relative, ex_l) != 0)
		(*print_func)(NULL, dirsrcinfo, dirtgtinfo, EPT_CLN_INFO,
			      "read %s/encap.exclude",
			      dirsrcinfo->src_target_path);

	if ((d = opendir(dirsrcinfo->src_target_path)) == NULL)
	{
		(*print_func)(NULL, dirsrcinfo, dirtgtinfo, EPT_CLN_ERROR,
			      "opendir");
		return -1;
	}

	while ((di = readdir(d)) != NULL)
	{
		if (strcmp(di->d_name, ".") == 0 ||
		    strcmp(di->d_name, "..") == 0)
			continue;

		memset(&srcinfo, 0, sizeof(srcinfo));
		snprintf(srcinfo.src_target_path,
		         sizeof(srcinfo.src_target_path), "%s/%s",
			 dirsrcinfo->src_target_path, di->d_name);
		strlcpy(srcinfo.src_target_relative,
			srcinfo.src_target_path + targetlen,
			sizeof(srcinfo.src_target_relative));

		if (encap_check_target(source, srcinfo.src_target_path,
				       &tgtinfo) == -1)
		{
			(*print_func)(NULL, &srcinfo, NULL, EPT_CLN_ERROR,
				      "encap_check_target");
			retval = -1;
			break;
		}

		/* if the filename matches an exclude entry, ignore it */
		encap_listptr_reset(&lp);
		if (BIT_ISSET(options, OPT_TARGETEXCLUDES) &&
		    encap_list_search(ex_l, &lp, srcinfo.src_target_relative,
				      NULL))
		{
			(*print_func)(NULL, &srcinfo, &tgtinfo, EPT_CLN_INFO,
				      "excluding");
			continue;
		}

		if (decision_func)
		{
			switch ((*decision_func)(&encap, &srcinfo, &tgtinfo))
			{
			case R_SKIP:
				continue;
			case R_RETURN:
			case R_ERR:
				retval = -1;
			case R_FILEOK:
			default:
				break;
			}
		}

		if (BIT_ISSET(tgtinfo.tgt_flags, TGT_ISLNK))
		{
			/* if it's not one of our links, don't mess with it */
			if (!BIT_ISSET(tgtinfo.tgt_flags, TGT_DEST_ENCAP_SRC))
			{
				(*print_func)(NULL, &srcinfo, &tgtinfo,
					      EPT_CLN_FAIL,
					      "not an Encap link");
				continue;
			}

			/* if it's a valid link, skip it */
			if (BIT_ISSET(tgtinfo.tgt_flags, TGT_DEST_EXISTS))
			{
				(*print_func)(NULL, &srcinfo, &tgtinfo,
					      EPT_CLN_NOOP, NULL);
				continue;
			}

			if (!BIT_ISSET(options, OPT_SHOWONLY) &&
			    unlink(srcinfo.src_target_path) != 0)
			{
				(*print_func)(NULL, &srcinfo, &tgtinfo,
					      EPT_CLN_ERROR, "unlink");
				retval = -1;
				break;
			}
			(*print_func)(NULL, &srcinfo, &tgtinfo, EPT_CLN_OK,
				      NULL);

			continue;
		}

		if (BIT_ISSET(tgtinfo.tgt_flags, TGT_ISDIR) &&
		    recursive_clean(source, options, print_func,
				    decision_func, targetlen, &srcinfo,
				    &tgtinfo, ex_l) != 0)
		{
			retval = -1;
			break;
		}
	}

	closedir(d);

	if (retval)
		return retval;

#ifdef DEBUG
	printf("    recursive_clean(): dirsrcinfo=\"%s\"\n",
	       dirsrcinfo->src_target_path);
#endif

	/* if the target directory is empty, blow it away */
	if (!BIT_ISSET(options, OPT_SHOWONLY) &&
	    BIT_ISSET(options, OPT_NUKETARGETDIRS))
	{
		if (rmdir(dirsrcinfo->src_target_path) != 0)
		{
			if (errno != EEXIST && errno != EBUSY &&
			    errno != ENOTEMPTY && errno != ENOENT)
			{
				(*print_func)(NULL, dirsrcinfo, dirtgtinfo,
					      EPT_CLN_ERROR, "rmdir");
				return -1;
			}
		}
		else
			(*print_func)(NULL, dirsrcinfo, dirtgtinfo,
				      EPT_CLN_OK, NULL);
	}

	return 0;
}


/*
** clean stale links out of target tree.
** returns:
**	0				success
**	-1 (and sets errno)		failure
*/
int
encap_target_clean(char *target, char *source, unsigned int options,
		   encap_print_func_t print_func,
		   encap_decision_func_t decision_func)
{
	encap_list_t *ex_l;
	int i;
	size_t targetlen = 0;
	encap_target_info_t tgtinfo;
	encap_source_info_t srcinfo;

	targetlen = strlen(target) + 1;
	ex_l = encap_list_new(LIST_USERFUNC, NULL);

	memset(&srcinfo, 0, sizeof(srcinfo));
	strlcpy(srcinfo.src_target_path, target,
		sizeof(srcinfo.src_target_path));

	if (encap_check_target(source, target, &tgtinfo) == -1)
	{
		(*print_func)(NULL, NULL, NULL, EPT_CLN_ERROR,
			      "encap_check_target(\"%s\")", target);
		return -1;
	}

	i = recursive_clean(source, options, print_func, decision_func,
			    targetlen, &srcinfo, &tgtinfo, ex_l);

	encap_list_free(ex_l, free);

	return i;
}


