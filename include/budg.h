/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2016, Dave Pare, Jeff Bailey, Thomas Ruschak,
 *                Ken Stevens, Steve McClure, Markus Armbruster
 *
 *  Empire is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  ---
 *
 *  See files README, COPYING and CREDITS in the root of the source
 *  tree for related information and legal notices.  It is expected
 *  that future projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  budg.h: Budget related definitions
 *
 *  Known contributors to this file:
 *     Ville Virrankoski, 1995
 *     Markus Armbruster, 2004-2016
 */

#ifndef BUDG_H
#define BUDG_H

#include "types.h"
#include "item.h"

#define SCT_EFFIC (SCT_TYPE_MAX + 1)
#define SCT_MAINT (SCT_TYPE_MAX + 2)
#define SCT_BUDG_MAX SCT_MAINT

struct bp *bp_alloc(void);
void bp_set_from_sect(struct bp *, struct sctstr *);
void bp_to_sect(struct bp *, struct sctstr *);

extern int money[MAXNOC];
extern int pops[MAXNOC];
extern int sea_money[MAXNOC];
extern int lnd_money[MAXNOC];
extern int air_money[MAXNOC];
extern int tpops[MAXNOC];
extern float levels[MAXNOC][4];

#endif
