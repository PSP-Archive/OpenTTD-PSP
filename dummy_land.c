/* $Id: dummy_land.c 6045 2006-08-22 14:38:37Z rubidium $ */

#include "stdafx.h"
#include "openttd.h"
#include "table/strings.h"
#include "functions.h"
#include "viewport.h"
#include "command.h"
#include "table/sprites.h"

static void DrawTile_Dummy(TileInfo *ti)
{
	DrawGroundSpriteAt(SPR_SHADOW_CELL, ti->x, ti->y, ti->z);
}


static uint GetSlopeZ_Dummy(TileIndex tile, uint x, uint y)
{
	return 0;
}

static Slope GetSlopeTileh_Dummy(TileIndex tile, Slope tileh)
{
	return SLOPE_FLAT;
}

static int32 ClearTile_Dummy(TileIndex tile, byte flags)
{
	return_cmd_error(STR_0001_OFF_EDGE_OF_MAP);
}


static void GetAcceptedCargo_Dummy(TileIndex tile, AcceptedCargo ac)
{
	/* not used */
}

static void GetTileDesc_Dummy(TileIndex tile, TileDesc *td)
{
	td->str = STR_EMPTY;
	td->owner = OWNER_NONE;
}

static void AnimateTile_Dummy(TileIndex tile)
{
	/* not used */
}

static void TileLoop_Dummy(TileIndex tile)
{
	/* not used */
}

static void ClickTile_Dummy(TileIndex tile)
{
	/* not used */
}

static void ChangeTileOwner_Dummy(TileIndex tile, PlayerID old_player, PlayerID new_player)
{
	/* not used */
}

static uint32 GetTileTrackStatus_Dummy(TileIndex tile, TransportType mode)
{
	return 0;
}

const TileTypeProcs _tile_type_dummy_procs = {
	DrawTile_Dummy,           /* draw_tile_proc */
	GetSlopeZ_Dummy,          /* get_slope_z_proc */
	ClearTile_Dummy,          /* clear_tile_proc */
	GetAcceptedCargo_Dummy,   /* get_accepted_cargo_proc */
	GetTileDesc_Dummy,        /* get_tile_desc_proc */
	GetTileTrackStatus_Dummy, /* get_tile_track_status_proc */
	ClickTile_Dummy,          /* click_tile_proc */
	AnimateTile_Dummy,        /* animate_tile_proc */
	TileLoop_Dummy,           /* tile_loop_clear */
	ChangeTileOwner_Dummy,    /* change_tile_owner_clear */
	NULL,                     /* get_produced_cargo_proc */
	NULL,                     /* vehicle_enter_tile_proc */
	GetSlopeTileh_Dummy,      /* get_slope_tileh_proc */
};
