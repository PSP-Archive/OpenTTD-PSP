/* $Id: roadveh.h 6560 2006-09-28 18:42:35Z peter1138 $ */

#ifndef ROADVEH_H
#define ROADVEH_H

#include "vehicle.h"


static inline bool IsRoadVehInDepot(const Vehicle* v)
{
	assert(v->type == VEH_Road);
	return v->u.road.state == 254;
}

static inline bool IsRoadVehInDepotStopped(const Vehicle* v)
{
	return IsRoadVehInDepot(v) && v->vehstatus & VS_STOPPED;
}

void CcCloneRoadVeh(bool success, TileIndex tile, uint32 p1, uint32 p2);

#endif /* ROADVEH_H */
