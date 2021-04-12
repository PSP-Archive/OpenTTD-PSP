/* $Id: namegen.h 6895 2006-10-22 10:08:49Z peter1138 $ */

#ifndef NAMEGEN_H
#define NAMEGEN_H

typedef byte TownNameGenerator(char *buf, uint32 seed, const char *last);

extern TownNameGenerator * const _town_name_generators[];

#endif /* NAMEGEN_H */
