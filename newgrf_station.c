/* $Id: newgrf_station.c 7675 2006-12-30 23:23:43Z Darkvater $ */

/** @file newgrf_station.c Functions for dealing with station classes and custom stations. */

#include "stdafx.h"
#include "openttd.h"
#include "variables.h"
#include "functions.h"
#include "debug.h"
#include "sprite.h"
#include "table/sprites.h"
#include "table/strings.h"
#include "station.h"
#include "station_map.h"
#include "newgrf_callbacks.h"
#include "newgrf_station.h"
#include "newgrf_spritegroup.h"
#include "date.h"

static StationClass station_classes[STAT_CLASS_MAX];

enum {
	MAX_SPECLIST = 255,
};

/**
 * Reset station classes to their default state.
 * This includes initialising the Default and Waypoint classes with an empty
 * entry, for standard stations and waypoints.
 */
void ResetStationClasses(void)
{
	StationClassID i;
	for (i = 0; i < STAT_CLASS_MAX; i++) {
		station_classes[i].id = 0;
		station_classes[i].name = STR_EMPTY;
		station_classes[i].stations = 0;

		free(station_classes[i].spec);
		station_classes[i].spec = NULL;
	}

	// Set up initial data
	station_classes[0].id = 'DFLT';
	station_classes[0].name = STR_STAT_CLASS_DFLT;
	station_classes[0].stations = 1;
	station_classes[0].spec = malloc(sizeof(*station_classes[0].spec));
	station_classes[0].spec[0] = NULL;

	station_classes[1].id = 'WAYP';
	station_classes[1].name = STR_STAT_CLASS_WAYP;
	station_classes[1].stations = 1;
	station_classes[1].spec = malloc(sizeof(*station_classes[1].spec));
	station_classes[1].spec[0] = NULL;
}

/**
 * Allocate a station class for the given class id.
 * @param classid A 32 bit value identifying the class.
 * @return Index into station_classes of allocated class.
 */
StationClassID AllocateStationClass(uint32 class)
{
	StationClassID i;

	for (i = 0; i < STAT_CLASS_MAX; i++) {
		if (station_classes[i].id == class) {
			// ClassID is already allocated, so reuse it.
			return i;
		} else if (station_classes[i].id == 0) {
			// This class is empty, so allocate it to the ClassID.
			station_classes[i].id = class;
			return i;
		}
	}

	DEBUG(grf, 2)("StationClassAllocate: Already allocated %d classes, using default.", STAT_CLASS_MAX);
	return STAT_CLASS_DFLT;
}

/** Set the name of a custom station class */
void SetStationClassName(StationClassID sclass, StringID name)
{
	assert(sclass < STAT_CLASS_MAX);
	station_classes[sclass].name = name;
}

/** Retrieve the name of a custom station class */
StringID GetStationClassName(StationClassID sclass)
{
	assert(sclass < STAT_CLASS_MAX);
	return station_classes[sclass].name;
}

/** Build a list of station class name StringIDs to use in a dropdown list
 * @return Pointer to a (static) array of StringIDs
 */
StringID *BuildStationClassDropdown(void)
{
	/* Allow room for all station classes, plus a terminator entry */
	static StringID names[STAT_CLASS_MAX + 1];
	uint i;

	/* Add each name */
	for (i = 0; i < STAT_CLASS_MAX && station_classes[i].id != 0; i++) {
		names[i] = station_classes[i].name;
	}
	/* Terminate the list */
	names[i] = INVALID_STRING_ID;

	return names;
}

/**
 * Get the number of station classes in use.
 * @return Number of station classes.
 */
uint GetNumStationClasses(void)
{
	uint i;
	for (i = 0; i < STAT_CLASS_MAX && station_classes[i].id != 0; i++);
	return i;
}

/**
 * Return the number of stations for the given station class.
 * @param sclass Index of the station class.
 * @return Number of stations in the class.
 */
uint GetNumCustomStations(StationClassID sclass)
{
	assert(sclass < STAT_CLASS_MAX);
	return station_classes[sclass].stations;
}

/**
 * Tie a station spec to its station class.
 * @param spec The station spec.
 */
void SetCustomStationSpec(StationSpec *statspec)
{
	StationClass *station_class;
	int i;

	/* If the station has already been allocated, don't reallocate it. */
	if (statspec->allocated) return;

	assert(statspec->sclass < STAT_CLASS_MAX);
	station_class = &station_classes[statspec->sclass];

	i = station_class->stations++;
	station_class->spec = realloc(station_class->spec, station_class->stations * sizeof(*station_class->spec));

	station_class->spec[i] = statspec;
	statspec->allocated = true;
}

/**
 * Retrieve a station spec from a class.
 * @param sclass Index of the station class.
 * @param station The station index with the class.
 * @return The station spec.
 */
const StationSpec *GetCustomStationSpec(StationClassID sclass, uint station)
{
	assert(sclass < STAT_CLASS_MAX);
	if (station < station_classes[sclass].stations)
		return station_classes[sclass].spec[station];

	// If the custom station isn't defined any more, then the GRF file
	// probably was not loaded.
	return NULL;
}


const StationSpec *GetCustomStationSpecByGrf(uint32 grfid, byte localidx)
{
	StationClassID i;
	uint j;

	for (i = STAT_CLASS_DFLT; i < STAT_CLASS_MAX; i++) {
		for (j = 0; j < station_classes[i].stations; j++) {
			const StationSpec *statspec = station_classes[i].spec[j];
			if (statspec == NULL) continue;
			if (statspec->grfid == grfid && statspec->localidx == localidx) return statspec;
		}
	}

	return NULL;
}


/* Evaluate a tile's position within a station, and return the result a bitstuffed format.
 * if not centred: .TNLcCpP, if centred: .TNL..CP
 * T = Tile layout number (GetStationGfx), N = Number of platforms, L = Length of platforms
 * C = Current platform number from start, c = from end
 * P = Position along platform from start, p = from end
 * if centred, C/P start from the centre and c/p are not available.
 */
uint32 GetPlatformInfo(Axis axis, byte tile, int platforms, int length, int x, int y, bool centred)
{
	uint32 retval = 0;

	if (axis == AXIS_X) {
		intswap(platforms, length);
		intswap(x, y);
	}

	/* Limit our sizes to 4 bits */
	platforms = min(15, platforms);
	length    = min(15, length);
	x = min(15, x);
	y = min(15, y);
	if (centred) {
		x -= platforms / 2;
		y -= length / 2;
		SB(retval,  0, 4, y & 0xF);
		SB(retval,  4, 4, x & 0xF);
	} else {
		SB(retval,  0, 4, y);
		SB(retval,  4, 4, length - y - 1);
		SB(retval,  8, 4, x);
		SB(retval, 12, 4, platforms - x - 1);
	}
	SB(retval, 16, 4, length);
	SB(retval, 20, 4, platforms);
	SB(retval, 24, 4, tile);

	return retval;
}


/* Find the end of a railway station, from the tile, in the direction of delta.
 * If check_type is set, we stop if the custom station type changes.
 * If check_axis is set, we stop if the station direction changes.
 */
static TileIndex FindRailStationEnd(TileIndex tile, TileIndexDiff delta, bool check_type, bool check_axis)
{
	bool waypoint;
	byte orig_type = 0;
	Axis orig_axis = AXIS_X;

	waypoint = IsTileType(tile, MP_RAILWAY);

	if (waypoint) {
		if (check_axis) orig_axis = GetWaypointAxis(tile);
	} else {
		if (check_type) orig_type = GetCustomStationSpecIndex(tile);
		if (check_axis) orig_axis = GetRailStationAxis(tile);
	}

	while (true) {
		TileIndex new_tile = TILE_ADD(tile, delta);

		if (waypoint) {
			if (!IsTileType(new_tile, MP_RAILWAY)) break;
			if (!IsRailWaypoint(new_tile)) break;
			if (check_axis && GetWaypointAxis(new_tile) != orig_axis) break;
		} else {
			if (!IsRailwayStationTile(new_tile)) break;
			if (check_type && GetCustomStationSpecIndex(new_tile) != orig_type) break;
			if (check_axis && GetRailStationAxis(new_tile) != orig_axis) break;
		}

		tile = new_tile;
	}
	return tile;
}


static uint32 GetPlatformInfoHelper(TileIndex tile, bool check_type, bool check_axis, bool centred)
{
	int tx = TileX(tile);
	int ty = TileY(tile);
	int sx = TileX(FindRailStationEnd(tile, TileDiffXY(-1,  0), check_type, check_axis));
	int sy = TileY(FindRailStationEnd(tile, TileDiffXY( 0, -1), check_type, check_axis));
	int ex = TileX(FindRailStationEnd(tile, TileDiffXY( 1,  0), check_type, check_axis)) + 1;
	int ey = TileY(FindRailStationEnd(tile, TileDiffXY( 0,  1), check_type, check_axis)) + 1;
	Axis axis = IsTileType(tile, MP_RAILWAY) ? GetWaypointAxis(tile) : GetRailStationAxis(tile);

	tx -= sx; ex -= sx;
	ty -= sy; ey -= sy;

	return GetPlatformInfo(axis, IsTileType(tile, MP_RAILWAY) ? 2 : GetStationGfx(tile), ex, ey, tx, ty, centred);
}


static uint32 GetRailContinuationInfo(TileIndex tile)
{
	/* Tile offsets and exit dirs for X axis */
	static Direction x_dir[8] = { DIR_SW, DIR_NE, DIR_SE, DIR_NW, DIR_S, DIR_E, DIR_W, DIR_N };
	static DiagDirection x_exits[8] = { DIAGDIR_SW, DIAGDIR_NE, DIAGDIR_SE, DIAGDIR_NW, DIAGDIR_SW, DIAGDIR_NE, DIAGDIR_SW, DIAGDIR_NE };

	/* Tile offsets and exit dirs for Y axis */
	static Direction y_dir[8] = { DIR_SE, DIR_NW, DIR_SW, DIR_NE, DIR_S, DIR_W, DIR_E, DIR_N };
	static DiagDirection y_exits[8] = { DIAGDIR_SE, DIAGDIR_NW, DIAGDIR_SW, DIAGDIR_NE, DIAGDIR_SE, DIAGDIR_NW, DIAGDIR_SE, DIAGDIR_NW };

	Axis axis = IsTileType(tile, MP_RAILWAY) ? GetWaypointAxis(tile) : GetRailStationAxis(tile);

	/* Choose appropriate lookup table to use */
	Direction *dir = axis == AXIS_X ? x_dir : y_dir;
	DiagDirection *diagdir = axis == AXIS_X ? x_exits : y_exits;

	uint32 res = 0;
	uint i;

	for (i = 0; i < lengthof(x_dir); i++, dir++, diagdir++) {
		uint32 ts = GetTileTrackStatus(tile + TileOffsByDir(*dir), TRANSPORT_RAIL);
		if (ts != 0) {
			/* If there is any track on the tile, set the bit in the second byte */
			SETBIT(res, i + 8);

			/* If any track reaches our exit direction, set the bit in the lower byte */
			if (ts & DiagdirReachesTracks(*diagdir)) SETBIT(res, i);
		}
	}

	return res;
}


/* Station Resolver Functions */
static uint32 StationGetRandomBits(const ResolverObject *object)
{
	const Station *st = object->u.station.st;
	const TileIndex tile = object->u.station.tile;
	return (st == NULL ? 0 : st->random_bits) | (tile == INVALID_TILE ? 0 : GetStationTileRandomBits(tile) << 16);
}


static uint32 StationGetTriggers(const ResolverObject *object)
{
	const Station *st = object->u.station.st;
	return st == NULL ? 0 : st->waiting_triggers;
}


static void StationSetTriggers(const ResolverObject *object, int triggers)
{
	Station *st = (Station*)object->u.station.st;
	assert(st != NULL);
	st->waiting_triggers = triggers;
}


static uint32 StationGetVariable(const ResolverObject *object, byte variable, byte parameter, bool *available)
{
	const Station *st = object->u.station.st;
	TileIndex tile = object->u.station.tile;

	if (st == NULL) {
		/* Station does not exist, so we're in a purchase list */
		switch (variable) {
			case 0x40:
			case 0x41:
			case 0x46:
			case 0x47:
			case 0x49: return 0x2110000;       /* Platforms, tracks & position */
			case 0x42: return 0;               /* Rail type (XXX Get current type from GUI?) */
			case 0x43: return _current_player; /* Station owner */
			case 0x44: return 2;               /* PBS status */
			case 0xFA: return max(_date - DAYS_TILL_ORIGINAL_BASE_YEAR, 0); /* Build date */
		}

		*available = false;
		return -1;
	}

	switch (variable) {
		/* Calculated station variables */
		case 0x40: return GetPlatformInfoHelper(tile, false, false, false);
		case 0x41: return GetPlatformInfoHelper(tile, true,  false, false);
		case 0x42: /* Terrain and rail type */
			return ((_opt.landscape == LT_HILLY && GetTileZ(tile) > _opt.snow_line) ? 4 : 0) |
			       (GetRailType(tile) << 8);
		case 0x43: return st->owner; /* Station owner */
		case 0x44: return 2;         /* PBS status */
		case 0x45: return GetRailContinuationInfo(tile);
		case 0x46: return GetPlatformInfoHelper(tile, false, false, true);
		case 0x47: return GetPlatformInfoHelper(tile, true,  false, true);
		case 0x48: { /* Accepted cargo types */
			CargoID cargo_type;
			uint32 value = 0;

			for (cargo_type = 0; cargo_type < NUM_CARGO; cargo_type++) {
				if (HASBIT(st->goods[cargo_type].waiting_acceptance, 15)) SETBIT(value, cargo_type);
			}
			return value;
		}
		case 0x49: return GetPlatformInfoHelper(tile, false, true, false);

		/* Variables which use the parameter */
		case 0x60: return GB(st->goods[parameter].waiting_acceptance, 0, 12);
		case 0x61: return st->goods[parameter].days_since_pickup;
		case 0x62: return st->goods[parameter].rating;
		case 0x63: return st->goods[parameter].enroute_time;
		case 0x64: return st->goods[parameter].last_speed | (st->goods[parameter].last_age << 8);
		case 0x65: return GB(st->goods[parameter].waiting_acceptance, 12, 4);

		/* General station properties */
		case 0x82: return 50;
		case 0x84: return st->string_id;
		case 0x86: return 0;
		case 0x8A: return st->had_vehicle_of_type;
		case 0xF0: return st->facilities;
		case 0xF1: return st->airport_type;
		case 0xF2: return st->truck_stops->status;
		case 0xF3: return st->bus_stops->status;
		case 0xF6: return st->airport_flags;
		case 0xF7: return GB(st->airport_flags, 8, 8);
		case 0xFA: return max(st->build_date - DAYS_TILL_ORIGINAL_BASE_YEAR, 0);
	}

	/* Handle cargo variables (deprecated) */
	if (variable >= 0x8C && variable <= 0xEC) {
		const GoodsEntry *g = &st->goods[GB(variable - 0x8C, 3, 4)];
		switch (GB(variable - 0x8C, 0, 3)) {
			case 0: return g->waiting_acceptance;
			case 1: return GB(g->waiting_acceptance, 8, 8);
			case 2: return g->days_since_pickup;
			case 3: return g->rating;
			case 4: return g->enroute_from;
			case 5: return g->enroute_time;
			case 6: return g->last_speed;
			case 7: return g->last_age;
		}
	}

	DEBUG(grf, 1)("Unhandled station property 0x%X", variable);

	*available = false;
	return -1;
}


static const SpriteGroup *StationResolveReal(const ResolverObject *object, const SpriteGroup *group)
{
	const Station *st = object->u.station.st;
	const StationSpec *statspec = object->u.station.statspec;
	uint set;

	uint cargo = 0;
	CargoID cargo_type = object->u.station.cargo_type;

	if (st == NULL || statspec->sclass == STAT_CLASS_WAYP) {
		return group->g.real.loading[0];
	}

	switch (cargo_type) {
		case GC_INVALID:
		case GC_DEFAULT_NA:
		case GC_PURCHASE:
			cargo = 0;
			break;

		case GC_DEFAULT:
			for (cargo_type = 0; cargo_type < NUM_CARGO; cargo_type++) {
				cargo += GB(st->goods[cargo_type].waiting_acceptance, 0, 12);
			}
			break;

		default:
			cargo = GB(st->goods[_local_cargo_id_ctype[cargo_type]].waiting_acceptance, 0, 12);
			break;
	}

	if (HASBIT(statspec->flags, 1)) cargo /= (st->trainst_w + st->trainst_h);
	cargo = min(0xfff, cargo);

	if (cargo > statspec->cargo_threshold) {
		if (group->g.real.num_loading > 0) {
			set = ((cargo - statspec->cargo_threshold) * group->g.real.num_loading) / (4096 - statspec->cargo_threshold);
			return group->g.real.loading[set];
		}
	} else {
		if (group->g.real.num_loaded > 0) {
			set = (cargo * group->g.real.num_loaded) / (statspec->cargo_threshold + 1);
			return group->g.real.loaded[set];
		}
	}

	return group->g.real.loading[0];
}


static void NewStationResolver(ResolverObject *res, const StationSpec *statspec, const Station *st, TileIndex tile)
{
	res->GetRandomBits = StationGetRandomBits;
	res->GetTriggers   = StationGetTriggers;
	res->SetTriggers   = StationSetTriggers;
	res->GetVariable   = StationGetVariable;
	res->ResolveReal   = StationResolveReal;

	res->u.station.st       = st;
	res->u.station.statspec = statspec;
	res->u.station.tile     = tile;

	res->callback        = 0;
	res->callback_param1 = 0;
	res->callback_param2 = 0;
	res->last_value      = 0;
	res->trigger         = 0;
	res->reseed          = 0;
}

static const SpriteGroup *ResolveStation(const StationSpec *statspec, const Station *st, ResolverObject *object)
{
	const SpriteGroup *group;
	CargoID ctype = GC_DEFAULT_NA;

	if (st == NULL) {
		/* No station, so we are in a purchase list */
		ctype = GC_PURCHASE;
	} else {
		CargoID cargo;

		/* Pick the first cargo that we have waiting */
		for (cargo = 0; cargo < NUM_GLOBAL_CID; cargo++) {
			CargoID lcid = _local_cargo_id_ctype[cargo];
			if (lcid != CT_INVALID && statspec->spritegroup[cargo] != NULL && GB(st->goods[lcid].waiting_acceptance, 0, 12) != 0) {
				ctype = cargo;
				break;
			}
		}
	}

	group = statspec->spritegroup[ctype];
	if (group == NULL) {
		ctype = GC_DEFAULT;
		group = statspec->spritegroup[ctype];
	}

	if (group == NULL) return NULL;

	/* Remember the cargo type we've picked */
	object->u.station.cargo_type = ctype;

	return Resolve(group, object);
}

SpriteID GetCustomStationRelocation(const StationSpec *statspec, const Station *st, TileIndex tile)
{
	const SpriteGroup *group;
	ResolverObject object;

	NewStationResolver(&object, statspec, st, tile);

	group = ResolveStation(statspec, st, &object);
	if (group == NULL || group->type != SGT_RESULT) return 0;
	return group->g.result.sprite - 0x42D;
}


SpriteID GetCustomStationGroundRelocation(const StationSpec *statspec, const Station *st, TileIndex tile)
{
	const SpriteGroup *group;
	ResolverObject object;

	NewStationResolver(&object, statspec, st, tile);
	object.callback_param1 = 1; /* Indicate we are resolving the ground sprite */

	group = ResolveStation(statspec, st, &object);
	if (group == NULL || group->type != SGT_RESULT) return 0;
	return group->g.result.sprite - 0x42D;
}


uint16 GetStationCallback(uint16 callback, uint32 param1, uint32 param2, const StationSpec *statspec, const Station *st, TileIndex tile)
{
	const SpriteGroup *group;
	ResolverObject object;

	NewStationResolver(&object, statspec, st, tile);

	object.callback        = callback;
	object.callback_param1 = param1;
	object.callback_param2 = param2;

	group = ResolveStation(statspec, st, &object);
	if (group == NULL || group->type != SGT_CALLBACK) return CALLBACK_FAILED;
	return group->g.callback.result;
}


/**
 * Allocate a StationSpec to a Station. This is called once per build operation.
 * @param spec StationSpec to allocate.
 * @param st Station to allocate it to.
 * @param exec Whether to actually allocate the spec.
 * @return Index within the Station's spec list, or -1 if the allocation failed.
 */
int AllocateSpecToStation(const StationSpec *statspec, Station *st, bool exec)
{
	uint i;

	if (statspec == NULL) return 0;

	/* Check if this spec has already been allocated */
	for (i = 1; i < st->num_specs && i < MAX_SPECLIST; i++) {
		if (st->speclist[i].spec == statspec) return i;
	}

	for (i = 1; i < st->num_specs && i < MAX_SPECLIST; i++) {
		if (st->speclist[i].spec == NULL && st->speclist[i].grfid == 0) break;
	}

	if (i == MAX_SPECLIST) return -1;

	if (exec) {
		if (i >= st->num_specs) {
			st->num_specs = i + 1;
			st->speclist = realloc(st->speclist, st->num_specs * sizeof(*st->speclist));

			if (st->num_specs == 2) {
				/* Initial allocation */
				st->speclist[0].spec     = NULL;
				st->speclist[0].grfid    = 0;
				st->speclist[0].localidx = 0;
			}
		}

		st->speclist[i].spec     = statspec;
		st->speclist[i].grfid    = statspec->grfid;
		st->speclist[i].localidx = statspec->localidx;
	}

	return i;
}


/** Deallocate a StationSpec from a Station. Called when removing a single station tile.
 * @param st Station to work with.
 * @param specindex Index of the custom station within the Station's spec list.
 * @return Indicates whether the StationSpec was deallocated.
 */
void DeallocateSpecFromStation(Station* st, byte specindex)
{
	/* specindex of 0 (default) is never freeable */
	if (specindex == 0) return;

	/* Check all tiles over the station to check if the specindex is still in use */
	BEGIN_TILE_LOOP(tile, st->trainst_w, st->trainst_h, st->train_tile) {
		if (IsTileType(tile, MP_STATION) && GetStationIndex(tile) == st->index && IsRailwayStation(tile) && GetCustomStationSpecIndex(tile) == specindex) {
			return;
		}
	} END_TILE_LOOP(tile, st->trainst_w, st->trainst_h, st->train_tile)

	/* This specindex is no longer in use, so deallocate it */
	st->speclist[specindex].spec     = NULL;
	st->speclist[specindex].grfid    = 0;
	st->speclist[specindex].localidx = 0;

	/* If this was the highest spec index, reallocate */
	if (specindex == st->num_specs - 1) {
		for (; st->speclist[st->num_specs - 1].grfid == 0 && st->num_specs > 1; st->num_specs--);

		if (st->num_specs > 1) {
			st->speclist = realloc(st->speclist, st->num_specs * sizeof(*st->speclist));
		} else {
			free(st->speclist);
			st->num_specs = 0;
			st->speclist  = NULL;
		}
	}
}

/** Draw representation of a station tile for GUI purposes.
 * @param x, y Position of image.
 * @param dir Direction.
 * @param railtype Rail type.
 * @param sclass, station Type of station.
 * @return True if the tile was drawn (allows for fallback to default graphic)
 */
bool DrawStationTile(int x, int y, RailType railtype, Axis axis, StationClassID sclass, uint station)
{
	const StationSpec *statspec;
	const DrawTileSprites *sprites;
	const DrawTileSeqStruct *seq;
	const RailtypeInfo *rti = GetRailTypeInfo(railtype);
	SpriteID relocation;
	PalSpriteID image;
	PalSpriteID colourmod = SPRITE_PALETTE(PLAYER_SPRITE_COLOR(_local_player));
	uint tile = 2;

	statspec = GetCustomStationSpec(sclass, station);
	if (statspec == NULL) return false;

	relocation = GetCustomStationRelocation(statspec, NULL, INVALID_TILE);

	if (HASBIT(statspec->callbackmask, CBM_CUSTOM_LAYOUT)) {
		uint16 callback = GetStationCallback(CBID_STATION_SPRITE_LAYOUT, 0x2110000, 0, statspec, NULL, INVALID_TILE);
		if (callback != CALLBACK_FAILED) tile = callback;
	}

	if (statspec->renderdata == NULL) {
		sprites = GetStationTileLayout(tile + axis);
	} else {
		sprites = &statspec->renderdata[(tile < statspec->tiles) ? tile + axis : axis];
	}

	image = sprites->ground_sprite;
	if (HASBIT(image, 31)) {
		CLRBIT(image, 31);
		image += GetCustomStationGroundRelocation(statspec, NULL, INVALID_TILE);
		image += rti->custom_ground_offset;
	} else {
		image += rti->total_offset;
	}

	if (image & PALETTE_MODIFIER_COLOR) image &= SPRITE_MASK;
	DrawSprite(image, x, y);

	foreach_draw_tile_seq(seq, sprites->seq) {
		Point pt;
		image = seq->image;
		if (HASBIT(image, 30)) {
			CLRBIT(image, 30);
			image += rti->total_offset;
		} else {
			image += relocation;
		}

		if ((byte)seq->delta_z != 0x80) {
			pt = RemapCoords(seq->delta_x, seq->delta_y, seq->delta_z);
			DrawSprite((image & SPRITE_MASK) | colourmod, x + pt.x, y + pt.y);
		}
	}

	return true;
}


static const StationSpec* GetStationSpec(TileIndex t)
{
	const Station* st;
	uint specindex;

	if (!IsCustomStationSpecIndex(t)) return NULL;

	st = GetStationByTile(t);
	specindex = GetCustomStationSpecIndex(t);
	return specindex < st->num_specs ? st->speclist[specindex].spec : NULL;
}


/* Check if a rail station tile is traversable.
 * XXX This could be cached (during build) in the map array to save on all the dereferencing */
bool IsStationTileBlocked(TileIndex tile)
{
	const StationSpec* statspec = GetStationSpec(tile);

	return statspec != NULL && HASBIT(statspec->blocked, GetStationGfx(tile));
}

/* Check if a rail station tile is electrifiable.
 * XXX This could be cached (during build) in the map array to save on all the dereferencing */
bool IsStationTileElectrifiable(TileIndex tile)
{
	const StationSpec* statspec = GetStationSpec(tile);

	return
		statspec == NULL ||
		HASBIT(statspec->pylons, GetStationGfx(tile)) ||
		!HASBIT(statspec->wires, GetStationGfx(tile));
}
