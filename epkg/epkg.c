/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  epkg.c - main driver program for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <epkg.h>

#include <stdio.h>
#include <sys/param.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_LIBTAR
# include <libtar.h>
#endif

#ifdef HAVE_LIBZ
# include <zlib.h>
#endif

#ifdef HAVE_LIBFGET
# include <libfget.h>
#endif

#ifdef HAVE_LIBCURL
# include <curl/curl.h>
#endif


/* global variables */
char platform[MAXPATHLEN];
unsigned short verbose = 1;
unsigned long options = OPT_DEFAULTS;
unsigned long epkg_opts = (EPKG_OPT_WRITELOG | EPKG_OPT_VERSIONING | EPKG_OPT_UPDATEALLDIRS);
encap_list_t *exclude_l = NULL;
encap_list_t *override_l = NULL;
encap_list_t *platform_suffix_l = NULL;


static void
usage(void)
{
	fprintf(stderr, "Options: [-1aCdDEfFlLnNopPqRSTvVx] "
		"[-s source] [-t target]\n");
	fprintf(stderr, "         [-O overrides] [-X excludes] "
		"[-U update_dir]\n");
	fprintf(stderr, "         [-H host_platform] [-A platform_suffix]\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   epkg [options] [-i] pkgspec ...\n");
	fprintf(stderr, "   epkg [options] -r pkgspec ...\n");
	fprintf(stderr, "   epkg [options] -k pkgspec ...\n");
	fprintf(stderr, "   epkg [options] -u [pkgspec ...]\n");
	fprintf(stderr, "   epkg [options] -b\n");
	fprintf(stderr, "   epkg [options] -c\n");
	exit(1);
}


static void
print_defaults(void)
{
	printf("epkg %s by Mark D. Roth <roth@feep.net>\n", PACKAGE_VERSION);
	printf("   Build Platform:\t\t%s\n", PLATFORM);
#ifdef HAVE_LIBTAR
	printf("   Builtin tar:\t\t\tlibtar-%s\n", libtar_version);
#else
	printf("   Builtin tar:\t\t\tN/A\n");
#endif
#ifdef HAVE_LIBZ
	printf("   Builtin gzip:\t\tzlib-%s\n", ZLIB_VERSION);
#else
	printf("   Builtin gzip:\t\tN/A\n");
#endif
#ifdef HAVE_LIBFGET
	printf("   Builtin FTP:\t\t\tlibfget-%s\n", libfget_version);
#else
	printf("   Builtin FTP:\t\t\tN/A\n");
#endif
#ifdef HAVE_LIBCURL
	printf("   Builtin HTTP:\t\tlibcurl-%s\n", LIBCURL_VERSION);
#else
	printf("   Builtin HTTP:\t\tN/A\n");
#endif
	printf("   Default Encap Source:\t%s\n", DEFAULT_SOURCE);
	printf("   Default Encap Target:\t%s\n", DEFAULT_TARGET);
	printf("   Default Exclude List:\t%s\n", DEFAULT_EXCLUDES);
	printf("   Default Override List:\t%s\n", DEFAULT_OVERRIDES);
	exit(0);
}


/* main() */
int
main(int argc, char *argv[])
{
	char buf[MAXPATHLEN], cwd[MAXPATHLEN];
	encap_listptr_t lp;
	short pkg_mode = 0;
	int i, j, exit_val = 0;
	char *opt_src = NULL, *opt_tgt = NULL;

	/* initialize exclude list */
	exclude_l = encap_list_new(LIST_USERFUNC, NULL);
	encap_list_add_str(exclude_l, DEFAULT_EXCLUDES, ":");

	/* initialize override list */
	override_l = encap_list_new(LIST_USERFUNC, NULL);
	encap_list_add_str(override_l, DEFAULT_OVERRIDES, ":");

	/* initialize update directory list */
	if (update_path_init() == -1)
	{
		perror("malloc()");
		exit(1);
	}

	/* initialize compatible platform suffix list */
	encap_platform_name(platform, sizeof(platform));
	platform_suffix_l = encap_list_new(LIST_QUEUE, NULL);

	/* parse arguments */
	while ((i = getopt(argc, argv,
			   "1aA:bcCdDEfFhH:ikKlL"
			   "nNoO:pPqrRs:St:TuU:vVxX:")) != -1)
	{
		switch (i)
		{
		case 'V':
			print_defaults();
		case 'u':
			if (pkg_mode)
				usage();
			pkg_mode = MODE_UPDATE;
			BIT_SET(epkg_opts, EPKG_OPT_VERSIONING);
			BIT_CLEAR(epkg_opts, EPKG_OPT_BACKOFF);
			break;
		case 'b':
			if (pkg_mode)
				usage();
			pkg_mode = MODE_BATCH;
			break;
		case 'i':
			if (pkg_mode)
				usage();
			pkg_mode = MODE_INSTALL;
			break;
		case 'r':
			if (pkg_mode)
				usage();
			pkg_mode = MODE_REMOVE;
			break;
		case 'k':
			if (pkg_mode)
				usage();
			pkg_mode = MODE_CHECK;
			break;
		case 'c':
			if (pkg_mode)
				usage();
			pkg_mode = MODE_CLEAN;
			break;
		case 'q':
			verbose = 0;
			break;
		case 'v':
			verbose++;
			break;
		case 'S':
			BIT_TOGGLE(epkg_opts, EPKG_OPT_VERSIONING);
			break;
		case '1':
			BIT_TOGGLE(epkg_opts, EPKG_OPT_BACKOFF);
			break;
		case 'l':
			BIT_TOGGLE(epkg_opts, EPKG_OPT_WRITELOG);
			break;
		case 'K':
			BIT_TOGGLE(epkg_opts, EPKG_OPT_UPDATEALLDIRS);
			break;
		case 'F':
			BIT_TOGGLE(dl_opts, DL_OPT_DEBUG);
			break;
		case 'T':
			BIT_TOGGLE(dl_opts, DL_OPT_CONNRETRY);
			break;
		case 'E':
			BIT_TOGGLE(epkg_opts, EPKG_OPT_OLDEXCLUDES);
			BIT_TOGGLE(options, OPT_TARGETEXCLUDES);
			break;
		case 'L':
			BIT_TOGGLE(options, OPT_LINKNAMES);
			break;
		case 'd':
			BIT_TOGGLE(options, OPT_NUKETARGETDIRS);
			break;
		case 'D':
			BIT_TOGGLE(options, OPT_LINKDIRS);
			break;
		case 'f':
			BIT_TOGGLE(options, OPT_FORCE);
			break;
		case 'n':
			BIT_TOGGLE(options, OPT_SHOWONLY);
			break;
		case 'a':
			BIT_TOGGLE(options, OPT_ABSLINKS);
			break;
		case 'R':
			BIT_TOGGLE(options, OPT_RUNSCRIPTS);
			break;
		case 'C':
			BIT_TOGGLE(options, OPT_RUNSCRIPTSONLY);
			break;
		case 'x':
			BIT_TOGGLE(options, OPT_EXCLUDES);
			break;
		case 'p':
			BIT_TOGGLE(options, OPT_PREREQS);
			break;
		case 'P':
			BIT_TOGGLE(options, OPT_PKGDIRLINKS);
			break;
		case 'N':
			encap_list_free(exclude_l, free);
			exclude_l = encap_list_new(LIST_USERFUNC, NULL);
			break;
		case 'H':
			strlcpy(platform, optarg, sizeof(platform));
			break;
		case 'A':
			encap_list_add(platform_suffix_l, strdup(optarg));
			break;
		case 'X':
			encap_list_add(exclude_l, strdup(optarg));
			break;
		case 'o':
			encap_list_free(override_l, free);
			override_l = encap_list_new(LIST_USERFUNC, NULL);
			break;
		case 'O':
			encap_list_add(override_l, strdup(optarg));
			break;
		case 'U':
			update_path_add(optarg, 1);
			break;
		case 's':
			opt_src = strdup(optarg);
			break;
		case 't':
			opt_tgt = strdup(optarg);
			break;
		case 'h':
		default:
			usage();
		}
	}

	/* set verbosity for download and archive modules */
	dl_verbose = verbose;
	archive_verbose = verbose;

	/* set default mode if not specified */
	if (pkg_mode == 0)
		pkg_mode = MODE_INSTALL;

	/* make sure we have the right number of arguments for this mode */
	i = argc - optind;
	switch (pkg_mode)
	{
	case MODE_UPDATE:
		/* arguments are optional in update mode */
		break;
	case MODE_CHECK:
	case MODE_REMOVE:
	case MODE_INSTALL:
		/* need arguments for install, remove, and check modes */
		if (i < 1)
			usage();
		break;
	case MODE_CLEAN:
	case MODE_BATCH:
		/* don't accept arguments for batch or clean modes */
		if (i >= 1)
			usage();
		break;
	}

	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		fprintf(stderr, "epkg: getcwd(): %s\n", strerror(errno));
		exit(1);
	}
	init_encap_paths(cwd, opt_src, opt_tgt);

	/* if source dir is under target dir, add it to the exclude list */
	encap_relativepath(target, source, buf, sizeof(buf));
	if (strncmp(buf, "..", 2) != 0)
		encap_list_add(exclude_l, strdup(buf));

	/* print out our options */
	if (verbose > 3)
	{
		printf("epkg: printing options:\n");
		printf("  * source directory:\t\t%s\n", source);
		printf("  * target directory:\t\t%s\n", target);
		printf("  * update path:\t\t");
		update_path_print();
		putchar('\n');
		printf("  * global exclude list:\t");
		encap_listptr_reset(&lp);
		while (encap_list_next(exclude_l, &lp) != 0)
			printf("%s%s", (exclude_l->first == lp ? "" : ":"),
			       (char *)encap_listptr_data(&lp));
		if (exclude_l->first == NULL)
			printf("N/A");
		putchar('\n');
		printf("  * override list:\t\t");
		encap_listptr_reset(&lp);
		while (encap_list_next(override_l, &lp) != 0)
			printf("%s%s", (override_l->first == lp ? "" : ":"),
			       (char *)encap_listptr_data(&lp));
		if (override_l->first == NULL)
			printf("N/A");
		putchar('\n');
		printf("  * host platform:\t\t%s\n", platform);
		printf("  * platform suffix list:\t");
		encap_listptr_reset(&lp);
		while (encap_list_next(platform_suffix_l, &lp) != 0)
			printf("%s%s",
			       (platform_suffix_l->first == lp ? "" : ":"),
			       (char *)encap_listptr_data(&lp));
		if (platform_suffix_l->first == NULL)
			printf("N/A");
		putchar('\n');
		printf("  * perform logging:\t\t%s\n",
		       (BIT_ISSET(epkg_opts, EPKG_OPT_WRITELOG) ? "yes" :
			"no"));
		printf("  * handle multiple versions:\t%s\n",
		       (BIT_ISSET(epkg_opts, EPKG_OPT_VERSIONING) ? "yes" :
			"no"));
		printf("  * default version:\t\t%slatest\n",
		       (BIT_ISSET(epkg_opts, EPKG_OPT_BACKOFF) ? "second-" :
			""));
		printf("  * update debug output:\t%s\n",
		       (BIT_ISSET(dl_opts, DL_OPT_DEBUG) ? "yes" :
			"no"));
		printf("  * read legacy excludes:\t%s\n",
		       (BIT_ISSET(epkg_opts, EPKG_OPT_OLDEXCLUDES) ? "yes" :
			"no"));
		printf("  * link style:\t\t\t%s\n",
		       (BIT_ISSET(options, OPT_ABSLINKS) ? "absolute" :
			"relative"));
		printf("  * check package prereqs:\t%s\n",
		       (BIT_ISSET(options, OPT_PREREQS) ? "yes" : "no"));
		printf("  * run package scripts:\t%s\n",
		       (BIT_ISSET(options, OPT_RUNSCRIPTS) ? "yes" : "no"));
		printf("  * honor package exclusions:\t%s\n",
		       (BIT_ISSET(options, OPT_EXCLUDES) ? "yes" : "no"));
		printf("  * link files in pkg dir:\t%s\n",
		       (BIT_ISSET(options, OPT_PKGDIRLINKS) ? "yes" : "no"));
		printf("  * remove empty target dirs:\t%s\n",
		       (BIT_ISSET(options, OPT_NUKETARGETDIRS) ? "yes" :
			"no"));
		printf("  * force mode:\t\t\t%s\n",
		       (BIT_ISSET(options, OPT_FORCE) ? "yes" : "no"));
		printf("  * display actions only:\t%s\n",
		       (BIT_ISSET(options, OPT_SHOWONLY) ? "yes" : "no"));
		printf("  * process package links:\t%s\n",
		       (BIT_ISSET(options, OPT_RUNSCRIPTSONLY) ? "no" :
			"yes"));
		printf("  * package mode:\t\t");
		switch (pkg_mode)
		{
		case MODE_UPDATE:
			printf("update");
			break;
		case MODE_INSTALL:
			printf("install");
			break;
		case MODE_REMOVE:
			printf("remove");
			break;
		case MODE_BATCH:
			printf("batch");
			break;
		case MODE_CHECK:
			printf("check");
			break;
		case MODE_CLEAN:
			printf("clean");
			break;
		}
		printf("\n\n");
	}

	switch (pkg_mode)
	{

	case MODE_CLEAN:
		exit_val = clean_mode();
		break;

	case MODE_UPDATE:
		if (!update_path_isset())
		{
			fprintf(stderr, "epkg: no update path set\n");
			exit(1);
		}
		if (putenv("ENCAP_MODE=update") != 0)
		{
			perror("putenv()");
			exit(1);
		}
		if (argc - optind == 0)
		{
			exit_val = update_mode(NULL);
			break;
		}
		for (i = 0; i < argc - optind; i++)
		{
			/* download and/or untar the new package */
			j = update_mode(argv[optind + i]);
			if (j)
			{
				exit_val++;
				if (j == -1)
					break;
			}
		}
		break;

	case MODE_CHECK:
		for (i = 0; i < argc - optind; i++)
		{
			strcpy(buf, basename(argv[optind + i]));
			if (check_mode(buf) != 0)
				exit_val++;
			if (verbose > 1)
				putchar('\n');
		}
		break;

	case MODE_INSTALL:
		if (putenv("ENCAP_MODE=install") != 0)
		{
			perror("putenv()");
			exit(1);
		}
		for (i = 0; i < argc - optind; i++)
		{
			j = install_mode(argv[optind + i]);
			if (j != 0)
			{
				exit_val++;
				if (j == -1)
					break;
			}
			if (verbose > 1)
				putchar('\n');
		}
		break;

	case MODE_REMOVE:
		if (putenv("ENCAP_MODE=remove") != 0)
		{
			perror("putenv()");
			exit(1);
		}
		for (i = 0; i < argc - optind; i++)
		{
			strcpy(buf, basename(argv[optind + i]));
			if (remove_mode(buf) != 0)
				exit_val++;
			if (verbose > 1)
				putchar('\n');
		}
		break;

	case MODE_BATCH:
		if (putenv("ENCAP_MODE=batch") != 0)
		{
			perror("putenv()");
			exit(1);
		}
		exit_val = batch_mode();
		if (verbose > 1)
			putchar('\n');
		break;

	} /* switch() */

	/* clean up */
	download_cleanup();
	update_path_free();

	exit(exit_val);
}


