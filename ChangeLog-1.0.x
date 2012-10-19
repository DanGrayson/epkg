epkg 1.0.5 - 7/5/00
----------

- portability fixes for BeOS
  (thanks to Sid Cammeresi <sac@cheesecake.org>)

- portability fixes for CYGWIN
  (thanks to Patrik Hagglund <patha@softlab.ericsson.se>)

- added missing #include directives
  (thanks to Jim Knoble <jmknoble@pobox.com>)

----------------------------------------------------------------------

epkg 1.0.4 - 6/21/00
----------

- changed configure.in to require libtar version 1.0.2

- more portability fixes for fnmatch() and glob()
  (thanks to Jim Knoble <jmknoble@pobox.com>)

- added missing #include directives
  (thanks to Jim Knoble <jmknoble@pobox.com>)

- removed use of the "__P()" macro from lib/compat.h
  (thanks to Jim Knoble <jmknoble@pobox.com>)

- fixed bug in epkg/clean.c which caused epkg to attempt to remove the
  target directory
  (thanks to Chuck Thompson <cthomp@cs.uiuc.edu> for reporting this)

----------------------------------------------------------------------

epkg 1.0.3 - 4/1/00
----------

- fixed segfault in mkencap when using "-d" option
  (thanks to Andy Reitz <areitz@uiuc.edu> for reporting this)

- changed epkg/installpkg.c to log the numeric status value if an
  unknown value is passed to write_encap_log()

- fixed typo in epkg/recursion.c ('==' instead of '|=')

- removed "inline" keyword from all source files to prevent portability
  problems

- added compatability modules lib/glob.c from OpenBSD and lib/gethostname.c
  (thanks to Jim Knoble <jmknoble@pobox.com> for the gethostname()
  code and the bug reports)

- added lib/fnmatch.c compatability module from OpenBSD

- changed Makefiles for epkg and mkencap to put '$(LIBS)' after
  libencap.a on the link command line
  (thanks to Jim Knoble <jmknoble@pobox.com> for the bug report)

- changed Makefiles for epkg and mkencap to use '$(CFLAGS)' even in
  link commands
  (thanks to Steven Engelhardt <sengelha@uiuc.edu> for the suggestion)

- fixed some objdirs problems related to the printplatform utility

- added the following configure options:
   --with-encap-source=PATH  Set default Encap source directory
   --with-encap-target=PATH  Set default Encap target directory
   --with-excludes=LIST      Set default exclude directories
   --with-overrides=LIST     Set default override list
   --with-pkgdirs=LIST       Set default package subdirectories

- changed configure test for libtar to accept versions higher than 1.0

- updated README file

----------------------------------------------------------------------

epkg 1.0.2 - 2/25/00
----------

- epkg now outputs the status of install and remove operations in
  addition to logging it to the logfile
  (thanks to Jim Robinson <robinson@wdg.mot.com> for the RFE)

- fixed a bug in lib/pkgspec.c in ver2str() which caused epkg to
  segfault when it encountered null version packages in batch mode
  (thanks to Mattox Beckman <beckman@dawn-treader.cs.uiuc.edu> for the
  bug report)

- fixed a bug in lib/Makefile.in which was breaking objdirs support
  (thanks to Geoff Raye <geoff@raye.com> for the bug report)

- misc Makefile changes (added $CPPFLAGS support, added -o flag to compile
  commands, etc)

- added "printplatform" utility to print the platform name at build time
  for hardcoding into epkg and mkencap binaries
  (previously, they erroneously reported that the platform they were
  running on was the platform they were built on)

----------------------------------------------------------------------

epkg 1.0.1 - 1/9/00
----------

- various changes in link.c, recursion.c, and installpkg.c to fix
  handling of status value from package recursion
  (fixed bugs relating to postinstall scripts and logging)

- mkencap now strips off a trailing slash from pkgspecs

- removed unneeded module strmode.c from libencap

----------------------------------------------------------------------

epkg 1.0 - 1/2/00
--------

- various portability fixes
  (submitted by Ed Thomson <ethomson@RAVECOMM.COM>)

- minor output changes in mkencap

- top-level Makefile changed to run mkencap during "make install"

- libmisc is now integrated into libencap

- fixed a bug in recursion.c to prevent infinite recursion when a package
  contains a symlink to "."

----------------------------------------------------------------------

epkg 0.6.8 beta - 12/17/99
---------------

- configure: libtar/zlib will now be used by default if available

- configure: removed -D_GNU_SOURCE from default CFLAGS

- default: added "lost+found" to default global exclude list
  (thanks to Jon Roma <roma@uiuc.edu> for reporting this)

- bug: fixed strcmp() fallback bug in vercmp() which was introduced
  in 0.6.6

- interface: miscellaneous interface changes for mkencap

- api: epkg and mkencap now report meaningful errors from libtar

- api: added new write_encapinfo() function in lib/encapinfo.c

- api: added new freever() function in lib/pkgspec.c

- documentation: added manpages for libencap API

----------------------------------------------------------------------

epkg 0.6.7 beta - 11/16/99
---------------

- fixed epkg/extract.c to use TAR_NOOVERWRITE option when not extracting
  archives if not in force mode

- changed epkg/extract.c and mkencap/create.c to use new tar_open()
  semantics

- replaced GNU basename(), dirname(), and strdup() compatibility code
  with OpenBSD versions

- configure performs super-anal checking of basename() and dirname()

----------------------------------------------------------------------

epkg 0.6.6 beta - 11/13/99
---------------

- portability fix: don't use Linux's broken basename() call

- portability fix: use ranlib instead of ar -s
  (thanks to Sid Cammeresi <cammeres@uiuc.edu> for reporting this)

- bug fix: rewrote find_relative_path() to avoid segfault if start_dir was "/"

- bug fix: added error checking for tar_extract_all() in epkg/extract.c

- bug fix: fixed expression in process_package() in epkg/installpkg.c which
  determined when package scripts would be invoked

- interface change: "-A" changed to "-a" (also removed a reference
  to the old encapper compatibility mode)

- new option: "-p" flag to toggle prerequisite checking

- algorithm change: vercmp() can now differentiate between 4.06 and 4.6

- algorithm change: in vercmp(), if a subversion has more parts than
  the one it's being compared with, it's not newer if it has no
  additional subvers and there are additional subvers in the one
  its being compared with

- algorithm change: if a target directory is not a linkdir but already
  exists as a symlink, refuse to process it (better handling of this
  will be added later)

- output change: "-V" option now displays platform and libtar version string

- output change: avoid spurious "Checking prerequisites" message for
  packages which don't have prereqs

- output change: fixed mkencap/pkgdirs.c to avoid duplicate '/'
  characters in pathnames

- code cleanup: many changes in lib/pkgspec.c to make ver_t a
  dynamicly-allocated structure with no limit in size

----------------------------------------------------------------------

epkg 0.6.5 beta - 10/03/99
---------------

- fixed handling of '>=' and '<=' ranges in "pkgspec" prereq type

- fixed dependancy typo in lib/Makefile.in

- configure.in no longer checks for strtok_r() in -lc_r because of
  incompatibilities with system() under AIX 4.2.1

- configure.in correctly handles the broken basename() definition
  in libgen.h on glibc-2.1 systems

----------------------------------------------------------------------

epkg 0.6.4 beta - 09/27/99
---------------

- minor interface change: mkencap -c now creates the tar archive in the
  current directory by default instead of in the source directory

- fixed a bug in run_hash() which caused "epkg -b" to segfault

- misc bugfixes in lib/pkgspec.c and epkg/versioning.c to handle a
  package with an illegal '-' as the last character

- merged old "compat" and "libds" directories into new "misc" directory
  and cleaned up Makefiles

----------------------------------------------------------------------

epkg 0.6.3 beta - 09/24/99
---------------

- fixed a bug in recursion.c which caused encap.exclude files to be
  read for Encap 1.0 packages, but not for Encap 1.1 packages

- various portability fixes: added compat/strdup.c, don't use getopt.h,
  added check for ANSI headers and unistd.h
  (thanks to Dave Terrell <dbt@meat.net> for reporting this)

- added call to tar_append_eof() in mkencap/create.c so that vendor tars
  don't get a directory checksum error when trying to extract archives
  (thanks to Allan Tuchman <tuchman@uiuc.edu> for reporting this)

----------------------------------------------------------------------

epkg 0.6.2 beta - 09/10/99
---------------

- miscellaneous fixes in encap_link_check() which sometimes caused
  epkg to segfault

- fixed a bug in my_realpath() which caused an infinite loop when
  squeezing multiple '/' characters

- strip off ".tar.gz" or ".tgz" suffix after extracting in epkg.c,
  rather than handling it in str2ver()

- mkencap changes:
  - now reads $ENCAP_CONACT for the "contact" field
  - mkencap will not overwrite a pre-existing encapinfo file
  - slight interface changes

----------------------------------------------------------------------

epkg 0.6.1 beta - 08/27/99
---------------

- moved hash and list code to libds subdirectory (libds is now an independant
  pseudo-package which will be included within larger packages)

- added "break" at recursion.c:251 to prevent vendor compiler breakage.
  (thanks to G.P. Musumeci <gdm@uiuc.edu> for the fix.)

- miscellaneous Makefile changes to fix building in seperate directories
  (thanks to Chuck Thompson <cthomp@cs.uiuc.edu> for the fix.)

- staticly link to libz and libtar if we're using gcc

----------------------------------------------------------------------

epkg 0.6 beta - 08/18/99
-------------

1) support for new Encap 2.0 package format:
   - new encapinfo file (includes old encap.exclude functionality)
   - prerequisite checking
   - ability to link directories
   - ability to rename individual links
   - ability to require individual links

2) configuration changes:
   - build-time configuration now uses GNU autoconf
   - "encap" and "encapper" links are no longer installed or supported
     by the epkg binary (they were causing too much confusion)
   - many functions now part of libencap.a, which will eventually be
     a complete Encap reference API

3) new functionality:
   - epkg can automaticly extract .tar.gz archives (using libtar/zlib, or
     using external binaries)
   - new "mkencap" executable can create Encap 2.0 packages (can
     initialize Encap dirs, create .tar.gz archives, etc)

4) minor fixes:
   - clean mode only worked with -v
   - versioning remove mode didn't work as advertised when a pkgspec
     included a version string
   - when installing a specified version, versioning install mode
     didn't remove the other versions of the same package
   - with -f, only re-links when necessary
   - in quiet mode, stdout of package scripts is sent to /dev/null
     (stderr is still visible)
   - with -n, only displays creation/deletion of links, not directories
   - better status reporting in recursion.c (single status variable for
     all levels)
   - fixed improper use of NULL as a char


----------------------------------------------------------------------

epkg 0.5.1 beta - 12/4/98
---------------

- bug: incorrectly parsed pkgspecs with more than one '-' in versioning
  install/remove modes.

- bug: didn't check that package versions were directories in versioning
  install/remove modes (detected foo-x.y.z.tar.gz as latest of foo-x.y.z).

- cosmetic buglet: only said "executing postinstall script" in verbose mode.

----------------------------------------------------------------------

epkg 0.5 beta - 11/1/98
-------------

1) major code cleanup:
   - compatability code moved into "compat" subdirectory.
   - source files and function interfaces reorganized.
   - all remaining gotos removed.
   - code is more modular (e.g., general-purpose linked list code
     in list.c which is used for versioning modes, exclude lists, etc).

2) package mode interface changes:
   - now only 3 package modes: install (-i), remove (-r), and batch (-b).
     all modes do versioning by default, use -S to disable.
     (-u is now -i, -i is now -Si, -U is now -b, -b is now -Sb).
   - versioning remove mode removes all versions of the package.
   - if you specify a version number in versioning install mode,
     that version will be selected for installation instead of the
     latest version.
  
3) new features:
   - client mode (-C) executes package scripts but does not link/remove.
     desireable for clients which NFS-mount /usr/local.
   - list of packages can be specified (-O override1,override2,...) whose
     links are overridden when installing other packages.  primarily meant
     to help people transition to Encap from old /usr/local setup.  default
     list is set at compile time.  use -o to disable override checking.
   - in versioning install mode, if no version is specified, you can use
     the -1 flag as shorthand for "next to latest version".  this is
     useful for quickly backing off to a previous version.
   - new flag (-P) to link/remove files directly in package directory.
   - new flag (-n) to show what would be linked/removed but not actually
     do the linking/removing.

4) miscellaneous changes:
   - old -a option is now -A (-a is needed for encapper compat code).
   - old -e option is now -R (-e for "execute" and -x for "exclude"
     was too confusing, so it's -R for "run").
   - if only one of source or target is specified on the commandline,
     epkg will assume the missing one relative to the supplied one.
   - in versioning modes, packages "foo" and "foo-x.y.z" can coexist
     ("foo" is considered lowest version).
   - return value of package scripts is checked.  if pre(install|remove)
     returns non-zero, installation/removal does not procede.
   - $ENCAP_MODE variable set to "install", "remove", or "batch" in
     package scripts.  this is useful for, e.g., knowing that if we're in
     "install" mode and we're running a remove script, you know that a new
     version of the package will be installed.
   - ".beta" dropped from epkg's version number - that was a dumb idea.
   - most yes/no switches are now toggles (specifying them twice will
     go back to the default).
   - manpage updated and revamped, general Encap description moved to
     encap(7).

5) exclude list changes:
   - encap.exclude files are now read in all package subdirectories.
     new flag (-E) to read encap.exclude files everywhere (in the
     source directory itself, and in the target tree).  encap.exclude in
     the source directory itself is treated specially - can only refer to
     files in the source dir itself.
   - new flag (-X exclude1,exclude2,...) to specify global exclude entries.
     global entries are used in clean mode too.  source dir is automaticly
     added if it lies under the target dir.  default list is set at
     compile time.

----------------------------------------------------------------------

epkg 0.4.5 beta - 10/10/98
---------------

- bug: "every solution breeds new problems".  My non-existant-package-
  detection fix in 0.4.4 caused epkg to hang when it encountered a
  file or directory whose name started with "." or "..".

----------------------------------------------------------------------

epkg 0.4.4 beta - 10/2/98
---------------

- bug: links were not properly removed when source directory was not
  an absolute path (it said "not encap link")

- bug: fixed detection of links into non-existant packages
  (looks like it's been broken since 0.4.2 - i need to flog my
  users for not reporting this)

- bug: incorrectly picked "foo-bar-1.11a" as a version of package "foo"
  (needed to check for the LAST instance of '-')

- bug: a few subtle yet really stupid bugs in is_bigger_version()
  ("I will not make novice C programming errors.   I will not make
  novice C programming errors.  I will not...")

- fixed typo in Makefile for Solaris 2.5.1 configuration
  (was "sprintf" instead of "snprintf")

----------------------------------------------------------------------

epkg 0.4.3 beta - 9/3/98
---------------

- added platform configurations to the Makefile and updated INSTALL

- minor new feature: $ENCAP_PKGNAME environment variable is now
  available in package scripts

- bug: when trying to remove a package which hadn't been installed,
  epkg was dying when trying to remove a non-existant subdirectory

- bug (Linux, AIX 3): when given a pkgname on the commandline with a
  trailing '/', epkg was segfaulting (-DTRAILING_SLASH_BREAKS_BASENAME)

- bug: trailing '/' in source or target was causing problems

- aesthetic buglet: 1-digit dates were not being padded to 2 digits in
  the log file

- code cleanup: removed a goto in hash_read() in hash.c

----------------------------------------------------------------------

epkg 0.4.2 beta - 6/24/98
---------------

- this time, I REALLY fixed the package scripts (someone please remind me
  to TEST stuff before I release it...)

- fixed a misleading error message in clean mode

- rewrote some of the sanity-checking code

----------------------------------------------------------------------

epkg 0.4.1 beta - 6/24/98
---------------

- fixed a bug where epkg wasn't finding the package scripts with the
  right names

- fixed a bug when epkg wasn't properly removing a dangling encap link
  when it had to replace it with a new one

- fixed update mode for packages with no version number

- slight bugfix to version-analysis algorithm, and corresponding change
  of package name from "epkg-0.4.1beta" to "epkg-0.4.1.beta"

- some minor modifications to the output

----------------------------------------------------------------------

epkg 0.4 beta - 6/11/98
-------------

- more bugfixes (most notably, better error handling)

- user interface cleaned up quite a bit, see manpage for details:
   - all interactive options removed
   - -q (quiet) option added
   - now only two conflict-handling modes: continue (default) and force (-f)

- new package mode: update specific package.

- new feature: transaction logging.  see manpage for details.

----------------------------------------------------------------------

epkg 0.3.1 beta - 6/9/98
---------------

- lots of bugfixes; epkg should be a lot more reliable now.

----------------------------------------------------------------------

epkg 0.3 beta - 6/8/98
-------------

- Wrote new hash routines, so epkg no longer requires libdb.

- Major code cleanup.

- A few miscellaneous bugfixes.

- clean and remove modes now remove empty target directories

----------------------------------------------------------------------

epkg 0.2 beta - 6/4/98
-------------

- Added update mode for package version control.  epkg now requires
  libdb 1.85.

- Added compatibility mode if invoked as "encap" or "encapper".  Thanks
  to Keith Garner <k-garner@uiuc.edu> for the suggestion.

- Added a lot of sanity checking, especially in check mode and remove mode.
  epkg now will not remove a symlink unless it points into the source
  tree.

----------------------------------------------------------------------

epkg 0.1 beta - 6/2/98
-------------

This is version 0.1 BETA of epkg, the encap package manager with
extentions.  This is the first release, so I expect that there are a
number of bugs that I didn't catch; please report them to me so they
can be fixed.  Feedback on features and functionality is also welcome.
