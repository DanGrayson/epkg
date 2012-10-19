/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  encap.h - header file for encap library
**
**  Mark D. Roth <roth@feep.net>
*/

#ifndef ENCAP_H
#define ENCAP_H

#include <encap_listhash.h>

#include <sys/param.h>
#include <sys/types.h>


/* macros for bitfield manipulation */
#define BIT_ISSET(bitfield, opt) ((bitfield) & (opt))
#define BIT_SET(bitfield, opt) ((bitfield) |= (opt))
#define BIT_CLEAR(bitfield, opt) ((bitfield) &= ~(opt))
#define BIT_TOGGLE(bitfield, opt) { \
  if (BIT_ISSET(bitfield, opt)) \
    BIT_CLEAR(bitfield, opt); \
  else \
    BIT_SET(bitfield, opt); \
}


/* forward declaration to appease the compiler */
struct encap;
typedef struct encap ENCAP;


/***** version.c *******************************************************/

/*
** plugin for encap_find_versions()
** arguments: state (as passed to encap_find_versions()),
**            package name, version found
*/
typedef int (*verfunc_t)(void *, char *, char *);

/*
** calls verfunc() for all versions of pkgname in the source directory.
** arguments: Encap source directory, package name, plugin function,
**            and optional state to be used by plugin
** returns:
**	number of versions found	success
**	-1 (and sets errno)		failure
**	-2				verfunc() returned -1
*/
int encap_find_versions(char *, char *, verfunc_t, void *);


/***** vercmp.c *******************************************************/

/*
** encap_vercmp() - compare ver1 and ver2
** returns:
**	-1				ver1 < ver2
**	0				ver1 == ver2
**	1				ver1 > ver2
*/
int encap_vercmp(char *, char *);


/***** pkgspec.c *******************************************************/

/*
** parse a pkgspec
** arguments: pkgspec to parse, name buffer and size, version buffer
**            and size, platform buffer and size, extension buffer and size
** returns:
**	0				success
**	-1 (and sets errno)		failure
*/
int encap_pkgspec_parse(char *, char *, size_t, char *, size_t,
			char *, size_t, char *, size_t);

/*
** join a package name with a package version to produce a full pkgspec.
** arguments: buffer in which to construct pkgspec, size of buffer,
**            string containing name, string containing version.
** returns:
**	0				success
**	-1 (and sets errno)		failure
**
** errno values:
**	ENAMETOOLONG
*/
int encap_pkgspec_join(char *, size_t , char *, char *);


/***** linkinfo.c **********************************************************/

/* description of source file */
struct encap_source_info
{
	int src_flags;			/* from encap_check_source() */
	char src_path[MAXPATHLEN];	/* absolute path to source file */
	char src_pkgdir_relative[MAXPATHLEN];
					/* relative path from pkg dir */
	char src_target_path[MAXPATHLEN];
					/* absolute path to target file */
	char src_target_relative[MAXPATHLEN];
					/* relative path from target dir */
	char src_link_expecting[MAXPATHLEN];
					/* what the link *should* point to */
};
typedef struct encap_source_info encap_source_info_t;

/* values for src_flags bitmask */
#define SRC_REQUIRED		1	/* file is required */
#define SRC_LINKDIR		2	/* file is a linkdir */
#define SRC_EXCLUDED		4	/* file matches exclude entry */
#define SRC_ISDIR		8	/* file is a directory */
#define SRC_ISLNK		16	/* file is a symlink */

/*
** set the fields of srcinfo to describe the file
** "encap->e_source/encap->e_pkgname/dir/file".
** arguments: ENCAP handle (used for Encap source directory and package name)
**            subdirectory name, file name,
**            and a pointer to the encap_source_info_t structure for the result
** returns:
**	0				success
**	-1 (and sets errno)		failure
*/
int encap_check_source(ENCAP *, char *, char *, encap_source_info_t *);


/* description of target link */
struct encap_target_info
{
	int tgt_flags;				/* from encap_check_target() */
	char tgt_link_existing[MAXPATHLEN];	/* what the link *does*
						   point to */
	char tgt_link_existing_pkg[MAXPATHLEN];	/* pkg the link points to */
	char tgt_link_existing_pkgdir_relative[MAXPATHLEN];
};
typedef struct encap_target_info encap_target_info_t;

/* values for tgt_flags bitmask */
#define TGT_EXISTS		1	/* link exists */
#define TGT_ISLNK		2	/* it's really a link */
#define TGT_ISDIR		4	/* it's a directory */
#define TGT_DEST_EXISTS		8	/* link points to an existing target */
#define TGT_DEST_ISDIR		16	/* link points to a directory */
#define TGT_DEST_ENCAP_SRC	32	/* link points under source dir */
#define TGT_DEST_PKGDIR_EXISTS	64	/* link points into existing pkgdir */

/*
** check status of an Encap target file.
** arguments: Encap source directory, relative path to file from Encap
**            target directory, and a pointer to the encap_target_info_t
**            structure for the result
** returns:
**	0				success
**	-1 (and sets errno)		failure
*/
int encap_check_target(char *, char *, encap_target_info_t *);



/***** recursion.c *****************************************************/

/* type values for encap_print_func() */
#define EPT_INST_OK	1	/* successfully installed dir or link */
#define EPT_INST_REPL	2	/* replaced pre-existing link */
#define EPT_INST_FAIL	3	/* failed to install dir or link */
#define EPT_INST_ERROR	4	/* system or library call set errno */
#define EPT_INST_NOOP	5	/* dir or link was already present */
#define EPT_REM_OK	6	/* successfully removed dir or link */
#define EPT_REM_FAIL	7	/* failed to remove dir or link */
#define EPT_REM_ERROR	8	/* system or library call set errno */
#define EPT_REM_NOOP	9	/* dir or link was already removed */
#define EPT_CHK_NOOP	10	/* dir or link is properly installed */
#define EPT_CHK_FAIL	11	/* dir or link is not properly installed */
#define EPT_CHK_ERROR	12	/* system or library call set errno */
#define EPT_PKG_INFO	13	/* package information message */
#define EPT_PKG_FAIL	14	/* general package failure */
#define EPT_PKG_ERROR	15	/* system or library call set errno */
#define EPT_PKG_RAW	16	/* pkg script output, READMEs, etc */
#define EPT_CLN_OK	17	/* removed file or directory */
#define EPT_CLN_INFO	18	/* cleaning information message */
#define EPT_CLN_FAIL	19	/* target is not an Encap link */
#define EPT_CLN_ERROR	20	/* system or library call set errno */
#define EPT_CLN_NOOP	21	/* link is valid */

/*
** last two args are the message type (one of the above macros) and a
** printf()-style format string
*/
typedef int (*encap_print_func_t)(ENCAP *, encap_source_info_t *,
				  encap_target_info_t *,
				  unsigned int, char *, ...);
#define encap_print_func encap_print_func_t


/* recursion actions returned by decision functions */
#define R_RETURN	-2	/* return to top level immediately */
#define R_ERR		-1	/* non-fatal error */
#define R_FILEOK	0	/* continue normally */
#define R_SKIP		1	/* skip to next file */

typedef int (*encap_decision_func_t)(ENCAP *, encap_source_info_t *,
				     encap_target_info_t *);
#define encap_decision_func encap_decision_func_t


/***** package.c *******************************************************/

/* return values for encap_install(), encap_remove(), and encap_check() */
#define ENCAP_STATUS_FAILED	-1	/* operation failed */
#define ENCAP_STATUS_NOOP	0	/* nothing was done */
#define ENCAP_STATUS_PARTIAL	1	/* operation partially successful */
#define ENCAP_STATUS_SUCCESS	2	/* operation succeeded */

/* install a package */
int encap_install(ENCAP *, encap_decision_func_t);

/* remove a package */
int encap_remove(ENCAP *, encap_decision_func_t);

/* check a package */
int encap_check(ENCAP *, encap_decision_func_t);


/***** handle.c ********************************************************/

extern const char libencap_version[];

/* package information structure */
struct encapinfo
{
	char *ei_pkgfmt;	/* package format */
	char *ei_platform;	/* platform name */
	char *ei_description;	/* one-line description */
	char *ei_date;		/* creation date */
	char *ei_contact;	/* contact address of creator */
	encap_list_t *ei_ex_l;	/* exclude list */
	encap_list_t *ei_rl_l;	/* requirelink list */
	encap_list_t *ei_ld_l;	/* linkdir list */
	encap_list_t *ei_pr_l;	/* prereq list */
	encap_hash_t *ei_ln_h;	/* linkname hash */
};
typedef struct encapinfo encapinfo_t;

struct encap
{
	char e_pkgname[MAXPATHLEN];	/* name of package */
	encapinfo_t e_pkginfo;		/* package information */
	unsigned long e_options;	/* options associated with package */
	char e_source[MAXPATHLEN];	/* Encap source directory */
	char e_target[MAXPATHLEN];	/* Encap target directory */
	encap_print_func_t e_print_func;	/* output function */
};

/* values for e_options bitmask */
#define OPT_FORCE		1	/* force action */
#define OPT_SHOWONLY		2	/* display action but do not execute */
#define OPT_ABSLINKS		4	/* use absolute symlinks */
#define OPT_PREREQS		8	/* check prerequisites */
#define OPT_RUNSCRIPTS		16	/* run package scripts */
#define OPT_EXCLUDES		32	/* honor package exclusions */
#define OPT_RUNSCRIPTSONLY	64	/* run scripts, don't link */
#define OPT_NUKETARGETDIRS	128	/* remove empty target dirs */
#define OPT_PKGDIRLINKS		256	/* link files directly from pkgdir */
#define OPT_LINKDIRS		512	/* honor linkdir directives */
#define OPT_LINKNAMES		1024	/* honor linkname directives */
#define OPT_TARGETEXCLUDES	2048	/* honor encap.exclude files under
					   target directory */

#define OPT_DEFAULTS		(OPT_PREREQS|OPT_RUNSCRIPTS|\
				 OPT_EXCLUDES|OPT_NUKETARGETDIRS|\
				 OPT_LINKDIRS|OPT_LINKNAMES)

/*
** open an Encap package.
** arguments: pointer to pointer to ENCAP handle (to be allocated),
**            source directory, target directory, package name,
**            options, print function
** returns:
**	0				success
**	-1 (and sets errno)		failure
*/
int encap_open(ENCAP **, char *, char *, char *, unsigned long,
	       encap_print_func_t);

/*
** close an Encap package.
*/
void encap_close(ENCAP *);


/***** clean.c *********************************************************/

/*
** clean stale links out of target tree.
** arguments: target directory, source directory, options, print
**            function, decision function
** returns:
**      0                               success
**      -1 (and sets errno)             failure
*/
int encap_target_clean(char *, char *, unsigned int,
		       encap_print_func_t, encap_decision_func_t);


/***** encapinfo.c *****************************************************/

struct linkname
{
	char ln_pkgdir_path[MAXPATHLEN];
	char ln_newname[MAXPATHLEN];
};
typedef struct linkname linkname_t;

/*
** encapinfo_parse_directive() - parse encapinfo directive into pkginfo
** returns:
**      0                       success
**      -1 (and sets errbuf)    failure
*/
int encapinfo_parse_directive(char *, encapinfo_t *, char *, size_t);

/*
** initialize data structures in encapinfo.
*/
int encapinfo_init(encapinfo_t *);

/*
** creates an encapinfo file.
** arguments: filename to write, info to encode in file
** returns:
**      0                               success
**      -1 (and sets errno)             failure
*/
int encapinfo_write(char *, encapinfo_t *);

/*
** free all memory associated with an encapinfo_t
*/
void encapinfo_free(encapinfo_t *);


/***** prereq.c ********************************************************/

/* first 4 bits of ep_type field are for specifying ranges */
#define ENCAP_PREREQ_NEWER		1
#define ENCAP_PREREQ_EXACT		2
#define ENCAP_PREREQ_OLDER		4
#define ENCAP_PREREQ_ANY		8
#define ENCAP_PREREQ_RANGEMASK		(ENCAP_PREREQ_NEWER|\
					 ENCAP_PREREQ_EXACT|\
					 ENCAP_PREREQ_OLDER|\
					 ENCAP_PREREQ_ANY)

/* prerequisite types */
#define ENCAP_PREREQ_PKGSPEC		16
#define ENCAP_PREREQ_DIRECTORY		32
#define ENCAP_PREREQ_REGFILE		64
#define ENCAP_PREREQ_TYPEMASK		(~ENCAP_PREREQ_RANGEMASK)

struct encap_prereq
{
	unsigned short ep_type;
	union
	{
		char *ep_pathname;
		char *ep_pkgspec;
	}
	ep_un;
};
typedef struct encap_prereq ENCAP_PREREQ;


/*
** check prerequisites for the package associated with encap.
** returns:
**	0				success
**	1				prereqs not satisfied
**	-1 (and sets errno)		failure
*/
int encap_check_prereqs(ENCAP *);


/***** util.c **********************************************************/

/*
** return the absolute path to what a link points to,
** based on the absolute path to the link in linkname.
** arguments: path to symlink, result buffer, buffer size
** returns:
**	0				success
**	-1 (and sets errno)		failure
*/
int get_link_dest(char *, char *, size_t);


/***** platform.c ******************************************************/

/*
** check a given package platform for compatibility with the host
** platform and the specified list of platform suffixes
*/
int encap_platform_compat(char *, char *, encap_list_t *);

/*
** split platform name into base and optional tag
*/
int encap_platform_split(char *, char *, size_t, char *, size_t);

/*
** returns the "official" platform name.
** arguments: result buffer, buffer size
** returns: pointer to buffer
*/
char *encap_platform_name(char *, size_t);

/*
** returns the old Encap 2.0 platform name
** for backward compat only!!!
*/
char *encap20_platform_name(char *, size_t);


#endif /* ENCAP_H */
