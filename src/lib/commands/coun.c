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
 *  coun.c: Do a country roster
 * 
 *  Known contributors to this file:
 *     
 */

#include "misc.h"
#include "player.h"
#include "sect.h"
#include "nat.h"
#include "xy.h"
#include "nsc.h"
#include "file.h"
#include <fcntl.h>
#include <ctype.h>
#include "commands.h"

static void coun_header(void);
static void coun_list(natid cn, struct natstr *natp);

int
coun(void)
{
    struct nstr_item ni;
    struct natstr nat;
    int first;

    pr("The 'country' command is temporarily out of order.\n");
    pr("Please use the 'players' command instead.\n");
    if (!snxtitem(&ni, EF_NATION, player->argp[1]))
	return RET_SYN;
    first = 1;
    while (nxtitem(&ni, (s_char *)&nat)) {
	if ((nat.nat_stat & STAT_INUSE) == 0)
	    continue;
	if (((nat.nat_stat & GOD) != GOD) && !player->god)
	    continue;
	if (first) {
	    coun_header();
	    first = 0;
	}
	coun_list((natid)ni.cur, &nat);
    }
    return RET_OK;
}

static void
coun_header(void)
{
    prdate();
    pr("  #   last access       time\tstatus\t\t country name\n");
}

static void
coun_list(natid cn, struct natstr *natp)
{
    s_char *status;
    struct sctstr sect;

    if (natp->nat_connected)
	pr("%3d  %-16.16s   [%d]", cn, " Now logged on", natp->nat_btu);
    else
	pr("%3d  %-16.16s   [%d]", cn, ctime(&natp->nat_last_login),
	   natp->nat_btu);

    if (natp->nat_stat & STAT_GOD)
	status = "DEITY";
    else if (natp->nat_stat & STAT_NEW)
	status = "New";
    else if (natp->nat_stat & STAT_SANCT)
	status = "Sanctuary";
    else if (natp->nat_stat & STAT_NORM) {
	getsect(natp->nat_xcap, natp->nat_ycap, &sect);
	if (sect.sct_own != cn ||
	    (sect.sct_type != SCT_CAPIT && sect.sct_type != SCT_MOUNT))
	    status = "In flux";
	else if (natp->nat_money < 0)
	    status = "Broke";
	else
	    status = "Active";
    } else {
	status = "Visitor";
    }
    pr("\t%-9.9s\t %s\n", status, natp->nat_cnam);
}
