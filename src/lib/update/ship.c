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
 *  ship.c: Do production for ships
 * 
 *  Known contributors to this file:
 *     Dave Pare, 1986
 *     Steve McClure, 1996
 */

#include "misc.h"
#include "plague.h"
#include "sect.h"
#include "nat.h"
#include "ship.h"
#include "news.h"
#include "file.h"
#include "product.h"
#include "land.h"
#include "xy.h"
#include "nsc.h"
#include "optlist.h"
#include "player.h"
#include "update.h"
#include "common.h"
#include "subs.h"
#include "gen.h"
#include "lost.h"
#include "budg.h"

#ifndef MIN
#define MIN(x,y)        ((x) > (y) ? (y) : (x))
#endif

static int shiprepair(struct shpstr *, struct natstr *,
		      int *, int);
static void upd_ship(struct shpstr *, int,
		     struct natstr *, int *, int);

int
prod_ship(int etus, int natnum, int *bp, int build)
		/* build = 1, maintain = 0 */
{
    struct shpstr *sp;
    struct natstr *np;
    int n, k = 0;
    int start_money;
    int lastx = 9999, lasty = 9999;

    bp_enable_cachepath();
    for (n = 0; NULL != (sp = getshipp(n)); n++) {
	if (sp->shp_own == 0)
	    continue;
	if (sp->shp_own != natnum)
	    continue;
	np = getnatp(sp->shp_own);
	start_money = np->nat_money;
	if (lastx == 9999 || lasty == 9999) {
	    lastx = sp->shp_x;
	    lasty = sp->shp_y;
	}
	if (lastx != sp->shp_x || lasty != sp->shp_y) {
	    /* Reset the cache */
	    bp_disable_cachepath();
	    bp_clear_cachepath();
	    bp_enable_cachepath();
	}
	upd_ship(sp, etus, np, bp, build);
	if (build && !player->simulation)	/* make sure to only autonav once */
	    nav_ship(sp);	/* autonav the ship */
	sea_money[sp->shp_own] += np->nat_money - start_money;
	if ((build && (np->nat_money != start_money)) || (!build))
	    k++;
	if (player->simulation)
	    np->nat_money = start_money;
    }
    bp_disable_cachepath();
    bp_clear_cachepath();

    if (opt_SAIL) {
	if (build && !player->simulation)	/* make sure to only sail once */
	    sail_ship(natnum);
    }
    return k;
}

static void
upd_ship(struct shpstr *sp, int etus,
	 struct natstr *np, int *bp, int build)
	       /* build = 1, maintain = 0 */
{
    struct sctstr *sectp;
    struct mchrstr *mp;
    int pstage, ptime;
    int oil_gained;
    int max_oil;
    int max_food;
    struct pchrstr *product;
    u_char *resource;
    int dep;
    int n;
    int mult;
    int needed;
    int cost;
    int eff;

    mp = &mchr[(int)sp->shp_type];
    if (build == 1) {
	if (np->nat_priorities[PRI_SBUILD] == 0 || np->nat_money < 0)
	    return;
	if (sp->shp_effic < SHIP_MINEFF || !shiprepair(sp, np, bp, etus)) {
	    makelost(EF_SHIP, sp->shp_own, sp->shp_uid, sp->shp_x,
		     sp->shp_y);
	    sp->shp_own = 0;
	    return;
	}
    } else {
	mult = 1;
	if (np->nat_level[NAT_TLEV] < sp->shp_tech * 0.85)
	    mult = 2;
	cost = -(mult * etus * dmin(0.0, money_ship * mp->m_cost));
	if ((np->nat_priorities[PRI_SMAINT] == 0 || np->nat_money < cost)
	    && !player->simulation) {
	    if ((eff = sp->shp_effic - etus / 5) < SHIP_MINEFF) {
		wu(0, sp->shp_own,
		   "%s lost to lack of maintenance\n", prship(sp));
		makelost(EF_SHIP, sp->shp_own, sp->shp_uid, sp->shp_x,
			 sp->shp_y);
		sp->shp_own = 0;
		return;
	    }
	    wu(0, sp->shp_own,
	       "%s lost %d%% to lack of maintenance\n",
	       prship(sp), sp->shp_effic - eff);
	    sp->shp_effic = eff;
	} else {
	    np->nat_money -= cost;
	}

	if (!player->simulation) {
	    sectp = getsectp(sp->shp_x, sp->shp_y);

	    if ((mp->m_flags & M_OIL) && sectp->sct_type == SCT_WATER) {
		/*
		 * take care of oil production
		 */
		product = &pchr[P_OIL];
		oil_gained = roundavg(total_work(100, etus,
						 sp->shp_item[I_CIVIL],
					         sp->shp_item[I_MILIT],
						 sp->shp_item[I_UW],
						 ITEM_MAX)
				      * (double)sp->shp_effic / 100.0
				      * (double)sectp->sct_oil / 100.0
				      * prod_eff(product, sp->shp_tech));
		max_oil = mp->m_item[I_OIL];
		if (sp->shp_item[I_OIL] + oil_gained > max_oil)
		    oil_gained = max_oil - sp->shp_item[I_OIL];
		sp->shp_item[I_OIL] += oil_gained;
		if (product->p_nrdep != 0 && oil_gained > 0) {
		    resource = (u_char *)sectp + product->p_nrndx;
		    dep = roundavg(oil_gained * product->p_nrdep / 100.0);
		    if (CANT_HAPPEN(dep > *resource))
			dep = *resource;
		    *resource -= dep;
		}
	    }
	    if ((mp->m_flags & M_FOOD) && sectp->sct_type == SCT_WATER) {
		product = &pchr[P_FOOD];
		sp->shp_item[I_FOOD]
		    += roundavg(total_work(100, etus,
					   sp->shp_item[I_CIVIL],
					   sp->shp_item[I_MILIT],
					   sp->shp_item[I_UW],
					   ITEM_MAX)
				* sp->shp_effic / 100.0
				* sectp->sct_fertil / 100.0
				* prod_eff(product, sp->shp_tech));
	    }
	    if ((n = feed_ship(sp, etus, &needed, 1)) > 0) {
		wu(0, sp->shp_own, "%d starved on %s\n", n, prship(sp));
		if (n > 10)
		    nreport(sp->shp_own, N_DIE_FAMINE, 0, 1);
	    }
	    max_food = mp->m_item[I_FOOD];
	    if (sp->shp_item[I_FOOD] > max_food)
		sp->shp_item[I_FOOD] = max_food;
	    /*
	     * do plague stuff.  plague can't break out on ships,
	     * but it can still kill people.
	     */
	    pstage = sp->shp_pstage;
	    ptime = sp->shp_ptime;
	    if (pstage != PLG_HEALTHY) {
		n = plague_people(np, sp->shp_item, &pstage, &ptime, etus);
		switch (n) {
		case PLG_DYING:
		    wu(0, sp->shp_own,
		       "PLAGUE deaths reported on %s\n", prship(sp));
		    nreport(sp->shp_own, N_DIE_PLAGUE, 0, 1);
		    break;
		case PLG_INFECT:
		    wu(0, sp->shp_own, "%s battling PLAGUE\n", prship(sp));
		    break;
		case PLG_INCUBATE:
		    /* Are we still incubating? */
		    if (n == pstage) {
			/* Yes. Will it turn "infectious" next time? */
			if (ptime <= etus) {
			    /* Yes.  Report an outbreak. */
			    wu(0, sp->shp_own,
			       "Outbreak of PLAGUE on %s!\n", prship(sp));
			    nreport(sp->shp_own, N_OUT_PLAGUE, 0, 1);
			}
		    } else {
			/* It has already moved on to "infectious" */
			wu(0, sp->shp_own,
			   "%s battling PLAGUE\n", prship(sp));
		    }
		    break;
		case PLG_EXPOSED:
		    /* Has the plague moved to "incubation" yet? */
		    if (n != pstage) {
			/* Yes. Will it turn "infectious" next time? */
			if (ptime <= etus) {
			    /* Yes.  Report an outbreak. */
			    wu(0, sp->shp_own,
			       "Outbreak of PLAGUE on %s!\n", prship(sp));
			    nreport(sp->shp_own, N_OUT_PLAGUE, 0, 1);
			}
		    }
		    break;
		default:
		    break;
		}

		sp->shp_pstage = pstage;
		sp->shp_ptime = ptime;
	    }
	    pops[sp->shp_own] += sp->shp_item[I_CIVIL];
	}
    }
}

/*
 * idea is: a sector full of workers can fix up eight
 * battleships +8 % eff each etu.  This will cost around
 * 8 * 8 * $40 = $2560!
 */
static int
shiprepair(struct shpstr *ship, struct natstr *np,
	   int *bp, int etus)
{
    int delta;
    struct sctstr *sp;
    struct mchrstr *mp;
    float leftp, buildp;
    int left, build;
    int lcm_needed, hcm_needed;
    int wf;
    int avail;
    int w_p_eff;
    int mult;
    int mvec[I_MAX + 1];
    int rel;

    mp = &mchr[(int)ship->shp_type];
    sp = getsectp(ship->shp_x, ship->shp_y);

    if ((sp->sct_own != ship->shp_own) && (sp->sct_own != 0)) {
	rel = getrel(getnatp(sp->sct_own), ship->shp_own);

	if (rel < FRIENDLY)
	    return 1;
    }

    wf = 0;
    /* only military can work on a military boat */
    if (ship->shp_glim > 0)
	wf = etus * ship->shp_item[I_MILIT] / 2;
    else
	wf = etus * (ship->shp_item[I_CIVIL] / 2 + ship->shp_item[I_MILIT] / 5);

    if (sp->sct_type != SCT_HARBR) {
	wf /= 3;
	avail = wf;
    } else {
	if (!player->simulation)
	    avail = wf + sp->sct_avail * 100;
	else
	    avail = wf + gt_bg_nmbr(bp, sp, I_MAX + 1) * 100;
    }

    w_p_eff = SHP_BLD_WORK(mp->m_lcm, mp->m_hcm);

    if ((sp->sct_off) && (sp->sct_own == ship->shp_own))
	return 1;

    mult = 1;
    if (np->nat_level[NAT_TLEV] < ship->shp_tech * 0.85)
	mult = 2;

    if (ship->shp_effic == 100) {
	/* ship is ok; no repairs needed */
	return 1;
    }

    left = 100 - ship->shp_effic;
    delta = roundavg((double)avail / w_p_eff);
    if (delta <= 0)
	return 1;
    if (delta > etus * ship_grow_scale)
	delta = etus * ship_grow_scale;
    if (delta > left)
	delta = left;

    /* delta is the max amount we can grow */

    left = 100 - ship->shp_effic;
    if (left > delta)
	left = delta;

    leftp = ((float)left / 100.0);
    memset(mvec, 0, sizeof(mvec));
    mvec[I_LCM] = lcm_needed = ldround((double)(mp->m_lcm * leftp), 1);
    mvec[I_HCM] = hcm_needed = ldround((double)(mp->m_hcm * leftp), 1);

    get_materials(sp, bp, mvec, 0);

    if (mvec[I_LCM] >= lcm_needed)
	buildp = leftp;
    else
	buildp = ((float)mvec[I_LCM] / (float)mp->m_lcm);
    if (mvec[I_HCM] < hcm_needed)
	buildp = MIN(buildp, ((float)mvec[I_HCM] / (float)mp->m_hcm));

    build = ldround((double)(buildp * 100.0), 1);
    memset(mvec, 0, sizeof(mvec));
    mvec[I_LCM] = lcm_needed = roundavg((double)(mp->m_lcm * buildp));
    mvec[I_HCM] = hcm_needed = roundavg((double)(mp->m_hcm * buildp));

    get_materials(sp, bp, mvec, 1);

    if (sp->sct_type != SCT_HARBR)
	build = delta;
    wf -= build * w_p_eff;
    if (wf < 0) {
	/*
	 * I didn't use roundavg here, because I want to penalize
	 * the player with a large number of ships.
	 */
	if (!player->simulation)
	    avail = (sp->sct_avail * 100 + wf) / 100;
	else
	    avail = (gt_bg_nmbr(bp, sp, I_MAX + 1) * 100 + wf) / 100;
	if (avail < 0)
	    avail = 0;
	if (!player->simulation)
	    sp->sct_avail = avail;
	else
	    pt_bg_nmbr(bp, sp, I_MAX + 1, avail);
    }
    if (sp->sct_type != SCT_HARBR)
	if ((build + ship->shp_effic) > 80) {
	    build = 80 - ship->shp_effic;
	    if (build < 0)
		build = 0;
	}

    np->nat_money -= mult * mp->m_cost * build / 100.0;
    if (!player->simulation)
	ship->shp_effic += (s_char)build;
    return 1;
}

/*
 * returns the number who starved, if any.
 */
int
feed_ship(struct shpstr *sp, int etus, int *needed, int doit)
{
    double food_eaten, land_eaten;
    int ifood_eaten;
    int can_eat, need;
    int total_people;
    int to_starve;
    int starved;
    struct nstr_item ni;
    struct lndstr *lp;

    if (opt_NOFOOD)
	return 0;		/* no food no work to do */

    total_people
	= sp->shp_item[I_CIVIL] + sp->shp_item[I_MILIT] + sp->shp_item[I_UW];
    food_eaten = etus * eatrate * total_people;
    ifood_eaten = (int)food_eaten;
    if (food_eaten - ifood_eaten > 0)
	ifood_eaten++;
    starved = 0;
    *needed = 0;
    if (!player->simulation && ifood_eaten > sp->shp_item[I_FOOD])
	sp->shp_item[I_FOOD]
	    += supply_commod(sp->shp_own, sp->shp_x, sp->shp_y,
			     I_FOOD, ifood_eaten - sp->shp_item[I_FOOD]);

/* doit - only steal food from land units during the update */
    if (ifood_eaten > sp->shp_item[I_FOOD] && sp->shp_nland > 0 && doit) {
	snxtitem_all(&ni, EF_LAND);
	while ((lp = (struct lndstr *)nxtitemp(&ni, 0)) &&
	       ifood_eaten > sp->shp_item[I_FOOD]) {
	    if (lp->lnd_ship != sp->shp_uid)
		continue;
	    need = ifood_eaten - sp->shp_item[I_FOOD];
	    land_eaten = etus * eatrate * lnd_getmil(lp);
	    if (lp->lnd_item[I_FOOD] - need > land_eaten) {
		sp->shp_item[I_FOOD] += need;
		lp->lnd_item[I_FOOD] -= need;
	    } else if (lp->lnd_item[I_FOOD] - land_eaten > 0) {
		sp->shp_item[I_FOOD] += lp->lnd_item[I_FOOD] - land_eaten;
		lp->lnd_item[I_FOOD] -= lp->lnd_item[I_FOOD] - land_eaten;
	    }
	}
    }

    if (ifood_eaten > sp->shp_item[I_FOOD]) {
	*needed = ifood_eaten - sp->shp_item[I_FOOD];
	can_eat = sp->shp_item[I_FOOD] / (etus * eatrate);
	/* only want to starve off at most 1/2 the populace. */
	if (can_eat < total_people / 2)
	    can_eat = total_people / 2;

	to_starve = total_people - can_eat;
	while (to_starve && sp->shp_item[I_UW]) {
	    to_starve--;
	    starved++;
	    sp->shp_item[I_UW]--;
	}
	while (to_starve && sp->shp_item[I_CIVIL]) {
	    to_starve--;
	    starved++;
	    sp->shp_item[I_CIVIL]--;
	}
	while (to_starve && sp->shp_item[I_MILIT]) {
	    to_starve--;
	    starved++;
	    sp->shp_item[I_MILIT]--;
	}

	sp->shp_item[I_FOOD] = 0;
    } else {
	sp->shp_item[I_FOOD] -= (int)food_eaten;
    }
    return starved;
}
