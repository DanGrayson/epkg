/*
**  Copyright 2002-2003 University of Illinois Board of Trustees
**  Copyright 2002-2004 Mark D. Roth
**  All rights reserved.
**
**  build.c - mkencap code for processing package profiles
**
**  Mark D. Roth <roth@feep.net>
*/

#include <mkencap.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif


#ifdef HAVE_LIBEXPAT

/********************************************************************
*****  utility functions
*********************************************************************/

static int
mkencap_chdir(char *dir)
{
	int i;
	char buf[MAXPATHLEN];

	/* don't do anything if we're already in the right place */
	getcwd(buf, sizeof(buf));
	if (strcmp(dir, buf) == 0)
		return 0;

	if (verbose)
		printf("mkencap: changing directory to \"%s\"\n", dir);

	i = chdir(dir);
	if (i != 0)
		fprintf(stderr, "mkencap: chdir(\"%s\"): %s\n",
			dir, strerror(errno));

	return i;
}


static void
mkencap_mkdirhier_print(void *state, char *dir)
{
	printf("mkencap: created directory \"%s\"\n", dir);
}


/********************************************************************
*****  URL checking functions
*********************************************************************/

static int
url_check(void *data, void *state)
{
	char *url = (char *)data;
	char *path = (char *)state;
	char buf[MAXPATHLEN];

	if (path[0] == '\0')
		snprintf(path, MAXPATHLEN, "%s/%s",
			 download_tree, basename(url));
	else
	{
		strlcpy(buf, basename(url), sizeof(buf));
		if (strcmp(buf, basename(path)) != 0)
		{
			fprintf(stderr, "mkencap: invalid profile: "
				"URLs point to different filenames "
				"(\"%s\" != \"%s\")\n",
				basename(path), buf);
			return -1;
		}
	}

	return 0;
}


static int
url_list_check(encap_list_t *url_l, char *localpath)
{
	localpath[0] = '\0';
	return encap_list_iterate(url_l, url_check, localpath);
}


/********************************************************************
*****  download functions
*********************************************************************/

struct dl_state
{
	char *path;			/* local file name */
	int state;			/* state variable */
};
typedef struct dl_state dl_state_t;

static int
url_download(void *data, void *state)
{
	char *url = (char *)data;
	dl_state_t *dlsp = (dl_state_t *)state;

	switch (dlsp->state)
	{
	case 1:
		/* already got it successfully */
		return 0;
	case -1:
		/* last attempt failed, try again */
		printf("mkencap: trying next URL...\n");
		/* FALLTHROUGH */
	case 0:
		/* first attempt, proceed normally */
		break;
	}

	if (download_file(url, dlsp->path) == -1)
	{
		fprintf(stderr, "mkencap: download failed\n");
		unlink(dlsp->path);
		dlsp->state = -1;
		return 0;
	}

	/* download succeeded */
	dlsp->state = 1;
	return 0;
}


static int
url_list_download(encap_list_t *url_l, char *localpath)
{
	dl_state_t dls;
	struct stat s;

	/* don't download again if it's already here */
	if (stat(localpath, &s) == 0)
	{
		if (! BIT_ISSET(mkencap_opts, MKENCAP_OPT_OVERWRITE))
		{
			if (verbose)
				printf("mkencap: source archive %s already "
				       "downloaded - not overwriting\n",
				       basename(localpath));
			return 0;
		}
	}
	else if (errno != ENOENT)
	{
		fprintf(stderr, "mkencap: stat(\"%s\"): %s\n",
			localpath, strerror(errno));
		return -1;
	}
	else
	{
		if (encap_mkdirhier(dirname(localpath),
				    mkencap_mkdirhier_print, NULL) == -1)
		{
			fprintf(stderr, "mkencap: mkdirhier(\"%s\"): %s\n",
				basename(localpath), strerror(errno));
			return -1;
		}
	}

	dls.path = localpath;
	dls.state = 0;
	encap_list_iterate(url_l, url_download, &dls);
	if (dls.state != 1)
	{
		fprintf(stderr, "mkencap: tried all URLs - download failed\n");
		return -1;
	}

	return 0;
}


/********************************************************************
*****  command handler
*********************************************************************/

static int
run_commands(char *label, char *cmds)
{
	char *start, *end, *cp;
	int backslash, status;

	printf("mkencap: executing %s commands...\n", label);

	start = cmds;
	while (*start != '\0')
	{
		end = start;
  find_end:
		end = strchr(end, '\n');
		if (end == NULL)
			end = start + strlen(start);
		else if (end > start && end[-1] == '\\')
		{
			backslash = 1;
			for (cp = end - 2; cp >= start && *cp == '\\'; cp--)
				backslash = !backslash;
			if (backslash)
			{
				end++;
				goto find_end;
			}
		}

		printf("\t%.*s\n", end - start, start);

		cp = malloc(end - start + 1);
		strlcpy(cp, start, end - start + 1);
		status = system(cp);
		free(cp);

		if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
		{
			fprintf(stderr,
				"mkencap: command failed (exit status %d)\n",
				WEXITSTATUS(status));
			return -1;
		}
		if (WIFSIGNALED(status))
		{
			fprintf(stderr,
				"mkencap: command terminated with signal %d\n",
				WTERMSIG(status));
			return -1;
		}

		start = end;
		if (*start != '\0')
			start++;
	}

	return 0;
}


static int
src_run_commands(void *data, void *state)
{
	profile_source_t *srcp;
	char *label, *cmds, *dir = NULL;

	srcp = (profile_source_t *)data;
	env_set_source(srcp);

	label = (char *)state;
	if (strcmp(label, "unpack") == 0)
	{
		cmds = srcp->ps_unpack_cmds;
		dir = dirname(srcp->ps_env_srcdir);
	}
	else if (strcmp(label, "configure") == 0)
		cmds = srcp->ps_configure_cmds;
	else if (strcmp(label, "build") == 0)
		cmds = srcp->ps_build_cmds;
	else if (strcmp(label, "install") == 0)
		cmds = srcp->ps_install_cmds;
	else if (strcmp(label, "clean") == 0)
		cmds = srcp->ps_clean_cmds;
	else
		return -1;

	if (cmds == NULL)
		return 0;

	if (dir == NULL)
		dir = srcp->ps_env_builddir;

	if (encap_mkdirhier(dir, mkencap_mkdirhier_print, NULL) == -1)
	{
		fprintf(stderr, "mkencap: mkdirhier(\"%s\"): %s\n",
			dir, strerror(errno));
		return -1;
	}
	if (mkencap_chdir(dir) == -1)
		return -1;

	return run_commands(label, cmds);
}


/********************************************************************
*****  patch handler
*********************************************************************/

struct patch_info
{
	int modes;
	encap_list_t *include_l;
};
typedef struct patch_info patch_info_t;

struct patch_state
{
	char *dir;
	patch_info_t *pip;
};
typedef struct patch_state patch_state_t;


/* FIXME: this duplicates some code from common/download.c */
static int
open_tmpfile(void)
{
	char *cp;
	char path[MAXPATHLEN];
	int fd;

	cp = getenv("TMPDIR");
	if (cp == NULL)
		cp = "/tmp";
	snprintf(path, sizeof(path), "%s/epkg-XXXXXX", cp);

	fd = mkstemp(path);
	if (fd == -1)
	{
		fprintf(stderr, "  ! cannot open temp file: "
			"mkstemp(\"%s\"): %s\n",
			path, strerror(errno));
		return -1;
	}

	unlink(path);

	return fd;
}


static int
apply_patch(void *data, void *state)
{
	profile_patch_t *ppp = (profile_patch_t *)data;
	patch_state_t *psp = (patch_state_t *)state;
	char command[1024], buf[1024];
	char subdir[MAXPATHLEN], patchfile[MAXPATHLEN];
	size_t sz;
	int status, i, fd_output, fd_tmpfile, fd_from_filter;
	const compress_type_t *ctp;

	if (ppp->pp_url_l != NULL)
	{
		/* validate URL list and determine download filename */
		if (url_list_check(ppp->pp_url_l, patchfile) == -1)
			return -1;

		/* save filename for later inclusion in package directory */
		encap_list_add(psp->pip->include_l, strdup(patchfile));

		/* download patch if needed */
		if (BIT_ISSET(psp->pip->modes, MKENCAP_PROFILE_DOWNLOAD)
		    && url_list_download(ppp->pp_url_l, patchfile) == -1)
			return -1;
	}

	if (! BIT_ISSET(psp->pip->modes, MKENCAP_PROFILE_PATCH))
		return 0;

	if (ppp->pp_url_l != NULL)
	{
		ppp->pp_fd = open(patchfile, O_RDONLY);
		if (ppp->pp_fd == -1)
		{
			fprintf(stderr, "mkencap: open(\"%s\"): %s\n",
				patchfile, strerror(errno));
			return -1;
		}

		ctp = compress_handler(patchfile);
		if (ctp->ct_openfunc != NULL)
		{
			fd_from_filter = ctp->ct_openfunc(ppp->pp_fd,
							O_RDONLY,
							ctp->ct_open_read_data);
			if (fd_from_filter == -1)
			{
				fprintf(stderr,
					"mkencap: cannot open compression "
					"handler for reading: %s\n",
					strerror(errno));
				return -1;
			}

			fd_tmpfile = open_tmpfile();
			if (fd_tmpfile == -1)
				return -1;

			while ((i = ctp->ct_tartype->readfunc(fd_from_filter,
							      buf,
							      sizeof(buf))) > 0)
			{
				if (write(fd_tmpfile, buf, i) == -1)
				{
					fprintf(stderr,
						"mkencap: write(): %s\n",
						strerror(errno));
					close(fd_tmpfile);
					return -1;
				}
			}
			if (i == -1)
			{
				fprintf(stderr,
					"mkencap: ctp->readfunc(): %s\n",
					strerror(errno));
				close(fd_tmpfile);
				return -1;
			}

			if (ctp->ct_tartype->closefunc(fd_from_filter) == -1)
			{
				fprintf(stderr,
					"mkencap: ctp->closefunc(): %s\n",
					strerror(errno));
				return -1;
			}

			if (lseek(fd_tmpfile, 0, SEEK_SET) == (off_t)-1)
			{
				fprintf(stderr, "mkencap: lseek(): %s\n",
					strerror(errno));
				close(fd_tmpfile);
				return -1;
			}

			ppp->pp_fd = fd_tmpfile;
		}
	}

	strlcpy(subdir, psp->dir, sizeof(subdir));
	if (ppp->pp_from_dir != NULL)
	{
		strlcat(subdir, "/", sizeof(subdir));
		strlcat(subdir, ppp->pp_from_dir, sizeof(subdir));
	}
	if (mkencap_chdir(subdir) != 0)
		return -1;

	if (ppp->pp_url_l != NULL)
		printf("mkencap: applying patch %s...\n", basename(patchfile));
	else
		puts("mkencap: applying patch from profile...");

	snprintf(command, sizeof(command), "%s %s",
		 getenv("PATCH"),
		 (ppp->pp_opts != NULL
		  ? ppp->pp_opts
		  : "-p1"));
	printf("\t%s\n", command);
	fd_output = pipe_open(ppp->pp_fd, O_RDONLY, command);
	if (fd_output == -1)
	{
		fprintf(stderr, "mkencap: pipe_open(\"%s\"): %s\n",
			command, strerror(errno));
		return -1;
	}

	while ((sz = read(fd_output, buf, sizeof(buf))) > 0)
		write(fileno(stdout), buf, sz);

	if (sz == (size_t)-1)
	{
		fprintf(stderr, "mkencap: read(): %s\n", strerror(errno));
		return -1;
	}

	/* check exit status to handle patch failures */
	do
	{
		if (wait(&status) == (pid_t)-1)
		{
			fprintf(stderr, "mkencap: wait(): %s\n",
				strerror(errno));
			return -1;
		}
	}
	while (!WIFEXITED(status) && !WIFSIGNALED(status));
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
	{
		fprintf(stderr, "mkencap: patch failed with exit code %d\n",
			WEXITSTATUS(status));
		return -1;
	}
	if (WIFSIGNALED(status))
	{
		fprintf(stderr, "mkencap: patch died with signal %d\n",
			WTERMSIG(status));
		return -1;
	}

	return close(fd_output);
}


static int
src_process_patches(void *data, void *state)
{
	profile_source_t *srcp = (profile_source_t *)data;
	patch_info_t *pip = (patch_info_t *)state;
	patch_state_t ps;

	if (srcp->ps_patches_l == NULL)
		return 0;

	env_set_source(srcp);

	ps.dir = srcp->ps_env_srcdir;
	ps.pip = pip;

	return encap_list_iterate(srcp->ps_patches_l, apply_patch, &ps);
}


/********************************************************************
*****  build-time prerequisite checking
*********************************************************************/

static int
check_prereqs(void *data, void *state)
{
	encap_profile_t *epp;
	profile_prereq_t *ppp;
	profile_environment_t *pep;
	char buf[MAXPATHLEN], pkgdir[MAXPATHLEN];
	struct stat s;

	ppp = (profile_prereq_t *)data;
	epp = (encap_profile_t *)state;

	snprintf(pkgdir, sizeof(buf), "%s/%s", source, ppp->pp_pkgspec);
	if (stat(pkgdir, &s) == -1)
	{
		if (errno == ENOENT)
		{
			fprintf(stderr, "mkencap: build-time "
				"prerequisite %s not found\n", pkgdir);
			return -1;
		}
		fprintf(stderr, "mkencap: stat(\"%s\"): %s\n",
			pkgdir, strerror(errno));
		return -1;
	}
	if (!S_ISDIR(s.st_mode))
	{
		fprintf(stderr, "mkencap: build-time prerequisite "
			"%s not found\n", pkgdir);
		return -1;
	}

	/* add environment changes */
	if (ppp->pp_use_bin == 1)
	{
		pep = (profile_environment_t *)calloc(1, sizeof(profile_environment_t));

		pep->pe_type = PROF_TYPE_PREPEND;
		pep->pe_variable = strdup("PATH");
		snprintf(buf, sizeof(buf), "%s/bin:", pkgdir);
		pep->pe_value = strdup(buf);

		if (epp->ep_env_vars_l == NULL)
			epp->ep_env_vars_l = encap_list_new(LIST_QUEUE, NULL);

		encap_list_add(epp->ep_env_vars_l, pep);
	}
	if (ppp->pp_use_lib == 1)
	{
		if (epp->ep_env_vars_l == NULL)
			epp->ep_env_vars_l = encap_list_new(LIST_QUEUE, NULL);

		pep = (profile_environment_t *)calloc(1, sizeof(profile_environment_t));

		pep->pe_type = PROF_TYPE_APPEND;
		pep->pe_variable = strdup("CPPFLAGS");
		snprintf(buf, sizeof(buf), " -I%s/include", pkgdir);
		pep->pe_value = strdup(buf);

		encap_list_add(epp->ep_env_vars_l, pep);

		pep = (profile_environment_t *)calloc(1, sizeof(profile_environment_t));

		pep->pe_type = PROF_TYPE_APPEND;
		pep->pe_variable = strdup("LDFLAGS");
		snprintf(buf, sizeof(buf), " -L%s/lib", pkgdir);
		pep->pe_value = strdup(buf);

		encap_list_add(epp->ep_env_vars_l, pep);
	}

	return 0;
}


/********************************************************************
*****  per-source initialization
*********************************************************************/

/* FIXME: integrate this into the compression and archive modules? */
static char *extensions[] = {
	".tar",
	".tgz",
	".tar.gz",
	".tar.Z",
	".tar.bz2",
	NULL
};

static int
src_init(void *data, void *state)
{
	encap_profile_t *epp;
	profile_source_t *srcp;
	char buf[MAXPATHLEN], *cp;
	int i;
	encap_listptr_t lp;

	epp = (encap_profile_t *)state;
	srcp = (profile_source_t *)data;

	/*
	** set archive name (use basename of first URL)
	*/
	encap_listptr_reset(&lp);
	encap_list_next(srcp->ps_url_l, &lp);
	cp = (char *)encap_listptr_data(&lp);
	strlcpy(srcp->ps_archive_name, basename(cp),
		sizeof(srcp->ps_archive_name));

	/*
	** set subdir name (archive name without extension)
	*/
	if (srcp->ps_subdir[0] == '\0')
	{
		strlcpy(buf, srcp->ps_archive_name, sizeof(buf));
		for (i = 0; extensions[i] != NULL; i++)
		{
			if (MATCH_EXTENSION(buf, extensions[i]))
			{
				buf[strlen(buf) - strlen(extensions[i])] = '\0';
				break;
			}
		}
		strlcpy(srcp->ps_subdir, buf, sizeof(srcp->ps_subdir));
	}

	/*
	** set absolute path to build files:
	**   - start with global build tree
	**   - add pkgspec subdir
	**   - if more than one source or source subdir does not match
	**     pkgspec, add source-specific subdir
	*/
	snprintf(srcp->ps_env_builddir, sizeof(srcp->ps_env_builddir),
		 "%s/%s", build_tree, epp->ep_pkgspec);
	if (encap_list_nents(epp->ep_sources_l) > 1
	    || strcmp(srcp->ps_subdir, epp->ep_pkgspec) != 0)
	{
		strlcat(srcp->ps_env_builddir, "/",
			sizeof(srcp->ps_env_builddir));
		strlcat(srcp->ps_env_builddir, srcp->ps_subdir,
			sizeof(srcp->ps_env_builddir));
	}

	/*
	** set absolute path to source files:
	**   - if common_src_tree is set and use_objdir is enabled
	**     for this source:
	**       - start with common_src_tree
	**       - add pkgspec subdir
	**       - if more than one source or source subdir does not match
	**         pkgspec, add source-specific subdir
	**   - otherwise, copy build path (calculated above)
	*/
	if (common_src_tree[0] != '\0' && srcp->ps_use_objdir)
	{
		snprintf(srcp->ps_env_srcdir, sizeof(srcp->ps_env_srcdir),
			 "%s/%s", common_src_tree, epp->ep_pkgspec);
		if (encap_list_nents(epp->ep_sources_l) > 1
		    || strcmp(srcp->ps_subdir, epp->ep_pkgspec) != 0)
		{
			strlcat(srcp->ps_env_srcdir, "/",
				sizeof(srcp->ps_env_srcdir));
			strlcat(srcp->ps_env_srcdir, srcp->ps_subdir,
				sizeof(srcp->ps_env_srcdir));
		}
	}
	else
		strlcpy(srcp->ps_env_srcdir, srcp->ps_env_builddir,
			sizeof(srcp->ps_env_srcdir));

	/*
	** finish path to build files:
	**   - add source-specific build subdir, if set
	*/
	if (srcp->ps_build_subdir[0] != '\0')
	{
		strlcat(srcp->ps_env_builddir, "/",
			sizeof(srcp->ps_env_builddir));
		strlcat(srcp->ps_env_builddir, srcp->ps_build_subdir,
			sizeof(srcp->ps_env_builddir));
	}

	return 0;
}


/********************************************************************
*****  download function
*********************************************************************/

static int
src_download(void *data, void *dummy)
{
	profile_source_t *srcp = (profile_source_t *)data;
	char localpath[MAXPATHLEN];

	if (url_list_check(srcp->ps_url_l, localpath) == -1)
		return -1;

	return url_list_download(srcp->ps_url_l, localpath);
}


/********************************************************************
*****  unpack function
*********************************************************************/

static int
src_unpack(void *data, void *dummy)
{
	profile_source_t *srcp;
	char buf[MAXPATHLEN];
	char srcdir[MAXPATHLEN];
	struct stat s;

	srcp = (profile_source_t *)data;
	env_set_source(srcp);

	strlcpy(srcdir, srcp->ps_env_srcdir, sizeof(srcdir));

	if (stat(srcdir, &s) == -1)
	{
		if (errno != ENOENT)
		{
			fprintf(stderr, "mkencap: stat(\"%s\"): %s\n",
				srcdir, strerror(errno));
			return -1;
		}
	}
	else if (BIT_ISSET(mkencap_opts, MKENCAP_OPT_OVERWRITE))
	{
		if (verbose)
			puts("mkencap: removing existing source tree...");

		if (S_ISDIR(s.st_mode))
		{
			if (encap_rmtree(srcdir, NULL, NULL) == -1)
			{
				fprintf(stderr,
					"mkencap: encap_rmtree(\"%s\"): %s\n", 
					srcdir, strerror(errno));
				return -1;
			}
		}
		else if (unlink(srcdir) == -1)
		{
			fprintf(stderr, "mkencap: unlink(\"%s\"): %s\n",
				srcdir, strerror(errno));
			return -1;
		}
	}
	else if (S_ISDIR(s.st_mode))
	{
		if (verbose)
			printf("mkencap: source archive %s already unpacked "
			       "- not overwriting\n",
			       srcp->ps_archive_name);
		return 0;
	}
	else
	{
		fprintf(stderr,
			"mkencap: cannot extract source archive: %s: %s\n",
			srcdir, strerror(EEXIST));
		return -1;
	}

	if (srcp->ps_create_subdir != 1)
		srcdir[strlen(srcdir) - strlen(srcp->ps_subdir) - 1] = '\0';
	if (encap_mkdirhier(srcdir, mkencap_mkdirhier_print, NULL) == -1)
	{
		fprintf(stderr, "mkencap: mkdirhier(\"%s\"): "
			"%s\n", srcdir, strerror(errno));
		return -1;
	}

	/* allow unpack commands to override native code */
	if (srcp->ps_unpack_cmds != NULL)
		return src_run_commands(srcp, "unpack");

	snprintf(buf, sizeof(buf), "%s/%s",
		 download_tree, srcp->ps_archive_name);
	return archive_extract(buf, srcdir, ARCHIVE_OPT_FORCE);
}


#endif /* HAVE_LIBEXPAT */


/********************************************************************
*****  package profile processing
*********************************************************************/

int
build_package(encap_profile_t *epp, unsigned long modes, char *profile,
	      char *env_file)
{
#ifndef HAVE_LIBEXPAT
	fprintf(stderr, "mkencap: profile code disabled\n");
	return -1;
#else /* HAVE_LIBEXPAT */
	char errbuf[1024];
	char buf[MAXPATHLEN];
	char *thisp, *nextp;
	patch_info_t pi;

# ifdef DEBUG
	printf("==> build_package()\n");
# endif

	if (verbose && epp->ep_notes != NULL)
	{
		printf("mkencap: displaying profile notes...\n%s\n",
		       epp->ep_notes);
	}

	pi.modes = modes;
	pi.include_l = encap_list_new(LIST_USERFUNC, NULL);
	encap_list_add(pi.include_l, strdup(profile));

	if (epp->ep_prereqs_l != NULL
	    && encap_list_iterate(epp->ep_prereqs_l, check_prereqs, epp) == -1)
		return -1;

	env_init(epp->ep_pkgspec, epp->ep_env_vars_l, env_file);

	if (epp->ep_prepare_cmds != NULL
	    && run_commands("prepare", epp->ep_prepare_cmds) == -1)
		return -1;

	if (epp->ep_sources_l != NULL)
	{
		if (encap_list_iterate(epp->ep_sources_l,
				       src_init,
				       epp) == -1)
			return -1;

		if (BIT_ISSET(modes, MKENCAP_PROFILE_DOWNLOAD)
		    && encap_list_iterate(epp->ep_sources_l,
					  src_download,
					  epp) == -1)
			return -1;

		if (BIT_ISSET(modes, MKENCAP_PROFILE_UNPACK)
		    && encap_list_iterate(epp->ep_sources_l,
					  src_unpack,
					  NULL) == -1)
			return -1;

		if (encap_list_iterate(epp->ep_sources_l,
				       src_process_patches, &pi) == -1)
			return -1;

		if (BIT_ISSET(modes, MKENCAP_PROFILE_CONFIGURE)
		    && encap_list_iterate(epp->ep_sources_l,
					  src_run_commands,
					  "configure") == -1)
			return -1;

		if (BIT_ISSET(modes, MKENCAP_PROFILE_BUILD)
		    && encap_list_iterate(epp->ep_sources_l,
					  src_run_commands,
					  "build") == -1)
			return -1;

		if (BIT_ISSET(modes, MKENCAP_PROFILE_INSTALL)
		    && encap_list_iterate(epp->ep_sources_l,
					  src_run_commands,
					  "install") == -1)
			return -1;
	}

	if (BIT_ISSET(modes, MKENCAP_PROFILE_INSTALL))
	{
		snprintf(buf, sizeof(buf), "%s/%s", source, epp->ep_pkgspec);
		if (mkencap_chdir(buf) == -1)
			return -1;

		if (epp->ep_prepackage_cmds != NULL
		    && run_commands("prepackage",
				    epp->ep_prepackage_cmds) == -1)
			return -1;

		if (epp->ep_include_files_l != NULL
		    && encap_list_iterate(epp->ep_include_files_l,
					  create_profile_include_files,
					  NULL) == -1)
			return -1;

		/* add profile and patch files to pkgdir */
		if (encap_list_iterate(pi.include_l, pkgdir_file_plugin,
				       NULL) == -1)
			return -1;

		/* parse encapinfo directives and add to pkginfo */
		nextp = epp->ep_encapinfo;
		while ((thisp = strsep(&nextp, "\n")) != NULL)
		{
			/* skip lines that are empty or all whitespace */
			thisp += strspn(thisp, " \t");
			if (*thisp == '\0')
				continue;

			if (encapinfo_parse_directive(thisp, &pkginfo,
						      errbuf,
						      sizeof(errbuf)) == -1)
			{
				fprintf(stderr, "mkencap: %s: %s\n",
					errbuf, thisp);
				return -1;
			}
		}

		/* create encapinfo file */
		if (mk_encapinfo(buf) == -1)
			return -1;
	}

	if (epp->ep_sources_l != NULL
	    && BIT_ISSET(modes, MKENCAP_PROFILE_TIDY)
	    && encap_list_iterate(epp->ep_sources_l, src_run_commands,
				  "clean") == -1)
		return -1;

	env_restore();
	return 0;
#endif /* HAVE_LIBEXPAT */
}


