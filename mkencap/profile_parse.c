/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  profile_parse.c - mkencap code to parse package profiles
**
**  Mark D. Roth <roth@feep.net>
*/

#include <mkencap.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <setjmp.h>

#ifdef STDC_HEADERS
# include <stdarg.h>
# include <string.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif


#ifdef HAVE_LIBEXPAT

# include <expat.h>


/* values for ps_newline_state field */
# define PS_NS_INIT		 0
# define PS_NS_SKIP_NEXT	 1
# define PS_NS_LAST_WAS_TEXT	 2
# define PS_NS_LAST_WAS_NL	 3


static void
text_handler(void *data, const XML_Char *string, int len)
{
	int i, j = 0;
	struct profile_state *psp = data;

# ifdef DEBUG2
	printf("==> text_handler()\n");
# endif

# ifdef DEBUG_XML
	for (i = 0; i < len; i++)
	{
		if (psp->ps_newline_state == PS_NS_SKIP_NEXT
		    && string[i] == '\n')
		{
			psp->ps_newline_state = PS_NS_LAST_WAS_NL;
			continue;
		}
		if (psp->ps_newline_state != PS_NS_LAST_WAS_TEXT)
		{
			for (j = 0; j < psp->ps_depth; j++)
				putchar('\t');
			psp->ps_newline_state = PS_NS_LAST_WAS_TEXT;
		}
		putchar(string[i]);
		if (string[i] == '\n')
			psp->ps_newline_state = PS_NS_LAST_WAS_NL;
	}
# endif

	/* skip leading newline */
	if (psp->ps_textsz == 0
	    && len > 0 
	    && string[0] == '\n')
	{
		string++;
		len--;
	}

	if (profile_elements[psp->ps_el_idx[psp->ps_depth]].el_text_handler != NULL)
	{
		psp->ps_text = (char *)realloc(psp->ps_text,
					       psp->ps_textsz + len + 1);
		memcpy(psp->ps_text + psp->ps_textsz, string, len);
		psp->ps_textsz += len;
		psp->ps_text[psp->ps_textsz] = '\0';
	}

# ifdef DEBUG2
	printf("<== text_handler()\n");
# endif
}


static void
el_start(void *data, const char *el, const char **attr)
{
	int i, el_idx;
	struct profile_state *psp = data;

# ifdef DEBUG2
	printf("==> el_start(\"%s\")\n", el);
# endif

# ifdef DEBUG_XML
	if (psp->ps_newline_state == PS_NS_LAST_WAS_TEXT)
		putchar('\n');
# endif

	for (el_idx = 0; profile_elements[el_idx].el_name != NULL; el_idx++)
		if (strcmp(profile_elements[el_idx].el_name, el) == 0)
			break;

	if (profile_elements[el_idx].el_name == NULL)
	{
		parse_error(psp->ps_parser, "unknown element \"%s\"", el);
		/* NOTREACHED */
	}

	if (psp->ps_depth == 0)
		i = PS_CTX_ROOT;
	else
		i = profile_elements[psp->ps_el_idx[psp->ps_depth]].el_context;
	if (! BIT_ISSET(profile_elements[el_idx].el_valid_context, i))
	{
		parse_error(psp->ps_parser,
			    "invalid context for element \"%s\"", el);
		/* NOTREACHED */
	}

# ifdef DEBUG_XML
	for (i = 0; i < psp->ps_depth; i++)
		putchar('\t');

	printf("<%s", el);
	for (i = 0; attr[i] != NULL; i += 2)
		printf(" %s=\"%s\"", attr[i], attr[i + 1]);
	printf(">\n");

	psp->ps_newline_state = PS_NS_SKIP_NEXT;
# endif

	psp->ps_depth++;
	psp->ps_el_idx[psp->ps_depth] = el_idx;

	if (profile_elements[el_idx].el_attr_handler != NULL)
		(*(profile_elements[el_idx].el_attr_handler))(psp, attr);

	psp->ps_textsz = 0;

# ifdef DEBUG2
	printf("<== el_start()\n");
# endif
}


static void
el_end(void *data, const char *el)
{
	int i;
	struct profile_state *psp = data;

# ifdef DEBUG2
	printf("==> el_end(\"%s\")\n", el);
# endif

# ifdef DEBUG_XML
	for (i = 0; i < psp->ps_depth; i++)
		putchar('\t');
	printf("</%s>\n", el);
	psp->ps_newline_state = PS_NS_SKIP_NEXT;
# endif

	if (psp->ps_text != NULL)
	{
		if (profile_elements[psp->ps_el_idx[psp->ps_depth]].el_text_handler != NULL)
		{
# ifdef DEBUG
			printf("<%s>\n\"%s\"\n</%s>\n", el, psp->ps_text, el);
# endif
			(*(profile_elements[psp->ps_el_idx[psp->ps_depth]].el_text_handler))(psp);
		}
		else
			free(psp->ps_text);
		psp->ps_text = NULL;
	}

	psp->ps_depth--;

# ifdef DEBUG2
	printf("<== el_end()\n");
# endif
}


static int
open_profile(char *filename, char *m4_cmd)
{
	char buf[10240];
	encap_listptr_t lp;

	/* make sure we can read the file */
	if (access(filename, R_OK) == -1)
		return -1;

	snprintf(buf, sizeof(buf),
		 "%s -P -DPLATFORM=%s " DATADIR "/mkencap.m4 %s",
		 m4_cmd, platform, filename);

	if (verbose > 1)
		printf("mkencap: preprocessing command is: \"%s\"\n", buf);

	return pipe_open(fileno(stdin), O_RDONLY, buf);
}


static jmp_buf env;

void
parse_error(void *handle, const char *fmt, ...)
{
	XML_Parser parser = *((XML_Parser *)handle);
	va_list args;
	const char *cp, *last, *next;
	int size, offset;

	fprintf(stderr, "mkencap: XML parse error near line %d: ",
		XML_GetCurrentLineNumber(parser));

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");

	cp = XML_GetInputContext(parser, &offset, &size);
	if (cp != NULL)
	{
		for (last = cp + offset;
		     last > cp && *last != '\n';
		     last--)
			;
		for (next = cp + offset;
		     next < (cp + size) && *next != '\n';
		     next++)
			;
		fprintf(stderr, "mkencap: offending line: \"%.*s\"\n",
			next - last - 1, last + 1);
	}

	longjmp(env, 1);
}


#endif /* HAVE_LIBEXPAT */


encap_profile_t *
parse_profile(char *filename, char *m4_cmd, char *m4_outfile)
{
#ifndef HAVE_LIBEXPAT
	printf("mkencap: not compiled with profile support\n");
	return NULL;
#else /* HAVE_LIBEXPAT */
	XML_Parser parser;
	char buf[1024];
	int ifd = -1, ofd = -1, done = 0;
	struct profile_state ps;
	size_t sz;
	encap_profile_t *retval = NULL;
	const char *cp, *last, *next;
	int offset, size, status;

	memset(&ps, 0, sizeof(ps));
# ifdef DEBUG_XML
	ps.ps_newline_state = PS_NS_INIT;
# endif

	if (verbose)
		printf("mkencap: reading profile \"%s\"\n", filename);

	parser = XML_ParserCreate(NULL);
	if (parser == NULL)
	{
		fprintf(stderr, "mkencap: XML_ParserCreate() failed\n");
		return NULL;
	}
	ps.ps_parser = &parser;

	ifd = open_profile(filename, m4_cmd);
	if (ifd == -1)
	{
		fprintf(stderr, "mkencap: open_profile(\"%s\"): %s\n",
			filename, strerror(errno));
		goto parse_done;
	}

	if (m4_outfile != NULL)
	{
		if (strcmp(m4_outfile, "-") == 0)
		{
			puts("mkencap: displaying preprocessed profile...");
			fflush(stdout);
			ofd = fileno(stdout);
		}
		else
		{
			ofd = open(m4_outfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
			if (ofd == -1)
			{
				fprintf(stderr, "mkencap: open(\"%s\"): %s\n",
					m4_outfile, strerror(errno));
				goto parse_done;
			}
		}
	}

	XML_SetElementHandler(parser, el_start, el_end);
	XML_SetCharacterDataHandler(parser, text_handler);
	XML_SetUserData(parser, &ps);

	while (!done)
	{
		sz = read(ifd, buf, sizeof(buf));
		if (sz == (size_t)-1)
		{
			fprintf(stderr, "mkencap: read(): %s\n",
				strerror(errno));
			goto parse_done;
		}

		/* if the file's empty, it's probably an m4 error */
		if (!done && sz == 0)
			break;

		done = (sz == 0);

		if (ofd != -1)
			write(ofd, buf, sz);
#ifdef DEBUG3
		write(1, buf, sz);
#endif

		if (setjmp(env) != 0)
			goto parse_done;

		if (XML_Parse(parser, buf, (int)sz, done) == 0)
		{
			parse_error(&parser,
				    XML_ErrorString(XML_GetErrorCode(parser)));
			/* NOTREACHED */
		}
	}

	/* make sure m4 succeeded */
	if (wait(&status) == (pid_t)-1)
	{
		fprintf(stderr, "mkencap: wait(): %s\n", strerror(errno));
		goto parse_done;
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
	{
		fprintf(stderr, "mkencap: cannot read profile: "
				"m4 failed with exit code %d\n",
			WEXITSTATUS(status));
		goto parse_done;
	}
	if (WIFSIGNALED(status))
	{
		fprintf(stderr, "mkencap: cannot read profile: "
				"m4 died with signal %d\n",
			WTERMSIG(status));
		goto parse_done;
	}

	/* everything's OK, so return the parsed profile */
	retval = ps.ps_profile;

  parse_done:
	XML_ParserFree(parser);
	if (ifd != -1)
		close(ifd);
	if (ofd != -1 && ofd != fileno(stdout))
		close(ofd);
	return retval;
#endif /* HAVE_LIBEXPAT */
}


