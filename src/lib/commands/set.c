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
 *  set.c: Place units/ships/planes/nukes up for sale.
 * 
 *  Known contributors to this file:
 *     Dave Pare, 1986
 *     Pat Loney, 1992
 *     Steve McClure, 1996
 */

#include "misc.h"
#include "sect.h"
#include "ship.h"
#include "land.h"
#include "plane.h"
#include "xy.h"
#include "nsc.h"
#include "nat.h"
#include "trade.h"
#include "file.h"
#include "player.h"
#include "commands.h"
#include "optlist.h"

/*
 * format: set <type> <SHIP/NUKE> <PRICE>
 */
int
set(void)
{
    char *p;
    int type;
    int price;
    char prompt[80];
    struct trdstr trade;
    struct nstr_item ni;
    struct nstr_item ni_trade;
    union trdgenstr item;
    union trdgenstr check;
    struct sctstr sect;
    int freeslot;
    int foundslot;
    int id;
    time_t now;
    s_char buf[1024];

    if (!opt_MARKET) {
	pr("The market is disabled.\n");
	return RET_FAIL;
    }
    check_market();
    check_trade();

    if ((p = getstarg(player->argp[1], "Item type? ", buf)) == 0)
	return RET_SYN;
    if ((type = ef_byname(p)) < 0) {
	pr("%s: not an item type\n", p);
	return RET_SYN;
    }
    if (type == EF_SECTOR)
	type = EF_SHIP;
    if (type == EF_NEWS)
	type = EF_NUKE;
    if (type == EF_LOAN)
	type = EF_LAND;
    if (!snxtitem(&ni, type, player->argp[2]))
	return RET_SYN;
    while (nxtitem(&ni, (char *)&item)) {
	if (!player->owner && !player->god)
	    continue;
	getsect(item.gen.trg_x, item.gen.trg_y, &sect);
	if (!military_control(&sect)) {
	    pr("Military control required to sell goods.\n");
	    return RET_FAIL;
	}
	trade.trd_type = type;
	sprintf(prompt, "%s #%d; Price? ",
		trade_nameof(&trade, &item), ni.cur);
	memcpy(&check, &item, sizeof(union trdgenstr));
	if ((p = getstarg(player->argp[3], prompt, buf)) == 0)
	    break;
	if (memcmp(&check, &item, sizeof(union trdgenstr))) {
	    pr("That item has changed!\n");
	    return RET_FAIL;
	}
	if ((price = atoi(p)) < 0)
	    continue;
	foundslot = -1;
	freeslot = -1;
	snxtitem_all(&ni_trade, EF_TRADE);
	while (nxtitem(&ni_trade, (char *)&trade)) {
	    if (trade.trd_owner == 0)
		freeslot = ni_trade.cur;
	    if (trade.trd_unitid == ni.cur && trade.trd_type == type) {
		foundslot = ni_trade.cur;
		break;
	    }
	}
	if (price <= 0) {
	    if (foundslot >= 0) {
		pr("%s #%d (lot #%d) removed from trading\n",
		   trade_nameof(&trade, &item), ni.cur, foundslot);
		memset(&trade, 0, sizeof(trade));
		puttrade(ni_trade.cur, &trade);
	    }
	} else {
	    if (foundslot >= 0)
		id = foundslot;
	    else if (freeslot >= 0)
		id = freeslot;
	    else {
		ef_extend(EF_TRADE, 1);
		id = ni_trade.cur;
	    }
	    trade.trd_x = item.gen.trg_x;
	    trade.trd_y = item.gen.trg_x;
	    trade.trd_type = type;
	    trade.trd_owner = player->cnum;
	    trade.trd_uid = id;
	    trade.trd_unitid = ni.cur;
	    trade.trd_price = price;
	    (void)time(&now);
	    trade.trd_markettime = now;
	    trade.trd_maxbidder = player->cnum;
	    puttrade(id, &trade);
	    pr("%s #%d (lot #%d) price %s to $%d\n",
	       trade_nameof(&trade, &item), ni.cur,
	       id, foundslot >= 0 ? "reset" : "set", price);
	}
    }
    return RET_OK;
}
