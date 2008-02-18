/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2008, Dave Pare, Jeff Bailey, Thomas Ruschak,
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
 *  empdump.c: Export/import Empire game state
 * 
 *  Known contributors to this file:
 *     Markus Armbruster, 2008
 */

#include <config.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "file.h"
#include "optlist.h"
#include "prototypes.h"
#include "version.h"
#include "xdump.h"

static void dump_table(int, int);
static void shp_fixup(void);
static void pln_fixup(void);
static void lnd_fixup(void);
static void nuk_fixup(void);

int
main(int argc, char *argv[])
{
    char *config_file = NULL;
    char *import = NULL;
    int export = 0;
    int private = 0;
    int human = 1;
    int opt, i, lineno, type;
    FILE *impf;
    int dirty[EF_MAX];

    while ((opt = getopt(argc, argv, "e:mtxhv")) != EOF) {
	switch (opt) {
	case 'e':
	    config_file = optarg;
	    break;
	case 'h':
	    printf("Usage: %s [OPTION]... [DUMP-FILE]\n"
		   "  -e CONFIG-FILE  configuration file\n"
		   "                  (default %s)\n"
		   "  -m              use machine-readable format\n"
		   "  -t              test import, don't update game state\n"
		   "  -x              export to standard output\n"
		   "  -h              display this help and exit\n"
		   "  -v              display version information and exit\n",
		   argv[0], dflt_econfig);
	    exit(0);
	case 'm':
	    human = 0;
	    break;
	case 't':
	    private = EFF_PRIVATE;
	    break;
	case 'x':
	    export = 1;
	    break;
	case 'v':
	    printf("%s\n\n%s", version, legal);
	    exit(0);
	default:
	    fprintf(stderr, "Try -h for help.\n");
	    exit(1);
	}
    }

    if (argv[optind])
	import = argv[optind++];

    if (import) {
	impf = fopen(import, "r");
	if (!impf) {
	    fprintf(stderr, "Cant open %s for reading (%s)\n",
		    import, strerror(errno));
	    exit(1);
	}
    } else
	private = EFF_PRIVATE;

    /* read configuration & initialize */
    empfile_init();
    if (emp_config(config_file) < 0)
	exit(1);
    empfile_fixup();
    nsc_init();
    if (read_builtin_tables() < 0)
	exit(1);
    if (read_custom_tables() < 0)
	exit(1);
    if (chdir(gamedir)) {
	fprintf(stderr, "Can't chdir to %s (%s)\n",
		gamedir, strerror(errno));
	exit(1);
    }
    global_init();

    for (i = 0; i < EF_MAX; i++) {
	if (!EF_IS_GAME_STATE(i))
	    continue;
	if (!ef_open(i, EFF_MEM | private))
	    exit(1);
    }

    /* import from IMPORT */
    memset(dirty, 0, sizeof(dirty));
    if (import) {
	lineno = 1;
	while ((type = xundump(impf, import, &lineno, EF_BAD)) >= 0)
	    dirty[type] = 1;
	if (type == EF_BAD)
	    exit(1);
	shp_fixup();
	pln_fixup();
	lnd_fixup();
	nuk_fixup();
    }

    if (ef_verify() < 0)
	exit(1);

    /* export to stdout */
    if (export) {
	for (i = 0; i < EF_MAX; i++) {
	    if (!EF_IS_GAME_STATE(i))
		continue;
	    dump_table(i, human);
	}
    }

    /* write out imported data */
    for (i = 0; i < EF_MAX; i++) {
	if (!EF_IS_GAME_STATE(i))
	    continue;
	if (!private && dirty[i]) {
	    if (!ef_close(i))
		exit(1);
	}
    }

    return 0;
}

static void
printf_wrapper(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

static void
dump_table(int type, int human)
{
    struct xdstr xd;
    struct castr *ca;
    int i;
    void *p;

    ca = ef_cadef(type);
    if (!ca)
	return;

    xdinit(&xd, 0, human, printf_wrapper);
    xdhdr(&xd, ef_nameof(type), 0);
    xdcolhdr(&xd, ca);
    for (i = 0; (p = ef_ptr(type, i)); i++) {
	xdflds(&xd, ca, p);
	printf("\n");
    }
    xdftr(&xd, i);
}


/* FIXME remove need for this */

#include <math.h>
#include "ship.h"
#include "plane.h"
#include "land.h"
#include "nuke.h"

static int fit_plane_on_ship(struct plnstr *, struct shpstr *);
static int fit_plane_on_land(struct plnstr *, struct lndstr *);

static void
shp_fixup(void)
{
    int i;
    struct shpstr *sp;

    for (i = 0; (sp = ef_ptr(EF_SHIP, i)); i++) {
	if (!sp->shp_own)
	    continue;
	shp_set_tech(sp, sp->shp_tech);
    }
}

static void
pln_fixup(void)
{
    int i;
    struct plnstr *pp;
    struct shpstr *csp;
    struct lndstr *clp;

    for (i = 0; (pp = ef_ptr(EF_PLANE, i)); i++) {
	if (!pp->pln_own)
	    continue;
	pln_set_tech(pp, pp->pln_tech);
	csp = ef_ptr(EF_SHIP, pp->pln_ship);
	clp = ef_ptr(EF_LAND, pp->pln_land);
	if (csp)
	    fit_plane_on_ship(pp, csp);
	else if (clp)
	    fit_plane_on_land(pp, clp);
    }
}

static void
lnd_fixup(void)
{
    int i;
    struct lndstr *lp;
    struct shpstr *csp;
    struct lndstr *clp;

    for (i = 0; (lp = ef_ptr(EF_LAND, i)); i++) {
	if (!lp->lnd_own)
	    continue;
	lnd_set_tech(lp, lp->lnd_tech);
	csp = ef_ptr(EF_SHIP, lp->lnd_ship);
	clp = ef_ptr(EF_LAND, lp->lnd_land);
	if (csp)
	    csp->shp_nland++;
	else if (clp)
	    clp->lnd_nland++;
    }
}

static void
nuk_fixup(void)
{
    int i;
    struct nukstr *np;
    struct plnstr *cpp;

    for (i = 0; (np = ef_ptr(EF_NUKE, i)); i++) {
	if (!np->nuk_own)
	    continue;
	cpp = ef_ptr(EF_PLANE, np->nuk_plane);
	if (cpp)
	    cpp->pln_nuketype = np->nuk_type;
    }
}

/* FIXME don't copy this from src/lib/subs/???sub.c */

/*
 * Set SP's tech to TLEV along with everything else that depends on it.
 */
void
shp_set_tech(struct shpstr *sp, int tlev)
{
    struct mchrstr *mcp = mchr + sp->shp_type;
    int tech_diff = tlev - mcp->m_tech;

    if (CANT_HAPPEN(tech_diff < 0)) {
      tlev -= tech_diff;
      tech_diff = 0;
    }

    sp->shp_tech = tlev;
    sp->shp_armor = (short)SHP_DEF(mcp->m_armor, tech_diff);
    sp->shp_speed = (short)SHP_SPD(mcp->m_speed, tech_diff);
    sp->shp_visib = (short)SHP_VIS(mcp->m_visib, tech_diff);
    sp->shp_frnge = (short)SHP_RNG(mcp->m_frnge, tech_diff);
    sp->shp_glim  = (short)SHP_FIR(mcp->m_glim, tech_diff);
}

/*
 * Fit a plane of PP's type on ship SP.
 * Adjust SP's plane counters.
 * Updating the plane accordingly is the caller's job.
 * Return whether it fits.
 */
static int
fit_plane_on_ship(struct plnstr *pp, struct shpstr *sp)
{
    struct plchrstr *pcp = plchr + pp->pln_type;
    struct mchrstr *mcp = mchr + sp->shp_type;
    int wanted;

    if (pcp->pl_flags & P_K) {
	/* chopper, try chopper slot first */
	if (sp->shp_nchoppers < mcp->m_nchoppers)
	    return ++sp->shp_nchoppers;
	/* else try plane slot */
	wanted = M_FLY;
    } else if (pcp->pl_flags & P_E) {
	/* x-light, try x-light slot first */
	if (sp->shp_nxlight < mcp->m_nxlight)
	    return ++sp->shp_nxlight;
	/* else try plane slot */
	wanted = M_MSL | M_FLY;
    } else if (!(pcp->pl_flags & P_L)) {
	/* not light, no go */
	wanted = 0;
    } else if (pcp->pl_flags & P_M) {
	/* missile, use plane slot */
	wanted = M_MSL | M_FLY;
    } else {
	/* fixed-wing plane, use plane slot */
	wanted = M_FLY;
    }

    if ((mcp->m_flags & wanted) == 0)
	return 0;		/* ship not capable */

    if (sp->shp_nplane < mcp->m_nplanes)
	return ++sp->shp_nplane;

    return 0;
}

/*
 * Fit a plane of PP's type on land unit LP.
 * Adjust LP's plane counters.
 * Updating the plane accordingly is the caller's job.
 * Return whether it fits.
 */
static int
fit_plane_on_land(struct plnstr *pp, struct lndstr *lp)
{
    struct plchrstr *pcp = plchr + pp->pln_type;
    struct lchrstr *lcp = lchr + lp->lnd_type;

    if ((pcp->pl_flags & P_E) && lp->lnd_nxlight < lcp->l_nxlight)
	return ++lp->lnd_nxlight;

    return 0;
}

/*
 * Set PP's tech to TLEV along with everything else that depends on it.
 */
void
pln_set_tech(struct plnstr *pp, int tlev)
{
    struct plchrstr *pcp = plchr + pp->pln_type;
    int tech_diff = tlev - pcp->pl_tech;
    int limited_range = pp->pln_range < pp->pln_range_max;

    if (CANT_HAPPEN(tech_diff < 0)) {
      tlev -= tech_diff;
      tech_diff = 0;
    }

    pp->pln_tech = tlev;
    pp->pln_att = PLN_ATTDEF(pcp->pl_att, tech_diff);
    pp->pln_def = PLN_ATTDEF(pcp->pl_def, tech_diff);
    pp->pln_acc = PLN_ACC(pcp->pl_acc, tech_diff);
    pp->pln_range_max = PLN_RAN(pcp->pl_range, tech_diff);
    pp->pln_load = PLN_LOAD(pcp->pl_load, tech_diff);

    if (!limited_range || pp->pln_range > pp->pln_range_max)
	pp->pln_range = pp->pln_range_max;
}

/*
 * Set LP's tech to TLEV along with everything else that depends on it.
 */
void
lnd_set_tech(struct lndstr *lp, int tlev)
{
    struct lchrstr *lcp = lchr + lp->lnd_type;
    int tech_diff = tlev - lcp->l_tech;

    if (CANT_HAPPEN(tech_diff < 0)) {
      tlev -= tech_diff;
      tech_diff = 0;
    }

    lp->lnd_tech = tlev;
    lp->lnd_att = (float)LND_ATTDEF(lcp->l_att, tech_diff);
    lp->lnd_def = (float)LND_ATTDEF(lcp->l_def, tech_diff);
    lp->lnd_vul = (int)LND_VUL(lcp->l_vul, tech_diff);
    lp->lnd_spd = (int)LND_SPD(lcp->l_spd, tech_diff);
    lp->lnd_vis = (int)LND_VIS(lcp->l_vis, tech_diff);
    lp->lnd_spy = (int)LND_SPY(lcp->l_spy, tech_diff);
    lp->lnd_rad = (int)LND_RAD(lcp->l_rad, tech_diff);
    lp->lnd_frg = (int)LND_FRG(lcp->l_frg, tech_diff);
    lp->lnd_acc = (int)LND_ACC(lcp->l_acc, tech_diff);
    lp->lnd_dam = (int)LND_DAM(lcp->l_dam, tech_diff);
    lp->lnd_ammo = (int)LND_AMM(lcp->l_ammo, tech_diff);
    lp->lnd_aaf = (int)LND_AAF(lcp->l_aaf, tech_diff);
    lp->lnd_fuelc = (int)LND_FC(lcp->l_fuelc, tech_diff);
    lp->lnd_fuelu = (int)LND_FU(lcp->l_fuelu, tech_diff);
    lp->lnd_maxlight = (int)LND_XPL(lcp->l_nxlight, tech_diff);
    lp->lnd_maxland = (int)LND_MXL(lcp->l_nland, tech_diff);
}

/* FIXME don't copy this from src/lib/update/nat.c */

double
logx(double d, double base)
{
    if (base == 1.0)
	return d;
    return log10(d) / log10(base);
}
