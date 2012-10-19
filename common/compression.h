/*
**  Copyright 2002-2003 University of Illinois Board of Trustees
**  Copyright 2002-2004 Mark D. Roth
**  All rights reserved.
**
**  compression.h - header file for epkg compression code
**
**  Mark D. Roth <roth@feep.net>
*/

#ifndef COMPRESSION_H
# define COMPRESSION_H

# include <config.h>

# include <sys/types.h>

# ifdef HAVE_LIBTAR
#  include <libtar.h>
# else /* ! HAVE_LIBTAR */

typedef int (*openfunc_t)(const char *, int, ...);
typedef int (*closefunc_t)(int);
typedef ssize_t (*readfunc_t)(int, void *, size_t);
typedef ssize_t (*writefunc_t)(int, const void *, size_t);

struct tartype
{
	openfunc_t openfunc;
	closefunc_t closefunc;
	readfunc_t readfunc;
	writefunc_t writefunc;
};
typedef struct tartype tartype_t;

# endif /* HAVE_LIBTAR */


struct compress_type
{
	char *ct_extension;
	tartype_t *ct_tartype;
	int (*ct_openfunc)(int, int, char *);
	char *ct_open_read_data;
	char *ct_open_write_data;
};
typedef struct compress_type compress_type_t;


# define MATCH_EXTENSION(filename, ext) \
	(strcmp((filename) + strlen(filename) - strlen(ext), (ext)) == 0)


const compress_type_t *compress_handler(char *filename);

int pipe_open(int fd_io, int mode, char *cmd);

#endif /* ! COMPRESSION_H */

