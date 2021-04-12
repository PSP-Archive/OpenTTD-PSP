/* $Id: tree_cmd.c 11073 2007-09-09 20:36:15Z rubidium $ */

#include "stdafx.h"
#include "openttd.h"
#include "clear_map.h"
#include "table/strings.h"
#include "table/sprites.h"
#include "table/tree_land.h"
#include "functions.h"
#include "map.h"
#include "tile.h"
#include "tree_map.h"
#include "viewport.h"
#include "command.h"
#include "town.h"
#include "sound.h"
#include "variables.h"
#include "genworld.h"

enum TreePlacer {
	TP_NONE,
	TP_ORIGINAL,
	TP_IMPROVED,
};

static TreeType GetRandomTreeType(TileIndex tile, uint seed)
{
	switch (_opt.landscape) {
		case LT_NORMAL:
			return seed * TREE_COUNT_TEMPERATE / 256 + TREE_TEMPERATE;

		case LT_HILLY:
			return seed * TREE_COUNT_SUB_ARCTIC / 256 + TREE_SUB_ARCTIC;

		case LT_DESERT:
			switch (GetTropicZone(tile)) {
				case TROPICZONE_INVALID: return seed * TREE_COUNT_SUB_TROPICAL / 256 + TREE_SUB_TROPICAL;
				case TROPICZONE_DESERT:  return (seed > 12) ? TREE_INVALID : TREE_CACTUS;
				default:                 return seed * TREE_COUNT_RAINFOREST / 256 + TREE_RAINFOREST;
			}

		default:
			return seed * TREE_COUNT_TOYLAND / 256 + TREE_TOYLAND;
	}
}

static void PlaceTree(TileIndex tile, uint32 r)
{
	TreeType tree = GetRandomTreeType(tile, GB(r, 24, 8));

	if (tree != TREE_INVALID) {
		MakeTree(tile, tree, GB(r, 22, 2), min(GB(r, 16, 3), 6), TREE_GROUND_GRASS, 0);

		// above snowline?
		if (_opt.landscape == LT_HILLY && GetTileZ(tile) > _opt.snow_line) {
			SetTreeGroundDensity(tile, TREE_GROUND_SNOW_DESERT, 3);
			SetTreeCounter(tile, GB(r, 24, 3));
		} else {
			SetTreeGroundDensity(tile, GB(r, 28, 1), 0);
			SetTreeCounter(tile, GB(r, 24, 4));
		}
	}
}

static void DoPlaceMoreTrees(TileIndex tile)
{
	uint i;

	for (i = 0; i < 1000; i++) {
		uint32 r = Random();
		int x = GB(r, 0, 5) - 16;
		int y = GB(r, 8, 5) - 16;
		uint dist = myabs(x) + myabs(y);
		TileIndex cur_tile = TILE_MASK(tile + TileDiffXY(x, y));

		if (dist <= 13 &&
				IsTileType(cur_tile, MP_CLEAR) &&
				!IsClearGround(cur_tile, CLEAR_FIELDS) &&
				!IsClearGround(cur_tile, CLEAR_ROCKS)) {
			PlaceTree(cur_tile, r);
		}
	}
}

static void PlaceMoreTrees(void)
{
	uint i = ScaleByMapSize(GB(Random(), 0, 5) + 25);
	do {
		DoPlaceMoreTrees(RandomTile());
	} while (--i);
}

/**
 * Place a tree at the same height as an existing tree.
 *  This gives cool effects to the map.
 */
void PlaceTreeAtSameHeight(TileIndex tile, uint height)
{
	uint i;

	for (i = 0; i < 1000; i++) {
		uint32 r = Random();
		int x = GB(r, 0, 5) - 16;
		int y = GB(r, 8, 5) - 16;
		TileIndex cur_tile = TILE_MASK(tile + TileDiffXY(x, y));

		/* Keep in range of the existing tree */
		if (myabs(x) + myabs(y) > 16) continue;

		/* Clear tile, no farm-tiles or rocks */
		if (!IsTileType(cur_tile, MP_CLEAR) ||
				IsClearGround(cur_tile, CLEAR_FIELDS) ||
				IsClearGround(cur_tile, CLEAR_ROCKS))
			continue;

		/* Not too much height difference */
		if (myabs(GetTileZ(cur_tile) - height) > 2) continue;

		/* Place one tree and quit */
		PlaceTree(cur_tile, r);
		break;
	}
}

void PlaceTreesRandomly(void)
{
	uint i, j, ht;

	i = ScaleByMapSize(1000);
	do {
		uint32 r = Random();
		TileIndex tile = RandomTileSeed(r);

		IncreaseGeneratingWorldProgress(GWP_TREE);

		if (IsTileType(tile, MP_CLEAR) &&
				!IsClearGround(tile, CLEAR_FIELDS) &&
				!IsClearGround(tile, CLEAR_ROCKS)) {
			PlaceTree(tile, r);
			if (_patches.tree_placer != TP_IMPROVED) continue;

			/* Place a number of trees based on the tile height.
			 *  This gives a cool effect of multiple trees close together.
			 *  It is almost real life ;) */
			ht = GetTileZ(tile);
			/* The higher we get, the more trees we plant */
			j = GetTileZ(tile) / TILE_HEIGHT * 2;
			while (j--) {
				/* Above snowline more trees! */
				if (_opt.landscape == LT_HILLY && ht > _opt.snow_line) {
					PlaceTreeAtSameHeight(tile, ht);
					PlaceTreeAtSameHeight(tile, ht);
				};

				PlaceTreeAtSameHeight(tile, ht);
			}
		}
	} while (--i);

	/* place extra trees at rainforest area */
	if (_opt.landscape == LT_DESERT) {
		i = ScaleByMapSize(15000);

		do {
			uint32 r = Random();
			TileIndex tile = RandomTileSeed(r);

			IncreaseGeneratingWorldProgress(GWP_TREE);

			if (IsTileType(tile, MP_CLEAR) && !IsClearGround(tile, CLEAR_FIELDS) && GetTropicZone(tile) == TROPICZONE_RAINFOREST) {
				PlaceTree(tile, r);
			}
		} while (--i);
	}
}

void GenerateTrees(void)
{
	uint i, total;

	if (_patches.tree_placer == TP_NONE) return;

	if (_opt.landscape != LT_CANDY) PlaceMoreTrees();

	switch (_patches.tree_placer) {
		case TP_ORIGINAL: i = _opt.landscape == LT_HILLY ? 15 : 6; break;
		case TP_IMPROVED: i = _opt.landscape == LT_HILLY ?  4 : 2; break;
		default: NOT_REACHED(); return;
	}

	total = ScaleByMapSize(1000);
	if (_opt.landscape == LT_DESERT) total += ScaleByMapSize(15000);
	total *= i;
	SetGeneratingWorldProgress(GWP_TREE, total);

	for (; i != 0; i--) {
		PlaceTreesRandomly();
	}
}

/** Plant a tree.
 * @param tile start tile of area-drag of tree plantation
 * @param p1 tree type, -1 means random.
 * @param p2 end tile of area-drag
 */
int32 CmdPlantTree(TileIndex tile, uint32 flags, uint32 p1, uint32 p2)
{
	StringID msg = INVALID_STRING_ID;
	int32 cost;
	int ex;
	int ey;
	int sx, sy, x, y;

	if (p2 >= MapSize()) return CMD_ERROR;
	/* Check the tree type. It can be random or some valid value within the current climate */
	if (p1 != (uint)-1 && p1 - _tree_base_by_landscape[_opt.landscape] >= _tree_count_by_landscape[_opt.landscape]) return CMD_ERROR;

	SET_EXPENSES_TYPE(EXPENSES_OTHER);

	// make sure sx,sy are smaller than ex,ey
	ex = TileX(tile);
	ey = TileY(tile);
	sx = TileX(p2);
	sy = TileY(p2);
	if (ex < sx) intswap(ex, sx);
	if (ey < sy) intswap(ey, sy);

	cost = 0; // total cost

	for (x = sx; x <= ex; x++) {
		for (y = sy; y <= ey; y++) {
			TileIndex tile = TileXY(x, y);

			if (!EnsureNoVehicle(tile)) continue;

			switch (GetTileType(tile)) {
				case MP_TREES:
					// no more space for trees?
					if (_game_mode != GM_EDITOR && GetTreeCount(tile) == 3) {
						msg = STR_2803_TREE_ALREADY_HERE;
						continue;
					}

					if (flags & DC_EXEC) {
						AddTreeCount(tile, 1);
						MarkTileDirtyByTile(tile);
					}
					// 2x as expensive to add more trees to an existing tile
					cost += _price.build_trees * 2;
					break;

				case MP_WATER:
					msg = STR_3807_CAN_T_BUILD_ON_WATER;
					continue;
					break;

				case MP_CLEAR:
					if (!IsTileOwner(tile, OWNER_NONE)) {
						msg = STR_2804_SITE_UNSUITABLE;
						continue;
					}

					switch (GetClearGround(tile)) {
						case CLEAR_FIELDS: cost += _price.clear_3; break;
						case CLEAR_ROCKS:  cost += _price.clear_2; break;
						default: break;
					}

					if (flags & DC_EXEC) {
						TreeType treetype;
						uint growth;

						if (_game_mode != GM_EDITOR && IsValidPlayer(_current_player)) {
							Town *t = ClosestTownFromTile(tile, _patches.dist_local_authority);
							if (t != NULL)
								ChangeTownRating(t, RATING_TREE_UP_STEP, RATING_TREE_MAXIMUM);
						}

						treetype = p1;
						if (treetype == TREE_INVALID) {
							treetype = GetRandomTreeType(tile, GB(Random(), 24, 8));
							if (treetype == TREE_INVALID) treetype = TREE_CACTUS;
						}

						growth = _game_mode == GM_EDITOR ? 3 : 0;
						switch (GetClearGround(tile)) {
							case CLEAR_ROUGH: MakeTree(tile, treetype, 0, growth, TREE_GROUND_ROUGH, 0); break;
							case CLEAR_SNOW:  MakeTree(tile, treetype, 0, growth, TREE_GROUND_SNOW_DESERT, GetClearDensity(tile)); break;
							default:          MakeTree(tile, treetype, 0, growth, TREE_GROUND_GRASS, 0); break;
						}
						MarkTileDirtyByTile(tile);

						if (_game_mode == GM_EDITOR && IS_INT_INSIDE(treetype, TREE_RAINFOREST, TREE_CACTUS))
							SetTropicZone(tile, TROPICZONE_RAINFOREST);
					}
					cost += _price.build_trees;
					break;

				default:
					msg = STR_2804_SITE_UNSUITABLE;
					break;
			}
		}
	}

	if (cost == 0) {
		return_cmd_error(msg);
	} else {
		return cost;
	}
}

typedef struct TreeListEnt {
	uint32 image;
	byte x,y;
} TreeListEnt;

static void DrawTile_Trees(TileInfo *ti)
{
	const uint32 *s;
	const TreePos* d;
	byte z;

	switch (GetTreeGround(ti->tile)) {
		case TREE_GROUND_GRASS: DrawClearLandTile(ti, 3); break;
		case TREE_GROUND_ROUGH: DrawHillyLandTile(ti); break;
		default: DrawGroundSprite(_tree_sprites_1[GetTreeDensity(ti->tile)] + _tileh_to_sprite[ti->tileh]); break;
	}

	DrawClearLandFence(ti);

	z = ti->z;
	if (ti->tileh != SLOPE_FLAT) {
		z += 4;
		if (IsSteepSlope(ti->tileh)) z += 4;
	}

	{
		uint16 tmp = ti->x;
		uint index;

		tmp = ROR(tmp, 2);
		tmp -= ti->y;
		tmp = ROR(tmp, 3);
		tmp -= ti->x;
		tmp = ROR(tmp, 1);
		tmp += ti->y;

		d = _tree_layout_xy[GB(tmp, 4, 2)];

		index = GB(tmp, 6, 2) + (GetTreeType(ti->tile) << 2);

		/* different tree styles above one of the grounds */
		if (GetTreeGround(ti->tile) == TREE_GROUND_SNOW_DESERT &&
				GetTreeDensity(ti->tile) >= 2 &&
				IS_INT_INSIDE(index, TREE_SUB_ARCTIC << 2, TREE_RAINFOREST << 2)) {
			index += 164 - (TREE_SUB_ARCTIC << 2);
		}

		assert(index < lengthof(_tree_layout_sprite));
		s = _tree_layout_sprite[index];
	}

	StartSpriteCombine();

	if (!(_display_opt & DO_TRANS_BUILDINGS) || !_patches.invisible_trees) {
		TreeListEnt te[4];
		uint i;

		/* put the trees to draw in a list */
		i = GetTreeCount(ti->tile) + 1;
		do {
			uint32 image = s[0] + (--i == 0 ? GetTreeGrowth(ti->tile) : 3);
			if (_display_opt & DO_TRANS_BUILDINGS) MAKE_TRANSPARENT(image);
			te[i].image = image;
			te[i].x = d->x;
			te[i].y = d->y;
			s++;
			d++;
		} while (i);

		/* draw them in a sorted way */
		for (;;) {
			byte min = 0xFF;
			TreeListEnt *tep = NULL;

			i = GetTreeCount(ti->tile) + 1;
			do {
				if (te[--i].image != 0 && te[i].x + te[i].y < min) {
					min = te[i].x + te[i].y;
					tep = &te[i];
				}
			} while (i);

			if (tep == NULL) break;

			AddSortableSpriteToDraw(tep->image, ti->x + tep->x, ti->y + tep->y, 5, 5, 0x10, z);
			tep->image = 0;
		}
	}

	EndSpriteCombine();
}


static uint GetSlopeZ_Trees(TileIndex tile, uint x, uint y)
{
	uint z;
	uint tileh = GetTileSlope(tile, &z);

	return z + GetPartialZ(x & 0xF, y & 0xF, tileh);
}

static Slope GetSlopeTileh_Trees(TileIndex tile, Slope tileh)
{
	return tileh;
}

static int32 ClearTile_Trees(TileIndex tile, byte flags)
{
	uint num;

	if ((flags & DC_EXEC) && IsValidPlayer(_current_player)) {
		Town *t = ClosestTownFromTile(tile, _patches.dist_local_authority);
		if (t != NULL)
			ChangeTownRating(t, RATING_TREE_DOWN_STEP, RATING_TREE_MINIMUM);
	}

	num = GetTreeCount(tile) + 1;
	if (IS_INT_INSIDE(GetTreeType(tile), TREE_RAINFOREST, TREE_CACTUS)) num *= 4;

	if (flags & DC_EXEC) DoClearSquare(tile);

	return num * _price.remove_trees;
}

static void GetAcceptedCargo_Trees(TileIndex tile, AcceptedCargo ac)
{
	/* not used */
}

static void GetTileDesc_Trees(TileIndex tile, TileDesc *td)
{
	TreeType tt = GetTreeType(tile);

	if (IS_INT_INSIDE(tt, TREE_RAINFOREST, TREE_CACTUS)) {
		td->str = STR_280F_RAINFOREST;
	} else if (tt == TREE_CACTUS) {
		td->str = STR_2810_CACTUS_PLANTS;
	} else {
		td->str = STR_280E_TREES;
	}

	td->owner = GetTileOwner(tile);
}

static void AnimateTile_Trees(TileIndex tile)
{
	/* not used */
}

static void TileLoopTreesDesert(TileIndex tile)
{
	switch (GetTropicZone(tile)) {
		case TROPICZONE_DESERT:
			if (GetTreeGround(tile) != TREE_GROUND_SNOW_DESERT) {
				SetTreeGroundDensity(tile, TREE_GROUND_SNOW_DESERT, 3);
				MarkTileDirtyByTile(tile);
			}
			break;

		case TROPICZONE_RAINFOREST: {
			static const SoundFx forest_sounds[] = {
				SND_42_LOON_BIRD,
				SND_43_LION,
				SND_44_MONKEYS,
				SND_48_DISTANT_BIRD
			};
			uint32 r = Random();

			if (CHANCE16I(1, 200, r)) SndPlayTileFx(forest_sounds[GB(r, 16, 2)], tile);
			break;
		}

		default: break;
	}
}

static void TileLoopTreesAlps(TileIndex tile)
{
	int k = GetTileZ(tile) - _opt.snow_line + TILE_HEIGHT;

	if (k < 0) {
		if (GetTreeGround(tile) != TREE_GROUND_SNOW_DESERT) return;
		SetTreeGroundDensity(tile, TREE_GROUND_GRASS, 0);
	} else {
		uint density = min((uint)k / TILE_HEIGHT, 3);

		if (GetTreeGround(tile) != TREE_GROUND_SNOW_DESERT ||
				GetTreeDensity(tile) != density) {
			SetTreeGroundDensity(tile, TREE_GROUND_SNOW_DESERT, density);
		} else {
			if (GetTreeDensity(tile) == 3) {
				uint32 r = Random();
				if (CHANCE16I(1, 200, r)) {
					SndPlayTileFx((r & 0x80000000) ? SND_39_HEAVY_WIND : SND_34_WIND, tile);
				}
			}
			return;
		}
	}
	MarkTileDirtyByTile(tile);
}

static void TileLoop_Trees(TileIndex tile)
{
	switch (_opt.landscape) {
		case LT_DESERT: TileLoopTreesDesert(tile); break;
		case LT_HILLY:  TileLoopTreesAlps(tile);   break;
	}

	TileLoopClearHelper(tile);

	if (GetTreeCounter(tile) < 15) {
		AddTreeCounter(tile, 1);
		return;
	}
	SetTreeCounter(tile, 0);

	switch (GetTreeGrowth(tile)) {
		case 3: /* regular sized tree */
			if (_opt.landscape == LT_DESERT &&
					GetTreeType(tile) != TREE_CACTUS &&
					GetTropicZone(tile) == TROPICZONE_DESERT) {
				AddTreeGrowth(tile, 1);
			} else {
				switch (GB(Random(), 0, 3)) {
					case 0: /* start destructing */
						AddTreeGrowth(tile, 1);
						break;

					case 1: /* add a tree */
						if (GetTreeCount(tile) < 3) {
							AddTreeCount(tile, 1);
							SetTreeGrowth(tile, 0);
							break;
						}
						/* FALL THROUGH */

					case 2: { /* add a neighbouring tree */
						TreeType treetype = GetTreeType(tile);

						tile += TileOffsByDir(Random() & 7);

						if (!IsTileType(tile, MP_CLEAR)) return;

						switch (GetClearGround(tile)) {
							case CLEAR_GRASS:
								if (GetClearDensity(tile) != 3) return;
								MakeTree(tile, treetype, 0, 0, TREE_GROUND_GRASS, 0);
								break;

							case CLEAR_ROUGH: MakeTree(tile, treetype, 0, 0, TREE_GROUND_ROUGH, 0); break;
							case CLEAR_SNOW:  MakeTree(tile, treetype, 0, 0, TREE_GROUND_SNOW_DESERT, GetClearDensity(tile)); break;
							default: return;
						}
						break;
					}

					default:
						return;
				}
			}
			break;

		case 6: /* final stage of tree destruction */
			if (GetTreeCount(tile) > 0) {
				/* more than one tree, delete it */
				AddTreeCount(tile, -1);
				SetTreeGrowth(tile, 3);
			} else {
				/* just one tree, change type into MP_CLEAR */
				switch (GetTreeGround(tile)) {
					case TREE_GROUND_GRASS: MakeClear(tile, CLEAR_GRASS, 3); break;
					case TREE_GROUND_ROUGH: MakeClear(tile, CLEAR_ROUGH, 3); break;
					default: MakeClear(tile, CLEAR_SNOW, GetTreeDensity(tile)); break;
				}
			}
			break;

		default:
			AddTreeGrowth(tile, 1);
			break;
	}

	MarkTileDirtyByTile(tile);
}

void OnTick_Trees(void)
{
	uint32 r;
	TileIndex tile;
	ClearGround ct;
	TreeType tree;

	/* place a tree at a random rainforest spot */
	if (_opt.landscape == LT_DESERT &&
			(r = Random(), tile = RandomTileSeed(r), GetTropicZone(tile) == TROPICZONE_RAINFOREST) &&
			IsTileType(tile, MP_CLEAR) &&
			(ct = GetClearGround(tile), ct == CLEAR_GRASS || ct == CLEAR_ROUGH) &&
			(tree = GetRandomTreeType(tile, GB(r, 24, 8))) != TREE_INVALID) {
		MakeTree(tile, tree, 0, 0, ct == CLEAR_ROUGH ? TREE_GROUND_ROUGH : TREE_GROUND_GRASS, 0);
	}

	// byte underflow
	if (--_trees_tick_ctr != 0) return;

	/* place a tree at a random spot */
	r = Random();
	tile = TILE_MASK(r);
	if (IsTileType(tile, MP_CLEAR) &&
			(ct = GetClearGround(tile), ct == CLEAR_GRASS || ct == CLEAR_ROUGH || ct == CLEAR_SNOW) &&
			(tree = GetRandomTreeType(tile, GB(r, 24, 8))) != TREE_INVALID) {
		switch (ct) {
			case CLEAR_GRASS: MakeTree(tile, tree, 0, 0, TREE_GROUND_GRASS, 0); break;
			case CLEAR_ROUGH: MakeTree(tile, tree, 0, 0, TREE_GROUND_ROUGH, 0); break;
			default: MakeTree(tile, tree, 0, 0, TREE_GROUND_SNOW_DESERT, GetClearDensity(tile)); break;
		}
	}
}

static void ClickTile_Trees(TileIndex tile)
{
	/* not used */
}

static uint32 GetTileTrackStatus_Trees(TileIndex tile, TransportType mode)
{
	return 0;
}

static void ChangeTileOwner_Trees(TileIndex tile, PlayerID old_player, PlayerID new_player)
{
	/* not used */
}

void InitializeTrees(void)
{
	_trees_tick_ctr = 0;
}


const TileTypeProcs _tile_type_trees_procs = {
	DrawTile_Trees,           /* draw_tile_proc */
	GetSlopeZ_Trees,          /* get_slope_z_proc */
	ClearTile_Trees,          /* clear_tile_proc */
	GetAcceptedCargo_Trees,   /* get_accepted_cargo_proc */
	GetTileDesc_Trees,        /* get_tile_desc_proc */
	GetTileTrackStatus_Trees, /* get_tile_track_status_proc */
	ClickTile_Trees,          /* click_tile_proc */
	AnimateTile_Trees,        /* animate_tile_proc */
	TileLoop_Trees,           /* tile_loop_clear */
	ChangeTileOwner_Trees,    /* change_tile_owner_clear */
	NULL,                     /* get_produced_cargo_proc */
	NULL,                     /* vehicle_enter_tile_proc */
	GetSlopeTileh_Trees,      /* get_slope_tileh_proc */
};
