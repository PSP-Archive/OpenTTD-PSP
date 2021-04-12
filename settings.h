/* $Id: settings.h 9584 2007-04-10 09:57:35Z truelight $ */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "saveload.h"

/** Convention/Type of settings. This is then further specified if necessary
 * with the SLE_ (SLE_VAR/SLE_FILE) enums in saveload.h
 * @see VarTypes
 * @see SettingDescBase */
enum SettingDescType {
	/* 4 bytes allocated a maximum of 16 types for GenericType */
	SDT_NUMX        = 0, // any number-type
	SDT_BOOLX       = 1, // a boolean number
	SDT_ONEOFMANY   = 2, // bitmasked number where only ONE bit may be set
	SDT_MANYOFMANY  = 3, // bitmasked number where MULTIPLE bits may be set
	SDT_INTLIST     = 4, // list of integers seperated by a comma ','
	SDT_STRING      = 5, // string with a pre-allocated buffer
	/* 10 more possible primitives */
};

enum SettingGuiFlag {
	/* 8 bytes allocated for a maximum of 8 flags
	 * Flags directing saving/loading of a variable */
	SGF_0ISDISABLED  = 1 << 0, ///< a value of zero means the feature is disabled
	SGF_NOCOMMA      = 1 << 1, ///< number without any thousand seperators (no formatting)
	SGF_MULTISTRING  = 1 << 2, ///< the value represents a limited number of string-options (internally integer)
	SGF_NETWORK_ONLY = 1 << 3, ///< this setting only applies to network games
	SGF_CURRENCY     = 1 << 4, ///< the number represents money, so when reading value multiply by exchange rate
	/* 3 more possible flags */
};

typedef int32 OnChange(int32 var);
typedef byte SettingDescType;
typedef byte SettingGuiFlag;

typedef struct SettingDescBase {
	const char *name;       ///< name of the setting. Used in configuration file and for console
	const void *def;        ///< default value given when none is present
	SettingDescType cmd;    ///< various flags for the variable
	SettingGuiFlag flags;   ///< handles how a setting would show up in the GUI (text/currency, etc.)
	int32 min, max;         ///< minimum and maximum values
	int32 interval;         ///< the interval to use between settings in the 'patches' window. If interval is '0' the interval is dynamically determined
	const char *many;       ///< ONE/MANY_OF_MANY: string of possible values for this type
	StringID str;           ///< (translated) string with descriptive text; gui and console
	OnChange *proc;         ///< callback procedure for when the value is changed
} SettingDescBase;

typedef struct SettingDesc {
	SettingDescBase desc;   ///< Settings structure (going to configuration file)
	SaveLoad save;          ///< Internal structure (going to savegame, parts to config)
} SettingDesc;

/* NOTE: The only difference between SettingDesc and SettingDescGlob is
 * that one uses global variables as a source and the other offsets
 * in a struct which are bound to a certain variable during runtime.
 * The only way to differentiate between these two is to check if an object
 * has been passed to the function or not. If not, then it is a global variable
 * and save->variable has its address, otherwise save->variable only holds the
 * offset in a certain struct */
typedef SettingDesc SettingDescGlobVarList;

typedef enum {
	IGT_VARIABLES = 0, ///< values of the form "landscape = hilly"
	IGT_LIST      = 1, ///< a list of values, seperated by \n and terminated by the next group block
} IniGroupType;

/** The patch values that are used for new games and/or modified in config file */
extern Patches _patches_newgame;

bool IConsoleSetPatchSetting(const char *name, int32 value);
void IConsoleGetPatchSetting(const char *name);
void IConsoleListPatches(void);
const SettingDesc *GetPatchFromName(const char *name, uint *i);
bool SetPatchValue(uint index, const Patches *object, int32 value);

#endif /* SETTINGS_H */
