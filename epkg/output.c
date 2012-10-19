/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  output.c - status printing function for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <epkg.h>

#include <stdio.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
#endif


int
epkg_print(ENCAP *encap, encap_source_info_t *srcinfo,
	   encap_target_info_t *tgtinfo, unsigned int type, char *fmt, ...)
{
	va_list args;
	char *srcref = NULL;

#ifdef DEBUG
	printf("==> epkg_print(encap=0x%lx (\"%s\"), srcinfo=0x%lx (\"%s\"), "
	       "tgtinfo=0x%lx (\"%s\"), type=%d, fmt=0x%lx)\n",
	       encap, encap->e_pkgname, srcinfo, srcinfo->src_pkgdir_relative,
	       tgtinfo, tgtinfo->tgt_link_existing_pkgdir_relative, type, fmt);
#endif

	if (srcinfo != NULL)
	{
		if (verbose > 2)
			srcref = srcinfo->src_target_path;
		else
			srcref = srcinfo->src_target_relative;
	}

	switch (type)
	{

	case EPT_INST_OK:
		if (verbose <= 1)
			return 0;
	case EPT_INST_REPL:
		if (srcinfo == NULL)
		{
			printf("ERROR: epkg_print(): type=%d srcinfo=0x%lx",
			       type, (unsigned long)srcinfo);
			break;
		}
		printf("     + %s", srcref);
		if (BIT_ISSET(srcinfo->src_flags, SRC_ISDIR) &&
		    !BIT_ISSET(srcinfo->src_flags, SRC_LINKDIR))
			putchar('/');
		else if (verbose > 2)
			printf(" -> %s", srcinfo->src_link_expecting);
		if (type == EPT_INST_REPL)
		{
			putchar(' ');
			if (fmt != NULL)
			{
				putchar('(');
				va_start(args, fmt);
				vprintf(fmt, args);
				va_end(args);
				putchar(')');
			}
			else
				printf("(replaced link to %spackage %s)",
				       (BIT_ISSET(tgtinfo->tgt_flags,
						  TGT_DEST_EXISTS)
					? "" : "non-existant "),
				       tgtinfo->tgt_link_existing_pkg);
		}
		break;

	case EPT_REM_FAIL:
		if (verbose <= 3)
			return 0;
	case EPT_REM_ERROR:
	case EPT_INST_FAIL:
	case EPT_INST_ERROR:
	case EPT_CLN_FAIL:
	case EPT_CLN_ERROR:
		if (srcinfo == NULL || fmt == NULL)
		{
			printf("ERROR: epkg_print(): type=%d srcinfo=0x%lx "
			       "fmt=0x%lx", type, (unsigned long)srcinfo,
			       (unsigned long)fmt);
			break;
		}
		printf("%s  !  %s: ",
		       (type != EPT_CLN_FAIL && type != EPT_CLN_ERROR
		        ? "  " : ""), srcref);
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		if (type == EPT_INST_ERROR ||
		    type == EPT_REM_ERROR || type == EPT_CLN_ERROR)
			printf(": %s", strerror(errno));
		break;

	case EPT_INST_NOOP:
	case EPT_REM_NOOP:
	case EPT_CHK_NOOP:
	case EPT_CLN_NOOP:
		if (srcinfo == NULL)
		{
			printf("ERROR: epkg_print(): type=%d srcinfo=0x%lx",
			       type, (unsigned long)srcinfo);
			break;
		}
		if (verbose <= 3)
			return 0;
		if (type != EPT_CLN_NOOP)
			printf("  ");
		if (type == EPT_CHK_NOOP || type == EPT_CLN_NOOP)
			printf("   * %s: valid link", srcref);
		else
			printf("   * %s: already %s", srcref,
			       (type == EPT_REM_NOOP
			        ? "removed" : "installed"));
		break;

	case EPT_REM_OK:
	case EPT_CLN_OK:
		if (verbose <= 1)
			return 0;
		if (tgtinfo == NULL)
		{
			printf("ERROR: epkg_print(): type=%d tgtinfo=0x%lx",
			       type, (unsigned long)tgtinfo);
			break;
		}
		printf("%s   - %s", ((type != EPT_CLN_OK) ? "  " : ""),
		       srcref);
		if (BIT_ISSET(tgtinfo->tgt_flags, TGT_ISDIR))
			putchar('/');
		break;

	case EPT_CHK_FAIL:
	case EPT_CHK_ERROR:
		if (srcinfo == NULL)
		{
			printf("ERROR: epkg_print(): type=%d srcinfo=0x%lx",
			       type, (unsigned long)srcinfo);
			break;
		}
		printf("    !  %s: ", srcref);
		if (fmt != NULL)
		{
			va_start(args, fmt);
			vprintf(fmt, args);
			va_end(args);
		}
		else
			printf("not installed");
		if (type == EPT_CHK_ERROR)
			printf(": %s", strerror(errno));
		break;

	case EPT_PKG_INFO:
	case EPT_CLN_INFO:
		if (fmt == NULL)
		{
			printf("ERROR: epkg_print(): type=%d fmt=0x%lx", type,
			       (unsigned long)fmt);
			break;
		}
		printf("%s  > ", ((type == EPT_CLN_INFO) ? "" : "  "));
		if (srcinfo != NULL)
			printf("%s: ", srcref);
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		break;

	case EPT_PKG_FAIL:
	case EPT_PKG_ERROR:
		if (fmt == NULL)
		{
			printf("ERROR: epkg_print(): type=%d fmt=0x%lx", type,
			       (unsigned long)fmt);
			break;
		}
		printf("    ! ");
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		if (type == EPT_PKG_ERROR)
			printf(": %s", strerror(errno));
		break;

	case EPT_PKG_RAW:
		if (fmt == NULL)
		{
			printf("ERROR: epkg_print(): type=%d fmt=0x%lx", type,
			       (unsigned long)fmt);
			break;
		}
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		return 0;

	default:
		printf("ERROR: epkg_print(): type=%d", type);
		break;
	}

	putchar('\n');
	return 0;
}


