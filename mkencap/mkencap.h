/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  mkencap.h - main header file for mkencap
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

#include <sys/types.h>
#include <sys/param.h>


/* options */
#define MKENCAP_OPT_EMPTYENCAPINFO	1
#define MKENCAP_OPT_OVERWRITE		2


/* global settings */
extern char platform[];			/* platform name */
extern short verbose;			/* verbosity level */
extern unsigned long mkencap_opts;	/* option bitmask */
extern encapinfo_t pkginfo;		/* package information */
extern char common_src_tree[];		/* common source code tree */
extern char build_tree[];		/* build tree */
extern char download_tree[];		/* download directory */


/***** addfiles.c ***************************************/

int mkencap_copy_file_to_fd(char *, int);

int create_profile_include_files(void *, void *);

int pkgdir_file_plugin(void *, void *);


/***** writeinfo.c ***************************************/

void mk_set_encapinfo_contact(encapinfo_t *);

int mk_encapinfo(char *);


/***** build.c ********************************************/

typedef struct encap_profile encap_profile_t;

/* values for build_package() modes bitmask */
#define MKENCAP_PROFILE_DOWNLOAD	 1
#define MKENCAP_PROFILE_UNPACK		 2
#define MKENCAP_PROFILE_PATCH		 4
#define MKENCAP_PROFILE_CONFIGURE	 8
#define MKENCAP_PROFILE_BUILD		16
#define MKENCAP_PROFILE_INSTALL		32
#define MKENCAP_PROFILE_TIDY		64

int build_package(encap_profile_t *, unsigned long, char *, char *);


/***** profile_parse.c ***************************************/

encap_profile_t *parse_profile(char *, char *, char *);

void parse_error(void *, const char *, ...);



/*********************************************************************
*****  data structure for parser state
*********************************************************************/

struct profile_state
{
	unsigned int ps_depth;		/* current depth */
	unsigned int ps_el_idx[10];	/* index into profile_elements[]
					   of element description for
					   each depth */
	void *ps_el_data[10];		/* pointer to element structure */
	encap_profile_t *ps_profile;	/* profile data structure being
					   constructed */
	char *ps_text;			/* temporary holder for text
					   while it's being read */
	size_t ps_textsz;		/* size of current text buffer */
	void *ps_parser;		/* parser handle */
#ifdef DEBUG_XML
	unsigned int ps_newline_state;	/* for debugging output */
#endif
};
typedef struct profile_state profile_state_t;


/*********************************************************************
*****  data structures for profile
*********************************************************************/

/* values for type field */
#define PROF_TYPE_SET		 0
#define PROF_TYPE_PREPEND	 1
#define PROF_TYPE_APPEND	 2
#define PROF_TYPE_UNSET		 3


/* default commands */
#define DEFAULT_CONFIGURE_CMDS \
	"${srcdir}/configure --prefix=\"${ENCAP_SOURCE}/${ENCAP_PKGNAME}\"" \
	" --sysconfdir=\"${ENCAP_TARGET}/etc\"\n"

#define DEFAULT_BUILD_CMDS	"${MAKE}\n"

#define DEFAULT_INSTALL_CMDS \
	"${MAKE} install sysconfdir=\"${ENCAP_SOURCE}/${ENCAP_PKGNAME}/etc\"\n"

#define DEFAULT_PREPACKAGE_CMDS \
	"find . -name lib -prune -o " \
	"\\( -perm -0100 -o -perm -0010 -o -perm -0001 \\) -type f -print " \
	"| xargs ${STRIP}\n"

#define DEFAULT_CLEAN_CMDS	"${MAKE} clean\n"


struct profile_change
{
	char *pc_ver;
	char *pc_date;
	char *pc_text;
};
typedef struct profile_change profile_change_t;

struct profile_prereq
{
	char pp_pkgspec[MAXPATHLEN];
	short pp_use_bin;
	short pp_use_lib;
};
typedef struct profile_prereq profile_prereq_t;

struct profile_environment
{
	short pe_type;
	char *pe_variable;
	char *pe_value;
};
typedef struct profile_environment profile_environment_t;

struct profile_patch
{
	int pp_fd;
	encap_list_t *pp_url_l;
	char *pp_opts;
	char *pp_from_dir;
};
typedef struct profile_patch profile_patch_t;

struct profile_source
{
	encap_list_t *ps_url_l;
	char ps_archive_name[MAXPATHLEN];
	short ps_use_objdir;
	char ps_subdir[MAXPATHLEN];	/* subdir for extracted source code */
	short ps_create_subdir;
	char ps_build_subdir[MAXPATHLEN];	/* subdir in which to build */
	char ps_env_srcdir[MAXPATHLEN];		/* abs path to source files */
	char ps_env_builddir[MAXPATHLEN];	/* abs path to build files */
	char *ps_unpack_cmds;
	encap_list_t *ps_patches_l;
	char *ps_configure_cmds;
	char *ps_build_cmds;
	char *ps_install_cmds;
	char *ps_clean_cmds;
};
typedef struct profile_source profile_source_t;

struct profile_include_file
{
	char pi_filename[MAXPATHLEN];
	mode_t pi_mode;
	uid_t pi_uid;
	gid_t pi_gid;
	char *pi_text;
};
typedef struct profile_include_file profile_include_file_t;

struct encap_profile
{
	char ep_pkgspec[MAXPATHLEN];
	char *ep_notes;
	encap_list_t *ep_changelog_l;
	encap_list_t *ep_prereqs_l;
	encap_list_t *ep_env_vars_l;
	char *ep_prepare_cmds;
	encap_list_t *ep_sources_l;
	encap_list_t *ep_patches_l;
	char *ep_prepackage_cmds;
	encap_list_t *ep_include_files_l;
	char *ep_encapinfo;
};


/*********************************************************************
*****  data structure for element description
*********************************************************************/

/* values for context field */
#define PS_CTX_ROOT		 1
#define PS_CTX_ENCAP_PROFILE	 2
#define PS_CTX_CHANGELOG	 4
#define PS_CTX_ENCAP_SOURCE	 8


typedef void (*element_attr_handler_t)(profile_state_t *, const char **);
typedef void (*element_text_handler_t)(profile_state_t *);

struct element
{
	char *el_name;
	unsigned int el_context;
	unsigned int el_valid_context;
	element_attr_handler_t el_attr_handler;
	element_text_handler_t el_text_handler;
};
typedef struct element element_t;

extern const element_t profile_elements[];


/***** environment.c **************************************/

void env_init(char *, encap_list_t *, char *);

void env_set_source(profile_source_t *);

void env_unset_source(void);

void env_restore(void);


