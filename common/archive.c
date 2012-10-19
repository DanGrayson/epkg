/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  archive.c - tar archive handling code for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <archive.h>

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

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

#ifdef HAVE_LIBTAR
# include <libtar.h>
#endif


#ifndef BIT_ISSET
# define BIT_ISSET(bitfield, opt) ((bitfield) & (opt))
#endif /* ! BIT_ISSET */



int archive_verbose = 0;


#ifdef HAVE_LIBTAR

static int
tar_add_files(TAR *tar, char *dir, unsigned long options)
{
	char realpath[MAXPATHLEN];
	struct dirent *dent;
	DIR *dp;
	struct stat s;
	static size_t parentlen = 0;

	if (parentlen == 0)
		parentlen = strlen(dirname(dir)) + 1;

	if (tar_append_file(tar, dir, dir + parentlen) != 0)
		return -1;

	if (BIT_ISSET(options, ARCHIVE_OPT_ENCAPINFO)
	    && strchr(dir + parentlen, '/') == NULL)
	{
		snprintf(realpath, sizeof(realpath), "%s/encapinfo", dir);
		if (tar_append_file(tar, realpath, realpath + parentlen) != 0)
			return -1;
	}

	dp = opendir(dir);
	if (dp == NULL)
	{
		fprintf(stderr, "!!! opendir(\"%s\"): %s\n", dir,
			strerror(errno));
		return -1;
	}

	while ((dent = readdir(dp)) != NULL)
	{
		if (strcmp(dent->d_name, ".") == 0 ||
		    strcmp(dent->d_name, "..") == 0)
			continue;

		if (BIT_ISSET(options, ARCHIVE_OPT_ENCAPINFO)
		    && strchr(dir + parentlen, '/') == NULL
		    && strcmp(dent->d_name, "encapinfo") == 0)
			continue;

		snprintf(realpath, sizeof(realpath), "%s/%s",
			 dir, dent->d_name);

		if (lstat(realpath, &s) != 0)
			return -1;

		if (S_ISDIR(s.st_mode))
		{
			if (tar_add_files(tar, realpath, options) != 0)
				return -1;
			continue;
		}

		if (tar_append_file(tar, realpath, realpath + parentlen) != 0)
			return -1;
	}

	closedir(dp);

	return 0;
}

#endif /* HAVE_LIBTAR */


/*
** archive_create() - create a tar archive
** returns:
**	0			success
**	-1 (and sets errno)	failure
*/
int
archive_create(char *archive, char *tree, unsigned long options)
{
	const compress_type_t *ctp;
	int i = 0;
	int fd_fromfilter, fd_tofilter;

#ifdef HAVE_LIBTAR
	TAR *tar;
	int tar_options;
#else
	char buf[1024];
	char command[4096];
	FILE *fp;
#endif

#ifdef DEBUG
	printf("==> archive_create(archive=\"%s\", tree=\"%s\", options=%d)\n",
	       archive, tree, options);
#endif

	if (archive_verbose)
		printf("> creating archive %s\n", archive);
	if (archive_verbose > 1)
		putchar('\n');

	fd_fromfilter = open(archive, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd_fromfilter == -1)
	{
		printf("!!! open(\"%s\"): %s\n", archive, strerror(errno));
		return -1;
	}

	ctp = compress_handler(archive);
	if (ctp->ct_openfunc != NULL)
	{
		fd_tofilter = (*(ctp->ct_openfunc))(fd_fromfilter, O_WRONLY,
						    ctp->ct_open_write_data);
		if (fd_tofilter == -1)
		{
			fprintf(stderr, "  ! cannot open compression "
				"handler for writing: %s\n", strerror(errno));
			return -1;
		}
	}
	else
		fd_tofilter = fd_fromfilter;

#ifdef HAVE_LIBTAR

	tar_options = TAR_GNU;
	if (archive_verbose > 1)
		tar_options |= TAR_VERBOSE;
	if (! BIT_ISSET(options, ARCHIVE_OPT_FORCE))
		tar_options |= TAR_NOOVERWRITE;

	if (tar_fdopen(&tar, fd_tofilter, "[dummy]",
		       ctp->ct_tartype, O_WRONLY | O_CREAT | O_EXCL, 0644,
		       tar_options) != 0)
	{
		printf("!!! tar_fdopen(): %s\n", strerror(errno));
		return -1;
	}

	if (tar_add_files(tar, tree, options) != 0)
	{
		printf("!!! tar_add_files(): %s\n", strerror(errno));
		tar_close(tar);
		return -1;
	}

	if (tar_append_eof(tar) != 0)
	{
		printf("!!! tar_append_eof(): %s\n", strerror(errno));
		tar_close(tar);
		return -1;
	}

	if (tar_close(tar) != 0)
	{
		printf("!!! tar_close(): %s\n", strerror(errno));
		return -1;
	}

#else /* ! HAVE_LIBTAR */

	snprintf(command, sizeof(command), "cd %s && tar -c%spf - %s",
		 dirname(tree), (archive_verbose ? "vv" : ""), basename(tree));
#ifdef DEBUG
	printf("    archive_create(): calling popen(\"%s\", \"r\")\n", command);
#endif
	fp = popen(command, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "!!! popen(): %s\n", strerror(errno));
		return -1;
	}

	while (fread(buf, sizeof(buf), 1, fp) != 0)
	{
		if (ctp->ct_tartype->writefunc(fd_tofilter, buf,
					       sizeof(buf)) == -1)
		{
			fprintf(stderr, "!!! writefunc(): %s\n",
				strerror(errno));
			return -1;
		}
	}
	if (ferror(fp))
	{
		fprintf(stderr, "!!! fread(): %s\n", strerror(errno));
		return -1;
	}

	if (ctp->ct_tartype->closefunc(fd_tofilter) == -1)
	{
		fprintf(stderr, "!!! tartype->closefunc(): %s\n",
			strerror(errno));
		return -1;
	}

	i = pclose(fp);
#if 0
	if (i > 127)
		i -= 127;
#else
	i = WEXITSTATUS(i);
#endif

#endif /* HAVE_LIBTAR */

	if (archive_verbose > 1)
		putchar('\n');

	return i;
}


/*
** is_archive() - determine whether a file is an archive (based on extension)
** returns:
**	1	file has a recognized archive extension
**	0	otherwise
*/
int
is_archive(char *filename)
{
	const compress_type_t *ctp;

	if (MATCH_EXTENSION(filename, ".tar"))
		return 1;

	ctp = compress_handler(filename);
	if (ctp != NULL && ctp->ct_extension != NULL)
		return 1;

	return 0;
}


/*
** archive_extract() - extract archive into destdir
** returns:
**	0			success
**	-1 (and sets errno)	failure
*/
int
archive_extract(char *archive, char *destdir, unsigned long options)
{
	int i, fd_download, fd_fromfilter;
	const compress_type_t *ctp;

#ifdef HAVE_LIBTAR
	TAR *tar;
	int tar_options;
#else
	char buf[1024];
	char command[4096];
	FILE *fp;
#endif

#ifdef DEBUG
	printf("==> archive_extract(archive=\"%s\", destdir=\"%s\")\n",
	       archive, destdir);
#endif

	fd_download = download_file(archive, NULL);
	if (fd_download == -1)
		return -1;

	if (archive_verbose)
		printf("  > extracting archive file %s into directory %s...\n",
		       basename(archive), destdir);

	ctp = compress_handler(archive);
	if (ctp->ct_openfunc != NULL)
	{
		fd_fromfilter = (*(ctp->ct_openfunc))(fd_download, O_RDONLY,
						      ctp->ct_open_read_data);
		if (fd_fromfilter == -1)
		{
			fprintf(stderr, "  ! cannot open compression "
				"handler for reading: %s\n", strerror(errno));
			return -1;
		}
	}
	else
		fd_fromfilter = fd_download;

#ifdef HAVE_LIBTAR

	tar_options = TAR_GNU|TAR_IGNORE_MAGIC;
	if (archive_verbose > 1)
		tar_options |= TAR_VERBOSE;
	if (! BIT_ISSET(options, ARCHIVE_OPT_FORCE))
		tar_options |= TAR_NOOVERWRITE;

	if (tar_fdopen(&tar, fd_fromfilter, "[dummy]",
		       ctp->ct_tartype, O_RDONLY, 0, tar_options) != 0)
	{
		printf("  ! tar_fdopen(): %s\n", strerror(errno));
		return -1;
	}

# ifdef DEBUG
	printf("    archive_extract(): tar_fdopen() succeeded, calling "
	       "tar_extract_all(TAR *tar, \"%s\")\n", destdir);
# endif
	i = tar_extract_all(tar, destdir);
	if (i == -1)
		printf("  ! tar_extract_all(): %s\n", strerror(errno));

# ifdef DEBUG
	printf("    archive_extract(): tar_extract_all() returned %d, "
	       "calling tar_close()\n", i);
# endif
	tar_close(tar);

#else /* ! HAVE_LIBTAR */

	snprintf(command, sizeof(command), "cd %s && tar x%spf -",
		 destdir, (archive_verbose > 1 ? "vv" : ""));
# ifdef DEBUG
	printf("    archive_extract(): calling popen(\"%s\", \"w\")\n",
	       command);
# endif
	fp = popen(command, "w");
	if (fp == NULL)
	{
		fprintf(stderr, "  ! popen(): %s\n", strerror(errno));
		return -1;
	}

	while ((i = ctp->ct_tartype->readfunc(fd_fromfilter, buf,
					      sizeof(buf))) > 0)
	{
		if (fwrite(buf, i, 1, fp) == 0 && ferror(fp))
		{
			fprintf(stderr, "  ! fwrite(): %s\n",
				strerror(errno));
			return -1;
		}
	}
	if (i == -1)
	{
		fprintf(stderr, "  ! tartype->readfunc(): %s\n",
			strerror(errno));
		return -1;
	}

	if (ctp->ct_tartype->closefunc(fd_fromfilter) == -1)
	{
		fprintf(stderr, "  ! tartype->closefunc(): %s\n",
			strerror(errno));
		return -1;
	}

	i = pclose(fp);
	if (i == -1)
	{
		fprintf(stderr, "  ! pclose(): %s\n", strerror(errno));
		return -1;
	}

# if 0
	if (i > 127)
		i -= 127;
# else
	i = WEXITSTATUS(i);
# endif

#endif /* HAVE_LIBTAR */

#ifdef DEBUG
	printf("<== archive_extract(): returning %d\n", i);
#endif
	return i;
}


