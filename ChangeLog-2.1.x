epkg 2.1.1 - 7/24/01
----------

- fixed debugging code
- fixed segfault when installing package with no version specified
  (thanks to Alex Lovell-Troy <alovell@uiuc.edu> for the bug report)

----------------------------------------------------------------------

epkg 2.1.0 - 7/20/01
----------

- added AR probe to configure.in
  (based on patch from Lars Kellogg-Stedman <lars@larsshack.org>)
- fixed vercmp() to compare only the numeric portion of strings when
  two numbers are equal
  (thanks to Jon Roma <roma@uiuc.edu> for the bug report)

----------------------------------------------------------------------

epkg 2.1.dev0 - 6/15/01
-------------

- updated epkg to use new libfget-1.2.x API
- updated autoconf macros and compat code
- reformatted code for readability
- use typedefs to specify args for listhash plugin functions
- added *_(list|hash)ptr_(reset|data)() functions to listhash code
- added "_t" suffix to encap_decision_func and encap_print_func typedefs
- portability fix in lib/compat/glob.c
  (thanks to Nick Siripipat <nsiripip@tangentis.com> for reporting this)
- rewrote cleanpath() and relativepath() to use static buffers
  and moved them out of the public API
- changed bufsize parameters to be size_t instead of int
- fixed segfault in mkencap when parsing invalid argument to "-P"
  (thanks to Charley LeCrone <lecrone@uiuc.edu> for the bug report)

