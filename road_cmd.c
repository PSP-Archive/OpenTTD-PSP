/* $Id: road_cmd.c 10952 2007-08-20 15:17:24Z glx $ */

#include "stdafx.h"
#include "openttd.h"
#include "bridge_map.h"
#include "rail_map.h"
#include "road_map.h"
#include "sprite.h"
#include "table/sprites.h"
#include "table/strings.h"
#include "functions.h"
#include "map.h"
#include "tile.h"
#include "town_map.h"
#include "vehicle.h"
#include "viewport.h"
#include "command.h"
#include "player.h"
#include "town.h"
#include "gfx.h"
#include "sound.h"
#include "yapf/yapf.h"
#include "depot.h"


static uint CountRoadBits(RoadBits r)
{
	uint count = 0;

	if (r & ROAD_NW) ++count;
	if (r & ROAD_SW) ++count;
	if (r & ROAD_SE) ++count;
	if (r & ROAD_NE) ++count;
	return count;
}


static bool CheckAllowRemoveRoad(TileIndex tile, RoadBits remove, bool* edge_road)
{
	RoadBits present;
	RoadBits n;
	Owner owner;
	*edge_road = true;

	if (_game_mode == GM_EDITOR) return true;

	// Only do the special processing for actual players.
	if (!IsValidPlayer(_current_player)) return true;

	owner = IsLevelCrossingTile(tile) ? GetCrossingRoadOwner(tile) : GetTileOwner(tile);

	// Only do the special processing if the road is owned
	// by a town
	if (owner != OWNER_TOWN) return (owner == OWNER_NONE) || CheckOwnership(owner);

	if (_cheats.magic_bulldozer.value) return true;

	// Get a bitmask of which neighbouring roads has a tile
	n = 0;
	present = GetAnyRoadBits(tile);
	if (present & ROAD_NE && GetAnyRoadBits(TILE_ADDXY(tile,-1, 0)) & ROAD_SW) n |= ROAD_NE;
	if (present & ROAD_SE && GetAnyRoadBits(TILE_ADDXY(tile, 0, 1)) & ROAD_NW) n |= ROAD_SE;
	if (present & ROAD_SW && GetAnyRoadBits(TILE_ADDXY(tile, 1, 0)) & ROAD_NE) n |= ROAD_SW;
	if (present & ROAD_NW && GetAnyRoadBits(TILE_ADDXY(tile, 0,-1)) & ROAD_SE) n |= ROAD_NW;

	// If 0 or 1 bits are set in n, or if no bits that match the bits to remove,
	// then allow it
	if ((n & (n - 1)) != 0 && (n & remove) != 0) {
		Town *t;
		*edge_road = false;
		// you can remove all kind of roads with extra dynamite
		if (_patches.extra_dynamite) return true;

		t = ClosestTownFromTile(tile, (uint)-1);

		SetDParam(0, t->index);
		_error_message = STR_2009_LOCAL_AUTHORITY_REFUSES;
		return false;
	}

	return true;
}


/** Delete a piece of road.
 * @param tile tile where to remove road from
 * @param p1 road piece flags
 * @param p2 unused
 */
int32 CmdRemoveRoad(TileIndex tile, uint32 flags, uint32 p1, uint32 p2)
{
	// cost for removing inner/edge -roads
	static const uint16 road_remove_cost[2] = {50, 18};

	Owner owner;
	Town *t;
	/* true if the roadpiece was always removeable,
	 * false if it was a center piece. Affects town ratings drop */
	bool edge_road;
	RoadBits pieces;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	/* Road pieces are max 4 bitset values (NE, NW, SE, SW) */
	if (p1 >> 4) return CMD_ERROR;
	pieces = p1;

	if (!IsTileType(tile, MP_STREET) && !IsTileType(tile, MP_TUNNELBRIDGE)) return CMD_ERROR;

	owner = IsLevelCrossingTile(tile) ? GetCrossingRoadOwner(tile) : GetTileOwner(tile);

	if (owner == OWNER_TOWN && _game_mode != GM_EDITOR) {
		/* Are we removing a piece of road below a bridge, or not. If below
		 * a bridge we need to calculate the town's index as it is not saved
		 * in the map array (no space) */
		if (IsTileType(tile, MP_TUNNELBRIDGE)) {
			t = ClosestTownFromTile(tile, _patches.dist_local_authority);
		} else {
			t = GetTownByTile(tile);
		}
	} else {
		t = NULL;
	}

	if (!CheckAllowRemoveRoad(tile, pieces, &edge_road)) return CMD_ERROR;

	switch (GetTileType(tile)) {
		case MP_TUNNELBRIDGE:
			if (!EnsureNoVehicleOnGround(tile)) return CMD_ERROR;

			if (!IsBridge(tile) ||
					!IsBridgeMiddle(tile) ||
					!IsTransportUnderBridge(tile) ||
					GetTransportTypeUnderBridge(tile) != TRANSPORT_ROAD ||
					(pieces & ComplementRoadBits(GetRoadBitsUnderBridge(tile))) != 0) {
				return CMD_ERROR;
			}

			if (flags & DC_EXEC) {
				ChangeTownRating(t, -road_remove_cost[(byte)edge_road], RATING_ROAD_MINIMUM);
				SetClearUnderBridge(tile);
				MarkTileDirtyByTile(tile);
			}
			return _price.remove_road * 2;

		case MP_STREET:
			if (!EnsureNoVehicle(tile)) return CMD_ERROR;

			// check if you're allowed to remove the street owned by a town
			// removal allowance depends on difficulty setting
			if (!CheckforTownRating(flags, t, ROAD_REMOVE)) return CMD_ERROR;

			switch (GetRoadTileType(tile)) {
				case ROAD_TILE_NORMAL: {
					RoadBits present = GetRoadBits(tile);
					RoadBits c = pieces;

					if (HasRoadWorks(tile)) return_cmd_error(STR_ROAD_WORKS_IN_PROGRESS);

					if (GetTileSlope(tile, NULL) != SLOPE_FLAT &&
							(present == ROAD_Y || present == ROAD_X)) {
						c |= (c & 0xC) >> 2;
						c |= (c & 0x3) << 2;
					}

					// limit the bits to delete to the existing bits.
					c &= present;
					if (c == 0) return CMD_ERROR;

					if (flags & DC_EXEC) {
						ChangeTownRating(t, -road_remove_cost[(byte)edge_road], RATING_ROAD_MINIMUM);

						present ^= c;
						if (present == 0) {
							DoClearSquare(tile);
						} else {
							SetRoadBits(tile, present);
							MarkTileDirtyByTile(tile);
						}
					}
					return CountRoadBits(c) * _price.remove_road;
				}

				case ROAD_TILE_CROSSING: {
					if (pieces & ComplementRoadBits(GetCrossingRoadBits(tile))) {
						return CMD_ERROR;
					}

					if (flags & DC_EXEC) {
						ChangeTownRating(t, -road_remove_cost[(byte)edge_road], RATING_ROAD_MINIMUM);

						MakeRailNormal(tile, GetTileOwner(tile), GetCrossingRailBits(tile), GetRailTypeCrossing(tile));
						MarkTileDirtyByTile(tile);
						YapfNotifyTrackLayoutChange(tile, FIND_FIRST_BIT(GetTrackBits(tile)));
					}
					return _price.remove_road * 2;
				}

				default:
				case ROAD_TILE_DEPOT:
					return CMD_ERROR;
			}

		default: return CMD_ERROR;
	}
}


static const RoadBits _valid_tileh_slopes_road[][15] = {
	// set of normal ones
	{
		ROAD_ALL, 0, 0,
		ROAD_X,   0, 0,  // 3, 4, 5
		ROAD_Y,   0, 0,
		ROAD_Y,   0, 0,  // 9, 10, 11
		ROAD_X,   0, 0
	},
	// allowed road for an evenly raised platform
	{
		0,
		ROAD_SW | ROAD_NW,
		ROAD_SW | ROAD_SE,
		ROAD_Y  | ROAD_SW,

		ROAD_SE | ROAD_NE, // 4
		ROAD_ALL,
		ROAD_X  | ROAD_SE,
		ROAD_ALL,

		ROAD_NW | ROAD_NE, // 8
		ROAD_X  | ROAD_NW,
		ROAD_ALL,
		ROAD_ALL,

		ROAD_Y  | ROAD_NE, // 12
		ROAD_ALL,
		ROAD_ALL
	},
};


static uint32 CheckRoadSlope(Slope tileh, RoadBits* pieces, RoadBits existing)
{
	RoadBits road_bits;

	if (IsSteepSlope(tileh)) {
		if (existing == 0) {
			// force full pieces.
			*pieces |= (*pieces & 0xC) >> 2;
			*pieces |= (*pieces & 0x3) << 2;
			if (*pieces == ROAD_X || *pieces == ROAD_Y) return _price.terraform;
		}
		return CMD_ERROR;
	}
	road_bits = *pieces | existing;

	// no special foundation
	if ((~_valid_tileh_slopes_road[0][tileh] & road_bits) == 0) {
		// force that all bits are set when we have slopes
		if (tileh != SLOPE_FLAT) *pieces |= _valid_tileh_slopes_road[0][tileh];
		return 0; // no extra cost
	}

	// foundation is used. Whole tile is leveled up
	if ((~_valid_tileh_slopes_road[1][tileh] & road_bits) == 0) {
		return existing != 0 ? 0 : _price.terraform;
	}

	// partly leveled up tile, only if there's no road on that tile
	if (existing == 0 && (tileh == SLOPE_W || tileh == SLOPE_S || tileh == SLOPE_E || tileh == SLOPE_N)) {
		// force full pieces.
		*pieces |= (*pieces & 0xC) >> 2;
		*pieces |= (*pieces & 0x3) << 2;
		if (*pieces == ROAD_X || *pieces == ROAD_Y) return _price.terraform;
	}
	return CMD_ERROR;
}

/** Build a piece of road.
 * @param tile tile where to build road
 * @param p1 road piece flags
 * @param p2 the town that is building the road (0 if not applicable)
 */
int32 CmdBuildRoad(TileIndex tile, uint32 flags, uint32 p1, uint32 p2)
{
	int32 cost = 0;
	int32 ret;
	RoadBits existing = 0;
	RoadBits pieces;
	Slope tileh;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	/* Road pieces are max 4 bitset values (NE, NW, SE, SW) and town can only be non-zero
	 * if a non-player is building the road */
	if ((p1 >> 4) || (IsValidPlayer(_current_player) && p2 != 0) || (_current_player == OWNER_TOWN && !IsValidTownID(p2))) return CMD_ERROR;
	pieces = p1;

	tileh = GetTileSlope(tile, NULL);

	switch (GetTileType(tile)) {
		case MP_STREET:
			switch (GetRoadTileType(tile)) {
				case ROAD_TILE_NORMAL:
					if (HasRoadWorks(tile)) return_cmd_error(STR_ROAD_WORKS_IN_PROGRESS);

					existing = GetRoadBits(tile);
					if ((existing & pieces) == pieces) {
						return_cmd_error(STR_1007_ALREADY_BUILT);
					}
					if (!EnsureNoVehicleOnGround(tile)) return CMD_ERROR;
					break;

				case ROAD_TILE_CROSSING:
					if (pieces != GetCrossingRoadBits(tile)) { // XXX is this correct?
						return_cmd_error(STR_1007_ALREADY_BUILT);
					}
					goto do_clear;

				default:
				case ROAD_TILE_DEPOT:
					goto do_clear;
			}
			break;

		case MP_RAILWAY: {
			Axis roaddir;

			if (IsSteepSlope(tileh)) {
				return_cmd_error(STR_1000_LAND_SLOPED_IN_WRONG_DIRECTION);
			}

#define M(x) (1 << (x))
			/* Level crossings may only be built on these slopes */
			if (!HASBIT(M(SLOPE_SEN) | M(SLOPE_ENW) | M(SLOPE_NWS) | M(SLOPE_NS) | M(SLOPE_WSE) | M(SLOPE_EW) | M(SLOPE_FLAT), tileh)) {
				return_cmd_error(STR_1000_LAND_SLOPED_IN_WRONG_DIRECTION);
			}
#undef M

			if (GetRailTileType(tile) != RAIL_TILE_NORMAL) goto do_clear;
			switch (GetTrackBits(tile)) {
				case TRACK_BIT_X:
					if (pieces & ROAD_X) goto do_clear;
					roaddir = AXIS_Y;
					break;

				case TRACK_BIT_Y:
					if (pieces & ROAD_Y) goto do_clear;
					roaddir = AXIS_X;
					break;

				default: goto do_clear;
			}

			if (!EnsureNoVehicleOnGround(tile)) return CMD_ERROR;

			if (flags & DC_EXEC) {
				YapfNotifyTrackLayoutChange(tile, FIND_FIRST_BIT(GetTrackBits(tile)));
				MakeRoadCrossing(tile, _current_player, GetTileOwner(tile), roaddir, GetRailType(tile), p2);
				MarkTileDirtyByTile(tile);
			}
			return _price.build_road * 2;
		}

		case MP_TUNNELBRIDGE:
			if (!IsBridge(tile) || !IsBridgeMiddle(tile)) goto do_clear;

			/* only allow roads pertendicular to bridge */
			if ((pieces & (GetBridgeAxis(tile) == AXIS_X ? ROAD_X : ROAD_Y)) != 0) {
				goto do_clear;
			}

			/* check if clear land under bridge */
			if (IsTransportUnderBridge(tile)) {
				switch (GetTransportTypeUnderBridge(tile)) {
					case TRANSPORT_ROAD: return_cmd_error(STR_1007_ALREADY_BUILT);
					default: return_cmd_error(STR_1008_MUST_REMOVE_RAILROAD_TRACK);
				}
			} else {
				if (IsWaterUnderBridge(tile)) {
					return_cmd_error(STR_3807_CAN_T_BUILD_ON_WATER);
				}
			}

			if (flags & DC_EXEC) {
				SetRoadUnderBridge(tile, _current_player);
				MarkTileDirtyByTile(tile);
			}
			return _price.build_road * 2;

		default:
do_clear:;
			ret = DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
			if (CmdFailed(ret)) return ret;
			cost += ret;
	}

	ret = CheckRoadSlope(tileh, &pieces, existing);
	/* Return an error if we need to build a foundation (ret != 0) but the
	 * current patch-setting is turned off (or stupid AI@work) */
	if (CmdFailed(ret) || (ret != 0 && (!_patches.build_on_slopes || _is_old_ai_player)))
		return_cmd_error(STR_1000_LAND_SLOPED_IN_WRONG_DIRECTION);

	cost += ret;

	if (IsTileType(tile, MP_STREET)) {
		// Don't put the pieces that already exist
		pieces &= ComplementRoadBits(existing);
	}

	cost += CountRoadBits(pieces) * _price.build_road;

	if (flags & DC_EXEC) {
		if (IsTileType(tile, MP_STREET)) {
			SetRoadBits(tile, existing | pieces);
		} else {
			MakeRoadNormal(tile, _current_player, pieces, p2);
		}

		MarkTileDirtyByTile(tile);
	}
	return cost;
}

/**
 * Switches the rail type on a level crossing.
 * @param tile        The tile on which the railtype is to be convert.
 * @param totype      The railtype we want to convert to
 * @param exec        Switches between test and execute mode
 * @return            The cost and state of the operation
 * @retval CMD_ERROR  An error occured during the operation.
 */
int32 DoConvertStreetRail(TileIndex tile, RailType totype, bool exec)
{
	// not a railroad crossing?
	if (!IsLevelCrossing(tile)) return CMD_ERROR;

	// not owned by me?
	if (!CheckTileOwnership(tile) || !EnsureNoVehicleOnGround(tile)) return CMD_ERROR;

	if (GetRailTypeCrossing(tile) == totype) return CMD_ERROR;

	// 'hidden' elrails can't be downgraded to normal rail when elrails are disabled
	if (_patches.disable_elrails && totype == RAILTYPE_RAIL && GetRailTypeCrossing(tile) == RAILTYPE_ELECTRIC) return CMD_ERROR;

	if (exec) {
		SetRailTypeCrossing(tile, totype);
		MarkTileDirtyByTile(tile);
		YapfNotifyTrackLayoutChange(tile, FIND_FIRST_BIT(GetCrossingRailBits(tile)));
	}

	return _price.build_rail / 2;
}


/** Build a long piece of road.
 * @param end_tile end tile of drag
 * @param p1 start tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit 0) - start tile starts in the 2nd half of tile (p2 & 1)
 * - p2 = (bit 1) - end tile starts in the 2nd half of tile (p2 & 2)
 * - p2 = (bit 2) - direction: 0 = along x-axis, 1 = along y-axis (p2 & 4)
 */
int32 CmdBuildLongRoad(TileIndex end_tile, uint32 flags, uint32 p1, uint32 p2)
{
	TileIndex start_tile, tile;
	int32 cost, ret;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	if (p1 >= MapSize()) return CMD_ERROR;

	start_tile = p1;

	/* Only drag in X or Y direction dictated by the direction variable */
	if (!HASBIT(p2, 2) && TileY(start_tile) != TileY(end_tile)) return CMD_ERROR; // x-axis
	if (HASBIT(p2, 2)  && TileX(start_tile) != TileX(end_tile)) return CMD_ERROR; // y-axis

	/* Swap start and ending tile, also the half-tile drag var (bit 0 and 1) */
	if (start_tile > end_tile || (start_tile == end_tile && HASBIT(p2, 0))) {
		TileIndex t = start_tile;
		start_tile = end_tile;
		end_tile = t;
		p2 ^= IS_INT_INSIDE(p2&3, 1, 3) ? 3 : 0;
	}

	cost = 0;
	tile = start_tile;
	// Start tile is the small number.
	for (;;) {
		RoadBits bits = HASBIT(p2, 2) ? ROAD_Y : ROAD_X;

		if (tile == end_tile && !HASBIT(p2, 1)) bits &= ROAD_NW | ROAD_NE;
		if (tile == start_tile && HASBIT(p2, 0)) bits &= ROAD_SE | ROAD_SW;

		ret = DoCommand(tile, bits, 0, flags, CMD_BUILD_ROAD);
		if (CmdFailed(ret)) {
			if (_error_message != STR_1007_ALREADY_BUILT) return CMD_ERROR;
			_error_message = INVALID_STRING_ID;
		} else {
			cost += ret;
		}

		if (tile == end_tile) break;

		tile += HASBIT(p2, 2) ? TileDiffXY(0, 1) : TileDiffXY(1, 0);
	}

	return (cost == 0) ? CMD_ERROR : cost;
}

/** Remove a long piece of road.
 * @param end_tile end tile of drag
 * @param p1 start tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit 0) - start tile starts in the 2nd half of tile (p2 & 1)
 * - p2 = (bit 1) - end tile starts in the 2nd half of tile (p2 & 2)
 * - p2 = (bit 2) - direction: 0 = along x-axis, 1 = along y-axis (p2 & 4)
 */
int32 CmdRemoveLongRoad(TileIndex end_tile, uint32 flags, uint32 p1, uint32 p2)
{
	TileIndex start_tile, tile;
	int32 cost, ret, money;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	if (p1 >= MapSize()) return CMD_ERROR;

	start_tile = p1;

	/* Only drag in X or Y direction dictated by the direction variable */
	if (!HASBIT(p2, 2) && TileY(start_tile) != TileY(end_tile)) return CMD_ERROR; // x-axis
	if (HASBIT(p2, 2)  && TileX(start_tile) != TileX(end_tile)) return CMD_ERROR; // y-axis

	/* Swap start and ending tile, also the half-tile drag var (bit 0 and 1) */
	if (start_tile > end_tile || (start_tile == end_tile && HASBIT(p2, 0))) {
		TileIndex t = start_tile;
		start_tile = end_tile;
		end_tile = t;
		p2 ^= IS_INT_INSIDE(p2 & 3, 1, 3) ? 3 : 0;
	}

	cost = 0;
	money = GetAvailableMoneyForCommand();
	tile = start_tile;
	// Start tile is the small number.
	for (;;) {
		RoadBits bits = HASBIT(p2, 2) ? ROAD_Y : ROAD_X;

		if (tile == end_tile && !HASBIT(p2, 1)) bits &= ROAD_NW | ROAD_NE;
		if (tile == start_tile && HASBIT(p2, 0)) bits &= ROAD_SE | ROAD_SW;

		// try to remove the halves.
		if (bits != 0) {
			ret = DoCommand(tile, bits, 0, flags& ~DC_EXEC, CMD_REMOVE_ROAD);
			if (!CmdFailed(ret)) {
				if (flags & DC_EXEC) {
					money -= ret;
					if (money < 0) {
						_additional_cash_required = DoCommand(end_tile, start_tile, p2, flags & ~DC_EXEC, CMD_REMOVE_LONG_ROAD);
						return cost;
					}
					DoCommand(tile, bits, 0, flags, CMD_REMOVE_ROAD);
				}
				cost += ret;
			}
		}

		if (tile == end_tile) break;

		tile += HASBIT(p2, 2) ? TileDiffXY(0, 1) : TileDiffXY(1, 0);
	}

	return (cost == 0) ? CMD_ERROR : cost;
}

/** Build a road depot.
 * @param tile tile where to build the depot
 * @param p1 entrance direction (DiagDirection)
 * @param p2 unused
 *
 * @todo When checking for the tile slope,
 * distingush between "Flat land required" and "land sloped in wrong direction"
 */
int32 CmdBuildRoadDepot(TileIndex tile, uint32 flags, uint32 p1, uint32 p2)
{
	int32 cost;
	Depot *dep;
	Slope tileh;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	if (p1 > 3) return CMD_ERROR; // check direction

	if (!EnsureNoVehicleOnGround(tile)) return CMD_ERROR;

	tileh = GetTileSlope(tile, NULL);
	if (tileh != SLOPE_FLAT && (
				!_patches.build_on_slopes ||
				IsSteepSlope(tileh) ||
				!CanBuildDepotByTileh(p1, tileh)
			)) {
		return_cmd_error(STR_0007_FLAT_LAND_REQUIRED);
	}

	cost = DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	if (CmdFailed(cost)) return CMD_ERROR;

	dep = AllocateDepot();
	if (dep == NULL) return CMD_ERROR;

	if (flags & DC_EXEC) {
		dep->xy = tile;
		dep->town_index = ClosestTownFromTile(tile, (uint)-1)->index;

		MakeRoadDepot(tile, _current_player, p1);
		MarkTileDirtyByTile(tile);
	}
	return cost + _price.build_road_depot;
}

static int32 RemoveRoadDepot(TileIndex tile, uint32 flags)
{
	if (!CheckTileOwnership(tile) && _current_player != OWNER_WATER)
		return CMD_ERROR;

	if (!EnsureNoVehicle(tile)) return CMD_ERROR;

	if (flags & DC_EXEC) DeleteDepot(GetDepotByTile(tile));

	return _price.remove_road_depot;
}

#define M(x) (1<<(x))

static int32 ClearTile_Road(TileIndex tile, byte flags)
{
	switch (GetRoadTileType(tile)) {
		case ROAD_TILE_NORMAL: {
			RoadBits b = GetRoadBits(tile);

			if (!((1 << b) & (M(1)|M(2)|M(4)|M(8))) &&
					(!(flags & DC_AI_BUILDING) || !IsTileOwner(tile, OWNER_TOWN)) &&
					flags & DC_AUTO) {
				return_cmd_error(STR_1801_MUST_REMOVE_ROAD_FIRST);
			}
			return DoCommand(tile, b, 0, flags, CMD_REMOVE_ROAD);
		}

		case ROAD_TILE_CROSSING: {
			int32 ret;

			if (flags & DC_AUTO) return_cmd_error(STR_1801_MUST_REMOVE_ROAD_FIRST);

			ret = DoCommand(tile, GetCrossingRoadBits(tile), 0, flags, CMD_REMOVE_ROAD);
			if (CmdFailed(ret)) return CMD_ERROR;

			if (flags & DC_EXEC) {
				DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
			}
			return ret;
		}

		default:
		case ROAD_TILE_DEPOT:
			if (flags & DC_AUTO) {
				return_cmd_error(STR_2004_BUILDING_MUST_BE_DEMOLISHED);
			}
			return RemoveRoadDepot(tile, flags);
	}
}


typedef struct DrawRoadTileStruct {
	uint16 image;
	byte subcoord_x;
	byte subcoord_y;
} DrawRoadTileStruct;

#include "table/road_land.h"


uint GetRoadFoundation(Slope tileh, RoadBits bits)
{
	uint i;

	// normal level sloped building
	if (!IsSteepSlope(tileh) &&
			(~_valid_tileh_slopes_road[1][tileh] & bits) == 0) {
		return tileh;
	}

	// inclined sloped building
	switch (bits) {
		case ROAD_X: i = 0; break;
		case ROAD_Y: i = 1; break;
		default:     return 0;
	}
	switch (tileh) {
		case SLOPE_W:
		case SLOPE_STEEP_W: i += 0; break;
		case SLOPE_S:
		case SLOPE_STEEP_S: i += 2; break;
		case SLOPE_E:
		case SLOPE_STEEP_E: i += 4; break;
		case SLOPE_N:
		case SLOPE_STEEP_N: i += 6; break;
		default: return 0;
	}
	return i + 15;
}

const byte _road_sloped_sprites[14] = {
	0,  0,  2,  0,
	0,  1,  0,  0,
	3,  0,  0,  0,
	0,  0
};

/**
 * Draw ground sprite and road pieces
 * @param ti TileInfo
 * @param road RoadBits to draw
 */
static void DrawRoadBits(TileInfo* ti)
{
	RoadBits road = GetRoadBits(ti->tile);
	const DrawRoadTileStruct *drts;
	PalSpriteID image = 0;
	Roadside roadside;

	if (ti->tileh != SLOPE_FLAT) {
		int foundation = GetRoadFoundation(ti->tileh, road);

		if (foundation != 0) DrawFoundation(ti, foundation);

		// DrawFoundation() modifies ti.
		// Default sloped sprites..
		if (ti->tileh != SLOPE_FLAT) image = _road_sloped_sprites[ti->tileh - 1] + 0x53F;
	}

	if (image == 0) image = _road_tile_sprites_1[road];

	roadside = GetRoadside(ti->tile);

	if (IsOnSnow(ti->tile)) {
		image += 19;
	} else {
		switch (roadside) {
			case ROADSIDE_BARREN:           image |= PALETTE_TO_BARE_LAND; break;
			case ROADSIDE_GRASS:            break;
			case ROADSIDE_GRASS_ROAD_WORKS: break;
			default:                        image -= 19; break; // Paved
		}
	}

	DrawGroundSprite(image);

	if (HasRoadWorks(ti->tile)) {
		// Road works
		DrawGroundSprite(road & ROAD_X ? SPR_EXCAVATION_X : SPR_EXCAVATION_Y);
		return;
	}

	// Return if full detail is disabled, or we are zoomed fully out.
	if (!(_display_opt & DO_FULL_DETAIL) || _cur_dpi->zoom == 2) return;

	// Draw extra details.
	for (drts = _road_display_table[roadside][road]; drts->image != 0; drts++) {
		int x = ti->x | drts->subcoord_x;
		int y = ti->y | drts->subcoord_y;
		byte z = ti->z;
		if (ti->tileh != SLOPE_FLAT) z = GetSlopeZ(x, y);
		AddSortableSpriteToDraw(drts->image, x, y, 2, 2, 0x10, z);
	}
}

static void DrawTile_Road(TileInfo *ti)
{
	switch (GetRoadTileType(ti->tile)) {
		case ROAD_TILE_NORMAL:
			DrawRoadBits(ti);
			break;

		case ROAD_TILE_CROSSING: {
			PalSpriteID image;

			if (ti->tileh != SLOPE_FLAT) DrawFoundation(ti, ti->tileh);

			image = GetRailTypeInfo(GetRailTypeCrossing(ti->tile))->base_sprites.crossing;

			if (GetCrossingRoadAxis(ti->tile) == AXIS_X) image++;
			if (IsCrossingBarred(ti->tile)) image += 2;

			if (IsOnSnow(ti->tile)) {
				image += 8;
			} else {
				switch (GetRoadside(ti->tile)) {
					case ROADSIDE_BARREN: image |= PALETTE_TO_BARE_LAND; break;
					case ROADSIDE_GRASS:  break;
					default:              image += 4; break; // Paved
				}
			}

			DrawGroundSprite(image);
			if (GetRailTypeCrossing(ti->tile) == RAILTYPE_ELECTRIC) DrawCatenary(ti);
			break;
		}

		default:
		case ROAD_TILE_DEPOT: {
			const DrawTileSprites* dts;
			const DrawTileSeqStruct* dtss;
			uint32 palette;

			if (ti->tileh != SLOPE_FLAT) DrawFoundation(ti, ti->tileh);

			palette = PLAYER_SPRITE_COLOR(GetTileOwner(ti->tile));

			dts =  &_road_depot[GetRoadDepotDirection(ti->tile)];
			DrawGroundSprite(dts->ground_sprite);

			for (dtss = dts->seq; dtss->image != 0; dtss++) {
				uint32 image = dtss->image;

				if (_display_opt & DO_TRANS_BUILDINGS) {
					MAKE_TRANSPARENT(image);
				} else if (image & PALETTE_MODIFIER_COLOR) {
					image |= palette;
				}

				AddSortableSpriteToDraw(
					image,
					ti->x + dtss->delta_x, ti->y + dtss->delta_y,
					dtss->size_x, dtss->size_y,
					dtss->size_z, ti->z
				);
			}
			break;
		}
	}
}

void DrawRoadDepotSprite(int x, int y, DiagDirection dir)
{
	uint32 palette = PLAYER_SPRITE_COLOR(_local_player);
	const DrawTileSprites* dts =  &_road_depot[dir];
	const DrawTileSeqStruct* dtss;

	x += 33;
	y += 17;

	DrawSprite(dts->ground_sprite, x, y);

	for (dtss = dts->seq; dtss->image != 0; dtss++) {
		Point pt = RemapCoords(dtss->delta_x, dtss->delta_y, dtss->delta_z);
		uint32 image = dtss->image;

		if (image & PALETTE_MODIFIER_COLOR) image |= palette;

		DrawSprite(image, x + pt.x, y + pt.y);
	}
}

static uint GetSlopeZ_Road(TileIndex tile, uint x, uint y)
{
	uint z;
	Slope tileh = GetTileSlope(tile, &z);

	if (tileh == SLOPE_FLAT) return z;
	if (GetRoadTileType(tile) == ROAD_TILE_NORMAL) {
		uint f = GetRoadFoundation(tileh, GetRoadBits(tile));

		if (f != 0) {
			if (IsSteepSlope(tileh)) {
				z += TILE_HEIGHT;
			} else if (f < 15) {
				return z + TILE_HEIGHT; // leveled foundation
			}
			tileh = _inclined_tileh[f - 15]; // inclined foundation
		}
		return z + GetPartialZ(x & 0xF, y & 0xF, tileh);
	} else {
		return z + TILE_HEIGHT;
	}
}

static Slope GetSlopeTileh_Road(TileIndex tile, Slope tileh)
{
	if (tileh == SLOPE_FLAT) return SLOPE_FLAT;
	if (GetRoadTileType(tile) == ROAD_TILE_NORMAL) {
		uint f = GetRoadFoundation(tileh, GetRoadBits(tile));

		if (f == 0) return tileh;
		if (f < 15) return SLOPE_FLAT; // leveled foundation
		return _inclined_tileh[f - 15]; // inclined foundation
	} else {
		return SLOPE_FLAT;
	}
}

static void GetAcceptedCargo_Road(TileIndex tile, AcceptedCargo ac)
{
	/* not used */
}

static void AnimateTile_Road(TileIndex tile)
{
	if (IsLevelCrossing(tile)) MarkTileDirtyByTile(tile);
}


static const Roadside _town_road_types[][2] = {
	{ ROADSIDE_GRASS,         ROADSIDE_GRASS },
	{ ROADSIDE_PAVED,         ROADSIDE_PAVED },
	{ ROADSIDE_PAVED,         ROADSIDE_PAVED },
	{ ROADSIDE_TREES,         ROADSIDE_TREES },
	{ ROADSIDE_STREET_LIGHTS, ROADSIDE_PAVED }
};

static const Roadside _town_road_types_2[][2] = {
	{ ROADSIDE_GRASS,         ROADSIDE_GRASS },
	{ ROADSIDE_PAVED,         ROADSIDE_PAVED },
	{ ROADSIDE_STREET_LIGHTS, ROADSIDE_PAVED },
	{ ROADSIDE_STREET_LIGHTS, ROADSIDE_PAVED },
	{ ROADSIDE_STREET_LIGHTS, ROADSIDE_PAVED }
};


static void TileLoop_Road(TileIndex tile)
{
	switch (_opt.landscape) {
		case LT_HILLY:
			if (IsOnSnow(tile) != (GetTileZ(tile) > _opt.snow_line)) {
				ToggleSnow(tile);
				MarkTileDirtyByTile(tile);
			}
			break;

		case LT_DESERT:
			if (GetTropicZone(tile) == TROPICZONE_DESERT && !IsOnDesert(tile)) {
				ToggleDesert(tile);
				MarkTileDirtyByTile(tile);
			}
			break;
	}

	if (GetRoadTileType(tile) == ROAD_TILE_DEPOT) return;

	if (!HasRoadWorks(tile)) {
		const Town* t = ClosestTownFromTile(tile, (uint)-1);
		int grp = 0;

		if (t != NULL) {
			grp = GetTownRadiusGroup(t, tile);

			// Show an animation to indicate road work
			if (t->road_build_months != 0 &&
					(DistanceManhattan(t->xy, tile) < 8 || grp != 0) &&
					GetRoadTileType(tile) == ROAD_TILE_NORMAL && (GetRoadBits(tile) == ROAD_X || GetRoadBits(tile) == ROAD_Y)) {
				if (GetTileSlope(tile, NULL) == SLOPE_FLAT && EnsureNoVehicleOnGround(tile) && CHANCE16(1, 20)) {
					StartRoadWorks(tile);

					SndPlayTileFx(SND_21_JACKHAMMER, tile);
					CreateEffectVehicleAbove(
						TileX(tile) * TILE_SIZE + 7,
						TileY(tile) * TILE_SIZE + 7,
						0,
						EV_BULLDOZER);
					MarkTileDirtyByTile(tile);
					return;
				}
			}
		}

		{
			/* Adjust road ground type depending on 'grp' (grp is the distance to the center) */
			const Roadside* new_rs = (_opt.landscape == LT_CANDY) ? _town_road_types_2[grp] : _town_road_types[grp];
			Roadside cur_rs = GetRoadside(tile);

			/* We have our desired type, do nothing */
			if (cur_rs == new_rs[0]) return;

			/* We have the pre-type of the desired type, switch to the desired type */
			if (cur_rs == new_rs[1]) {
				cur_rs = new_rs[0];
			/* We have barren land, install the pre-type */
			} else if (cur_rs == ROADSIDE_BARREN) {
				cur_rs = new_rs[1];
			/* We're totally off limits, remove any installation and make barren land */
			} else {
				cur_rs = ROADSIDE_BARREN;
			}
			SetRoadside(tile, cur_rs);
			MarkTileDirtyByTile(tile);
		}
	} else if (IncreaseRoadWorksCounter(tile)) {
		TerminateRoadWorks(tile);
		MarkTileDirtyByTile(tile);
	}
}

static void ClickTile_Road(TileIndex tile)
{
	if (GetRoadTileType(tile) == ROAD_TILE_DEPOT) ShowDepotWindow(tile, VEH_Road);
}

static const byte _road_trackbits[16] = {
	0x0, 0x0, 0x0, 0x10, 0x0, 0x2, 0x8, 0x1A, 0x0, 0x4, 0x1, 0x15, 0x20, 0x26, 0x29, 0x3F,
};

static uint32 GetTileTrackStatus_Road(TileIndex tile, TransportType mode)
{
	switch (mode) {
		case TRANSPORT_RAIL:
			if (!IsLevelCrossing(tile)) return 0;
			return GetCrossingRailBits(tile) * 0x101;

		case TRANSPORT_ROAD:
			switch (GetRoadTileType(tile)) {
				case ROAD_TILE_NORMAL:
					return HasRoadWorks(tile) ? 0 : _road_trackbits[GetRoadBits(tile)] * 0x101;

				case ROAD_TILE_CROSSING: {
					uint32 r = AxisToTrackBits(GetCrossingRoadAxis(tile)) * 0x101;

					if (IsCrossingBarred(tile)) r *= 0x10001;
					return r;
				}

				default:
				case ROAD_TILE_DEPOT:
					return AxisToTrackBits(DiagDirToAxis(GetRoadDepotDirection(tile))) * 0x101;
			}
			break;

		default: break;
	}
	return 0;
}

static const StringID _road_tile_strings[] = {
	STR_1814_ROAD,
	STR_1814_ROAD,
	STR_1814_ROAD,
	STR_1815_ROAD_WITH_STREETLIGHTS,
	STR_1814_ROAD,
	STR_1816_TREE_LINED_ROAD,
	STR_1814_ROAD,
	STR_1814_ROAD,
};

static void GetTileDesc_Road(TileIndex tile, TileDesc *td)
{
	td->owner = GetTileOwner(tile);
	switch (GetRoadTileType(tile)) {
		case ROAD_TILE_CROSSING: td->str = STR_1818_ROAD_RAIL_LEVEL_CROSSING; break;
		case ROAD_TILE_DEPOT: td->str = STR_1817_ROAD_VEHICLE_DEPOT; break;
		default: td->str = _road_tile_strings[GetRoadside(tile)]; break;
	}
}

static const byte _roadveh_enter_depot_unk0[4] = {
	8, 9, 0, 1
};

static uint32 VehicleEnter_Road(Vehicle *v, TileIndex tile, int x, int y)
{
	switch (GetRoadTileType(tile)) {
		case ROAD_TILE_CROSSING:
			if (v->type == VEH_Train && !IsCrossingBarred(tile)) {
				/* train crossing a road */
				SndPlayVehicleFx(SND_0E_LEVEL_CROSSING, v);
				BarCrossing(tile);
				MarkTileDirtyByTile(tile);
			}
			break;

		case ROAD_TILE_DEPOT:
			if (v->type == VEH_Road &&
					v->u.road.frame == 11 &&
					_roadveh_enter_depot_unk0[GetRoadDepotDirection(tile)] == v->u.road.state) {
				VehicleEnterDepot(v);
				return 4;
			}
			break;

		default: break;
	}
	return 0;
}


static void ChangeTileOwner_Road(TileIndex tile, PlayerID old_player, PlayerID new_player)
{
	if (IsLevelCrossing(tile) && GetCrossingRoadOwner(tile) == old_player) {
		SetCrossingRoadOwner(tile, new_player == PLAYER_SPECTATOR ? OWNER_NONE : new_player);
	}

	if (!IsTileOwner(tile, old_player)) return;

	if (new_player != PLAYER_SPECTATOR) {
		SetTileOwner(tile, new_player);
	} else {
		switch (GetRoadTileType(tile)) {
			case ROAD_TILE_NORMAL:
				SetTileOwner(tile, OWNER_NONE);
				break;

			case ROAD_TILE_CROSSING:
				MakeRoadNormal(tile, GetCrossingRoadOwner(tile), GetCrossingRoadBits(tile), GetTownIndex(tile));
				break;

			default:
			case ROAD_TILE_DEPOT:
				DoCommand(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
				break;
		}
	}
}


const TileTypeProcs _tile_type_road_procs = {
	DrawTile_Road,           /* draw_tile_proc */
	GetSlopeZ_Road,          /* get_slope_z_proc */
	ClearTile_Road,          /* clear_tile_proc */
	GetAcceptedCargo_Road,   /* get_accepted_cargo_proc */
	GetTileDesc_Road,        /* get_tile_desc_proc */
	GetTileTrackStatus_Road, /* get_tile_track_status_proc */
	ClickTile_Road,          /* click_tile_proc */
	AnimateTile_Road,        /* animate_tile_proc */
	TileLoop_Road,           /* tile_loop_clear */
	ChangeTileOwner_Road,    /* change_tile_owner_clear */
	NULL,                    /* get_produced_cargo_proc */
	VehicleEnter_Road,       /* vehicle_enter_tile_proc */
	GetSlopeTileh_Road,      /* get_slope_tileh_proc */
};
