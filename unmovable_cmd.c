/* $Id: unmovable_cmd.c 6774 2006-10-14 15:49:43Z Darkvater $ */

#include "stdafx.h"
#include "openttd.h"
#include "table/strings.h"
#include "table/sprites.h"
#include "functions.h"
#include "map.h"
#include "tile.h"
#include "command.h"
#include "viewport.h"
#include "player.h"
#include "gui.h"
#include "station.h"
#include "economy.h"
#include "town.h"
#include "sprite.h"
#include "unmovable_map.h"
#include "variables.h"
#include "table/unmovable_land.h"
#include "genworld.h"

/** Destroy a HQ.
 * During normal gameplay you can only implicitely destroy a HQ when you are
 * rebuilding it. Otherwise, only water can destroy it.
 * @param tile tile coordinates where HQ is located to destroy
 * @param flags docommand flags of calling function
 */
static int32 DestroyCompanyHQ(PlayerID pid, uint32 flags)
{
	Player* p = GetPlayer(pid);

	SET_EXPENSES_TYPE(EXPENSES_PROPERTY);

	if (flags & DC_EXEC) {
		TileIndex t = p->location_of_house;

		DoClearSquare(t + TileDiffXY(0, 0));
		DoClearSquare(t + TileDiffXY(0, 1));
		DoClearSquare(t + TileDiffXY(1, 0));
		DoClearSquare(t + TileDiffXY(1, 1));
		p->location_of_house = 0; // reset HQ position
		InvalidateWindow(WC_COMPANY, pid);
	}

	// cost of relocating company is 1% of company value
	return CalculateCompanyValue(p) / 100;
}

void UpdateCompanyHQ(Player *p, uint score)
{
	byte val;
	TileIndex tile = p->location_of_house;

	if (tile == 0) return;

	(val = 0, score < 170) ||
	(val++, score < 350) ||
	(val++, score < 520) ||
	(val++, score < 720) ||
	(val++, true);

	EnlargeCompanyHQ(tile, val);

	MarkTileDirtyByTile(tile + TileDiffXY(0, 0));
	MarkTileDirtyByTile(tile + TileDiffXY(0, 1));
	MarkTileDirtyByTile(tile + TileDiffXY(1, 0));
	MarkTileDirtyByTile(tile + TileDiffXY(1, 1));
}

/** Build or relocate the HQ. This depends if the HQ is already built or not
 * @param tile tile where the HQ will be built or relocated to
 * @param p1 unused
 * @param p2 unused
 */
extern int32 CheckFlatLandBelow(TileIndex tile, uint w, uint h, uint flags, uint invalid_dirs, int *);
int32 CmdBuildCompanyHQ(TileIndex tile, uint32 flags, uint32 p1, uint32 p2)
{
	Player *p = GetPlayer(_current_player);
	int cost;
	int32 ret;

	SET_EXPENSES_TYPE(EXPENSES_PROPERTY);

	ret = CheckFlatLandBelow(tile, 2, 2, flags, 0, NULL);
	if (CmdFailed(ret)) return ret;
	cost = ret;

	if (p->location_of_house != 0) { /* Moving HQ */
		cost += DestroyCompanyHQ(_current_player, flags);
	}

	if (flags & DC_EXEC) {
		int score = UpdateCompanyRatingAndValue(p, false);

		p->location_of_house = tile;

		MakeCompanyHQ(tile, _current_player);

		UpdateCompanyHQ(p, score);
		InvalidateWindow(WC_COMPANY, p->index);
	}

	return cost;
}

static void DrawTile_Unmovable(TileInfo *ti)
{
	uint32 image, ormod;

	switch (GetUnmovableType(ti->tile)) {
		case UNMOVABLE_TRANSMITTER:
		case UNMOVABLE_LIGHTHOUSE: {
			const DrawTileUnmovableStruct* dtus;

			if (ti->tileh != SLOPE_FLAT) DrawFoundation(ti, ti->tileh);
			DrawClearLandTile(ti, 2);

			dtus = &_draw_tile_unmovable_data[GetUnmovableType(ti->tile)];

			image = dtus->image;
			if (_display_opt & DO_TRANS_BUILDINGS) MAKE_TRANSPARENT(image);

			AddSortableSpriteToDraw(
				image, ti->x | dtus->subcoord_x, ti->y | dtus->subcoord_y,
				dtus->width, dtus->height, dtus->z_size, ti->z
			);
			break;
		}

		case UNMOVABLE_STATUE:
			DrawGroundSprite(SPR_CONCRETE_GROUND);

			image = PLAYER_SPRITE_COLOR(GetTileOwner(ti->tile));
			image += PALETTE_MODIFIER_COLOR | SPR_STATUE_COMPANY;
			if (_display_opt & DO_TRANS_BUILDINGS) MAKE_TRANSPARENT(image);
			AddSortableSpriteToDraw(image, ti->x, ti->y, 16, 16, 25, ti->z);
			break;

		case UNMOVABLE_OWNED_LAND:
			DrawClearLandTile(ti, 0);

			AddSortableSpriteToDraw(
				PLAYER_SPRITE_COLOR(GetTileOwner(ti->tile)) + PALETTE_MODIFIER_COLOR + SPR_BOUGHT_LAND,
				ti->x + TILE_SIZE / 2, ti->y + TILE_SIZE / 2, 1, 1, 10, GetSlopeZ(ti->x + TILE_SIZE / 2, ti->y + TILE_SIZE / 2)
			);
			break;

		default: {
			const DrawTileSeqStruct* dtss;
			const DrawTileSprites* t;

			assert(IsCompanyHQ(ti->tile));
			if (ti->tileh != SLOPE_FLAT) DrawFoundation(ti, ti->tileh);

			ormod = PLAYER_SPRITE_COLOR(GetTileOwner(ti->tile));

			t = &_unmovable_display_datas[GetCompanyHQSection(ti->tile)];
			DrawGroundSprite(t->ground_sprite | ormod);

			foreach_draw_tile_seq(dtss, t->seq) {
				image = dtss->image;
				if (_display_opt & DO_TRANS_BUILDINGS) {
					MAKE_TRANSPARENT(image);
				} else {
					image |= ormod;
				}
				AddSortableSpriteToDraw(
					image,
					ti->x + dtss->delta_x, ti->y + dtss->delta_y,
					dtss->size_x, dtss->size_y,
					dtss->size_z, ti->z + dtss->delta_z
				);
			}
			break;
		}
	}
}

static uint GetSlopeZ_Unmovable(TileIndex tile, uint x, uint y)
{
	if (IsOwnedLand(tile)) {
		uint z;
		uint tileh = GetTileSlope(tile, &z);

		return z + GetPartialZ(x & 0xF, y & 0xF, tileh);
	} else {
		return GetTileMaxZ(tile);
	}
}

static Slope GetSlopeTileh_Unmovable(TileIndex tile, Slope tileh)
{
	return IsOwnedLand(tile) ? tileh : SLOPE_FLAT;
}

static int32 ClearTile_Unmovable(TileIndex tile, byte flags)
{
	if (IsCompanyHQ(tile)) {
		if (_current_player == OWNER_WATER) {
			return DestroyCompanyHQ(GetTileOwner(tile), DC_EXEC);
		} else {
			return_cmd_error(STR_5804_COMPANY_HEADQUARTERS_IN);
		}
	}

	if (IsOwnedLand(tile)) {
		return DoCommand(tile, 0, 0, flags, CMD_SELL_LAND_AREA);
	}

	// checks if you're allowed to remove unmovable things
	if (_game_mode != GM_EDITOR && _current_player != OWNER_WATER && ((flags & DC_AUTO || !_cheats.magic_bulldozer.value)) )
		return_cmd_error(STR_5800_OBJECT_IN_THE_WAY);

	if (flags & DC_EXEC) {
		DoClearSquare(tile);
	}

	return 0;
}

static void GetAcceptedCargo_Unmovable(TileIndex tile, AcceptedCargo ac)
{
	uint level; // HQ level (depends on company performance) in the range 1..5.

	if (!IsCompanyHQ(tile)) return;

	/* HQ accepts passenger and mail; but we have to divide the values
	 * between 4 tiles it occupies! */

	level = GetCompanyHQSize(tile) + 1;

	// Top town building generates 10, so to make HQ interesting, the top
	// type makes 20.
	ac[CT_PASSENGERS] = max(1, level);

	// Top town building generates 4, HQ can make up to 8. The
	// proportion passengers:mail is different because such a huge
	// commercial building generates unusually high amount of mail
	// correspondence per physical visitor.
	ac[CT_MAIL] = max(1, level / 2);
}


static void GetTileDesc_Unmovable(TileIndex tile, TileDesc *td)
{
	switch (GetUnmovableType(tile)) {
		case UNMOVABLE_TRANSMITTER: td->str = STR_5801_TRANSMITTER; break;
		case UNMOVABLE_LIGHTHOUSE:  td->str = STR_5802_LIGHTHOUSE; break;
		case UNMOVABLE_STATUE:      td->str = STR_2016_STATUE; break;
		case UNMOVABLE_OWNED_LAND:  td->str = STR_5805_COMPANY_OWNED_LAND; break;
		default:                    td->str = STR_5803_COMPANY_HEADQUARTERS; break;
	}
	td->owner = GetTileOwner(tile);
}

static void AnimateTile_Unmovable(TileIndex tile)
{
	/* not used */
}

static void TileLoop_Unmovable(TileIndex tile)
{
	uint level; // HQ level (depends on company performance) in the range 1..5.
	uint32 r;

	if (!IsCompanyHQ(tile)) return;

	/* HQ accepts passenger and mail; but we have to divide the values
	 * between 4 tiles it occupies! */

	level = GetCompanyHQSize(tile) + 1;
	assert(level < 6);

	r = Random();
	// Top town buildings generate 250, so the top HQ type makes 256.
	if (GB(r, 0, 8) < (256 / 4 / (6 - level))) {
		uint amt = GB(r, 0, 8) / 8 / 4 + 1;
		if (_economy.fluct <= 0) amt = (amt + 1) >> 1;
		MoveGoodsToStation(tile, 2, 2, CT_PASSENGERS, amt);
	}

	// Top town building generates 90, HQ can make up to 196. The
	// proportion passengers:mail is about the same as in the acceptance
	// equations.
	if (GB(r, 8, 8) < (196 / 4 / (6 - level))) {
		uint amt = GB(r, 8, 8) / 8 / 4 + 1;
		if (_economy.fluct <= 0) amt = (amt + 1) >> 1;
		MoveGoodsToStation(tile, 2, 2, CT_MAIL, amt);
	}
}


static uint32 GetTileTrackStatus_Unmovable(TileIndex tile, TransportType mode)
{
	return 0;
}

static void ClickTile_Unmovable(TileIndex tile)
{
	if (IsCompanyHQ(tile)) ShowPlayerCompany(GetTileOwner(tile));
}


/* checks, if a radio tower is within a 9x9 tile square around tile */
static bool IsRadioTowerNearby(TileIndex tile)
{
	TileIndex tile_s = tile - TileDiffXY(4, 4);

	BEGIN_TILE_LOOP(tile, 9, 9, tile_s)
		if (IsTransmitterTile(tile)) return true;
	END_TILE_LOOP(tile, 9, 9, tile_s)

	return false;
}

void GenerateUnmovables(void)
{
	int i, li, j, loop_count;
	TileIndex tile;
	uint h;
	uint maxx;
	uint maxy;

	if (_opt.landscape == LT_CANDY) return;

	/* add radio tower */
	i = ScaleByMapSize(1000);
	j = ScaleByMapSize(15); // maximum number of radio towers on the map
	li = ScaleByMapSize1D((Random() & 3) + 7);
	SetGeneratingWorldProgress(GWP_UNMOVABLE, j + li);

	do {
		tile = RandomTile();
		if (IsTileType(tile, MP_CLEAR) && GetTileSlope(tile, &h) == SLOPE_FLAT && h >= TILE_HEIGHT * 4) {
			if (IsRadioTowerNearby(tile)) continue;
			MakeTransmitter(tile);
			IncreaseGeneratingWorldProgress(GWP_UNMOVABLE);
			if (--j == 0) break;
		}
	} while (--i);

	if (_opt.landscape == LT_DESERT) return;

	/* add lighthouses */
	i = li;
	maxx = MapMaxX();
	maxy = MapMaxY();
	loop_count = 0;
	do {
		uint32 r;
		DiagDirection dir;
		int perimeter;

restart:
		/* Avoid infinite loops */
		if (++loop_count > 1000) break;

		r = Random();

		/* Scatter the lighthouses more evenly around the perimeter */
		perimeter = (GB(r, 16, 16) % (2 * (maxx + maxy))) - maxy;
		for (dir = DIAGDIR_NE; perimeter > 0; dir++) {
			perimeter -= (DiagDirToAxis(dir) == AXIS_X) ? maxx : maxy;
		}

		switch (dir) {
			default:
			case DIAGDIR_NE: tile = TileXY(maxx,     r % maxy); break;
			case DIAGDIR_SE: tile = TileXY(r % maxx, 0);        break;
			case DIAGDIR_SW: tile = TileXY(0,        r % maxy); break;
			case DIAGDIR_NW: tile = TileXY(r % maxx, maxy);     break;
		}
		j = 20;
		do {
			if (--j == 0) goto restart;
			tile = TILE_MASK(tile + TileOffsByDiagDir(dir));
		} while (!(IsTileType(tile, MP_CLEAR) && GetTileSlope(tile, &h) == SLOPE_FLAT && h <= TILE_HEIGHT * 2));

		assert(tile == TILE_MASK(tile));

		MakeLighthouse(tile);
		IncreaseGeneratingWorldProgress(GWP_UNMOVABLE);
	} while (--i);
}

static void ChangeTileOwner_Unmovable(TileIndex tile, PlayerID old_player, PlayerID new_player)
{
	if (!IsTileOwner(tile, old_player)) return;

	if (IsOwnedLand(tile) && new_player != PLAYER_SPECTATOR) {
		SetTileOwner(tile, new_player);
	} else {
		DoClearSquare(tile);
	}
}

const TileTypeProcs _tile_type_unmovable_procs = {
	DrawTile_Unmovable,             /* draw_tile_proc */
	GetSlopeZ_Unmovable,            /* get_slope_z_proc */
	ClearTile_Unmovable,            /* clear_tile_proc */
	GetAcceptedCargo_Unmovable,     /* get_accepted_cargo_proc */
	GetTileDesc_Unmovable,          /* get_tile_desc_proc */
	GetTileTrackStatus_Unmovable,   /* get_tile_track_status_proc */
	ClickTile_Unmovable,            /* click_tile_proc */
	AnimateTile_Unmovable,          /* animate_tile_proc */
	TileLoop_Unmovable,             /* tile_loop_clear */
	ChangeTileOwner_Unmovable,      /* change_tile_owner_clear */
	NULL,                           /* get_produced_cargo_proc */
	NULL,                           /* vehicle_enter_tile_proc */
	GetSlopeTileh_Unmovable,        /* get_slope_tileh_proc */
};
