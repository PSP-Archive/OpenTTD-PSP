/* $Id: bridge_map.c 6406 2006-09-05 23:21:41Z Darkvater $ */

#include "stdafx.h"
#include "openttd.h"
#include "bridge_map.h"


TileIndex GetBridgeEnd(TileIndex tile, DiagDirection dir)
{
	TileIndexDiff delta = TileOffsByDiagDir(dir);

	assert(DiagDirToAxis(dir) == GetBridgeAxis(tile));

	do {
		tile += delta;
	} while (!IsBridgeRamp(tile));

	return tile;
}


TileIndex GetSouthernBridgeEnd(TileIndex t)
{
	return GetBridgeEnd(t, AxisToDiagDir(GetBridgeAxis(t)));
}


TileIndex GetOtherBridgeEnd(TileIndex tile)
{
	TileIndexDiff delta = TileOffsByDiagDir(GetBridgeRampDirection(tile));

	do {
		tile += delta;
	} while (!IsBridgeRamp(tile));

	return tile;
}
