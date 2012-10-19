/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  epkg.h - main header file for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <config.h>
#include <compat.h>
#include <encap.h>
#include <encap_pathcode.h>
#include <archive.h>
#include <compression.h>
#include <download.h>
#include <init.h>

#include <sys/param.h>


struct package_state;		/* advance declaration to pacify cc */


/***** batch.c ***************************************************************/

/* batch mode */
int batch_mode(void);


/***** clean.c ***************************************************************/

/* decision function to handle the global exclude list */
int exclude_decision(ENCAP *, encap_source_info_t *,
		     encap_target_info_t *);

/* recursively clean stale links out of target directory */
int clean_mode(void);


/***** check.c ***************************************************************/

/* check whether or not a package is installed */
int check_pkg(char *);

/* check mode */
int check_mode(char *);


/***** epkg.c ****************************************************************/

/* global variables... oh well */
extern char platform[];		/* host platform name */
extern unsigned short verbose;	/* verbosity level */
extern unsigned long options;	/* option bitmask */
extern unsigned long epkg_opts;	/* option bitmask */
extern encap_list_t *exclude_l;	/* global exclude list */
extern encap_list_t *override_l;	/* package override list */
extern encap_list_t *platform_suffix_l;	/* compatible platform suffix list */

/* operating modes */
#define MODE_INSTALL		1
#define MODE_REMOVE		2
#define MODE_CHECK		3
#define MODE_BATCH		4
#define MODE_UPDATE		5
#define MODE_CLEAN		6

/* flags for epkg_opts bitmask */
#define EPKG_OPT_WRITELOG	 1	/* write log file */
#define EPKG_OPT_VERSIONING	 2	/* operate on multiple versions */
#define EPKG_OPT_BACKOFF	 4	/* back off to second-latest version */
#define EPKG_OPT_OLDEXCLUDES	 8	/* read encap.exclude file in
					   source and target directories */
#define EPKG_OPT_UPDATEALLDIRS	16	/* keep checking for latest
					   version in any directory */


/***** install.c *************************************************************/

/* install mode */
int install_mode(char *);

/* install package */
int install_pkg(char *);


/***** output.c **************************************************************/

int epkg_print(ENCAP *, encap_source_info_t *,
	       encap_target_info_t *,
	       unsigned int, char *, ...);


/***** remove.c **************************************************************/

/* remove mode */
int remove_mode(char *);

/* remove package */
int remove_pkg(char *);


/***** state.c ***************************************************************/

/* write log entry - returns 0 on success, -1 on error */
int write_encap_log(char *, short, int);


/***** update.c **************************************************************/

/* install updated packages from update_dir */
int update_mode(char *);


/***** updatedir.c ***********************************************************/

/* add dir to update path */
int update_path_add(char *, int);

/* print update path */
void update_path_print(void);

/* return true if update path is set */
int update_path_isset(void);

/* initialize update path */
int update_path_init(void);

/* free update path */
void update_path_free(void);


struct update_file
{
	char *uf_url;		/* URL (or local path) of tar archive */
	char *uf_ver;		/* package version */
};
typedef struct update_file update_file_t;

/* matching update_file_t by version */
int uf_match(char *, update_file_t *);

/* free memory associated with an update_file structure */
void uf_free(update_file_t *);

/*
** return a list of update_file structs which point to the versions of
** pkgname which are available in update_dir
*/
encap_list_t *find_update_versions(char *);


/***** versions.c ************************************************************/

struct package_versions
{
	char pv_pkgname[MAXPATHLEN];
	encap_list_t *pv_ver_l;
};
typedef struct package_versions package_versions_t;

/* version matching function */
int ver_match(char *, char *);

/* remove all listed versions of a package */
int remove_package_versionlist(char *, encap_list_t *);

/* select one version to be installed, and remove all others */
int install_package_version(char *, char *, encap_list_t *);

/* plugin for find_pkg_versions() */
int version_list_add(void *, char *, char *);

/* inserts a specific pkgspec into the hash/list structure for update mode */
int insert_pkg_in_hash(char *, encap_hash_t *);

/* return a hash of all packages in the source directory */
encap_hash_t *all_packages(void);

/* free memory associated with a list of package_versions structures */
void free_package_versions(package_versions_t *);


