/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  install.c - package installation mode for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <epkg.h>

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
# include <stdarg.h>
#else
# include <varargs.h>
#endif


int
override_decision(ENCAP *encap, encap_source_info_t *srcinfo,
		  encap_target_info_t *tgtinfo)
{
	encap_listptr_t lp;
	int i;

#ifdef DEBUG
	printf("==> override_decision(encap=0x%lx, srcinfo=\"%s\", "
	       "tgtinfo=\"%s\")\n", encap, srcinfo->src_pkgdir_relative,
	       tgtinfo->tgt_link_existing_pkgdir_relative);
#endif

	i = exclude_decision(encap, srcinfo, tgtinfo);
	if (i != R_FILEOK)
		return i;

	encap_listptr_reset(&lp);
	if (BIT_ISSET(tgtinfo->tgt_flags, TGT_DEST_ENCAP_SRC) &&
	    strcmp(encap->e_pkgname, tgtinfo->tgt_link_existing_pkg) != 0 &&
	    encap_list_search(override_l, &lp,
	    		      tgtinfo->tgt_link_existing_pkg, NULL) != 0)
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo, EPT_PKG_INFO,
				       "overriding link to package %s",
				       tgtinfo->tgt_link_existing_pkg);

		if (remove(srcinfo->src_target_path) == -1)
		{
			(*encap->e_print_func)(encap, srcinfo, tgtinfo,
					       EPT_INST_ERROR, "remove");
			return R_ERR;
		}

		if (encap_check_target(encap->e_source,
				       srcinfo->src_target_path, tgtinfo) == -1)
		{
			(*encap->e_print_func)(encap, srcinfo, tgtinfo,
					       EPT_INST_ERROR,
					       "encap_check_target");
			return R_ERR;
		}
	}

	return R_FILEOK;
}


/* install package */
int
install_pkg(char *pkg)
{
	ENCAP *encap;
	int status;

	if (verbose)
		printf("  > installing package %s\n", pkg);

	if (encap_open(&encap, source, target, pkg,
		       options, epkg_print) == -1)
	{
		printf("  ! cannot open package %s - installation aborted\n",
		       pkg);
		return -1;
	}
	status = encap_install(encap, override_decision);
	encap_close(encap);

	/* write logfile */
	if (status != ENCAP_STATUS_NOOP &&
	    !BIT_ISSET(options, OPT_SHOWONLY) &&
	    BIT_ISSET(epkg_opts, EPKG_OPT_WRITELOG))
		write_encap_log(pkg, MODE_INSTALL, status);

	switch (status)
	{
	case ENCAP_STATUS_FAILED:
		if (verbose)
			printf("    ! installation failed\n");
		break;
	case ENCAP_STATUS_SUCCESS:
		if (verbose)
			printf("    > installation successful\n");
		break;
	case ENCAP_STATUS_PARTIAL:
		if (verbose)
			printf("    > installation partially successful\n");
		break;
	case ENCAP_STATUS_NOOP:
		break;
	default:
		printf("    ! unknown status %d\n", status);
		return -1;
	}

	if (status == ENCAP_STATUS_FAILED ||
	    (status != ENCAP_STATUS_NOOP && BIT_ISSET(options, OPT_SHOWONLY)))
		return 1;

	return 0;
}


/* install mode */
int
install_mode(char *pkgspec)
{
	encap_list_t *ver_l;
	int i;
	char buf[MAXPATHLEN], name[MAXPATHLEN], ver[MAXPATHLEN];

	if (verbose)
		printf("epkg: installing package %s...\n", pkgspec);

	if (is_archive(pkgspec))
	{
		if (archive_extract(pkgspec, source,
				    (BIT_ISSET(options, OPT_FORCE)
				     ? ARCHIVE_OPT_FORCE
				     : 0)) != 0)
			return -1;
	}

	/*
	** remove any possible extra stuff from archive name
	** and put the raw pkgspec back into buf
	*/
	strlcpy(buf, basename(pkgspec), sizeof(buf));
	encap_pkgspec_parse(buf, name, sizeof(name), ver, sizeof(ver),
			    NULL, 0, NULL, 0);
	encap_pkgspec_join(buf, sizeof(buf), name, ver);

	/* if versioning is off, simply install the specified package */
	if (!BIT_ISSET(epkg_opts, EPKG_OPT_VERSIONING))
		return install_pkg(buf);

	if (verbose)
		printf("  > reading Encap source directory...\n");

	ver_l = encap_list_new(LIST_USERFUNC, (encap_cmpfunc_t)encap_vercmp);
	if (ver_l == NULL)
	{
		fprintf(stderr, "    ! encap_list_new(): %s\n",
			strerror(errno));
		return -1;
	}

	i = encap_find_versions(source, buf, version_list_add, ver_l);
	if (i == 0)
	{
		i = encap_find_versions(source, name, version_list_add, ver_l);
		if (i == 0)
		{
			fprintf(stderr, "    ! no versions of package %s "
				"found!\n", buf);
			return -1;
		}
	}
	else
	{
		strlcpy(name, buf, sizeof(name));
		ver[0] = '\0';
	}
	if (i == -2)
		return -1;
	if (i == -1)
	{
		fprintf(stderr, "    ! encap_find_versions(): %s\n",
			strerror(errno));
		return -1;
	}

	i = install_package_version(name, ver, ver_l);

	encap_list_free(ver_l, (encap_freefunc_t)free);

	return i;
}


