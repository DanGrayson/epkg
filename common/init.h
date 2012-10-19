/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  init.h - header file for epkg initialization code
**
**  Mark D. Roth <roth@feep.net>
*/

#include <config.h>


/* global variables */
extern char source[];		/* Encap source directory */
extern char target[];		/* Encap target directory */


/* initialize source and target */
void init_encap_paths(char *, char *, char *);


