/* config.c - MemTest-86  Version 4.1
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 * ----------------------------------------------------
 * MemTest86+ V1.11 Specific code (GPL V2.0)
 * By Samuel DEMEULEMEESTER, sdemeule@memtest.org
 * http://www.x86-secret.com - http://www.memtest.org
 * Copyright 2016-2017 Reneo, Inc; Author: Minesh B. Amin
 */
#include "test.h"

void adj_mem(void)
{
	int i;

	v->selected_pages = 0;
	for (i=0; i< v->msegs; i++) {
		/* Segment inside limits ? */
		if (v->pmap[i].start >= v->plim_lower &&
				v->pmap[i].end <= v->plim_upper) {
			v->selected_pages += (v->pmap[i].end - v->pmap[i].start);
			continue;
		}
		/* Segment starts below limit? */
		if (v->pmap[i].start < v->plim_lower) {
			/* Also ends below limit? */
			if (v->pmap[i].end < v->plim_lower) {
				continue;
			}
			
			/* Ends past upper limit? */
			if (v->pmap[i].end > v->plim_upper) {
				v->selected_pages += 
					v->plim_upper - v->plim_lower;
			} else {
				/* Straddles lower limit */
				v->selected_pages += 
					(v->pmap[i].end - v->plim_lower);
			}
			continue;
		}
		/* Segment ends above limit? */
		if (v->pmap[i].end > v->plim_upper) {
			/* Also starts above limit? */
			if (v->pmap[i].start > v->plim_upper) {
				continue;
			}
			/* Straddles upper limit */
			v->selected_pages += 
				(v->plim_upper - v->pmap[i].start);
		}
	}
}
