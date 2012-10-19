/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2004 Mark D. Roth
**  All rights reserved.
**
**  clean.c - clean mode for epkg
**
**  Mark D. Roth <roth@feep.net>
*/

#include <epkg.h>

#include <stdio.h>
#include <errno.h>

#ifdef STDC_HEADERS
# include <string.h>
#endif


/* decision function to handle the global exclude list */
int
exclude_decision(ENCAP *encap, encap_source_info_t *srcinfo,
		 encap_target_info_t *tgtinfo)
{
	encap_listptr_t lp;

#ifdef DEBUG
	printf("==> exclude_decision(encap=0x%lx, srcinfo=\"%s\", ",
	       "tgtinfo=\"%s\")\n",
	       encap, (srcinfo ? srcinfo->src_target_relative : "NULL"),
	       (tgtinfo ? tgtinfo->tgt_link_existing : "NULL"));
#endif

	encap_listptr_reset(&lp);
	if (encap_list_search(exclude_l, &lp, srcinfo->src_target_relative,
			      NULL) != 0)
	{
		(*encap->e_print_func)(encap, srcinfo, tgtinfo,
				       (encap->e_pkgname[0]
					? EPT_PKG_INFO : EPT_CLN_INFO),
				       "excluding");
		return R_SKIP;
	}

	return R_FILEOK;
}



/* clean target directory */
int
clean_mode(void)
{
	if (verbose)
		printf("epkg: cleaning target directory:\n");

	return encap_target_clean(target, source, options, epkg_print,
				  exclude_decision);
}


