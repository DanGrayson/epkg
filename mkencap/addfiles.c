/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  addfiles.c - code to add files to a package for mkencap
**
**  Mark D. Roth <roth@feep.net>
*/

#include <mkencap.h>

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/*
** mkencap_open_outfile() - utility function to open file for writing
** returns:
**	file descriptor		success
**	-1 (and sets errno)	failure
*/
static int
mkencap_open_outfile(char *file)
{
	int fd, omode;

	if (BIT_ISSET(mkencap_opts, MKENCAP_OPT_OVERWRITE))
		omode = O_TRUNC;
	else
		omode = O_EXCL;

	fd = open(file, O_WRONLY|O_CREAT|omode, 0666);
	if (fd == -1)
	{
		fprintf(stderr, "mkencap: open(\"%s\"): %s\n",
			file, strerror(errno));
		return -1;
	}

	return fd;
}


/*
** mkencap_create_file() - create new file with given text and permissions
** returns:
**	0			success
**	-1			failure
*/
static int
mkencap_create_file(char *file, char *text,
		    mode_t mode, uid_t uid, gid_t gid)
{
	int fd;

	if (mode == (mode_t)-1)
		mode = 0644;
	if (uid == (uid_t)-1)
		uid = 0;
	if (gid == (gid_t)-1)
		gid = 0;

	fd = mkencap_open_outfile(file);
	if (fd == -1)
		return -1;

	if (write(fd, text, strlen(text)) == -1)
	{
		fprintf(stderr, "mkencap: write: %s\n", strerror(errno));
		return -1;
	}

	if (close(fd) == -1)
	{
		fprintf(stderr, "mkencap: close: %s\n", strerror(errno));
		return -1;
	}

	if (chmod(file, mode) == -1)
	{
		fprintf(stderr, "mkencap: chmod(\"%s\"): %s\n",
			file, strerror(errno));
		return -1;
	}

	if (chown(file, uid, gid) == -1)
	{
		fprintf(stderr, "mkencap: chown(\"%s\"): %s\n",
			file, strerror(errno));
		return -1;
	}

	return 0;
}


/*
** create_profile_include_files() - list plugin to create package
**                                  profile <include_file> files
** returns:
**	0			success
**	-1			failure
*/
int
create_profile_include_files(void *data, void *dummy)
{
	profile_include_file_t *ifp;

	ifp = (profile_include_file_t *)data;

	if (verbose)
		printf("mkencap: creating file \"%s\"\n", ifp->pi_filename);

	return mkencap_create_file(ifp->pi_filename, ifp->pi_text,
				   ifp->pi_mode, ifp->pi_uid, ifp->pi_gid);
}


/*
** mkencap_copy_file_to_fd() - copy from a file to a file descriptor
** returns:
**	0			success
**	-1			failure
*/
int
mkencap_copy_file_to_fd(char *file1, int fd2)
{
	char buf[10240];
	int fd1, retval = 0;
	ssize_t sz;

	fd1 = open(file1, O_RDONLY);
	if (fd1 == -1)
	{
		fprintf(stderr, "mkencap: open(\"%s\"): %s\n",
			file1, strerror(errno));
		return -1;
	}

	while ((sz = read(fd1, buf, sizeof(buf))) > 0)
	{
		if (write(fd2, buf, sz) == -1)
		{
			fprintf(stderr, "mkencap: write(): %s\n",
				strerror(errno));
			retval = -1;
			goto copy_done;
		}
	}
	if (sz == (ssize_t)-1)
	{
		fprintf(stderr, "mkencap: read(): %s\n", strerror(errno));
		retval = -1;
	}

  copy_done:
	close(fd1);
	return retval;
}



/*
** mkencap_copy_file() - copy a file (use stdout if destination is NULL)
** returns:
**	0			success
**	-1			failure
*/
static int
mkencap_copy_file(char *file1, char *file2)
{
	int fd2, retval = 0;

	fd2 = mkencap_open_outfile(file2);
	if (fd2 == -1)
		return -1;

	if (mkencap_copy_file_to_fd(file1, fd2) == -1)
		retval = 1;

	close(fd2);
	return retval;
}


/*
** pkgdir_file_plugin() - list plugin for mkencap_copy_pkgdir_files()
** returns:
**	0			success
**	-1			failure
*/
int
pkgdir_file_plugin(void *data, void *dummy)
{
	char *filename;
	char buf[MAXPATHLEN];

	filename = (char *)data;
	strlcpy(buf, basename(filename), sizeof(buf));

	if (verbose)
		printf("mkencap: creating file \"%s\"\n", buf);

	return mkencap_copy_file(filename, buf);
}


