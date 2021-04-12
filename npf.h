/* $Id: npf.h 4153 2006-03-29 19:00:56Z celestar $ */

#ifndef NPF_H
#define NPF_H

#include "openttd.h"
#include "aystar.h"
#include "station.h"
#include "vehicle.h"
#include "tile.h"

//mowing grass
enum {
	NPF_HASH_BITS = 12, /* The size of the hash used in pathfinding. Just changing this value should be sufficient to change the hash size. Should be an even value. */
	/* Do no change below values */
	NPF_HASH_SIZE = 1 << NPF_HASH_BITS,
	NPF_HASH_HALFBITS = NPF_HASH_BITS / 2,
	NPF_HASH_HALFMASK = (1 << NPF_HASH_HALFBITS) - 1
};

/* For new pathfinding. Define here so it is globally available without having
 * to include npf.h */
enum {
	NPF_TILE_LENGTH = 100
};

enum {
	/** This penalty is the equivalent of "inifite", which means that paths that
	 * get this penalty will be chosen, but only if there is no other route
	 * without it. Be careful with not applying this penalty to often, or the
	 * total path cost might overflow..
	 * For now, this is just a Very Big Penalty, we might actually implement
	 * this in a nicer way :-)
	 */
	NPF_INFINITE_PENALTY = 1000 * NPF_TILE_LENGTH
};

typedef struct NPFFindStationOrTileData { /* Meant to be stored in AyStar.targetdata */
	TileIndex dest_coords; /* An indication of where the station is, for heuristic purposes, or the target tile */
	StationID station_index; /* station index we're heading for, or INVALID_STATION when we're heading for a tile */
} NPFFindStationOrTileData;

enum { /* Indices into AyStar.userdata[] */
	NPF_TYPE = 0, /* Contains a TransportTypes value */
	NPF_OWNER, /* Contains an Owner value */
	NPF_RAILTYPES, /* Contains a bitmask the compatible RailTypes of the engine when NPF_TYPE == TRANSPORT_RAIL. Unused otherwise. */
};

enum { /* Indices into AyStarNode.userdata[] */
	NPF_TRACKDIR_CHOICE = 0, /* The trackdir chosen to get here */
	NPF_NODE_FLAGS,
};

typedef enum { /* Flags for AyStarNode.userdata[NPF_NODE_FLAGS]. Use NPFGetBit() and NPFGetBit() to use them. */
	NPF_FLAG_SEEN_SIGNAL, /* Used to mark that a signal was seen on the way, for rail only */
	NPF_FLAG_REVERSE, /* Used to mark that this node was reached from the second start node, if applicable */
	NPF_FLAG_LAST_SIGNAL_RED, /* Used to mark that the last signal on this path was red */
} NPFNodeFlag;

typedef struct NPFFoundTargetData { /* Meant to be stored in AyStar.userpath */
	uint best_bird_dist; /* The best heuristic found. Is 0 if the target was found */
	uint best_path_dist; /* The shortest path. Is (uint)-1 if no path is found */
	Trackdir best_trackdir; /* The trackdir that leads to the shortest path/closest birds dist */
	AyStarNode node; /* The node within the target the search led us to */
} NPFFoundTargetData;

/* These functions below are _not_ re-entrant, in favor of speed! */

/* Will search from the given tile and direction, for a route to the given
 * station for the given transport type. See the declaration of
 * NPFFoundTargetData above for the meaning of the result. */
NPFFoundTargetData NPFRouteToStationOrTile(TileIndex tile, Trackdir trackdir, NPFFindStationOrTileData* target, TransportType type, Owner owner, RailTypeMask railtypes);

/* Will search as above, but with two start nodes, the second being the
 * reverse. Look at the NPF_FLAG_REVERSE flag in the result node to see which
 * direction was taken (NPFGetBit(result.node, NPF_FLAG_REVERSE)) */
NPFFoundTargetData NPFRouteToStationOrTileTwoWay(TileIndex tile1, Trackdir trackdir1, TileIndex tile2, Trackdir trackdir2, NPFFindStationOrTileData* target, TransportType type, Owner owner, RailTypeMask railtypes);

/* Will search a route to the closest depot. */

/* Search using breadth first. Good for little track choice and inaccurate
 * heuristic, such as railway/road.*/
NPFFoundTargetData NPFRouteToDepotBreadthFirst(TileIndex tile, Trackdir trackdir, TransportType type, Owner owner, RailTypeMask railtypes);
/* Same as above but with two start nodes, the second being the reverse. Call
 * NPFGetBit(result.node, NPF_FLAG_REVERSE) to see from which node the path
 * orginated. All pathfs from the second node will have the given
 * reverse_penalty applied (NPF_TILE_LENGTH is the equivalent of one full
 * tile).
 */
NPFFoundTargetData NPFRouteToDepotBreadthFirstTwoWay(TileIndex tile1, Trackdir trackdir1, TileIndex tile2, Trackdir trackdir2, TransportType type, Owner owner, RailTypeMask railtypes, uint reverse_penalty);
/* Search by trying each depot in order of Manhattan Distance. Good for lots
 * of choices and accurate heuristics, such as water. */
NPFFoundTargetData NPFRouteToDepotTrialError(TileIndex tile, Trackdir trackdir, TransportType type, Owner owner, RailTypeMask railtypes);

void NPFFillWithOrderData(NPFFindStationOrTileData* fstd, Vehicle* v);


/*
 * Functions to manipulate the various NPF related flags on an AyStarNode.
 */

/**
 * Returns the current value of the given flag on the given AyStarNode.
 */
static inline bool NPFGetFlag(const AyStarNode* node, NPFNodeFlag flag)
{
	return HASBIT(node->user_data[NPF_NODE_FLAGS], flag);
}

/**
 * Sets the given flag on the given AyStarNode to the given value.
 */
static inline void NPFSetFlag(AyStarNode* node, NPFNodeFlag flag, bool value)
{
	if (value)
		SETBIT(node->user_data[NPF_NODE_FLAGS], flag);
	else
		CLRBIT(node->user_data[NPF_NODE_FLAGS], flag);
}

#endif /* NPF_H */
