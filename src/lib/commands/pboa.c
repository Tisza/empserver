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
 *  pboard.c: Capture an enemy plane
 *
 *  Known contributors to this file:
 *     John Yockey, 2001
 */

#include "misc.h"
#include "file.h"
#include "sect.h"
#include "plane.h"
#include "commands.h"

int
pboa(void)
{
    struct sctstr sect;
    struct nstr_item np;
    struct plnstr plane;

    if (!snxtitem(&np, EF_PLANE, player->argp[1]))
	return RET_SYN;
    np.flags = 0;
    while (nxtitem(&np, (s_char *)&plane)) {
	getsect(plane.pln_x, plane.pln_y, &sect);
	if (sect.sct_own != player->cnum)
	    continue;
	takeover_plane(&plane, player->cnum);
    }
    return RET_OK;
}
