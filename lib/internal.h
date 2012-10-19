/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  internal.h - internal header file for encap library
**
**  Mark D. Roth <roth@feep.net>
*/

#include <config.h>
#include <compat.h>
#include <encap.h>
#include <encap_pathcode.h>


/***** encapinfo.c *****************************************************/

int encap_get_info(ENCAP *encap);


/*
** reads the encap.exclude file in directory dir and adds entries to list
** ex_l.  if prefix is non-NULL, it and '/' are prepended to each entry read.
** arguments: directory to read encap.exclude file in, prefix to prepend
**            to each path in the encap.exclude file, and the list to
**            add resulting paths to
** returns:
**	1				success
**	0				encap.exclude file doesn't exist
**	-1 (and sets errno)		error
*/
int encap_read_exclude_file(char *, char *, encap_list_t *);


/***** prereq.c ********************************************************/

/*
** encap_prereq_parse() - calls the appropriate parsing function for the
** given prereq type.
** argument: the encapinfo_t structure to fill in, the string to parse
** returns:
**      0       success
**      -1      failure (and sets errno)
*/
int encap_prereq_parse(encapinfo_t *, char *);


/*
** arguments: the prereq to print, result buffer, buffer size
*/
size_t encap_prereq_print(ENCAP_PREREQ *, char *, size_t);


/***** recursion.c *****************************************************/

/* modes used by encap_recursion() */
#define ENCAP_INSTALL	1
#define ENCAP_REMOVE	2
#define ENCAP_CHECK	3


/* flags for status bitmask maintained by encap_recursion() */
#define ESTAT_NONEED	1	/* at least one link is already there */
#define ESTAT_OK	2	/* successfully processed at least one link */
#define ESTAT_ERR	4	/* encountered at least one error */
#define ESTAT_FATAL	8	/* fatal error: required link failed */


typedef int (*encap_action_func)(ENCAP *, encap_source_info_t *,
				 encap_target_info_t *);

/*
** core directory recursion.
** arguments: ENCAP handle, encap_source_info_t describing the
**            directory, a pointer to a status bitfield, current mode,
**            and a decision function
*/
int encap_recursion(ENCAP *, encap_source_info_t *, int *,
		    int, encap_decision_func_t);


/***** package.c *******************************************************/

struct mode_info
{
	int mode;
	char *name;
	encap_action_func predir_func;
	encap_action_func postdir_func;
	encap_action_func file_func;
};
typedef struct mode_info mode_info_t;

/*
** argument: ENCAP_INSTALL, ENCAP_CHECK, or ENCAP_REMOVE
*/
const mode_info_t *mode_info(int);


/***** check_pkg.c *****************************************************/

int check_link(ENCAP *, encap_source_info_t *, encap_target_info_t *);


/***** install_pkg.c ***************************************************/

int install_dir(ENCAP *, encap_source_info_t *, encap_target_info_t *);
int install_link(ENCAP *, encap_source_info_t *, encap_target_info_t *);


/***** remove_pkg.c ****************************************************/

int remove_dir(ENCAP *, encap_source_info_t *, encap_target_info_t *);
int remove_link(ENCAP *, encap_source_info_t *, encap_target_info_t *);


/***** util.c **********************************************************/

#ifdef DEBUG
/*
** arguments: ENCAP handle, encap_source_info_t for current file,
**            type, printf()-style format string
*/
int encap_printf(ENCAP *, encap_source_info_t *, int, char *, ...);
#endif

/*
** glob matching function
** arguments: filename, pattern
*/
int glob_match(char *, char *);

/*
** subdir matching function
** arguments: directory, pattern
*/
int partial_glob_match(char *, char *);

/*
** pkgspec matching function
** arguments: specific package being searched for, package from list
*/
int pkg_match(char *, char *);


