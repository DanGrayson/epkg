/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  profile_elements.c - Encap package profile element handlers
**
**  Mark D. Roth <roth@feep.net>
*/

#include <mkencap.h>

#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


#ifdef HAVE_LIBEXPAT

/***************************************************************************
*****  helper functions
***************************************************************************/

static int
parse_boolean(char *value)
{
	if (strcmp(value, "yes") == 0)
		return 1;
	if (strcmp(value, "no") == 0)
		return 0;
	return -1;
}


static int
parse_type(char *value)
{
	if (strcmp(value, "set") == 0)
		return PROF_TYPE_SET;
	if (strcmp(value, "unset") == 0)
		return PROF_TYPE_UNSET;
	if (strcmp(value, "prepend") == 0)
		return PROF_TYPE_PREPEND;
	if (strcmp(value, "append") == 0)
		return PROF_TYPE_APPEND;
	return -1;
}


static void
tmpfile_text_handler(profile_state_t *psp, int *fdp)
{
	char buf[MAXPATHLEN];
	char *cp;
	size_t total, sz;

#ifdef DEBUG
	printf("==> tmpfile_text_handler()\n");
#endif

	cp = getenv("TMPDIR");
	if (cp == NULL)
		cp = "/tmp";
	snprintf(buf, sizeof(buf), "%s/mkencap-XXXXXX", cp);
	*fdp = mkstemp(buf);
	if (*fdp == -1)
	{
		parse_error(psp->ps_parser,
			    "patch_text_handler: cannot create temp file: %s",
			    strerror(errno));
		/* NOTREACHED */
	}
	unlink(buf);

	for (total = 0;
	     total < psp->ps_textsz;
	     total += sz)
	{
		sz = write(*fdp, psp->ps_text + total,
			   psp->ps_textsz - total);
		if (sz == (size_t)-1)
		{
			parse_error(psp->ps_parser,
				    "patch_text_handler: cannot write to "
				    "temp file: %s", strerror(errno));
			/* NOTREACHED */
		}
	}

	if (lseek(*fdp, 0, SEEK_SET) == -1)
	{
		parse_error(psp->ps_parser,
			    "patch_text_handler: lseek: %s",
			    strerror(errno));
		/* NOTREACHED */
	}
}


/***************************************************************************
*****  <encap_profile>
***************************************************************************/

static void
encap_profile_attr_handler(profile_state_t *psp, const char **attr)
{
	int i, got_profile_ver = 0;

#ifdef DEBUG
	printf("==> encap_profile_attr_handler()\n");
#endif
	if (psp->ps_profile != NULL)
		return;

	psp->ps_profile = (encap_profile_t *)calloc(1, sizeof(encap_profile_t));
	psp->ps_profile->ep_prepackage_cmds = strdup(DEFAULT_PREPACKAGE_CMDS);

	for (i = 0; attr[i] != NULL; i += 2)
	{
		if (strcmp(attr[i], "pkgspec") == 0)
		{
			strlcpy(psp->ps_profile->ep_pkgspec, attr[i + 1],
				sizeof(psp->ps_profile->ep_pkgspec));
			continue;
		}

		if (strcmp(attr[i], "profile_ver") == 0)
		{
			if (strcmp(attr[i + 1], "1.0") != 0)
			{
				parse_error(psp->ps_parser,
					    "unknown profile version \"%s\"",
					    attr[i + 1]);
				/* NOTREACHED */
			}
			got_profile_ver = 1;
			continue;
		}

		parse_error(psp->ps_parser,
			    "<encap_profile> element: unknown attribute \"%s\"",
			    attr[i]);
		/* NOTREACHED */
	}

	if (psp->ps_profile->ep_pkgspec[0] == '\0')
	{
		parse_error(psp->ps_parser, "<encap_profile> element: "
			    "missing pkgspec attribute");
		/* NOTREACHED */
	}

	if (!got_profile_ver)
	{
		parse_error(psp->ps_parser, "<encap_profile> element: "
			    "missing profile_ver attribute");
		/* NOTREACHED */
	}

	psp->ps_el_data[psp->ps_depth] = psp->ps_profile;
}


/***************************************************************************
*****  <notes>
***************************************************************************/

static void
notes_text_handler(profile_state_t *psp)
{
#ifdef DEBUG
	printf("==> notes_text_handler()\n");
#endif
	if (psp->ps_profile->ep_notes != NULL)
		free(psp->ps_profile->ep_notes);
	psp->ps_profile->ep_notes = psp->ps_text;
}


/***************************************************************************
*****  <change>
***************************************************************************/

static void
change_attr_handler(profile_state_t *psp, const char **attr)
{
	int i;
	profile_change_t *pcp;

#ifdef DEBUG
	printf("==> change_attr_handler()\n");
#endif

	pcp = (profile_change_t *)calloc(1, sizeof(profile_change_t));

	for (i = 0; attr[i] != NULL; i += 2)
	{
		if (strcmp(attr[i], "version") == 0)
		{
			pcp->pc_ver = strdup((char *)attr[i + 1]);
			continue;
		}

		if (strcmp(attr[i], "date") == 0)
		{
			pcp->pc_date = strdup((char *)attr[i + 1]);
			continue;
		}

		parse_error(psp->ps_parser,
			    "<change> element: unknown attribute \"%s\"",
			    attr[i]);
		/* NOTREACHED */
	}

	if (pcp->pc_ver == NULL)
	{
		parse_error(psp->ps_parser,
			    "<change> element: missing version attribute");
		/* NOTREACHED */
	}

	if (psp->ps_profile->ep_changelog_l == NULL)
		psp->ps_profile->ep_changelog_l = encap_list_new(LIST_QUEUE,
								 NULL);

	encap_list_add(psp->ps_profile->ep_changelog_l, pcp);

	psp->ps_el_data[psp->ps_depth] = pcp;
}


static void
change_text_handler(profile_state_t *psp)
{
	profile_change_t *pcp;

#ifdef DEBUG
	printf("==> change_text_handler()\n");
#endif

	pcp = (profile_change_t *)psp->ps_el_data[psp->ps_depth];

	pcp->pc_text = psp->ps_text;
}


/***************************************************************************
*****  <prereq>
***************************************************************************/

static void
prereq_attr_handler(profile_state_t *psp, const char **attr)
{
	int i;
	profile_prereq_t *ppp;

#ifdef DEBUG
	printf("==> prereq_attr_handler()\n");
#endif

	ppp = (profile_prereq_t *)calloc(1, sizeof(profile_prereq_t));
	ppp->pp_use_bin = -1;
	ppp->pp_use_lib = -1;

	for (i = 0; attr[i] != NULL; i += 2)
	{
		if (strcmp(attr[i], "package") == 0)
		{
			strlcpy(ppp->pp_pkgspec, attr[i + 1],
				sizeof(ppp->pp_pkgspec));
			continue;
		}

		if (strcmp(attr[i], "use_bin") == 0)
		{
			ppp->pp_use_bin = parse_boolean((char *)attr[i + 1]);
			continue;
		}

		if (strcmp(attr[i], "use_lib") == 0)
		{
			ppp->pp_use_lib = parse_boolean((char *)attr[i + 1]);
			continue;
		}

		parse_error(psp->ps_parser,
			    "<prereq> element: unknown attribute \"%s\"",
			    attr[i]);
		/* NOTREACHED */
	}

	if (ppp->pp_pkgspec[0] == '\0')
	{
		parse_error(psp->ps_parser,
			    "<prereq> element: missing package attribute");
		/* NOTREACHED */
	}

	if (psp->ps_profile->ep_prereqs_l == NULL)
		psp->ps_profile->ep_prereqs_l = encap_list_new(LIST_QUEUE, NULL);

	encap_list_add(psp->ps_profile->ep_prereqs_l, ppp);
}


/***************************************************************************
*****  <environment>
***************************************************************************/

static void
environment_attr_handler(profile_state_t *psp, const char **attr)
{
	int i;
	profile_environment_t *pep;

#ifdef DEBUG
	printf("==> environment_attr_handler()\n");
#endif

	pep = (profile_environment_t *)calloc(1, sizeof(profile_environment_t));
	pep->pe_type = -1;

	for (i = 0; attr[i] != NULL; i += 2)
	{
		if (strcmp(attr[i], "type") == 0)
		{
			pep->pe_type = parse_type((char *)attr[i + 1]);
			continue;
		}

		if (strcmp(attr[i], "variable") == 0)
		{
			pep->pe_variable = strdup((char *)attr[i + 1]);
			continue;
		}

		if (strcmp(attr[i], "value") == 0)
		{
			pep->pe_value = strdup((char *)attr[i + 1]);
			continue;
		}

		parse_error(psp->ps_parser,
			    "<environment> element: unknown attribute \"%s\"",
			    attr[i]);
		/* NOTREACHED */
	}

	if (pep->pe_variable == NULL)
	{
		parse_error(psp->ps_parser, "<environment> element: "
			    "missing variable attribute");
		/* NOTREACHED */
	}

	if (psp->ps_profile->ep_env_vars_l == NULL)
		psp->ps_profile->ep_env_vars_l = encap_list_new(LIST_QUEUE,
								NULL);

	encap_list_add(psp->ps_profile->ep_env_vars_l, pep);
	psp->ps_el_data[psp->ps_depth] = pep;
}


/***************************************************************************
*****  <source>
***************************************************************************/

static void
source_attr_handler(profile_state_t *psp, const char **attr)
{
	int i;
	profile_source_t *psrcp;

#ifdef DEBUG
	printf("==> source_attr_handler()\n");
#endif

	psrcp = (profile_source_t *)calloc(1, sizeof(profile_source_t));
	psrcp->ps_use_objdir = -1;
	psrcp->ps_create_subdir = -1;
	psrcp->ps_configure_cmds = strdup(DEFAULT_CONFIGURE_CMDS);
	psrcp->ps_build_cmds = strdup(DEFAULT_BUILD_CMDS);
	psrcp->ps_install_cmds = strdup(DEFAULT_INSTALL_CMDS);
	psrcp->ps_clean_cmds = strdup(DEFAULT_CLEAN_CMDS);
	psrcp->ps_url_l = encap_list_new(LIST_QUEUE, NULL);

	for (i = 0; attr[i] != NULL; i += 2)
	{
		if (strcmp(attr[i], "url") == 0)
		{
			encap_list_add_str(psrcp->ps_url_l,
					   (char *)attr[i + 1],
					   " \t\n");
			continue;
		}

		if (strcmp(attr[i], "use_objdir") == 0)
		{
			psrcp->ps_use_objdir = parse_boolean((char *)attr[i + 1]);
			continue;
		}

		if (strcmp(attr[i], "subdir") == 0)
		{
			strlcpy(psrcp->ps_subdir, (char *)attr[i + 1],
				sizeof(psrcp->ps_subdir));
			continue;
		}

		if (strcmp(attr[i], "build_subdir") == 0)
		{
			strlcpy(psrcp->ps_build_subdir, (char *)attr[i + 1],
				sizeof(psrcp->ps_build_subdir));
			continue;
		}

		if (strcmp(attr[i], "create_subdir") == 0)
		{
			psrcp->ps_create_subdir = parse_boolean((char *)attr[i + 1]);
			continue;
		}

#if 0
		if (strcmp(attr[i], "use_DESTDIR") == 0)
		{
			psrcp->ps_use_DESTDIR = parse_boolean((char *)attr[i + 1]);
			continue;
		}
#endif

		parse_error(psp->ps_parser,
			    "<source> element: unknown attribute \"%s\"",
			    attr[i]);
		/* NOTREACHED */
	}

	if (encap_list_nents(psrcp->ps_url_l) == 0)
	{
		parse_error(psp->ps_parser,
			    "<source> element: missing url attribute");
		/* NOTREACHED */
	}

	if (psp->ps_profile->ep_sources_l == NULL)
		psp->ps_profile->ep_sources_l = encap_list_new(LIST_QUEUE,
							       NULL);

	encap_list_add(psp->ps_profile->ep_sources_l, psrcp);
	psp->ps_el_data[psp->ps_depth] = psrcp;
}


/***************************************************************************
*****  common code for all command-list elements
***************************************************************************/

struct cmd_state
{
	short cs_type;
	char **cs_textp;
};
typedef struct cmd_state cmd_state_t;

static void
command_attr_handler(profile_state_t *psp, const char **attr,
		     char **textp, char *element)
{
	int i, type = -1;
	cmd_state_t *cmd_state;

	for (i = 0; attr[i] != NULL; i += 2)
	{
#if 0
		printf("=== i=%d\n", i);
		printf("=== attr[i]=\"%s\"\n", attr[i]);
		printf("=== attr[i+1]=\"%s\"\n", attr[i+1]);
#endif

		if (strcmp(attr[i], "type") == 0)
		{
			type = parse_type((char *)attr[i + 1]);
			continue;
		}

		parse_error(psp->ps_parser,
			    "<%s> element: unknown attribute \"%s\"",
			    element, attr[i]);
		/* NOTREACHED */
	}

	if (type == PROF_TYPE_UNSET)
	{
		if (*textp != NULL)
			free(*textp);
		*textp = NULL;
		psp->ps_el_data[psp->ps_depth] = NULL;
	}
	else
	{
		cmd_state = (cmd_state_t *)calloc(1, sizeof(cmd_state_t));
		cmd_state->cs_type = type;
		cmd_state->cs_textp = textp;
		psp->ps_el_data[psp->ps_depth] = cmd_state;
	}
}


static void
command_text_handler(profile_state_t *psp)
{
	size_t len;
	char *cp;
	cmd_state_t *cmd_state;

#ifdef DEBUG
	printf("==> command_text_handler()\n");
#endif

	cmd_state = (cmd_state_t *)psp->ps_el_data[psp->ps_depth];
	if (cmd_state == NULL)
		return;

	switch (cmd_state->cs_type)
	{
	default:
	case PROF_TYPE_SET:
		if (*(cmd_state->cs_textp) != NULL)
			free(*(cmd_state->cs_textp));
		*(cmd_state->cs_textp) = strdup(psp->ps_text);
		break;
	case PROF_TYPE_PREPEND:
		len = strlen(*(cmd_state->cs_textp)) + strlen(psp->ps_text) + 1;
		cp = malloc(len);
		strcpy(cp, psp->ps_text);
		strlcat(cp, *(cmd_state->cs_textp), len);
		if (*(cmd_state->cs_textp) != NULL)
			free(*(cmd_state->cs_textp));
		*(cmd_state->cs_textp) = cp;
		break;
	case PROF_TYPE_APPEND:
		len = strlen(*(cmd_state->cs_textp)) + strlen(psp->ps_text) + 1;
		*(cmd_state->cs_textp) = realloc(*(cmd_state->cs_textp), len);
		strlcat(*(cmd_state->cs_textp), psp->ps_text, len);
		break;
	case PROF_TYPE_UNSET:
		/* this is handled in command_attr_handler() */
		break;
	}

	free(cmd_state);
}


/***************************************************************************
*****  <patch>
***************************************************************************/

static void
patch_attr_handler(profile_state_t *psp, const char **attr)
{
	int i;
	profile_patch_t *ppp;
	profile_source_t *psrcp;
	encap_list_t **pclp;

#ifdef DEBUG
	printf("==> patch_attr_handler()\n");
#endif

	ppp = (profile_patch_t *)calloc(1, sizeof(profile_patch_t));
	ppp->pp_fd = -1;

	for (i = 0; attr[i] != NULL; i += 2)
	{
		if (strcmp(attr[i], "url") == 0)
		{
			ppp->pp_url_l = encap_list_new(LIST_QUEUE, NULL);
			encap_list_add_str(ppp->pp_url_l,
					   (char *)attr[i + 1],
					   " \t\n");
			continue;
		}

		if (strcmp(attr[i], "options") == 0)
		{
			ppp->pp_opts = strdup((char *)attr[i + 1]);
			continue;
		}

		if (strcmp(attr[i], "from_dir") == 0)
		{
			ppp->pp_from_dir = strdup((char *)attr[i + 1]);
			continue;
		}

		parse_error(psp->ps_parser,
			    "<patch> element: unknown attribute \"%s\"",
			    attr[i]);
		/* NOTREACHED */
	}

	if (profile_elements[psp->ps_el_idx[psp->ps_depth - 1]].el_context
	    == PS_CTX_ENCAP_SOURCE)
	{
		psrcp = (profile_source_t *)psp->ps_el_data[psp->ps_depth - 1];
		pclp = &(psrcp->ps_patches_l);
	}
	else
		pclp = &(psp->ps_profile->ep_patches_l);

	if ((*pclp) == NULL)
		*pclp = encap_list_new(LIST_QUEUE, NULL);

	encap_list_add(*pclp, ppp);
	psp->ps_el_data[psp->ps_depth] = ppp;
}


static void
patch_text_handler(profile_state_t *psp)
{
	profile_patch_t *ppp;

#ifdef DEBUG
	printf("==> patch_text_handler()\n");
#endif

	ppp = (profile_patch_t *)psp->ps_el_data[psp->ps_depth];
	tmpfile_text_handler(psp, &(ppp->pp_fd));
}


/***************************************************************************
*****  <unpack>
***************************************************************************/

static void
unpack_attr_handler(profile_state_t *psp, const char **attr)
{
	profile_source_t *psrcp;
	char **textp;

#ifdef DEBUG
	printf("==> unpack_attr_handler()\n");
#endif

	psrcp = (profile_source_t *)psp->ps_el_data[psp->ps_depth - 1];
	textp = &(psrcp->ps_unpack_cmds);

	command_attr_handler(psp, attr, textp, "unpack");
}


/***************************************************************************
*****  <configure>
***************************************************************************/

static void
configure_attr_handler(profile_state_t *psp, const char **attr)
{
	profile_source_t *psrcp;
	char **textp;

#ifdef DEBUG
	printf("==> configure_attr_handler()\n");
#endif

	psrcp = (profile_source_t *)psp->ps_el_data[psp->ps_depth - 1];
	textp = &(psrcp->ps_configure_cmds);

	command_attr_handler(psp, attr, textp, "configure");
}


/***************************************************************************
*****  <build>
***************************************************************************/

static void
build_attr_handler(profile_state_t *psp, const char **attr)
{
	profile_source_t *psrcp;
	char **textp;

#ifdef DEBUG
	printf("==> build_attr_handler()\n");
#endif

	psrcp = (profile_source_t *)psp->ps_el_data[psp->ps_depth - 1];
	textp = &(psrcp->ps_build_cmds);

	command_attr_handler(psp, attr, textp, "build");
}


/***************************************************************************
*****  <install>
***************************************************************************/

static void
install_attr_handler(profile_state_t *psp, const char **attr)
{
	profile_source_t *psrcp;
	char **textp;

#ifdef DEBUG
	printf("==> install_attr_handler()\n");
#endif

	psrcp = (profile_source_t *)psp->ps_el_data[psp->ps_depth - 1];
	textp = &(psrcp->ps_install_cmds);

	command_attr_handler(psp, attr, textp, "install");
}


/***************************************************************************
*****  <clean>
***************************************************************************/

static void
clean_attr_handler(profile_state_t *psp, const char **attr)
{
	profile_source_t *psrcp;
	char **textp;

#ifdef DEBUG
	printf("==> clean_attr_handler()\n");
#endif

	psrcp = (profile_source_t *)psp->ps_el_data[psp->ps_depth - 1];
	textp = &(psrcp->ps_clean_cmds);

	command_attr_handler(psp, attr, textp, "clean");
}


/***************************************************************************
*****  <prepare>
***************************************************************************/

static void
prepare_attr_handler(profile_state_t *psp, const char **attr)
{
	char **textp;

#ifdef DEBUG
	printf("==> prepare_attr_handler()\n");
#endif

	textp = &(psp->ps_profile->ep_prepare_cmds);
	command_attr_handler(psp, attr, textp, "prepare");
}


/***************************************************************************
*****  <prepackage>
***************************************************************************/

static void
prepackage_attr_handler(profile_state_t *psp, const char **attr)
{
	char **textp;

#ifdef DEBUG
	printf("==> prepackage_attr_handler()\n");
#endif

	textp = &(psp->ps_profile->ep_prepackage_cmds);
	command_attr_handler(psp, attr, textp, "prepackage");
}


/***************************************************************************
*****  <include_file>
***************************************************************************/

static void
include_file_attr_handler(profile_state_t *psp, const char **attr)
{
	int i;
	profile_include_file_t *pifp;
	struct passwd *pwd;
	struct group *grp;
	unsigned long ul;
	unsigned int ui;

#ifdef DEBUG
	printf("==> include_file_attr_handler()\n");
#endif

	pifp = (profile_include_file_t *)calloc(1, sizeof(profile_include_file_t));
	pifp->pi_mode = (mode_t)-1;
	pifp->pi_uid = (uid_t)-1;
	pifp->pi_gid = (gid_t)-1;

	for (i = 0; attr[i] != NULL; i += 2)
	{
		if (strcmp(attr[i], "name") == 0)
		{
			strlcpy(pifp->pi_filename, (char *)attr[i + 1],
				sizeof(pifp->pi_filename));
			continue;
		}

		if (strcmp(attr[i], "mode") == 0)
		{
			sscanf((char *)attr[i + 1], "%o", &ui);
			pifp->pi_mode = (mode_t)ui;
			continue;
		}

		if (strcmp(attr[i], "owner") == 0)
		{
			if (!isdigit((char)attr[i + 1][0]))
			{
				pwd = getpwnam((char *)attr[i + 1]);
				if (pwd == NULL)
				{
					printf("no such user \"%s\"\n",
					       (char *)attr[i + 1]);
					continue;
				}
				pifp->pi_uid = pwd->pw_uid;
			}
			else
			{
				sscanf((char *)attr[i + 1], "%ul", &ul);
				pifp->pi_uid = (uid_t)ul;
			}
			continue;
		}

		if (strcmp(attr[i], "group") == 0)
		{
			if (!isdigit((char)attr[i + 1][0]))
			{
				grp = getgrnam((char *)attr[i + 1]);
				if (grp == NULL)
				{
					printf("no such group \"%s\"\n",
					       (char *)attr[i + 1]);
					continue;
				}
				pifp->pi_gid = grp->gr_gid;
			}
			else
			{
				sscanf((char *)attr[i + 1], "%ul", &ul);
				pifp->pi_gid = (uid_t)ul;
			}
			continue;
		}

		parse_error(psp->ps_parser,
			    "<include_file> element: unknown attribute \"%s\"",
			    attr[i]);
		/* NOTREACHED */
	}

	if (pifp->pi_filename[0] == '\0')
	{
		free(pifp);
		parse_error(psp->ps_parser,
			    "<include_file> element: missing name attribute");
		/* NOTREACHED */
	}

	if (psp->ps_profile->ep_include_files_l == NULL)
		psp->ps_profile->ep_include_files_l = encap_list_new(LIST_QUEUE, NULL);

	encap_list_add(psp->ps_profile->ep_include_files_l, pifp);
	psp->ps_el_data[psp->ps_depth] = pifp;
}


static void
include_file_text_handler(profile_state_t *psp)
{
	profile_include_file_t *pifp;

#ifdef DEBUG
	printf("==> include_file_text_handler()\n");
#endif

	pifp = (profile_include_file_t *)psp->ps_el_data[psp->ps_depth];

	pifp->pi_text = psp->ps_text;
}


/***************************************************************************
*****  <encapinfo>
***************************************************************************/

static void
encapinfo_text_handler(profile_state_t *psp)
{
#ifdef DEBUG
	printf("==> encapinfo_text_handler()\n");
#endif

	psp->ps_profile->ep_encapinfo = psp->ps_text;
}


/***************************************************************************
*****  master element description array
***************************************************************************/

const element_t profile_elements[] = {
  { "encap_profile",
		PS_CTX_ENCAP_PROFILE,
		PS_CTX_ROOT,
		encap_profile_attr_handler,
		NULL },
  { "notes",
		0,
		PS_CTX_ENCAP_PROFILE,
		NULL,
		notes_text_handler },
  { "ChangeLog",
		PS_CTX_CHANGELOG,
		PS_CTX_ENCAP_PROFILE,
		NULL,
		NULL },
  { "change",
		0,
		PS_CTX_CHANGELOG,
		change_attr_handler,
		change_text_handler },
  { "prereq",
		0,
		PS_CTX_ENCAP_PROFILE,
		prereq_attr_handler,
		NULL },
  { "environment",
		0,
		PS_CTX_ENCAP_PROFILE,
		environment_attr_handler,
		NULL },
  { "prepare",
		0,
		PS_CTX_ENCAP_PROFILE,
		prepare_attr_handler,
		command_text_handler },
  { "source",
		PS_CTX_ENCAP_SOURCE,
		PS_CTX_ENCAP_PROFILE,
		source_attr_handler,
		NULL },
  { "unpack",
		0,
		PS_CTX_ENCAP_SOURCE,
		unpack_attr_handler,
		command_text_handler },
  { "patch",
		0,
		PS_CTX_ENCAP_SOURCE,
		patch_attr_handler,
		patch_text_handler },
  { "configure",
		0,
		PS_CTX_ENCAP_SOURCE,
		configure_attr_handler,
		command_text_handler },
  { "build",
		0,
		PS_CTX_ENCAP_SOURCE,
		build_attr_handler,
		command_text_handler },
  { "install",
		0,
		PS_CTX_ENCAP_SOURCE,
		install_attr_handler,
		command_text_handler },
  { "clean",
		0,
		PS_CTX_ENCAP_SOURCE,
		clean_attr_handler,
		command_text_handler },
  { "include_file",
		0,
		PS_CTX_ENCAP_PROFILE,
		include_file_attr_handler,
		include_file_text_handler },
  { "encapinfo",
		0,
		PS_CTX_ENCAP_PROFILE,
		NULL,
		encapinfo_text_handler },
  { "prepackage",
		0,
		PS_CTX_ENCAP_PROFILE,
		prepackage_attr_handler,
		command_text_handler },
  { NULL,
		0,
		0,
		NULL,
		NULL }
};


#endif /* HAVE_LIBEXPAT */

