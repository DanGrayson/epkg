epkg 2.0.4 - 3/4/01
----------

- fixed declaration syntax problem in epkg/update.c for non-gcc compilers
- updated mkencap(1) and epkg(1) manpages
- fixed update mode for packages with null versions
- include <unistd.h> from epkg/extract.c and mkencap/create.c
  (needed for Solaris, reported by Joe Doyle <doyle@nebcorp.com> and
  Jason Petrone <jpetrone@cnri.reston.va.us>)
- add missing GLOB_NOESCAPE definition to compat.h
  (thanks to Yasuhide OMORI <omori@m-t.com> for the bug report)
- use ':' as the seperator for exclude, override, and pkgdir lists

----------------------------------------------------------------------

epkg 2.0.3 - 1/12/01
----------

- fixed remove mode to honor global excludes
- changed encapinfo_t ei_date field from a time_t back to a string
  to avoid non-portable strptime()
  (thanks to Mike Hollyman <mikeh@hollyman.com> for the bug report)
- check errors from encap_open()
- code cleanups to make -Wall happy
  (including a patch from Jim Knoble <jmknoble@JMKNOBLE.CX>)
- include <sys/types.h> from <encap.h> (needed for AIX 4.2.1)

----------------------------------------------------------------------

epkg 2.0.2 - 1/7/01
----------

- fixed segfault when updating a package of which no versions are
  installed locally
- if a version is specified in update mode, look for the requested
  version instead of the latest available
- accept glob patterns for require and linkdir directives
- updated WSG_ENCAP autoconf macro
- fixed doc/Makefile.in to create links during compilation, not
  installation

----------------------------------------------------------------------

epkg 2.0.1 - 1/5/01
----------

- fixed package prerequisite range checking bug
- fixed mkencap to prevent encapinfo_free() from trying to free
  static memory
- fixed autoconf macros to behave properly when a config.cache file
  is present

----------------------------------------------------------------------

epkg 2.0 - 1/4/01
--------

- minor code cleanups
- fixed a directory recursion bug with local update directories

----------------------------------------------------------------------

epkg 1.1.b24 - 1/2/01
------------

- in update mode, don't specifically say when no updates are available
  unless verbosity is increased
- changed mkencap to honor -I in all modes
- updated WSG_ENCAP autoconf macro to handle ${ENCAP_TARGET}

----------------------------------------------------------------------

epkg 1.1.b23 - 12/13/00
------------

- fixed autoconf snprintf() test to make sure it NUL-terminates
- fixed mkencap to return meaningful exit code

----------------------------------------------------------------------

epkg 1.1.b22 - 11/30/00
------------

- fixed epkg's -C option
- fixed a bug in epkg/versions.c which printed garbage instead of the
  version number when trying to install a non-existant version of a pkg
- fixed vertostr() and strtover() to handle versions that start with '.'
  (thanks to Anh Lai <anhlai@uiuc.edu> for the bug report)
- added $(DESTDIR) to Makefiles
- Makefile changes to support WSG_PKG autoconf macro

----------------------------------------------------------------------

epkg 1.1.b21 - 11/5/00
------------

- fixed pkgspec prereq bug
- fixed encap_recursion() to honor OPT_PKGDIRLINKS
- fixed handling of "epkg pkgname.tar.gz" for local tar archive

----------------------------------------------------------------------

epkg 1.1.b20 - 10/30/00
------------

- fixed a serious bug in strtover() which caused epkg to segfault
- fixed a few segfault bugs in update_pkg() and ftp_versions()

----------------------------------------------------------------------

epkg 1.1.b19 - 10/29/00
------------

- fixed doc/Makefile.in for install target

----------------------------------------------------------------------

epkg 1.1.b18 - 10/29/00
------------

- minor portability fixes
- minor output format fixes
- minor Makefile cleanups
- fixed memory leak in strtover()
- fixed bug in encap_str_hashfunc() which caused epkg to segfault when
  it encountered packages whose first letter was capitalized
- modified epkg/updatedir.c to search subdirectories of update dir
- more directory structure tweaking for CVS
- added function typecasting to avoid compiler warnings

----------------------------------------------------------------------

epkg 1.1.b17 - 10/26/00
------------

- fixed bugs caused by specifying $ENCAP_SOURCE with a trailing slash
- use dynamically allocated memory to add Encap source directory to global
  exclude list
- fixed bugs in epkg_print() relating to linkdirs
- minor directory structure changes because of CVS setup
- fixed bug in transaction logging code
- updated README
- added EPT_PKG_RAW and updated doc/encap_open.3

----------------------------------------------------------------------

epkg 1.1.b16 - 10/5/00
------------

- moved epkg and mkencap options to their own bitmasks
- added OPT_LINKNAMES and OPT_LINKDIRS options to libencap
- added epkg options "-L" and "-D" to toggle new libencap options
- tweaked code to avoid compiler warnings
- various portability fixes and code/API cleanups
- changed encapinfo date field to time_t and modified code to use
  strftime() and strptime()
- removed mkencap "-E" option
- updated libencap manpages
- re-implemented override list functionality
- added mkencap "-F" and "-D" options and updated "-P" option
- updated code to use fget-0.10 API
- changed encapinfo_write() to encode the version of libencap in the
  new encapinfo file
- replaced linkpath_t with encap_source_info_t and encap_target_info_t
- moved target cleaning code into libencap
- re-implemented global exclude list
- fixed autoconf test for snprintf() and patched bounds-checking accordingly
- redesigned epkg output to make more use of granular verbosity levels

----------------------------------------------------------------------

epkg 1.1.b15 - 8/24/00
------------

- improved FTP download progress indicator
- set up epkg exit codes in a sane manner
- fixed a bug which caused files in the package directory to be linked
  even when "-P" was not used

----------------------------------------------------------------------

epkg 1.1.b14 - 8/22/00
------------

- updated to use new libtar-1.1.b1 API
- added support for arbitrary external compression tools
  (currently compress, gzip, and bzip2)
- re-added support for handling archives in install mode

----------------------------------------------------------------------

epkg 1.1.b13 - 7/9/00
------------

- replaced my_realpath() and find_relative_path() with cleanpath() and
  relativepath() respectively
- redesigned interface to get_link_dest() to avoid dynamic memory
- replaced strtok_r() with strsep()
- redesigned prerequisite code for increased modularity
- cleaned up mkencap code
- changed all list options to be invoked multiple times, rather than
  taking a comma-delimited list as a single argument
  (e.g., "-X foo,bar,baz" is now "-X foo -X bar -X baz")

----------------------------------------------------------------------

epkg 1.1.b12 - 7/8/00
------------

- changed FTP code to use libfget
- updated mkencap to use new libtar API
- minor autoconf cleanup

----------------------------------------------------------------------

epkg 1.1.b11 - 7/2/00
------------

- major API redesign - moved a huge amount of code from epkg into
  libencap and added encap_open() call to return ENCAP handle
- added encap_hash_getkey() and changed encap_hash_search() to
  simple iteration
- changed find_pkg_versions() to use a plugin function instead of
  returning an encap_list_t
- clean mode can no longer be used in conjunction with other modes
- added aclocal.m4 to clean up configure.in

----------------------------------------------------------------------

epkg 1.1.b10 - 4/25/00
------------

- various libencap API cleanups

----------------------------------------------------------------------

epkg 1.1.b9 - 4/23/00
-----------

- added package check mode
- added '-d' option to toggle removal of empty target directories
  (affects clean and remove modes)

----------------------------------------------------------------------

epkg 1.1.b8 - 4/21/00
-----------

- removed updatelist mode
- revamped update mode
- code cleanup centered around epkg/versions.c

----------------------------------------------------------------------

epkg 1.1.b7 - 4/19/00
-----------

- major code restructuring in epkg subdirectory:
  - global options are now a bitfield
  - functions regrouped into different source files to ease maintenance

----------------------------------------------------------------------

epkg 1.1.b6 - 4/18/00
-----------

- fixed bugs in updatelist mode

- added '-A' flag to list all versions of all available updates

----------------------------------------------------------------------

epkg 1.1.b5 - 4/11/00
-----------

- redesigned interface for list and hash functions

----------------------------------------------------------------------

epkg 1.1.b4 - 4/5/00
-----------

- modified epkg/list.c to only display info on updates which are not
  already available in the source directory

- fixed bug in update.c which caused a segfault when the update dir
  contained two versions of a given package

----------------------------------------------------------------------

epkg 1.1.b3 - 4/5/00
-----------

- rewrote epkg/clean.c as a recursion plugin

- removed "-Wl,-Bdynamic" and "-Wl,-Bstatic" from LIBS in configure.in

- libtar code updated for libtar-1.1.x and can now handle both .tar.gz
  as well as ordinary .tar files

- added list-updates mode to list versions of all packages locally
  and in update dir

----------------------------------------------------------------------

epkg 1.1.b2 - 4/4/00
-----------

- merged changes from 1.1.b0 and 1.1.b1

- fixed epkg/remove.c to use target_path instead of link_target

- additional portability fixes for fnmatch() and glob() merged from 1.0.4
  (thanks to Jim Knoble <jmknoble@pobox.com>)

----------------------------------------------------------------------

epkg 1.1.b1 - 4/3/00
-----------

- new "update" mode to grab a package tar archive from an update dir,
  extract it into the source dir, and install it

- added FTP capability using ftplib so update dir can be accessed remotely

----------------------------------------------------------------------

epkg 1.1.b0 - 4/4/00
-----------

- revamped and modularized directory recursion code

