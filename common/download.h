/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  download.h - header file for epkg download code
**
**  Mark D. Roth <roth@feep.net>
*/

#ifndef DOWNLOAD_H
# define DOWNLOAD_H

# include <config.h>
# include <encap_listhash.h>


extern int dl_verbose;
extern unsigned long dl_opts;

/* values for dl_opts bitmask */
# define DL_OPT_CONNRETRY		1	/* retry connections */
# define DL_OPT_DEBUG			2	/* debug connections */


/*
** plugin function for download_dir()
** (return -1 to abort immediately, 0 otherwise)
*/
typedef int (*dir_entry_func_t)(char *, void *);


/*
** download_file() - download a local file or remote URL
**                   if outfile is specified, write to outfile and
**                   return 0, otherwise return file descriptor
** returns:
**	0 or file descriptor	success
**	-1 (and sets errno)	failure
*/
int download_file(char *, char *);


/*
** download_dir() - scan a local or remote directory
** returns:
**	0			success
**	-1 (and sets errno)	failure
*/
int download_dir(char *, dir_entry_func_t, void *);


/*
** download_cleanup() - clean up any cached connections
*/
void download_cleanup(void);

#endif /* ! DOWNLOAD_H */


