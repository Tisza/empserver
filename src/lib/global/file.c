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
 *  fileglb.c: Empire selection global structures.
 * 
 *  Known contributors to this file:
 *     
 */

#include <stddef.h>
#include "misc.h"
#include "xy.h"
#include "loan.h"
#include "nsc.h"
#include "nuke.h"
#include "plane.h"
#include "ship.h"
#include "land.h"
#include "sect.h"
#include "trade.h"
#include "treaty.h"
#include "file.h"
#include "power.h"
#include "news.h"
#include "nat.h"
#include "lost.h"

#include "gamesdef.h"
#include "commodity.h"

struct empfile empfile[] = {
    {"sect", "sector", EFF_XY | EFF_OWNER,
     0, sizeof(struct sctstr), 0, 0, 0, offsetof(struct sctstr, sct_item),
     -1, -1, 0, 0, 0, 0, 0},
    {"ship", "ship", EFF_XY | EFF_OWNER | EFF_GROUP,
     0, sizeof(struct shpstr), 0, 0, 0, offsetof(struct shpstr, shp_item),
     -1, -1, 0, 0, 0, 0, 0},
    {"plane", "plane", EFF_XY | EFF_OWNER | EFF_GROUP,
     0, sizeof(struct plnstr), 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"land", "land", EFF_XY | EFF_OWNER | EFF_GROUP,
     0, sizeof(struct lndstr), 0, 0, 0, offsetof(struct lndstr, lnd_item),
     -1, -1, 0, 0, 0, 0, 0},
    {"nuke", "nuke", EFF_XY | EFF_OWNER,
     0, sizeof(struct nukstr), 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"news", "news", 0,
     0, sizeof(struct nwsstr), 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"treaty", "treaty", 0,
     0, sizeof(struct trtstr), 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"trade", "trade", 0,
     0, sizeof(struct trdstr), 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"pow", "power", 0,
     0, sizeof(struct powstr), 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"nat", "nation", 0,
     0, sizeof(struct natstr), 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"loan", "loan", 0,
     0, sizeof(struct lonstr), 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"map", "map", 0,
     0, DEF_WORLD_X * DEF_WORLD_Y / 2, 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"bmap", "bmap", 0,
     0, DEF_WORLD_X * DEF_WORLD_Y / 2, 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"commodity", "commodity", 0,
     0, sizeof(struct comstr), 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0},
    {"lost", "lostitems", 0,
     0, sizeof(struct loststr), 0, 0, 0, 0,
     -1, -1, 0, 0, 0, 0, 0}
};
