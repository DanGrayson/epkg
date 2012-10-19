/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  download.c - download code for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <download.h>
#include <compat.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <netdb.h>
#include <pwd.h>
#include <dirent.h>

/*
** FIXME: need to find a better way to do this...
** work-around for conflicting definitions in <libgen.h> and <regex.h>
*/
#define regex dummy_regex
#define regcmp dummy_regcmp
#include <regex.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else
# include <time.h>
#endif

#ifdef HAVE_LIBFGET
# include <libfget.h>
#endif

#ifdef HAVE_LIBCURL
# include <curl/curl.h>
#endif


#ifndef MAXUSERNAMELEN
# define MAXUSERNAMELEN		16	/* max length of user name */
#endif

#ifndef MAXURLLEN
# define MAXURLLEN	(sizeof("https://") \
			 + MAXHOSTNAMELEN \
			 + MAXUSERNAMELEN \
			 + MAXPATHLEN)
#endif


int dl_verbose = 0;
unsigned long dl_opts = DL_OPT_CONNRETRY;


/*************************************************************************
 ***  update handle cache code
 *************************************************************************/

typedef int (*dc_closefunc_t)(void *);

struct download_connection
{
	char dc_label[MAXURLLEN];
	void *dc_handle;
	dc_closefunc_t dc_closefunc;
};
typedef struct download_connection download_connection_t;

static encap_hash_t *download_connections_h = NULL;


/*
** domain_of() - find TLD componant of an FQDN
*/
static char *
domain_of(char *host)
{
	char *cp;
	int numdots = 0;

	cp = host + strlen(host) - 1;
	if (*cp == '.')
		cp--;

	for (; numdots < 2 && cp > host; cp--)
	{
		if (*cp == '.')
			numdots++;
	}

	if (cp != host)
		cp += 2;

	return cp;
}


static int
dc_hashfunc(download_connection_t *dcp, int numbuckets)
{
	char *cp;

	cp = strstr(dcp->dc_label, "://");
	if (cp == NULL)
		cp = dcp->dc_label;
	else
		cp = domain_of(cp + 3);

	return (*cp % numbuckets);
}


static int
download_connection_register(char *label, void *handle,
			     dc_closefunc_t closefunc)
{
	download_connection_t *dcp;

	dcp = (download_connection_t *)malloc(sizeof(download_connection_t));
	if (dcp == NULL)
		return -1;

	strlcpy(dcp->dc_label, label, sizeof(dcp->dc_label));
	dcp->dc_handle = handle;
	dcp->dc_closefunc = closefunc;

	if (download_connections_h == NULL)
	{
		download_connections_h = encap_hash_new(128,
						(encap_hashfunc_t)dc_hashfunc);
		if (download_connections_h == NULL)
			return -1;
	}

	encap_hash_add(download_connections_h, dcp);

	return 0;
}


static void *
download_connection_get(char *label)
{
	encap_hashptr_t hp;
	download_connection_t *dcp;

	if (download_connections_h == NULL)
	{
		download_connections_h = encap_hash_new(128,
						(encap_hashfunc_t)dc_hashfunc);
		if (download_connections_h == NULL)
			return NULL;
	}

	encap_hashptr_reset(&hp);
	if (encap_hash_getkey(download_connections_h, &hp, label,
			      (encap_matchfunc_t)encap_str_match) != 0)
	{
		dcp = (download_connection_t *)encap_hashptr_data(&hp);
		return dcp->dc_handle;
	}

	return NULL;
}


static int
download_connection_close(char *label, int call_close)
{
	encap_hashptr_t hp;
	download_connection_t *dcp;
	int ret = 0;

	encap_hashptr_reset(&hp);

	if (download_connections_h == NULL
	    || encap_hash_getkey(download_connections_h, &hp, label,
				 (encap_matchfunc_t)encap_str_match) == 0)
	{
		errno = ENOENT;
		return -1;
	}

	dcp = (download_connection_t *)encap_hashptr_data(&hp);
	encap_hash_del(download_connections_h, &hp);
	if (call_close)
		ret = dcp->dc_closefunc(dcp->dc_handle);
	free(dcp);

	return ret;
}


void
download_cleanup(void)
{
	encap_hashptr_t hp;
	download_connection_t *dcp;

	if (download_connections_h == NULL)
		return;

	encap_hashptr_reset(&hp);
	while (encap_hash_next(download_connections_h, &hp) != 0)
	{
		dcp = (download_connection_t *)encap_hashptr_data(&hp);
		dcp->dc_closefunc(dcp->dc_handle);
	}
}


/*
** utility function to return a file descriptor for a tmpfile
*/
static int
download_tmpfile(char *outfile)
{
	int fd;
	char *cp, path[MAXPATHLEN];

	if (outfile == NULL)
	{
		cp = getenv("TMPDIR");
		if (cp == NULL)
			cp = "/tmp";

		snprintf(path, sizeof(path), "%s/download-XXXXXX", cp);
		fd = mkstemp(path);
		if (fd == -1)
		{
			fprintf(stderr, "  ! cannot open temp file: "
				"mkstemp(\"%s\"): %s\n",
				path, strerror(errno));
			return -1;
		}
		unlink(path);
	}
	else
	{
		fd = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if (fd == -1)
		{
			fprintf(stderr, "  ! cannot open file: "
				"open(\"%s\"): %s\n",
				outfile, strerror(errno));
			return -1;
		}
	}

	return fd;
}


/*************************************************************************
 ***  progress indicator function
 *************************************************************************/

#define DL_PROGRESS_INTERVAL		1024

struct dl_progress
{
	size_t dp_bytecount;
	size_t dp_nextreport;
	size_t dp_total;
	struct timeval dp_start;
};
typedef struct dl_progress dl_progress_t;


static void
progress_init(dl_progress_t *dpp, size_t total)
{
	memset(dpp, 0, sizeof(dl_progress_t));
	gettimeofday(&(dpp->dp_start), NULL);
	dpp->dp_total = total;
}


static void
progress_update(dl_progress_t *dpp, size_t add_bytes)
{
	dpp->dp_bytecount += add_bytes;

	if (dl_verbose
	    && isatty(fileno(stdout))
	    && dpp->dp_bytecount > dpp->dp_nextreport)
	{
		printf("    * received %lu", (unsigned long)dpp->dp_bytecount);
		if (dpp->dp_total != 0)
			printf("/%lu", (unsigned long)dpp->dp_total);
		printf(" bytes\r");
		fflush(stdout);
		dpp->dp_nextreport += DL_PROGRESS_INTERVAL;
	}
}


static void
progress_done(dl_progress_t *dpp)
{
	float elapsed;
	struct timeval finish;

	gettimeofday(&finish, NULL);
	elapsed = (finish.tv_sec - dpp->dp_start.tv_sec) +
			((finish.tv_usec - dpp->dp_start.tv_usec) / 1000000.0);
	printf("    * received %lu bytes in %.2f seconds (%.2f K/s)\n",
	       (unsigned long)dpp->dp_bytecount, elapsed,
	       ((float)(dpp->dp_bytecount / 1024) / elapsed));
}


/*************************************************************************
 ***  HTTP update module code
 *************************************************************************/

#ifdef HAVE_LIBCURL

static CURL *curl = NULL;
static char curl_error[CURL_ERROR_SIZE];
static regex_t html_link_regexp;

struct curl_dir_state
{
	char *cs_dir_url;
	dir_entry_func_t cs_dirfunc;
	void *cs_state;
};
typedef struct curl_dir_state curl_dir_state_t;

struct curl_file_state
{
	int cfs_fd;
	dl_progress_t cfs_dp;
};
typedef struct curl_file_state curl_file_state_t;


static int
curl_cleanup(CURL *curl)
{
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	curl = NULL;
	regfree(&html_link_regexp);
	return 0;
}


static int
curl_init(void)
{
	char buf[1024];
	int i;

	i = regcomp(&html_link_regexp,
		    "<[ \t\n]*A[ \t\n]+HREF=\"?([^\">]+)\"?>",
		    REG_EXTENDED|REG_ICASE);
	if (i != 0)
	{
		regerror(i, &html_link_regexp, buf, sizeof(buf));
		fprintf(stderr, "regcomp(): %s\n", buf);
		return -1;
	}

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

	download_connection_register("curl", curl,
				     (dc_closefunc_t)curl_cleanup);

	return 0;
}


static size_t
curl_dir_parse(void *buffer, size_t size, size_t nmemb, void *userp)
{
	regmatch_t regex_matches[3];
	char path[MAXURLLEN], url[MAXURLLEN];
	char errbuf[1024];
	static char *save = NULL;
	char *currentp, *startp, *cp;
	size_t buflen = 0;
	int i;
	curl_dir_state_t *csp = (curl_dir_state_t *)userp;

# ifdef DEBUG
	printf("==> curl_read(buffer=0x%lx, size=%lu, nmemb=%lu, "
	       "userp=0x%lx)\n", buffer, size, nmemb, userp);
	printf("buffer = \"%s\"\n", buffer);
# endif

	if (save != NULL)
	{
		buflen = strlen(save) + (size * nmemb) + 1;
		startp = (char *)malloc(buflen);
		strlcpy(startp, save, buflen);
		strlcat(startp, buffer, buflen);
		currentp = startp;
		free(save);
		save = NULL;
	}
	else
	{
		startp = buffer;
		currentp = buffer;
	}

	while ((i = regexec(&html_link_regexp, currentp,
			    3, regex_matches, 0)) == 0)
	{
		snprintf(path, sizeof(path), "%.*s",
			 regex_matches[1].rm_eo - regex_matches[1].rm_so,
			 currentp + regex_matches[1].rm_so);

		/* canonify relative URLs */
		/* FIXME: this needs to be more rigorous, as per RFC 2396 */
		if (strstr(path, "://") == NULL)
		{
			strlcpy(url, csp->cs_dir_url, sizeof(url));

			/* strip query string from base URI */
			cp = strrchr(url, '?');
			if (cp != NULL)
				*cp = '\0';

			/* strip last path component */
			cp = strrchr(url, '/');
			if (cp != NULL && cp != (url + strlen(url) - 1))
			{
				if (path[0] != '/')
					cp++;
				*cp = '\0';
			}

			/* append relative URI */
			strlcat(url, path, sizeof(url));
		}
		else
			strlcpy(url, path, sizeof(url));

# ifdef DEBUG
		printf("\t[%s]\n", url);
# endif

		if ((*(csp->cs_dirfunc))(url, csp->cs_state) == -1)
			return -1;

		currentp += regex_matches[0].rm_eo;
	}
	if (i != REG_NOMATCH)
	{
		regerror(i, &html_link_regexp, errbuf, sizeof(errbuf));
		fprintf(stderr, "  ! regcomp(): %s\n", errbuf);
		return -1;
	}

	currentp = strrchr(startp, '<');
	if (currentp != NULL
	    && strchr(currentp, '>') == NULL)
		save = strdup(currentp);

	if (buflen > 0)
		free(startp);

	return (size * nmemb);
}


static int
http_scan_dir(char *dir, dir_entry_func_t dirfunc, void *state)
{
	CURLcode status;
	curl_dir_state_t cs;

# ifdef DEBUG
	printf("==> http_scan_dir(\"%s\")\n", download_dir);
# endif

	cs.cs_dir_url = dir;
	cs.cs_dirfunc = dirfunc;
	cs.cs_state = state;

	if (curl == NULL
	    && curl_init() == -1)
		return -1;

	curl_easy_setopt(curl, CURLOPT_URL, dir);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_dir_parse);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cs);

	status = curl_easy_perform(curl);
	if (status != 0)
	{
		fprintf(stderr, "  ! curl_easy_perform(): %s\n", curl_error);
		return -1;
	}

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);

	return 0;
}


static int
curl_download(void *buffer, size_t size, size_t nmemb, void *userp)
{
	curl_file_state_t *cfsp = (curl_file_state_t *)userp;

	progress_update(&(cfsp->cfs_dp), (size * nmemb));

	return write(cfsp->cfs_fd, buffer, (size * nmemb));
}


/* download a given filename */
static int
http_download(char *url, char *outfile)
{
	CURLcode status;
	curl_file_state_t cfs;

	if (curl == NULL
	    && curl_init() == -1)
		return -1;

	printf("  > downloading %s...\n", url);

	curl_easy_setopt(curl, CURLOPT_URL, url);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_download);

	cfs.cfs_fd = download_tmpfile(outfile);
	if (cfs.cfs_fd == -1)
		return -1;
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&cfs);

	progress_init(&(cfs.cfs_dp), 0);

	status = curl_easy_perform(curl);
	if (status != 0)
	{
		fprintf(stderr, "  ! curl_easy_perform(): %s\n", curl_error);
		return -1;
	}

	progress_done(&(cfs.cfs_dp));

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);

	if (outfile != NULL)
	{
		close(cfs.cfs_fd);
		return 0;
	}

	if (lseek(cfs.cfs_fd, 0, SEEK_SET) == -1)
	{
		fprintf(stderr, "  ! lseek(): %s\n", strerror(errno));
		return -1;
	}
	return cfs.cfs_fd;
}

#endif /* HAVE_LIBCURL */


/*************************************************************************
 ***  FTP update module code
 *************************************************************************/

#ifdef HAVE_LIBFGET

static void
send_debug(char *text, FTP *ftp, void *dummy)
{
	printf(">>> %s\n", text);
}


static void
recv_debug(char *text, FTP *ftp, void *dummy)
{
	printf("<<< %s\n", text);
}


static int
ftp_closefunc(FTP *ftp)
{
	return ftp_quit(ftp, 0);
}


static FTP *
ftp_get_handle(char *download_dir, struct ftp_url *ftpurl,
	       char *dc_key, size_t dc_key_len)
{
	FTP *ftp = NULL;
	struct passwd *pwd;
	char *cp;
	int i;
	char banner[1024];

	ftp_url_parse(download_dir, ftpurl);
	if (ftpurl->fu_login[0] == '\0')
	{
		strlcpy(ftpurl->fu_login, "ftp", sizeof(ftpurl->fu_login));
		if (ftpurl->fu_passwd[0] == '\0')
		{
			pwd = getpwuid(getuid());
			i = snprintf(ftpurl->fu_passwd,
				     sizeof(ftpurl->fu_passwd),
				     "%s@", pwd->pw_name);
			gethostname(ftpurl->fu_passwd + i,
				    sizeof(ftpurl->fu_passwd) - i);
			ftpurl->fu_passwd[sizeof(ftpurl->fu_passwd) - 1] = '\0';
		}
	}

	snprintf(dc_key, dc_key_len, "ftp://%s@%s/",
		 ftpurl->fu_login, ftpurl->fu_hostname);

	ftp = (FTP *)download_connection_get(dc_key);

	if (ftp == NULL)
	{
# ifdef DEBUG
		printf("    ftp_get_handle(): creating new connection for %s\n",
		       download_dir);
# endif

		if (ftp_connect(&ftp, ftpurl->fu_hostname,
				banner, sizeof(banner),
				FTP_CONNECT_DNS_RR,
				FTP_OPT_SEND_HOOK,
				(BIT_ISSET(dl_opts, DL_OPT_DEBUG)
				 ? send_debug : NULL),
				FTP_OPT_RECV_HOOK,
				(BIT_ISSET(dl_opts, DL_OPT_DEBUG)
				 ? recv_debug : NULL),
				0) == -1)
		{
			fprintf(stderr, "  ! ftp_connect(): %s\n",
				strerror(errno));
			return NULL;
		}

		if (ftpurl->fu_passwd[0] == '\0' && isatty(fileno(stdin)))
			cp = getpass("Password: ");
		else
			cp = ftpurl->fu_passwd;
		if (ftp_login(ftp, ftpurl->fu_login, cp) == -1)
		{
			fprintf(stderr, "  ! ftp_login(): %s\n",
				strerror(errno));
			return NULL;
		}

		download_connection_register(dc_key, ftp,
					   (dc_closefunc_t)ftp_closefunc);
	}

	return ftp;
}

	
static int
ftp_scan_dir(char *dir, dir_entry_func_t dirfunc, void *state)
{
	FTP *ftp;
	struct ftp_url ftpurl;
	FTPDIR *ftpdir;
	struct ftpdirent fde;
	struct ftpstat fs;
	char pathname[MAXPATHLEN];
	char url[MAXURLLEN];
	char dc_key[MAXURLLEN];

#ifdef DEBUG
	printf("==> ftp_scan_dir(\"%s\")\n", dir);
#endif

	ftp = ftp_get_handle(dir, &ftpurl, dc_key, sizeof(dc_key));
	if (ftp == NULL)
		return -1;

	if (ftp_opendir(&ftpdir, ftp, ftpurl.fu_path) == -1)
	{
		fprintf(stderr, "  ! ftp_opendir(\"%s\"): %s\n", ftpurl.fu_path,
			strerror(errno));
		return -1;
	}

	while (ftp_readdir(ftpdir, &fde) == 1)
	{
# ifdef DEBUG
		printf("    ftp_update(): fde.fd_name=\"%s\"\n", fde.fd_name);
# endif

		strlcpy(pathname, ftpurl.fu_path, sizeof(pathname));
		if (pathname[strlen(pathname) - 1] != '/')
			strlcat(pathname, "/", sizeof(pathname));
		strlcat(pathname, fde.fd_name, sizeof(pathname));

		if (ftp_stat(ftp, pathname, &fs) == -1)
		{
			fprintf(stderr, "  ! ftp_stat(\"%s\"): %s\n",
				pathname, strerror(errno));
			ftp_closedir(ftpdir);
			return -1;
		}

		if (S_ISDIR(fs.fs_mode))
			continue;

		strlcpy(url, dir, sizeof(url));
		if (url[strlen(url) - 1] != '/')
			strlcat(url, "/", sizeof(url));
		strlcat(url, fde.fd_name, sizeof(url));

		if ((*dirfunc)(url, state) == -1)
			break;
	}

	ftp_closedir(ftpdir);
	return 0;
}


/* download a given filename */
static int
ftp_download(char *url, char *outfile)
{
	FTP *ftp;
	struct ftp_url ftpurl;
	FTPFILE *ftpfile = NULL;
	char dc_key[MAXURLLEN];
	int i, j, fd;
	char buf[1024];
	struct ftpstat fs;
	dl_progress_t dp;

# ifdef DEBUG
	printf("==> ftp_download(url=\"%s\")\n", url);
# endif

	printf("  > downloading %s...\n", url);

	/* create output file */
	fd = download_tmpfile(outfile);
	if (fd == -1)
		return -1;

ftp_download_retry:
	ftp = ftp_get_handle(url, &ftpurl, dc_key, sizeof(dc_key));
	if (ftp == NULL)
		goto ftp_download_fail;

	if (ftp_stat(ftp, ftpurl.fu_path, &fs) != 0)
	{
		fprintf(stderr, "  ! ftp_stat(\"%s\"): %s\n", ftpurl.fu_path,
			strerror(errno));
		goto ftp_download_fail;
	}

	if (ftp_open(&ftpfile, ftp, ftpurl.fu_path, O_RDONLY) == -1)
	{
		if (errno == ECONNRESET
		    && BIT_ISSET(dl_opts, DL_OPT_CONNRETRY))
		{
			/* kill connection and try again */
			download_connection_close(dc_key, 0);
			ftp_quit(ftp, FTP_QUIT_FAST);
			if (dl_verbose)
				printf("  ! FTP server closed connection - "
				       "attempting to reconnect\n");
			goto ftp_download_retry;
		}
		fprintf(stderr, "  ! ftp_open(): %s\n", strerror(errno));
		goto ftp_download_fail;
	}

	progress_init(&dp, fs.fs_size);

	while ((i = ftp_read(ftpfile, buf, sizeof(buf))) > 0)
	{
		j = write(fd, buf, i);
		if (j == -1)
		{
			fprintf(stderr, "  ! write(): %s\n", strerror(errno));
			goto ftp_download_fail;
		}
		else if (j != i)
		{
			fprintf(stderr,
				"  ! short write (%d of %d bytes) - donwload "
				"failed\n", j, i);
			goto ftp_download_fail;
		}

		progress_update(&dp, (size_t)i);
	}

	progress_done(&dp);

	if (ftp_close(ftpfile) == -1)
	{
		fprintf(stderr, "  ! ftp_close(): %s\n", strerror(errno));
		goto ftp_download_fail;
	}
	ftpfile = NULL;

	if (outfile != NULL)
	{
		close(fd);
		return 0;
	}

	if (lseek(fd, 0, SEEK_SET) == -1)
	{
		fprintf(stderr, "  ! lseek(): %s\n", strerror(errno));
		goto ftp_download_fail;
	}
	return fd;

ftp_download_fail:
	if (ftpfile != NULL)
		ftp_close(ftpfile);
	close(fd);
	return -1;
}

#endif /* HAVE_LIBFGET */


/*************************************************************************
 ***  local update module code
 *************************************************************************/

static int
local_scan_dir(char *dir, dir_entry_func_t dirfunc, void *state)
{
	DIR *dirp;
	struct dirent *dep;
	char path[MAXPATHLEN];
	struct stat s;

#ifdef DEBUG
	printf("==> local_scan_dir(dir=\"%s\", dirfunc=0x%lx, "
	       "state=0x%lx)\n", dir, dirfunc, state);
#endif

	if (strncmp(dir, "file://", 7) == 0)
		dir += 7;

	dirp = opendir(dir);
	if (dirp == NULL)
		return -1;
	
	while ((dep = readdir(dirp)) != NULL)
	{
		/* skip "." and ".." */
		if (strcmp(dep->d_name, ".") == 0
		    || strcmp(dep->d_name, "..") == 0)
			continue;

		strlcpy(path, dir, sizeof(path));
		if (path[strlen(path) - 1] != '/')
			strlcat(path, "/", sizeof(path));
		strlcat(path, dep->d_name, sizeof(path));

		/* skip directories */
		if (stat(path, &s) == -1)
		{
			fprintf(stderr, "  ! stat(\"%s\"): %s\n", path,
				strerror(errno));
			closedir(dirp);
			return -1;
		}
		if (S_ISDIR(s.st_mode))
			continue;

		if ((*dirfunc)(path, state) == -1)
			break;
	}

	closedir(dirp);
	return 0;
}


static int
local_download(char *file, char *outfile)
{
	int i;

	if (strncmp(file, "file://", 7) == 0)
		file += 7;

	if (outfile == NULL)
	{
		i = open(file, O_RDONLY);
		if (i == -1)
			fprintf(stderr, "  ! open(\"%s\"): %s\n",
				file, strerror(errno));
	}
	else
	{
		i = symlink(file, outfile);
		if (i == -1)
			fprintf(stderr, "  ! symlink(\"%s\", \"%s\"): %s\n",
				file, outfile, strerror(errno));
	}

	return i;
}


/*************************************************************************
 ***  public access functions
 *************************************************************************/

struct download_type
{
	char *ut_type;
	int (*ut_scandir)(char *, dir_entry_func_t, void *);
	int (*ut_download)(char *, char *);
};
typedef struct download_type download_type_t;

static download_type_t download_types[] = {
#ifdef HAVE_LIBFGET
	{ "ftp://",	ftp_scan_dir,		ftp_download },
#else
	{ "ftp://",	NULL,			NULL },
#endif
#ifdef HAVE_LIBCURL
	{ "http://",	http_scan_dir,		http_download },
	{ "https://",	http_scan_dir,		http_download },
#else
	{ "http://",	NULL,			NULL },
	{ "https://",	NULL,			NULL },
#endif
	{ "file://",	local_scan_dir,		local_download },
	{ NULL,		local_scan_dir,		local_download }
};


/* utility function */
static int
dl_get_type(char *file)
{
	int i;

	for (i = 0; download_types[i].ut_type != NULL; i++)
	{
		if (strncmp(download_types[i].ut_type, file,
			    strlen(download_types[i].ut_type)) == 0)
			break;
	}

	return i;
}


/*
** download_file() - download a local file or remote URL
**                   if outfile is specified, write to outfile and
**                   return 0, otherwise return file descriptor
** returns:
**	0 or file descriptor	success
**	-1 (and sets errno)	failure
*/
int
download_file(char *file, char *outfile)
{
	int i;

#ifdef DEBUG
	printf("==> download_file(file=\"%s\", outfile=\"%s\")\n",
	       file, outfile);
#endif

	i = dl_get_type(file);

	if (download_types[i].ut_download == NULL)
	{
		fprintf(stderr,
			"  ! download_file(): %.*s URLs are not supported\n",
			strcspn(download_types[i].ut_type, ":"),
			download_types[i].ut_type);
		errno = EINVAL;
		return -1;
	}

	return (*(download_types[i].ut_download))(file, outfile);
}


/*
** download_dir() - scan a local or remote directory
** returns:
**	0			success
**	-1 (and sets errno)	failure
*/
int
download_dir(char *dir, dir_entry_func_t dirfunc, void *state)
{
	int i;

	i = dl_get_type(dir);

	if (download_types[i].ut_download == NULL)
	{
		fprintf(stderr,
			"  ! download_dir(): %.*s URLs are not supported\n",
			strcspn(download_types[i].ut_type, ":"),
			download_types[i].ut_type);
		errno = EINVAL;
		return -1;
	}

	return (*(download_types[i].ut_scandir))(dir, dirfunc, state);
}


