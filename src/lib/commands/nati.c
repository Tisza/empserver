/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2005, Dave Pare, Jeff Bailey, Thomas Ruschak,
 *                           Ken Stevens, Steve McClure
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  ---
 *
 *  See the "LEGAL", "LICENSE", "CREDITS" and "README" files for all the
 *  related information and legal notices. It is expected that any future
 *  projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  nati.c: List nation information
 * 
 *  Known contributors to this file:
 *     
 */

#include "misc.h"
#include "player.h"
#include "nat.h"
#include "sect.h"
#include "file.h"
#include "xy.h"
#include "optlist.h"
#include "commands.h"

int
nati(void)
{
    struct natstr *natp;
    struct sctstr sect;
    float hap;
    int mil;
    int civ;
    int poplimit, safepop, uwpop;
    double pfac;

    if ((natp = getnatp(player->cnum)) == 0) {
	pr("Bad country number %d\n", player->cnum);
	return RET_SYN;
    }
    pr("\n(#%i) %s Nation Report\t", player->cnum, cname(player->cnum));
    prdate();
    pr("Nation status is %s", natstate(natp));
    pr("     Bureaucratic Time Units: %d\n", natp->nat_btu);
    if (natp->nat_stat & STAT_INUSE) {
	getsect(natp->nat_xcap, natp->nat_ycap, &sect);
	if (!player->owner || (sect.sct_type != SCT_CAPIT &&
			       sect.sct_type != SCT_MOUNT &&
			       sect.sct_type != SCT_SANCT))
	    pr("No capital. (was at %s)\n",
	       xyas(sect.sct_x, sect.sct_y, player->cnum));
	else {
	    civ = sect.sct_item[I_CIVIL];
	    mil = sect.sct_item[I_MILIT];
	    pr("%d%% eff %s at %s has %d civilian%s & %d military\n",
	       sect.sct_effic,
	       (sect.sct_type ==
		SCT_CAPIT ? "capital" : "mountain capital"),
	       xyas(sect.sct_x, sect.sct_y, player->cnum), civ, splur(civ),
	       mil);
	}
    }
    pr(" The treasury has $%.2f", (double)natp->nat_money);
    pr("     Military reserves: %ld\n", natp->nat_reserve);
    pr("Education..........%6.2f       Happiness.......%6.2f\n",
       (double)natp->nat_level[NAT_ELEV],
       (double)natp->nat_level[NAT_HLEV]);
    pr("Technology.........%6.2f       Research........%6.2f\n",
       (double)natp->nat_level[NAT_TLEV],
       (double)natp->nat_level[NAT_RLEV]);
    pr("Technology factor :%6.2f%%", tfact(player->cnum, 100.));

    if (opt_NO_PLAGUE)
	pfac = 0.0;
    else
	pfac = ((double)natp->nat_level[NAT_TLEV] + 100.) /
	    ((double)natp->nat_level[NAT_RLEV] + 100.);
    pr("     Plague factor : %6.2f%%\n", pfac);
    pr("\n");

    poplimit = max_population(natp->nat_level[NAT_RLEV], SCT_MINE, 0);
    pr("Max population : %d\n", poplimit);

    safepop =
	(int)((double)poplimit / (1.0 + obrate * (double)etu_per_update));
    uwpop =
	(int)((double)poplimit / (1.0 + uwbrate * (double)etu_per_update));
    safepop++;
    if (((double)safepop * (1.0 + obrate * (double)etu_per_update)) >
	((double)poplimit + 0.0000001))
	safepop--;
    uwpop++;
    if (((double)uwpop * (1.0 + uwbrate * (double)etu_per_update)) >
	((double)poplimit + 0.0000001))
	uwpop--;

    pr("Max safe population for civs/uws: %d/%d\n", safepop, uwpop);

    hap = ((natp->nat_level[NAT_TLEV] - 40) / 40.0 +
	   natp->nat_level[NAT_ELEV] / 3.0);

    if (hap > 0.0)
	pr("Happiness needed is %f\n", hap);
    else
	pr("No happiness needed\n");

    return RET_OK;
}
