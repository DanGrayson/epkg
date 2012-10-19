/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  archive.h - header file for epkg archive handling code
**
**  Mark D. Roth <roth@feep.net>
*/

#include <config.h>
#include <compat.h>

#include <compression.h>
#include <download.h>


extern int archive_verbose;

#define ARCHIVE_OPT_FORCE	1	/* overwrite existing files */
#define ARCHIVE_OPT_ENCAPINFO	2	/* add encapinfo at start of archive */


/*
** archive_create() - create a tar archive
** returns:
**	0			success
**	-1 (and sets errno)	failure
*/
int archive_create(char *, char *, unsigned long);


/*
** is_archive() - determine whether a file is an archive (based on extension)
** returns:
**	1	file has a recognized archive extension
**	0	otherwise
*/
int is_archive(char *);

/*
** archive_extract() - extract archive into destdir
** returns:
**	0			success
**	-1 (and sets errno)	failure
*/
int archive_extract(char *, char *, unsigned long);


