/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  compression.c - archive compression functions
**
**  Mark D. Roth <roth@feep.net>
*/

#include <compression.h>
#include <compat.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_LIBZ
# include <zlib.h>
#endif


static tartype_t filetype = { NULL, close, read, write };



/*
** pipe_open() - run a command with fd_io as input (or output)
**               and return output (or input) fd
*/
int
pipe_open(int fd_io, int mode, char *cmd)
{
	int pipe_fd[2];
	char *thisp, *nextp;
	char **argv = NULL;
	int argc = 0;

#ifdef DEBUG
	printf("==> pipe_open(fd_to_cmd=%d, cmd=\"%s\", mode=%d)\n",
	       fd_io, cmd, mode);
#endif

	if (mode != O_RDONLY
	    && mode != O_WRONLY)
	{
		errno = EINVAL;
		return -1;
	}

	if (pipe(pipe_fd) == -1)
		return -1;

	switch (fork())
	{
	case -1:
		return -1;

	case 0:
		/* child */

		/* set up I/O */
		if (mode == O_RDONLY)
		{
			dup2(fd_io, 0);		/* set stdin */
			dup2(pipe_fd[1], 1);	/* set stdout */
			close(pipe_fd[0]);
		}
		else
		{
			dup2(pipe_fd[0], 0);	/* set stdin */
			dup2(fd_io, 1);		/* set stdout */
			close(pipe_fd[1]);
		}

		/* parse command */
		/* FIXME: need to handle quoted strings */
		nextp = cmd;
		while ((thisp = strsep(&nextp, " \t")) != NULL)
		{
			/* skip duplicate spaces */
			if (*thisp == '\0')
				continue;

			argv = (char **)realloc(argv,
						(argc + 2) * sizeof(char *));
			if (argv == NULL)
			{
				fprintf(stderr, "ERROR: realloc() failed!\n");
				exit(1);
			}

			argv[argc++] = thisp;
		}
		argv[argc] = NULL;

		/* exec command */
		execvp(argv[0], argv);

		fprintf(stderr, "ERROR: execvp() failed!\n");
		exit(1);

	default:
		/* parent */
		break;
	}

	if (mode == O_RDONLY)
	{
		close(pipe_fd[1]);
#ifdef DEBUG
		printf("<== pipe_open(): returning pipe_fd[0]=%d\n",
		       pipe_fd[0]);
#endif
		return pipe_fd[0];
	}
	/* else */

	close(pipe_fd[0]);
#ifdef DEBUG
	printf("<== pipe_open(): returning pipe_fd[1]=%d\n", pipe_fd[1]);
#endif
	return pipe_fd[1];
}


#ifdef HAVE_LIBZ

static int
gzip_open(int fd_io, int mode, char *not_used)
{
	gzFile gzf;

	if (mode != O_RDONLY
	    && mode != O_WRONLY)
	{
		errno = EINVAL;
		return -1;
	}

	gzf = gzdopen(fd_io, (mode == O_RDONLY ? "r" : "w"));
	if (gzf == NULL)
	{
		errno = ENOMEM;
		return -1;
	}

	return (int)gzf;
}

static tartype_t gztype = {
	NULL,
	(closefunc_t)gzclose,
	(readfunc_t)gzread,
	(writefunc_t)gzwrite
};

#endif /* HAVE_LIBZ */


static const compress_type_t compress_types[] = {
#ifdef HAVE_LIBZ
	{ ".gz",	&gztype,	gzip_open,
		NULL,		NULL },
	{ ".tgz",	&gztype,	gzip_open,
		NULL,		NULL },
#else
	{ ".gz",	&filetype,	pipe_open,
		"gzip -dc",	"gzip" },
	{ ".tgz",	&filetype,	pipe_open,
		"gzip -dc",	"gzip" },
#endif
	{ ".Z",		&filetype,	pipe_open,
		"zcat",		"compress" },
	{ ".bz2",	&filetype,	pipe_open,
		"bzip2 -dc",	"bzip2" },
	{ NULL,		&filetype,	NULL,
		NULL,		NULL }
};


const compress_type_t *
compress_handler(char *filename)
{
	int i;

	for (i = 0; compress_types[i].ct_extension != NULL; i++)
	{
		if (MATCH_EXTENSION(filename, compress_types[i].ct_extension))
			break;
	}

	return &(compress_types[i]);
}


