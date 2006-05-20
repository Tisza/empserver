/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2006, Dave Pare, Jeff Bailey, Thomas Ruschak,
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
 *  See files README, COPYING and CREDITS in the root of the source
 *  tree for related information and legal notices.  It is expected
 *  that future projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  new.c: Create a new capital for a player
 * 
 *  Known contributors to this file:
 *     Dave Pare, 1986
 */

#include <config.h>

#include "misc.h"
#include "player.h"
#include "nat.h"
#include "sect.h"
#include "path.h"
#include "file.h"
#include "xy.h"
#include "tel.h"
#include "land.h"
#include "nsc.h"
#include "optlist.h"
#include "commands.h"

#include <fcntl.h>

static int isok(int x, int y);
static void ok(signed char *map, int x, int y);

static struct range defrealm = { -8, -5, 10, 5, 0, 0 };

#define	MAXAVAIL	300

int
new(void)
{
    struct sctstr sect;
    struct natstr *natp;
    struct realmstr newrealm;
    struct range absrealm;
    natid num;
    coord x, y;
    int i;
    char *p;
    char buf[1024];
    time_t current_time = time(NULL);

    natp = getnatp(player->cnum);
    if (natp->nat_xorg != 0 || natp->nat_yorg != 0) {
	pr("Must be at 0,0 to add a new country\n");
	return RET_FAIL;
    }
    if (!(natp = natargp(player->argp[1], "Country? ")))
	return RET_SYN;
    num = natp->nat_cnum;
    if (natp->nat_stat != STAT_NEW) {
	pr("Country #%d (%s) isn't a new country!\n", num, cname(num));
	return RET_SYN;
    }
    if (player->argp[2] != 0) {
	if ((p = getstarg(player->argp[2], "sanctuary pair : ", buf)) == 0)
	    return RET_SYN;
	if (!sarg_xy(p, &x, &y) || !getsect(x, y, &sect))
	    return RET_SYN;
	if (sect.sct_type != SCT_RURAL) {
	    pr("%s is a %s; try again...\n",
	       xyas(x, y, player->cnum), dchr[sect.sct_type].d_name);
	    return RET_SYN;
	}
	getsect(x + 2, y, &sect);
	if (sect.sct_type != SCT_RURAL) {
	    pr("%s is a %s; try again...\n",
	       xyas(x + 2, y, player->cnum), dchr[sect.sct_type].d_name);
	    return RET_SYN;
	}
    } else {
	for (i = 0; i < 300 && !player->aborted; i++) {
	    /* Both x and y should be either odd or even */
	    x = (random() % WORLD_X) - (WORLD_X / 2);
	    y = (((random() % WORLD_Y) - (WORLD_Y / 2)) & ~1) | (x & 1);
	    /*
	     * If either of the two potential
	     * sanctuary sectors are already
	     * owned by someone else, pick
	     * another place on the map.
	     */
	    getsect(x, y, &sect);
	    if (sect.sct_type == SCT_WATER || sect.sct_own != 0)
		continue;
	    getsect(x + 2, y, &sect);
	    if (sect.sct_type == SCT_WATER || sect.sct_own != 0)
		continue;
	    if (isok(x, y))
		break;
	}
	if (i == 300) {
	    pr("couldn't find an empty slot!\n");
	    return RET_FAIL;
	}
    }

    if (player->aborted)
	return RET_FAIL;
    pr("added country %d at %s\n", num, xyas(x, y, player->cnum));
    getsect(x, y, &sect);
    sect.sct_own = num;
    sect.sct_type = SCT_SANCT;
    sect.sct_newtype = SCT_SANCT;
    sect.sct_effic = 100;
    sect.sct_road = 0;
    sect.sct_rail = 0;
    sect.sct_defense = 0;
    sect.sct_mobil = startmob;
    sect.sct_work = 100;
    sect.sct_oldown = num;
    if (at_least_one_100) {
	sect.sct_oil = 100;
	sect.sct_fertil = 100;
	sect.sct_uran = 100;
	sect.sct_min = 100;
	sect.sct_gmin = 100;
    }
    sect.sct_item[I_CIVIL] = opt_RES_POP ? 550 : 999;
    sect.sct_item[I_MILIT] = 55;
    sect.sct_item[I_FOOD] = 1000;
    sect.sct_item[I_UW] = 75;
    putsect(&sect);
    getsect(x + 2, y, &sect);
    sect.sct_own = num;
    sect.sct_type = SCT_SANCT;
    sect.sct_newtype = SCT_SANCT;
    sect.sct_effic = 100;
    sect.sct_road = 0;
    sect.sct_rail = 0;
    sect.sct_defense = 0;
    sect.sct_work = 100;
    sect.sct_oldown = num;
    sect.sct_mobil = startmob;
    if (at_least_one_100) {
	sect.sct_oil = 100;
	sect.sct_fertil = 100;
	sect.sct_uran = 100;
	sect.sct_min = 100;
	sect.sct_gmin = 100;
    }
    sect.sct_item[I_CIVIL] = opt_RES_POP ? 550 : 999;
    sect.sct_item[I_MILIT] = 55;
    sect.sct_item[I_FOOD] = 100;
    sect.sct_item[I_UW] = 75;
    putsect(&sect);
    natp->nat_btu = max_btus;
    natp->nat_stat = STAT_SANCT;
    natp->nat_xcap = x;
    natp->nat_ycap = y;
    natp->nat_xorg = x;
    natp->nat_yorg = y;
    xyabsrange(natp, &defrealm, &absrealm);
    for (i = 0; i < MAXNOR; i++) {
	getrealm(i, num, &newrealm);
	newrealm.r_xl = absrealm.lx;
	newrealm.r_xh = absrealm.hx;
	newrealm.r_yl = absrealm.ly;
	newrealm.r_yh = absrealm.hy;
	newrealm.r_timestamp = current_time;
	putrealm(&newrealm);
    }
    if (players_at_00) {
	natp->nat_xorg = 0;
	natp->nat_yorg = 0;
    }
    natp->nat_money = start_cash;
    natp->nat_level[NAT_HLEV] = start_happiness;
    natp->nat_level[NAT_RLEV] = start_research;
    natp->nat_level[NAT_TLEV] = start_technology;
    natp->nat_level[NAT_ELEV] = start_education;
    natp->nat_tgms = 0;
    (void)close(open(mailbox(buf, num), O_RDWR | O_TRUNC | O_CREAT, 0660));
    putnat(natp);
    return RET_OK;
}

static int nmin, ngold, noil, nur;
static int nfree, navail, nowned;

static int
isok(int x, int y)
{
    signed char *map;
    char *p;
    char buf[1024];

    nmin = ngold = noil = nur = 0;
    navail = nfree = nowned = 0;
    if ((map = malloc((WORLD_X * WORLD_Y) / 2)) == 0) {
	logerror("malloc failed in isok\n");
	pr("Memory error.  Tell the deity.\n");
	return 0;
    }
    memset(map, 0, (WORLD_X * WORLD_Y) / 2);
    ok(map, x, y);
    free(map);
    if (nfree < 5)
	return 0;
    pr("Cap at %s; owned sectors: %d, free sectors: %d, avail: %d\n",
       xyas(x, y, player->cnum), nowned, nfree, navail);
    pr("min: %d, oil: %d, gold: %d, uranium: %d\n",
       nmin, noil, ngold, nur);
    p = getstring("This setup ok? ", buf);
    if (p == 0 || *p != 'y')
	return 0;
    return 1;
}

static void
ok(signed char *map, int x, int y)
{
    struct sctstr sect;
    int dir;
    int id;

    if (navail > MAXAVAIL)
	return;
    id = sctoff(x, y);
    if (map[id])
	return;
    if (!ef_read(EF_SECTOR, id, &sect))
	return;
    if (sect.sct_type == SCT_WATER || sect.sct_type == SCT_BSPAN)
	return;
    navail++;
    if (navail >= MAXAVAIL) {
	pr("At least %d...\n", MAXAVAIL);
	return;
    }
    if (sect.sct_type != SCT_MOUNT && sect.sct_type != SCT_PLAINS) {
	if (sect.sct_own == 0)
	    nfree++;
	else
	    nowned++;
	if (sect.sct_min > 9)
	    nmin++;
	if (sect.sct_gmin > 9)
	    ngold++;
	if (sect.sct_uran > 9)
	    nur++;
	if (sect.sct_oil > 9)
	    noil++;
    }
    map[id] = 1;
    for (dir = DIR_FIRST; dir <= DIR_LAST; dir++)
	ok(map, diroff[dir][0] + x, diroff[dir][1] + y);
}
