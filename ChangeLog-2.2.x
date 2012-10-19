epkg 2.2.6 - 4/26/02
----------

- fixed encapinfo parsing code to handle extra whitespace
  (thanks to Ted Irons <ironst@saic.com> for the bug report)
- fixed encapinfo parsing code to ignore escaped comment character
- if updating to a package that is already installed, don't print warning
  unless "-v" is specified
- fixed update mode to not abort if the specific version requested
  cannot be found
- added "-K" option to stop scanning update directories at the first one
  that contains any version of the requested package

----------------------------------------------------------------------

epkg 2.2.5 - 4/3/02
----------

- various portability and compiler warning fixes
  (based on patch from Jim Knoble <jmknoble@pobox.com>)
- fixed potential segfault in batch mode
- fixed encap_platform_name() to report correct value under OS X
  (thanks to Alex Lovell-Troy <alovell@uiuc.edu> for the bug report)

----------------------------------------------------------------------

epkg 2.2.4 - 3/31/02
----------

- fixed more update mode segfaults
  (thanks to Jon Marks <j-marks@uiuc.edu> and
  Erik Coleman <ecc@uiuc.edu> for the bug reports)
- don't increment exit value in update mode if requested update is
  already installed

----------------------------------------------------------------------

epkg 2.2.3 - 3/21/02
----------

- added prototype for encap_pkgspec_join() to <encap.h>
  (thanks to Jim Knoble <jmknoble@pobox.com> for the patch)
- fixed a fairly stupid logic bug that caused a segfault in update mode
  (thanks to Mike Drzal <drzal@uiuc.edu> and
  "Alex J. Lovell-Troy" <alovell@uiuc.edu> for the bug reports)

----------------------------------------------------------------------

epkg 2.2.2 - 3/19/02
----------

- updated for autoconf-2.53
- print useful error messages in encapinfo_read()
- fixed encap_pkgspec_parse() to handle trailing '-' in pkgspecs
- rewrote encap_find_versions() to use readdir() and
  encap_pkgspec_parse() instead of glob()
- fixed "prereq pkgspec" code to handle pkgspecs with dashes
- fixed epkg to handle pkgspecs with dashes
- added encap20_platform_name() function
- added mkencap -E option to allow creation of Encap 2.0 packages
- fixed "make install" to create an Encap 2.0 package for epkg

----------------------------------------------------------------------

epkg 2.2.1 - 3/5/02
----------

- fixed default archive name for mkencap
  (thanks to Melissa Woo <m-woo@uiuc.edu> for the bug report)

----------------------------------------------------------------------

epkg 2.2.0 - 2/26/02
----------

- public release (no changes)

----------------------------------------------------------------------

epkg 2.2.dev5 - 2/22/02
-------------

- fixed cpp conditionals in <encap_listhash.h> and <encap_pathcode.h>

----------------------------------------------------------------------

epkg 2.2.dev4 - 2/20/02
-------------

- add package-specific prefix to shared pathcode functions to avoid
  duplicate symbol errors from some versions of ld

----------------------------------------------------------------------

epkg 2.2.dev3 - 2/20/02
-------------

- fixed str_replace() function to avoid concatenating update directories
  (thanks to Mike Drzal <drzal@uiuc.edu> for the bug report)
- fixed mkdirhier() to avoid duplicate '/' at the beginning of a path
- minor documentation improvements
- minor Makefile fixes

----------------------------------------------------------------------

epkg 2.2.dev2 - 2/18/02
-------------

- minor code cleanup
- added epkg -F flag to debug connections to remote update directories
- in update directories, special token "%p" is replaced with host
  platform name
- updated for fget-1.2.7 API changes

----------------------------------------------------------------------

epkg 2.2.dev1 - 2/7/02
-------------

- updated documentation
- fixed mkencap to add encapinfo file to beginning of archive if built
  with libtar support
- added -A option to mkencap to specify platform suffix
- changed epkg output for update mode to be more terse by default
- changed update path code to honor path order when multiple directories 
  contain the same version of a package
- fixed encap_platform_split() and encap_platform_compat() to handle
  the special platform name "share"
- if $ENCAP_UPDATE_PATH is not set, try $ENCAP_UPDATEDIR for backward
  compatibility
- re-added epkg -U option to prepend a directory to the update list
- changed mkencap -p option to override the package's platform name
- added epkg -H option to set the host platform name
- minor directory structure changes to accomodate shared path
  manipulation code

----------------------------------------------------------------------

epkg 2.2.dev0 - 1/25/02
-------------

- added support for Encap 2.1 packages:
  - introduced new platform naming scheme
  - introduced new package archive naming scheme
  - rewrote encap_vercmp() to handle package versions
- replaced $ENCAP_UPDATEDIR with $ENCAP_UPDATE_PATH (space-delimited
  list of update directories)
- removed epkg -U option
- allow full URLs to be specified in install mode
- added epkg -A option
- changed mkencap to use new archive names by default
- reworked API to remove pkgspec_t and ver_t types
- upgraded to autoconf-2.52
- improved Makefile portability
- updated copyright dates

