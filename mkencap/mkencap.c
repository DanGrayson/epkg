/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  mkencap.c - encap package creation tool
**
**  Mark D. Roth <roth@feep.net>
*/

#include <mkencap.h>

#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

#ifdef HAVE_LIBEXPAT
# include <expat.h>
#endif

#ifdef HAVE_LIBFGET
# include <libfget.h>
#endif

#ifdef HAVE_LIBCURL
# include <curl/curl.h>
#endif


/* modes */
#define MODE_BUILD		1
#define MODE_CREATE		2
#define MODE_ENCAPINFO		4


/* global settings */
char platform[MAXPATHLEN] = "";
short verbose = 1;
unsigned long mkencap_opts = 0;
encapinfo_t pkginfo;
char common_src_tree[MAXPATHLEN] = DEFAULT_COMMON_SRC_TREE;
char build_tree[MAXPATHLEN] = DEFAULT_BUILD_TREE;
char download_tree[MAXPATHLEN] = DEFAULT_DOWNLOAD_DIR;


static void
usage(void)
{
	fprintf(stderr, "Options: [-qvVf] [-s source] [-t target]\n");
	fprintf(stderr, "         [-E encap_format] [-p platform] "
		"[-A platform_suffix]\n");
	fprintf(stderr, "Usage:\n");
#ifdef HAVE_LIBEXPAT
	fprintf(stderr, "   mkencap [options] -b [-DUPCBIT] [-m m4_cmd] "
		"[-M m4_outfile] \\\n");
	fprintf(stderr, "           [-d download_dir] "
		"[-F environment_file] \\\n");
	fprintf(stderr, "           [-O build_tree] [-S src_tree] profile\n");
#endif
	fprintf(stderr, "   mkencap [options] \\\n");
	fprintf(stderr, "           [ -e [-n] "
		"[-a \"encapinfo_directive ...\"] ] \\\n");
	fprintf(stderr, "           [ -c [-o outfile] ] pkgspec\n");
	exit(1);
}


static void
print_defaults(void)
{
#ifdef HAVE_LIBEXPAT
	XML_Expat_Version expat_ver;
#endif

	printf("mkencap %s by Mark D. Roth <roth@feep.net>\n", PACKAGE_VERSION);
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
#ifdef HAVE_LIBEXPAT
	expat_ver = XML_ExpatVersionInfo();
	printf("   Builtin XML:\t\t\texpat-%d.%d.%d\n",
	       expat_ver.major,
	       expat_ver.minor,
	       expat_ver.micro);
#else
	printf("   Builtin XML:\t\t\tN/A\n");
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
	printf("   Default Download Dir:\t%s\n", DEFAULT_DOWNLOAD_DIR);
	printf("   Default Build Tree:\t\t%s\n", DEFAULT_BUILD_TREE);
	printf("   Default Common Source Tree:\t%s\n",
	       strlen(DEFAULT_COMMON_SRC_TREE) > 0
	       ? DEFAULT_COMMON_SRC_TREE
	       : "N/A");
	exit(0);
}


#ifdef HAVE_LIBEXPAT

static void
mkencap_init_paths(char *cwd, char *opt_download,
		   char *opt_common_src, char *opt_build)
{
	char buf[MAXPATHLEN];
	char *cp;

	/* initialize download dir */
	if (opt_download == NULL)
	{
		cp = getenv("MKENCAP_DOWNLOAD_DIR");
		if (cp != NULL)
			strlcpy(download_tree, cp, sizeof(download_tree));
	}
	else if (opt_download[0] != '/')
	{
		snprintf(buf, sizeof(buf), "%s/%s", cwd, opt_download);
		encap_cleanpath(buf, download_tree, sizeof(download_tree));
	}
	else
		strlcpy(download_tree, opt_download, sizeof(download_tree));

	/* initialize common src dir */
	if (opt_common_src == NULL)
	{
		cp = getenv("MKENCAP_SRC_TREE");
		if (cp != NULL)
			strlcpy(common_src_tree, cp, sizeof(common_src_tree));
	}
	else if (opt_common_src[0] != '/')
	{
		snprintf(buf, sizeof(buf), "%s/%s", cwd, opt_common_src);
		encap_cleanpath(buf, common_src_tree, sizeof(common_src_tree));
	}
	else
		strlcpy(common_src_tree, opt_common_src,
			sizeof(common_src_tree));

	/* initialize build dir */
	if (opt_build == NULL)
	{
		cp = getenv("MKENCAP_BUILD_TREE");
		if (cp != NULL)
			strlcpy(build_tree, cp, sizeof(build_tree));
	}
	else if (opt_build[0] != '/')
	{
		snprintf(buf, sizeof(buf), "%s/%s", cwd, opt_build);
		encap_cleanpath(buf, build_tree, sizeof(build_tree));
	}
	else
		strlcpy(build_tree, opt_build, sizeof(build_tree));

	/* replace "%p" with platform name in build_tree */
	strlcpy(buf, build_tree, sizeof(buf));
	encap_gsub(buf, "%p", platform, build_tree, sizeof(build_tree));
}

#endif /* HAVE_LIBEXPAT */


static int
mkencap_print_func(ENCAP *e, encap_source_info_t *srcinfo,
		   encap_target_info_t *tgtinfo, unsigned int type,
		   char *fmt, ...)
{
	va_list args;

	printf("mkencap: ");
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	return 0;
}


/* main() */
int
main(int argc, char *argv[])
{
	int i, exitval = 0;
	char pkgdir[MAXPATHLEN], cwd[MAXPATHLEN];
	char outfile[MAXPATHLEN] = "";
	char errbuf[1024];
	char *platform_suffix = NULL;
	char *opt_src = NULL, *opt_tgt = NULL;
	unsigned short mode = 0;
	char *pkgspec;
	ENCAP *e = NULL;
	encapinfo_t *eip;
#ifdef HAVE_LIBEXPAT
	char profile[MAXPATHLEN];
	unsigned long profile_modes = 0;
	encap_profile_t *epp;
	char *m4_cmd;
	char *opt_env_file = NULL, *opt_m4_outfile = NULL;
	char *opt_common_src = NULL, *opt_build = NULL, *opt_download = NULL;
#endif /* HAVE_LIBEXPAT */

	/* initialize encapinfo structure */
	encapinfo_init(&pkginfo);
	pkginfo.ei_pkgfmt = strdup("2.1");

#ifdef HAVE_LIBEXPAT
	/* initialize m4 command */
	m4_cmd = getenv("MKENCAP_M4_COMMAND");
	if (m4_cmd == NULL)
		m4_cmd = "m4";
#endif /* HAVE_LIBEXPAT */

	/* parse arguments */
	while ((i = getopt(argc, argv,
			   "a:A:bBcCd:DeE:fF:hIm:M:"
			   "no:O:p:Pqs:S:t:TUvV")) != -1)
	{
		switch (i)
		{
		case 'V':
			print_defaults();
			break;
		case 'v':
			verbose++;
			break;
		case 'q':
			verbose = 0;
			break;
		case 's':
			opt_src = optarg;
			break;
		case 't':
			opt_tgt = optarg;
			break;
		case 'f':
			BIT_TOGGLE(mkencap_opts, MKENCAP_OPT_OVERWRITE);
			break;
		case 'E':
			free(pkginfo.ei_pkgfmt);
			pkginfo.ei_pkgfmt = strdup(optarg);
			break;
		case 'p':
			strlcpy(platform, optarg, sizeof(platform));
			break;
		case 'A':
			platform_suffix = optarg;
			break;
		case 'c':
			BIT_SET(mode, MODE_CREATE);
			break;
		case 'e':
			BIT_SET(mode, MODE_ENCAPINFO);
			break;
#ifdef HAVE_LIBEXPAT
		case 'b':
			BIT_SET(mode, MODE_BUILD);
			break;
		case 'D':
			BIT_TOGGLE(profile_modes,
				   MKENCAP_PROFILE_DOWNLOAD);
			continue;
		case 'U':
			BIT_TOGGLE(profile_modes,
				   MKENCAP_PROFILE_UNPACK);
			continue;
		case 'P':
			BIT_TOGGLE(profile_modes,
				   MKENCAP_PROFILE_PATCH);
			continue;
		case 'C':
			BIT_TOGGLE(profile_modes,
				   MKENCAP_PROFILE_CONFIGURE);
			continue;
		case 'B':
			BIT_TOGGLE(profile_modes,
				   MKENCAP_PROFILE_BUILD);
			continue;
		case 'I':
			BIT_TOGGLE(profile_modes,
				   MKENCAP_PROFILE_INSTALL);
			continue;
		case 'T':
			BIT_TOGGLE(profile_modes,
				   MKENCAP_PROFILE_TIDY);
			continue;
		case 'O':
			opt_build = optarg;
			break;
		case 'S':
			opt_common_src = optarg;
			break;
		case 'm':
			m4_cmd = optarg;
			break;
		case 'd':
			opt_download = optarg;
			break;
		case 'M':
			opt_m4_outfile = optarg;
			break;
		case 'F':
			opt_env_file = optarg;
			break;
#endif /* HAVE_LIBEXPAT */
		case 'o':
			strlcpy(outfile, optarg, sizeof(outfile));
			break;
		case 'n':
			BIT_TOGGLE(mkencap_opts, MKENCAP_OPT_EMPTYENCAPINFO);
			break;
		case 'a':
			if (encapinfo_parse_directive(optarg, &pkginfo,
						      errbuf,
						      sizeof(errbuf)) == -1)
			{
				fprintf(stderr,
					"mkencap: invalid encapinfo directive "
					"\"%s\": %s\n", optarg, errbuf);
				usage();
				/* NOTREACHED */
			}
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	}

	/* set verbosity for download and archive modules */
	dl_verbose = verbose;
	archive_verbose = verbose;

	/* make sure our arguments are sane */
	if (argc - optind != 1)
	{
		usage();
		/* NOTREACHED */
	}

	if (mode == 0)
		BIT_SET(mode, MODE_CREATE | MODE_ENCAPINFO);

	/*
	** validate Encap package format version
	** and set platform name (needed before profile parsing)
	*/
	if (encap_vercmp(pkginfo.ei_pkgfmt, "2.0") == 0)
	{
		if (platform[0] == '\0')
			encap20_platform_name(platform, sizeof(platform));
	}
	else if (encap_vercmp(pkginfo.ei_pkgfmt, "2.1") == 0)
	{
		if (platform[0] == '\0')
			encap_platform_name(platform, sizeof(platform));
		if (platform_suffix != NULL)
		{
			strlcat(platform, "-", sizeof(platform));
			strlcat(platform, platform_suffix, sizeof(platform));
		}
	}
	else
	{
		fprintf(stderr, "mkencap: unsupported Encap package format "
			"version \"%s\"\n", pkginfo.ei_pkgfmt);
		exit(1);
	}

	/* get current directory */
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		fprintf(stderr, "mkencap: getcwd(): %s\n", strerror(errno));
		exit(1);
	}

	/* initialize global paths */
	init_encap_paths(cwd, opt_src, opt_tgt);

	if (verbose > 1)
		printf("### source directory:\t\t%s\n", source);

	/* before we actually write anything... */
	/* FIXME: should we really be doing this? */
	umask(022);

	/*
	** set contact field here, because $ENCAP_CONTACT won't be
	** available when we need it in build.c
	*/
	if (BIT_ISSET(mode, MODE_BUILD|MODE_ENCAPINFO)
	    && !BIT_ISSET(mkencap_opts, MKENCAP_OPT_EMPTYENCAPINFO))
	{
		mk_set_encapinfo_contact(&pkginfo);

		/* FIXME */
		pkginfo.ei_platform = strdup(platform);
	}

#ifdef HAVE_LIBEXPAT
	/* in build mode, read package profile to get pkgspec */
	if (BIT_ISSET(mode, MODE_BUILD))
	{
		if (profile_modes == 0)
			BIT_SET(profile_modes, MKENCAP_PROFILE_DOWNLOAD
					       | MKENCAP_PROFILE_UNPACK
					       | MKENCAP_PROFILE_PATCH
					       | MKENCAP_PROFILE_CONFIGURE
					       | MKENCAP_PROFILE_BUILD);

		mkencap_init_paths(cwd, opt_download, opt_common_src,
				   opt_build);

		if (verbose > 1)
		{
			printf("### target directory:\t\t%s\n", target);
			printf("### download directory:\t\t%s\n",
			       download_tree);
			printf("### common source tree:\t\t%s\n",
			       (common_src_tree[0] != '\0'
			        ? common_src_tree
			        : "N/A"));
			printf("### build tree:\t\t\t%s\n", build_tree);
			putchar('\n');
		}

		if (argv[optind][0] == '/')
			strlcpy(profile, argv[optind], sizeof(profile));
		else
			snprintf(profile, sizeof(profile), "%s/%s",
				 cwd, argv[optind]);
		epp = parse_profile(profile, m4_cmd, opt_m4_outfile);
		if (epp == NULL)
			exit(1);

		/* install mode creates the encapinfo file for us */
		if (BIT_ISSET(profile_modes, MKENCAP_PROFILE_INSTALL))
			BIT_CLEAR(mode, MODE_ENCAPINFO);

		/*
		** don't honor "-a" options in build mode
		** FIXME: this is really messy... 
		*/
		encap_list_empty(pkginfo.ei_ex_l, free);
		encap_list_empty(pkginfo.ei_rl_l, free);
		encap_list_empty(pkginfo.ei_ld_l, free);
		encap_list_empty(pkginfo.ei_pr_l, free);
		encap_hash_empty(pkginfo.ei_ln_h, free);

		if (build_package(epp, profile_modes, profile,
				  opt_env_file) != 0)
		{
			exitval = 1;
			goto mkencap_done;
		}

		pkgspec = epp->ep_pkgspec;
	}
	else
#endif /* HAVE_LIBEXPAT */
	{
		pkgspec = argv[optind];
		if (pkgspec[strlen(pkgspec) - 1] == '/')
			pkgspec[strlen(pkgspec) - 1] = '\0';
	}

	/*
	** set package directory (used by both encapinfo and create modes)
	*/
	snprintf(pkgdir, sizeof(pkgdir), "%s/%s", source, pkgspec);

	/* write encapinfo file */
	if (BIT_ISSET(mode, MODE_ENCAPINFO))
	{
		if (verbose > 1)
			printf("### Encap package format:\t%s\n\n",
			       pkginfo.ei_pkgfmt);

		if (mk_encapinfo(pkgdir) != 0)
		{
			exitval = 1;
			goto mkencap_done;
		}
	}

	/* create a package archive */
	if (BIT_ISSET(mode, MODE_CREATE))
	{
		/* set default archive file name */
		if (outfile[0] == '\0')
		{
			/*
			** if not in build or encapinfo mode, read
			** existing encapinfo file to get platform and
			** encap format version
			*/
			if (!BIT_ISSET(mode, MODE_BUILD)
			    && !BIT_ISSET(mode, MODE_ENCAPINFO))
			{
				if (encap_open(&e, source, target, pkgspec,
					       OPT_DEFAULTS,
					       mkencap_print_func) == -1)
				{
					fprintf(stderr, "mkencap: "
						"encap_open(\"%s\"): %s\n",
						pkgspec, strerror(errno));
					exitval = 1;
					goto mkencap_done;
				}

				eip = &e->e_pkginfo;
			}
			else
				eip = &pkginfo;

			/*
			** set default output filename
			** based on Encap package format version
			*/
			if (encap_vercmp(eip->ei_pkgfmt, "2.0") == 0)
			{
				snprintf(outfile, sizeof(outfile),
					 "%s.tar.gz",
					 pkgspec);
			}
			else	/* must be Encap 2.1, since we checked above */
			{
				snprintf(outfile, sizeof(outfile),
					 "%s-encap-%s.tar.gz", pkgspec,
					 (eip->ei_platform != NULL
					  ? eip->ei_platform
					  : platform));
			}

			if (e != NULL)
				encap_close(e);
		}

		if (verbose > 1)
			printf("### output file:\t\t%s\n\n", outfile);

		i = ARCHIVE_OPT_ENCAPINFO;
		if (BIT_ISSET(mkencap_opts, MKENCAP_OPT_OVERWRITE))
			i |= ARCHIVE_OPT_FORCE;

		if (archive_create(outfile, pkgdir, i) != 0)
			exitval = 1;
	}

  mkencap_done:
	encapinfo_free(&pkginfo);
	download_cleanup();
	exit(exitval);
}


