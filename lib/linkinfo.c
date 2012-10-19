/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  linkinfo.c - encap link analysis routines
**
**  Mark D. Roth <roth@feep.net>
*/

#include <internal.h>

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/* Check the status of a link. */
int
encap_check_target(char *encap_source, char *tgt_path,
		   encap_target_info_t *tgtinfo)
{
	struct stat s1, s2;
	int i, j;
	char buf[MAXPATHLEN];

	memset(tgtinfo, 0, sizeof(encap_target_info_t));

	/* does link exist? */
	if (lstat(tgt_path, &s1) == 0)
		BIT_SET(tgtinfo->tgt_flags, TGT_EXISTS);
	else if (errno == ENOENT)
		return 0;
	else
		return -1;

	/* is it really a link? */
	if (!S_ISLNK(s1.st_mode))
	{
		/* is it a directory? */
		if (S_ISDIR(s1.st_mode))
			BIT_SET(tgtinfo->tgt_flags, TGT_ISDIR);
		return 0;
	}
	BIT_SET(tgtinfo->tgt_flags, TGT_ISLNK);

	/* does the link's target exist? */
	if (stat(tgt_path, &s2) == 0)
	{
		BIT_SET(tgtinfo->tgt_flags, TGT_DEST_EXISTS);

		/* is the link's target a directory? */
		if (S_ISDIR(s2.st_mode))
			BIT_SET(tgtinfo->tgt_flags, TGT_DEST_ISDIR);
	}
	else if (errno != ENOENT)
		return -1;

	/* does the link point to or under the source dir? */
	get_link_dest(tgt_path, tgtinfo->tgt_link_existing,
		      sizeof(tgtinfo->tgt_link_existing));
	i = strlen(encap_source);
	if (strncmp(encap_source, tgtinfo->tgt_link_existing, i) != 0 ||
	    tgtinfo->tgt_link_existing[i] != '/')
		return 0;
	BIT_SET(tgtinfo->tgt_flags, TGT_DEST_ENCAP_SRC);

	/* since it's an encap link, set the return values */
	j = strcspn(tgtinfo->tgt_link_existing + i + 1, "/");
	strlcpy(buf, tgtinfo->tgt_link_existing,
		(size_t)(j + i + 2) < sizeof(buf) ? (j + i + 2) : sizeof(buf));
	strlcpy(tgtinfo->tgt_link_existing_pkg, basename(buf),
		sizeof(tgtinfo->tgt_link_existing_pkg));
	strlcpy(tgtinfo->tgt_link_existing_pkgdir_relative,
		tgtinfo->tgt_link_existing + j + i + 2,
		sizeof(tgtinfo->tgt_link_existing_pkgdir_relative));

	/* does the package directory pointed to by the link exist? */
	if (stat(buf, &s1) == 0)
		BIT_SET(tgtinfo->tgt_flags, TGT_DEST_PKGDIR_EXISTS);
	else if (errno != ENOENT)
		return -1;

	return 0;
}


/* check status of an Encap source file */
int
encap_check_source(ENCAP *encap, char *dir, char *file,
		   encap_source_info_t *srcinfo)
{
	encap_listptr_t lp;
	encap_hashptr_t hp;
	linkname_t *lnp;
	struct stat s;

#ifdef DEBUG
	printf("==> encap_check_source(encap=\"%s\", dir=\"%s\", file=\"%s\", "
	       "srcinfo=0x%lx)\n", encap->e_pkgname, dir ? dir : "NULL",
	       file ? file : "NULL", srcinfo);
#endif

	if (file == NULL)
		file = "";
	if (dir == NULL)
		dir = "";
	memset(srcinfo, 0, sizeof(encap_source_info_t));

	/* determine various paths */
	snprintf(srcinfo->src_pkgdir_relative,
		 sizeof(srcinfo->src_pkgdir_relative), "%s%s%s",
		 (dir[0] ? dir : ""), (dir[0] ? "/" : ""), file);
	snprintf(srcinfo->src_path, sizeof(srcinfo->src_path), "%s/%s/%s",
		 encap->e_source, encap->e_pkgname,
		 srcinfo->src_pkgdir_relative);
	if (BIT_ISSET(encap->e_options, OPT_LINKNAMES))
	{
		encap_hashptr_reset(&hp);
		if (encap_hash_getkey(encap->e_pkginfo.ei_ln_h, &hp,
				      srcinfo->src_pkgdir_relative, NULL) != 0)
			lnp = (linkname_t *)encap_hashptr_data(&hp);
		else
			lnp = NULL;
	}
	else
		lnp = NULL;
	srcinfo->src_target_relative[0] = '\0';
	if (dir[0] != '\0')
	{
		strlcat(srcinfo->src_target_relative, dir,
			sizeof(srcinfo->src_target_relative));
		strlcat(srcinfo->src_target_relative, "/",
			sizeof(srcinfo->src_target_relative));
	}
	strlcat(srcinfo->src_target_relative, (lnp ? lnp->ln_newname : file),
		sizeof(srcinfo->src_target_relative));
	snprintf(srcinfo->src_target_path, sizeof(srcinfo->src_target_path),
		 "%s/%s", encap->e_target, srcinfo->src_target_relative);
	if (BIT_ISSET(encap->e_options, OPT_ABSLINKS))
		strlcpy(srcinfo->src_link_expecting, srcinfo->src_path,
			sizeof(srcinfo->src_link_expecting));
	else
		encap_relativepath(dirname(srcinfo->src_target_path),
				   srcinfo->src_path,
				   srcinfo->src_link_expecting,
				   sizeof(srcinfo->src_link_expecting));

#ifdef DEBUG
	printf("    encap_check_source(): src_pkgdir_relative=\"%s\"\n",
	       srcinfo->src_pkgdir_relative);
	printf("    encap_check_source(): src_path=\"%s\"\n",
	       srcinfo->src_path);
	printf("    encap_check_source(): src_target_relative=\"%s\"\n",
	       srcinfo->src_target_relative);
	printf("    encap_check_source(): src_target_path=\"%s\"\n",
	       srcinfo->src_target_path);
	printf("    encap_check_source(): src_link_expecting=\"%s\"\n",
	       srcinfo->src_link_expecting);
#endif

	/* if the filename matches an exclude entry, ignore it */
	if (BIT_ISSET(encap->e_options, OPT_EXCLUDES))
	{
		encap_listptr_reset(&lp);
		if (encap_list_search(encap->e_pkginfo.ei_ex_l, &lp,
				      srcinfo->src_pkgdir_relative,
				      (encap_vercmp(encap->e_pkginfo.ei_pkgfmt,
						    "2.0") < 0
				       ? (encap_matchfunc_t)encap_str_match
				       : (encap_matchfunc_t)glob_match)) != 0)
			BIT_SET(srcinfo->src_flags, SRC_EXCLUDED);
	}

	/* set required if this is a required link */
	encap_listptr_reset(&lp);
	if (encap_list_search(encap->e_pkginfo.ei_rl_l, &lp,
			      srcinfo->src_pkgdir_relative,
			      (encap_matchfunc_t)glob_match) != 0)
		BIT_SET(srcinfo->src_flags, SRC_REQUIRED);

	/* set linkdir if this is on the linkdir list */
	if (BIT_ISSET(encap->e_options, OPT_LINKDIRS))
	{
		encap_listptr_reset(&lp);
		if (encap_list_search(encap->e_pkginfo.ei_ld_l, &lp,
				      srcinfo->src_pkgdir_relative,
				      (encap_matchfunc_t)glob_match) != 0)
			BIT_SET(srcinfo->src_flags, SRC_LINKDIR);
	}

	/*
	 * if this is a linkdir and a required file lives under it, set
	 * the required flag
	 * (if the linkdir fails, the required file under it also fails)
	 */
	if (BIT_ISSET(srcinfo->src_flags, SRC_LINKDIR) &&
	    !BIT_ISSET(srcinfo->src_flags, SRC_REQUIRED))
	{
		encap_listptr_reset(&lp);
		if (encap_list_search(encap->e_pkginfo.ei_rl_l, &lp,
				      srcinfo->src_pkgdir_relative,
				      (encap_matchfunc_t)partial_glob_match)
		    != 0)
			BIT_SET(srcinfo->src_flags, SRC_REQUIRED);
	}

	/* stat the file to see what it is */
	if (lstat(srcinfo->src_path, &s) != 0)
		return -1;
	if (S_ISDIR(s.st_mode))
		BIT_SET(srcinfo->src_flags, SRC_ISDIR);
	if (S_ISLNK(s.st_mode))
		BIT_SET(srcinfo->src_flags, SRC_ISLNK);

#ifdef DEBUG
	printf("    encap_check_source(): src_flags for \"%s\" == %d\n",
	       srcinfo->src_pkgdir_relative, srcinfo->src_flags);
#endif

	return 0;
}


