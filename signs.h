/* $Id: signs.h 7372 2006-12-05 13:58:20Z matthijs $ */

#ifndef SIGNS_H
#define SIGNS_H

#include "oldpool.h"

typedef struct Sign {
	StringID     str;
	ViewportSign sign;
	int32        x;
	int32        y;
	byte         z;
	PlayerID     owner; // placed by this player. Anyone can delete them though. OWNER_NONE for gray signs from old games.

	SignID       index;
} Sign;

DECLARE_OLD_POOL(Sign, Sign, 2, 16000)

static inline SignID GetMaxSignIndex(void)
{
	/* TODO - This isn't the real content of the function, but
	 *  with the new pool-system this will be replaced with one that
	 *  _really_ returns the highest index. Now it just returns
	 *  the next safe value we are sure about everything is below.
	 */
	return GetSignPoolSize() - 1;
}

static inline uint GetNumSigns(void)
{
	return GetSignPoolSize();
}

/**
 * Check if a Sign really exists.
 */
static inline bool IsValidSign(const Sign *si)
{
	return si->str != STR_NULL;
}

static inline bool IsValidSignID(uint index)
{
	return index < GetSignPoolSize() && IsValidSign(GetSign(index));
}

void DestroySign(Sign *si);

static inline void DeleteSign(Sign *si)
{
	DestroySign(si);
	si->str = STR_NULL;
}

#define FOR_ALL_SIGNS_FROM(ss, start) for (ss = GetSign(start); ss != NULL; ss = (ss->index + 1U < GetSignPoolSize()) ? GetSign(ss->index + 1U) : NULL) if (IsValidSign(ss))
#define FOR_ALL_SIGNS(ss) FOR_ALL_SIGNS_FROM(ss, 0)

VARDEF bool _sign_sort_dirty;

void UpdateAllSignVirtCoords(void);
void PlaceProc_Sign(TileIndex tile);

/* misc.c */
void ShowRenameSignWindow(const Sign *si);

#endif /* SIGNS_H */
