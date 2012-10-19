/*
**  Copyright 2002-2003 University of Illinois Board of Trustees
**  Copyright 2002-2004 Mark D. Roth
**  All rights reserved.
**
**  environment.c - environment variable code for mkencap
**
**  Mark D. Roth <roth@feep.net>
*/

#include <mkencap.h>

#include <stdio.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif


extern char **environ;

static char **save_env = NULL;
static char **base_env = NULL;
static int base_env_len = 0;


static int
env_match(char *var, char *env)
{
	char buf[1024];

	snprintf(buf, sizeof(buf), "%s=", var);
	return !strncmp(env, buf, strlen(buf));
}


/*
** env_read_file() - read environment settings from a file
** returns:
**	1			filename does not exist
**	-1 (and sets errno)	error
**	0			success
*/
static int
env_read_file(char *filename, encap_hash_t *env_h)
{
	FILE *fp;
	char buf[1024];

	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		if (errno == ENOENT)
			return 1;

		fprintf(stderr, "mkencap: fopen(\"%s\"): %s\n",
			filename, strerror(errno));
		return -1;
	}

	if (verbose)
		printf("mkencap: reading environment settings from \"%s\"\n",
		       filename);

	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		/* strip trailing newline */
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';

		/* add to hash */
		encap_hash_add(env_h, strdup(buf));
	}

	if (ferror(fp))
	{
		fprintf(stderr, "mkencap: fgets(): %s\n", strerror(errno));
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}


static void
env_set(encap_hash_t *env_h, char *var, char *value, int overwrite)
{
	encap_hashptr_t hp;
	char buf[1024];

	encap_hashptr_reset(&hp);
	if (encap_hash_getkey(env_h, &hp, var,
			      (encap_matchfunc_t)env_match) != 0)
	{
		if (!overwrite)
			return;
		encap_hash_del(env_h, &hp);
	}

	snprintf(buf, sizeof(buf), "%s=%s", var, value);
	encap_hash_add(env_h, strdup(buf));
}


static void
env_expand(encap_hash_t *env_h, char *value, char *buf, size_t buflen)
{
	char *cp;
	char tmpbuf[1024];
	char var[1024];
	encap_hashptr_t hp;

	strlcpy(buf, value, buflen);

	while (1)
	{
		cp = strstr(buf, "${");
		if (cp == NULL)
			break;

		strlcpy(var, cp, sizeof(var));
		cp = strchr(var, '}');
		if (cp == NULL)
			break;
		*cp = '\0';

		encap_hashptr_reset(&hp);
		if (encap_hash_getkey(env_h, &hp, var + 2,
				      (encap_matchfunc_t)env_match) != 0)
		{
			cp = strchr((char *)encap_hashptr_data(&hp), '=');
			if (cp != NULL)
				cp++;
		}
		else
			/* variable not found, replace with empty string */
			cp = "";

		strlcat(var, "}", sizeof(var));
		encap_gsub(buf, var, cp, tmpbuf, sizeof(tmpbuf));
		strlcpy(buf, tmpbuf, buflen);
	}
}


/*
** environment_setup() - set up environment for subprocesses
*/
void
env_init(char *pkgname, encap_list_t *env_l, char *env_file)
{
	int i;
	encap_listptr_t lp;
	profile_environment_t *envp;
	encap_hash_t *env_h;
	encap_hashptr_t hp;
	char buf[1024], valbuf[1024];
	char *cp;

	env_h = encap_hash_new(128, NULL);

	env_set(env_h, "ENCAP_PKGNAME", pkgname, 1);
	env_set(env_h, "ENCAP_SOURCE", source, 1);
	env_set(env_h, "ENCAP_TARGET", target, 1);
	env_set(env_h, "MKENCAP_DOWNLOAD_DIR", download_tree, 1);
	env_set(env_h, "MKENCAP_SRC_TREE", common_src_tree, 1);
	env_set(env_h, "MKENCAP_BUILD_TREE", build_tree, 1);

	if (env_file == NULL)
		env_file = SYSCONFDIR "/mkencap_environment";
	env_read_file(env_file, env_h);

	if (env_l != NULL)
	{
		encap_listptr_reset(&lp);
		while (encap_list_next(env_l, &lp) != 0)
		{
			envp = (profile_environment_t *)encap_listptr_data(&lp);

			encap_hashptr_reset(&hp);
			i = encap_hash_getkey(env_h, &hp, envp->pe_variable,
					      (encap_matchfunc_t)env_match);

			if (i)
			{
				cp = strchr((char *)encap_hashptr_data(&hp),
					    '=') + 1;
				encap_hash_del(env_h, &hp);
			}
			else
				cp = NULL;

			if (envp->pe_type == PROF_TYPE_UNSET)
				continue;

			snprintf(buf, sizeof(buf), "%s=", envp->pe_variable);

			env_expand(env_h, envp->pe_value,
				   valbuf, sizeof(valbuf));

			switch (envp->pe_type)
			{
			default:
			case PROF_TYPE_SET:
				strlcat(buf, valbuf, sizeof(buf));
				break;

			case PROF_TYPE_PREPEND:
				strlcat(buf, valbuf, sizeof(buf));
				if (cp != NULL)
					strlcat(buf, cp, sizeof(buf));
				break;

			case PROF_TYPE_APPEND:
				if (cp != NULL)
					strlcat(buf, cp, sizeof(buf));
				strlcat(buf, valbuf, sizeof(buf));
				break;
			}
			encap_hash_add(env_h, strdup(buf));
		}
	}

	/* assume certain defaults if not already set */
	env_set(env_h, "MAKE", "make", 0);
	env_set(env_h, "STRIP", "strip", 0);
	env_set(env_h, "PATCH", "patch", 0);
	env_set(env_h, "HOME", "/", 0);
	env_set(env_h, "PATH", "/bin:/usr/bin", 0);

	cp = getenv("TZ");
	if (cp != NULL)
		env_set(env_h, "TZ", cp, 0);

	base_env = (char **)malloc((encap_hash_nents(env_h) + 1)
				   * sizeof(char *));
	base_env_len = 0;

	encap_hashptr_reset(&hp);
	while (encap_hash_next(env_h, &hp) != 0)
		base_env[base_env_len++] = (char *)encap_hashptr_data(&hp);

	base_env[base_env_len] = NULL;
	save_env = environ;
	environ = base_env;

	encap_hash_free(env_h, NULL);
}


void
env_restore(void)
{
	int i;

	if (environ != base_env)
		env_unset_source();

	environ = save_env;

	for (i = 0; i < base_env_len; i++)
		free(base_env[i]);
	free(base_env);
}


void
env_set_source(profile_source_t *srcp)
{
	char **src_env;
	char buf[10240];

	if (environ != base_env)
		env_unset_source();

	src_env = (char **)calloc(base_env_len + 3, sizeof(char *));
	memcpy(src_env, base_env, base_env_len * sizeof(char *));

	snprintf(buf, sizeof(buf), "srcdir=%s", srcp->ps_env_srcdir);
	src_env[base_env_len] = strdup(buf);

	snprintf(buf, sizeof(buf), "builddir=%s", srcp->ps_env_builddir);
	src_env[base_env_len + 1] = strdup(buf);

	environ = src_env;
}


void
env_unset_source(void)
{
	int i;
	char **old_env;

	old_env = environ;
	environ = base_env;

	for (i = base_env_len; old_env[i] != NULL; i++)
		free(old_env[i]);

	free(old_env);
}


