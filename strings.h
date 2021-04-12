/* $Id: strings.h 7182 2006-11-16 22:05:33Z peter1138 $ */

#ifndef STRINGS_H
#define STRINGS_H

char *InlineString(char *buf, uint16 string);
char *GetString(char *buffr, uint16 string, const char* last);

extern char _userstring[128];

void InjectDParam(int amount);
int32 GetParamInt32(void);

#endif /* STRINGS_H */
