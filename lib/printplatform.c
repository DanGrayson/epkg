/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  printplatform.c - platform name to be hard-coded into epkg and mkencap
**
**  Mark D. Roth <roth@feep.net>
*/

#include <config.h>
#include <encap.h>

#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

int
main(int argc, char *argv[])
{
	char buf[1024];

	printf("%s\n", encap_platform_name(buf, sizeof(buf)));
	exit(0);
}


