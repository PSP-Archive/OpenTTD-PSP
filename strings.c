/* $Id: strings.c 9355 2007-03-19 22:08:18Z rubidium $ */

#include "stdafx.h"
#include "openttd.h"
#include "currency.h"
#include "functions.h"
#include "string.h"
#include "strings.h"
#include "table/strings.h"
#include "namegen.h"
#include "station.h"
#include "town.h"
#include "vehicle.h"
#include "news.h"
#include "screenshot.h"
#include "waypoint.h"
#include "industry.h"
#include "variables.h"
#include "newgrf_text.h"
#include "table/landscape_const.h"
#include "table/control_codes.h"
#include "music.h"
#include "date.h"
#include "industry.h"

/* for opendir/readdir/closedir */
# include "fios.h"

char _userstring[128];

static char *StationGetSpecialString(char *buff, int x, const char* last);
static char *GetSpecialTownNameString(char *buff, int ind, uint32 seed, const char* last);
static char *GetSpecialPlayerNameString(char *buff, int ind, const int32 *argv, const char* last);

static char *FormatString(char *buff, const char *str, const int32 *argv, uint casei, const char* last);

typedef struct LanguagePack {
	uint32 ident;
	uint32 version;     // 32-bits of auto generated version info which is basically a hash of strings.h
	char name[32];      // the international name of this language
	char own_name[32];  // the localized name of this language
	char isocode[16];   // the ISO code for the language (not country code)
	uint16 offsets[32]; // the offsets
	byte plural_form;   // how to compute plural forms
	byte pad[3];        // pad header to be a multiple of 4
	char data[VARARRAY_SIZE];
} LanguagePack;

static char **_langpack_offs;
static LanguagePack *_langpack;
static uint _langtab_num[32]; // Offset into langpack offs
static uint _langtab_start[32]; // Offset into langpack offs

static const StringID _cargo_string_list[NUM_LANDSCAPE][NUM_CARGO] = {
	{ /* LT_NORMAL */
		STR_PASSENGERS,
		STR_TONS,
		STR_BAGS,
		STR_LITERS,
		STR_ITEMS,
		STR_CRATES,
		STR_TONS,
		STR_TONS,
		STR_TONS,
		STR_TONS,
		STR_BAGS,
		STR_RES_OTHER
	},

	{ /* LT_HILLY */
		STR_PASSENGERS,
		STR_TONS,
		STR_BAGS,
		STR_LITERS,
		STR_ITEMS,
		STR_CRATES,
		STR_TONS,
		STR_TONS,
		STR_RES_OTHER,
		STR_TONS,
		STR_BAGS,
		STR_TONS
	},

	{ /* LT_DESERT */
		STR_PASSENGERS,
		STR_LITERS,
		STR_BAGS,
		STR_LITERS,
		STR_TONS,
		STR_CRATES,
		STR_TONS,
		STR_TONS,
		STR_TONS,
		STR_LITERS,
		STR_BAGS,
		STR_TONS
	},

	{ /* LT_CANDY */
		STR_PASSENGERS,
		STR_TONS,
		STR_BAGS,
		STR_NOTHING,
		STR_NOTHING,
		STR_TONS,
		STR_TONS,
		STR_LITERS,
		STR_TONS,
		STR_NOTHING,
		STR_LITERS,
		STR_NOTHING
	}
};

#if defined(PSP)
#define MyAlwaysReadUnaligned16(addr)  \
        ( ((((unsigned char *)(addr))[0])) | \
          ((((unsigned char *)(addr))[1]) << 8) )

static inline uint16 ALWAYS_READ_LE_UINT16(const uint16 *b)
{
	        return (uint16)MyAlwaysReadUnaligned16(b);
}
#endif

// Read an int64 from the argv array.
static inline int64 GetInt64(const int32 **argv)
{
	int64 result;

	assert(argv);
	result = (uint32)(*argv)[0] + ((uint64)(uint32)(*argv)[1] << 32);
	(*argv)+=2;
	return result;
}

// Read an int32 from the argv array.
static inline int32 GetInt32(const int32 **argv)
{
	assert(argv);
	return *(*argv)++;
}

// Read an array from the argv array.
static inline const int32 *GetArgvPtr(const int32 **argv, int n)
{
	const int32 *result;
	assert(*argv);
	result = *argv;
	(*argv) += n;
	return result;
}


#define NUM_BOUND_STRINGS 8

// Array to hold the bound strings.
static const char *_bound_strings[NUM_BOUND_STRINGS];

// This index is used to implement a "round-robin" allocating of
// slots for BindCString. NUM_BOUND_STRINGS slots are reserved.
// Which means that after NUM_BOUND_STRINGS calls to BindCString,
// the indices will be reused.
static int _bind_index;

static const char *GetStringPtr(StringID string)
{
	return _langpack_offs[_langtab_start[string >> 11] + (string & 0x7FF)];
}

// The highest 8 bits of string contain the "case index".
// These 8 bits will only be set when FormatString wants to print
// the string in a different case. No one else except FormatString
// should set those bits, therefore string CANNOT be StringID, but uint32.
static char *GetStringWithArgs(char *buffr, uint string, const int32 *argv, const char* last)
{
	uint index = GB(string,  0, 11);
	uint tab   = GB(string, 11,  5);
	char buff[512];

	if (GB(string, 0, 16) == 0) error("!invalid string id 0 in GetString");

	switch (tab) {
		case 4:
			if (index >= 0xC0)
				return GetSpecialTownNameString(buffr, index - 0xC0, GetInt32(&argv), last);
			break;

		case 14:
			if (index >= 0xE4)
				return GetSpecialPlayerNameString(buffr, index - 0xE4, argv, last);
			break;

		// User defined name
		case 15:
			return GetName(buffr, index, last);

		case 26:
			/* Include string within newgrf text (format code 81) */
			if (HASBIT(index, 10)) {
				StringID string = GetGRFStringID(0, 0xD000 + GB(index, 0, 10));
				return GetStringWithArgs(buffr, string, argv, last);
			}
			break;

		case 28:
			GetGRFString(buff, index, lastof(buff));
			return FormatString(buffr, buff, argv, 0, last);

		case 29:
			GetGRFString(buff, index + 0x800, lastof(buff));
			return FormatString(buffr, buff, argv, 0, last);

		case 30:
			GetGRFString(buff, index + 0x1000, lastof(buff));
			return FormatString(buffr, buff, argv, 0, last);

		case 31:
			// dynamic strings. These are NOT to be passed through the formatter,
			// but passed through verbatim.
			if (index < (STR_SPEC_USERSTRING & 0x7FF)) {
				return strecpy(buffr, _bound_strings[index], last);
			}

			return FormatString(buffr, _userstring, NULL, 0, last);
	}

	if (index >= _langtab_num[tab]) {
		error(
			"!String 0x%X is invalid. "
			"Probably because an old version of the .lng file.\n", string
		);
	}

	return FormatString(buffr, GetStringPtr(GB(string, 0, 16)), argv, GB(string, 24, 8), last);
}

char *GetString(char *buffr, StringID string, const char* last)
{
	return GetStringWithArgs(buffr, string, (int32*)_decode_parameters, last);
}


char *InlineString(char *buf, StringID string)
{
	buf += Utf8Encode(buf, SCC_STRING_ID);
	buf += Utf8Encode(buf, string);
	return buf;
}


/**
 * This function takes a C-string and allocates a temporary string ID.
 * The StringID of the bound string is valid until BindCString is called
 * another NUM_BOUND_STRINGS times. So be careful when using it.
 *
 * @note formatting a DATE_TINY calls BindCString twice, thus reduces the
 *       amount of 'user' bound strings by 2.
 * @todo rewrite the BindCString system to make the limit flexible and
 *       non-round-robin. For example by using smart pointers that free
 *       the allocated StringID when they go out-of-scope/are freed.
 */
StringID BindCString(const char *str)
{
	int idx = (++_bind_index) & (NUM_BOUND_STRINGS - 1);
	_bound_strings[idx] = str;
	return idx + STR_SPEC_DYNSTRING;
}

// This function is used to "bind" a C string to a OpenTTD dparam slot.
void SetDParamStr(uint n, const char *str)
{
	SetDParam(n, BindCString(str));
}

void InjectDParam(int amount)
{
	memmove(_decode_parameters + amount, _decode_parameters, sizeof(_decode_parameters) - amount * sizeof(uint32));
}

static const uint32 _divisor_table[] = {
	1000000000,
	100000000,
	10000000,
	1000000,

	100000,
	10000,
	1000,
	100,
	10,
	1
};

// TODO
static char *FormatCommaNumber(char *buff, int32 number, const char* last)
{
	uint32 quot,divisor;
	int i;
	uint32 tot;
	uint32 num;

	if (number < 0) {
		*buff++ = '-';
		number = -number;
	}

	num = number;

	tot = 0;
	for (i = 0; i != 10; i++) {
		divisor = _divisor_table[i];
		quot = 0;
		if (num >= divisor) {
			quot = num / _divisor_table[i];
			num = num % _divisor_table[i];
		}
		if (tot |= quot || i == 9) {
			*buff++ = '0' + quot;
			if (i == 0 || i == 3 || i == 6) *buff++ = ',';
		}
	}

	*buff = '\0';

	return buff;
}

// TODO
static char *FormatNoCommaNumber(char *buff, int32 number, const char* last)
{
	uint32 quot,divisor;
	int i;
	uint32 tot;
	uint32 num;

	if (number < 0) {
		buff = strecpy(buff, "-", last);
		number = -number;
	}

	num = number;

	tot = 0;
	for (i = 0; i != 10; i++) {
		divisor = _divisor_table[i];
		quot = 0;
		if (num >= divisor) {
			quot = num / _divisor_table[i];
			num = num % _divisor_table[i];
		}
		if (tot |= quot || i == 9) {
			*buff++ = '0' + quot;
		}
	}

	*buff = '\0';

	return buff;
}


static char *FormatYmdString(char *buff, Date date, const char* last)
{
	YearMonthDay ymd;
	int32 args[3];
	ConvertDateToYMD(date, &ymd);

	args[0] = ymd.day + STR_01AC_1ST - 1;
	args[1] = STR_0162_JAN + ymd.month;
	args[2] = ymd.year;
	return FormatString(buff, GetStringPtr(STR_DATE_LONG), args, 0, last);
}

static char *FormatMonthAndYear(char *buff, Date date, const char* last)
{
	YearMonthDay ymd;
	int32 args[2];
	ConvertDateToYMD(date, &ymd);

	args[0] = STR_MONTH_JAN + ymd.month;
	args[1] = ymd.year;
	return FormatString(buff, GetStringPtr(STR_DATE_SHORT), args, 0, last);
}

static char *FormatTinyDate(char *buff, Date date, const char* last)
{
	YearMonthDay ymd;
	int32 args[3];
	char day[3];
	char month[3];
	ConvertDateToYMD(date, &ymd);

	/* We want to zero-pad the days and months */
	snprintf(day,   lengthof(day),   "%02i", ymd.day);
	snprintf(month, lengthof(month), "%02i", ymd.month + 1);

	args[0] = BindCString(day);
	args[1] = BindCString(month);
	args[2] = ymd.year;
	return FormatString(buff, GetStringPtr(STR_DATE_TINY), args, 0, last);
}

static char *FormatGenericCurrency(char *buff, const CurrencySpec *spec, int64 number, bool compact, const char* last)
{
	const char* multiplier = "";
	char buf[40];
	char* p;
	int j;

	// multiply by exchange rate
	number *= spec->rate;

	// convert from negative
	if (number < 0) {
		buff = strecpy(buff, "-", last);
		number = -number;
	}

	/* Add prefix part, folowing symbol_pos specification.
	 * Here, it can can be either 0 (prefix) or 2 (both prefix anf suffix).
	 * The only remaining value is 1 (suffix), so everything that is not 1 */
	if (spec->symbol_pos != 1) buff = strecpy(buff, spec->prefix, last);

	// for huge numbers, compact the number into k or M
	if (compact) {
		if (number >= 1000000000) {
			number = (number + 500000) / 1000000;
			multiplier = "M";
		} else if (number >= 1000000) {
			number = (number + 500) / 1000;
			multiplier = "k";
		}
	}

	// convert to ascii number and add commas
	p = endof(buf);
	*--p = '\0';
	j = 4;
	do {
		if (--j == 0) {
			*--p = spec->separator;
			j = 3;
		}
		*--p = '0' + number % 10;
	} while ((number /= 10) != 0);
	buff = strecpy(buff, p, last);

	buff = strecpy(buff, multiplier, last);

	/* Add suffix part, folowing symbol_pos specification.
	 * Here, it can can be either 1 (suffix) or 2 (both prefix anf suffix).
	 * The only remaining value is 1 (prefix), so everything that is not 0 */
	if (spec->symbol_pos != 0) buff = strecpy(buff, spec->suffix, last);

	return buff;
}

static int DeterminePluralForm(int32 n)
{
	// The absolute value determines plurality
	if (n < 0) n = -n;

	switch (_langpack->plural_form) {
	// Two forms, singular used for one only
	// Used in:
	//   Danish, Dutch, English, German, Norwegian, Swedish, Estonian, Finnish,
	//   Greek, Hebrew, Italian, Portuguese, Spanish, Esperanto
	case 0:
	default:
		return n != 1;

	// Only one form
	// Used in:
	//   Hungarian, Japanese, Korean, Turkish
	case 1:
		return 0;

	// Two forms, singular used for zero and one
	// Used in:
	//   French, Brazilian Portuguese
	case 2:
		return n > 1;

	// Three forms, special case for zero
	// Used in:
	//   Latvian
	case 3:
		return n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2;

	// Three forms, special case for one and two
	// Used in:
	//   Gaelige (Irish)
	case 4:
		return n==1 ? 0 : n==2 ? 1 : 2;

	// Three forms, special case for numbers ending in 1[2-9]
	// Used in:
	//   Lithuanian
	case 5:
		return n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2;

	// Three forms, special cases for numbers ending in 1 and 2, 3, 4, except those ending in 1[1-4]
	// Used in:
	//   Croatian, Czech, Russian, Slovak, Ukrainian
	case 6:
		return n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2;

	// Three forms, special case for one and some numbers ending in 2, 3, or 4
	// Used in:
	//   Polish
	case 7:
		return n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2;

	// Four forms, special case for one and all numbers ending in 02, 03, or 04
	// Used in:
	//   Slovenian
	case 8:
		return n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3;
	}
}

static const char *ParseStringChoice(const char *b, uint form, char *dst, int *dstlen)
{
	//<NUM> {Length of each string} {each string}
	uint n = (byte)*b++;
	uint pos,i, mylen=0,mypos=0;

	for (i = pos = 0; i != n; i++) {
		uint len = (byte)*b++;
		if (i == form) {
			mypos = pos;
			mylen = len;
		}
		pos += len;
	}
	*dstlen = mylen;
	memcpy(dst, b + mypos, mylen);
	return b + pos;
}

typedef struct Units {
	int s_m;           ///< Multiplier for velocity
	int s_s;           ///< Shift for velocity
	StringID velocity; ///< String for velocity
	int p_m;           ///< Multiplier for power
	int p_s;           ///< Shift for power
	StringID power;    ///< String for velocity
	int w_m;           ///< Multiplier for weight
	int w_s;           ///< Shift for weight
	StringID s_weight; ///< Short string for weight
	StringID l_weight; ///< Long string for weight
	int v_m;           ///< Multiplier for volume
	int v_s;           ///< Shift for volume
	StringID s_volume; ///< Short string for volume
	StringID l_volume; ///< Long string for volume
	int f_m;           ///< Multiplier for force
	int f_s;           ///< Shift for force
	StringID force;    ///< String for force
} Units;

/* Unit conversions */
static const Units units[] = {
	{ // Imperial (Original, mph, hp, metric ton, litre, metric ton force)
		  10,  4, STR_UNITS_VELOCITY_IMPERIAL,
		   1,  0, STR_UNITS_POWER_IMPERIAL,
		   1,  0, STR_UNITS_WEIGHT_SHORT_METRIC, STR_UNITS_WEIGHT_LONG_METRIC,
		1000,  0, STR_UNITS_VOLUME_SHORT_METRIC, STR_UNITS_VOLUME_LONG_METRIC,
		 835, 13, STR_UNITS_FORCE_METRIC,
	},
	{ // Metric (km/h, hp, metric ton, litre, metric ton force)
		   1,  0, STR_UNITS_VELOCITY_METRIC,
		   1,  0, STR_UNITS_POWER_METRIC,
		   1,  0, STR_UNITS_WEIGHT_SHORT_METRIC, STR_UNITS_WEIGHT_LONG_METRIC,
		1000,  0, STR_UNITS_VOLUME_SHORT_METRIC, STR_UNITS_VOLUME_LONG_METRIC,
		 835, 13, STR_UNITS_FORCE_METRIC,
	},
	{ // SI (m/s, kilowatt, kilogram, cubic metres, kilonewton)
		 284, 10, STR_UNITS_VELOCITY_SI,
		 764, 10, STR_UNITS_POWER_SI,
		1000,  0, STR_UNITS_WEIGHT_SHORT_SI, STR_UNITS_WEIGHT_LONG_SI,
		   1,  0, STR_UNITS_VOLUME_SHORT_SI, STR_UNITS_VOLUME_LONG_SI,
		   1,  0, STR_UNITS_FORCE_SI,
	},
};

static char* FormatString(char* buff, const char* str, const int32* argv, uint casei, const char* last)
{
	extern const char _openttd_revision[];
	WChar b;
	const int32 *argv_orig = argv;
	uint modifier = 0;

	while ((b = Utf8Consume(&str)) != '\0') {
		switch (b) {
			case SCC_SETX: // {SETX}
				if (buff + Utf8CharLen(SCC_SETX) + 1 < last) {
					buff += Utf8Encode(buff, SCC_SETX);
					*buff++ = *str++;
				}
				break;

			case SCC_SETXY: // {SETXY}
				if (buff + Utf8CharLen(SCC_SETXY) + 2 < last) {
					buff += Utf8Encode(buff, SCC_SETXY);
					*buff++ = *str++;
					*buff++ = *str++;
				}
				break;

			case SCC_STRING_ID: // {STRINL}
				buff = GetStringWithArgs(buff, Utf8Consume(&str), argv, last);
				break;

			case SCC_DATE_LONG: // {DATE_LONG}
				buff = FormatYmdString(buff, GetInt32(&argv), last);
				break;

			case SCC_DATE_SHORT: // {DATE_SHORT}
				buff = FormatMonthAndYear(buff, GetInt32(&argv), last);
				break;

			case SCC_VELOCITY: {// {VELOCITY}
				int32 args[1];
				assert(_opt_ptr->units < lengthof(units));
				args[0] = GetInt32(&argv) * units[_opt_ptr->units].s_m >> units[_opt_ptr->units].s_s;
				buff = FormatString(buff, GetStringPtr(units[_opt_ptr->units].velocity), args, modifier >> 24, last);
				modifier = 0;
				break;
			}

			case SCC_CURRENCY_COMPACT: /* {CURRCOMPACT} */
				buff = FormatGenericCurrency(buff, _currency, GetInt32(&argv), true, last);
				break;

			case SCC_REVISION: /* {REV} */
				buff = strecpy(buff, _openttd_revision, last);
				break;

			case SCC_CARGO_SHORT: { /* {SHORTCARGO} */
				// Short description of cargotypes. Layout:
				// 8-bit = cargo type
				// 16-bit = cargo count
				StringID cargo_str = _cargo_types_base_values[_opt_ptr->landscape].units_volume[GetInt32(&argv)];
				switch (cargo_str) {
					case STR_TONS: {
						int32 args[1];
						assert(_opt_ptr->units < lengthof(units));
						args[0] = GetInt32(&argv) * units[_opt_ptr->units].w_m >> units[_opt_ptr->units].w_s;
						buff = FormatString(buff, GetStringPtr(units[_opt_ptr->units].l_weight), args, modifier >> 24, last);
						modifier = 0;
						break;
					}

					case STR_LITERS: {
						int32 args[1];
						assert(_opt_ptr->units < lengthof(units));
						args[0] = GetInt32(&argv) * units[_opt_ptr->units].v_m >> units[_opt_ptr->units].v_s;
						buff = FormatString(buff, GetStringPtr(units[_opt_ptr->units].l_volume), args, modifier >> 24, last);
						modifier = 0;
						break;
					}

					default:
						buff = FormatCommaNumber(buff, GetInt32(&argv), last);
						buff = strecpy(buff, " ", last);
						buff = strecpy(buff, GetStringPtr(cargo_str), last);
						break;
				}
			} break;

			case SCC_CURRENCY_COMPACT_64: { /* {CURRCOMPACT64} */
				// 64 bit compact currency-unit
				buff = FormatGenericCurrency(buff, _currency, GetInt64(&argv), true, last);
				break;
			}

			case SCC_STRING1: { /* {STRING1} */
				// String that consumes ONE argument
				uint str = modifier + GetInt32(&argv);
				buff = GetStringWithArgs(buff, str, GetArgvPtr(&argv, 1), last);
				modifier = 0;
				break;
			}

			case SCC_STRING2: { /* {STRING2} */
				// String that consumes TWO arguments
				uint str = modifier + GetInt32(&argv);
				buff = GetStringWithArgs(buff, str, GetArgvPtr(&argv, 2), last);
				modifier = 0;
				break;
			}

			case SCC_STRING3: { /* {STRING3} */
				// String that consumes THREE arguments
				uint str = modifier + GetInt32(&argv);
				buff = GetStringWithArgs(buff, str, GetArgvPtr(&argv, 3), last);
				modifier = 0;
				break;
			}

			case SCC_STRING4: { /* {STRING4} */
				// String that consumes FOUR arguments
				uint str = modifier + GetInt32(&argv);
				buff = GetStringWithArgs(buff, str, GetArgvPtr(&argv, 4), last);
				modifier = 0;
				break;
			}

			case SCC_STRING5: { /* {STRING5} */
				// String that consumes FIVE arguments
				uint str = modifier + GetInt32(&argv);
				buff = GetStringWithArgs(buff, str, GetArgvPtr(&argv, 5), last);
				modifier = 0;
				break;
			}

			case SCC_STATION_FEATURES: { /* {STATIONFEATURES} */
				buff = StationGetSpecialString(buff, GetInt32(&argv), last);
				break;
			}

			case SCC_INDUSTRY_NAME: { /* {INDUSTRY} */
				const Industry* i = GetIndustry(GetInt32(&argv));
				int32 args[2];

				// industry not valid anymore?
				if (!IsValidIndustry(i)) break;

				// First print the town name and the industry type name
				// The string STR_INDUSTRY_PATTERN controls the formatting
				args[0] = i->town->index;
				args[1] = GetIndustrySpec(i->type)->name;
				buff = FormatString(buff, GetStringPtr(STR_INDUSTRY_FORMAT), args, modifier >> 24, last);
				modifier = 0;
				break;
			}

			case SCC_VOLUME: { // {VOLUME}
				int32 args[1];
				assert(_opt_ptr->units < lengthof(units));
				args[0] = GetInt32(&argv) * units[_opt_ptr->units].v_m >> units[_opt_ptr->units].v_s;
				buff = FormatString(buff, GetStringPtr(units[_opt_ptr->units].l_volume), args, modifier >> 24, last);
				modifier = 0;
				break;
			}

			case SCC_GENDER_LIST: { // {G 0 Der Die Das}
				const char* s = GetStringPtr(argv_orig[(byte)*str++]); // contains the string that determines gender.
				int len;
				int gender = 0;
				if (s != NULL && Utf8Consume(&s) == SCC_GENDER_INDEX) gender = (byte)s[0];
				str = ParseStringChoice(str, gender, buff, &len);
				buff += len;
				break;
			}

			case SCC_DATE_TINY: { // {DATE_TINY}
				buff = FormatTinyDate(buff, GetInt32(&argv), last);
				break;
			}

			case SCC_CARGO: { // {CARGO}
				// Layout now is:
				//   8bit   - cargo type
				//   16-bit - cargo count
				CargoID cargo = GetInt32(&argv);
				StringID cargo_str = (cargo == CT_INVALID) ? STR_8838_N_A : _cargoc.names_long[cargo];
				buff = GetStringWithArgs(buff, cargo_str, argv++, last);
				break;
			}

			case SCC_POWER: { // {POWER}
				int32 args[1];
				assert(_opt_ptr->units < lengthof(units));
				args[0] = GetInt32(&argv) * units[_opt_ptr->units].p_m >> units[_opt_ptr->units].p_s;
				buff = FormatString(buff, GetStringPtr(units[_opt_ptr->units].power), args, modifier >> 24, last);
				modifier = 0;
				break;
			}

			case SCC_VOLUME_SHORT: { // {VOLUME_S}
				int32 args[1];
				assert(_opt_ptr->units < lengthof(units));
				args[0] = GetInt32(&argv) * units[_opt_ptr->units].v_m >> units[_opt_ptr->units].v_s;
				buff = FormatString(buff, GetStringPtr(units[_opt_ptr->units].s_volume), args, modifier >> 24, last);
				modifier = 0;
				break;
			}

			case SCC_WEIGHT: { // {WEIGHT}
				int32 args[1];
				assert(_opt_ptr->units < lengthof(units));
				args[0] = GetInt32(&argv) * units[_opt_ptr->units].w_m >> units[_opt_ptr->units].w_s;
				buff = FormatString(buff, GetStringPtr(units[_opt_ptr->units].l_weight), args, modifier >> 24, last);
				modifier = 0;
				break;
			}

			case SCC_WEIGHT_SHORT: { // {WEIGHT_S}
				int32 args[1];
				assert(_opt_ptr->units < lengthof(units));
				args[0] = GetInt32(&argv) * units[_opt_ptr->units].w_m >> units[_opt_ptr->units].w_s;
				buff = FormatString(buff, GetStringPtr(units[_opt_ptr->units].s_weight), args, modifier >> 24, last);
				modifier = 0;
				break;
			}

			case SCC_FORCE: { // {FORCE}
				int32 args[1];
				assert(_opt_ptr->units < lengthof(units));
				args[0] = GetInt32(&argv) * units[_opt_ptr->units].f_m >> units[_opt_ptr->units].f_s;
				buff = FormatString(buff, GetStringPtr(units[_opt_ptr->units].force), args, modifier >> 24, last);
				modifier = 0;
				break;
			}

			case SCC_SKIP: // {SKIP}
				argv++;
				break;

			// This sets up the gender for the string.
			// We just ignore this one. It's used in {G 0 Der Die Das} to determine the case.
			case SCC_GENDER_INDEX: // {GENDER 0}
				str++;
				break;

			case SCC_STRING: {// {STRING}
				uint str = modifier + GetInt32(&argv);
				// WARNING. It's prohibited for the included string to consume any arguments.
				// For included strings that consume argument, you should use STRING1, STRING2 etc.
				// To debug stuff you can set argv to NULL and it will tell you
				buff = GetStringWithArgs(buff, str, argv, last);
				modifier = 0;
				break;
			}

			case SCC_COMMA: // {COMMA}
				buff = FormatCommaNumber(buff, GetInt32(&argv), last);
				break;

			case SCC_ARG_INDEX: // Move argument pointer
				argv = argv_orig + (byte)*str++;
				break;

			case SCC_PLURAL_LIST: { // {P}
				int32 v = argv_orig[(byte)*str++]; // contains the number that determines plural
				int len;
				str = ParseStringChoice(str, DeterminePluralForm(v), buff, &len);
				buff += len;
				break;
			}

			case SCC_NUM: // {NUM}
				buff = FormatNoCommaNumber(buff, GetInt32(&argv), last);
				break;

			case SCC_CURRENCY: // {CURRENCY}
				buff = FormatGenericCurrency(buff, _currency, GetInt32(&argv), false, last);
				break;

			case SCC_WAYPOINT_NAME: { // {WAYPOINT}
				int32 temp[2];
				Waypoint *wp = GetWaypoint(GetInt32(&argv));
				StringID str;
				if (wp->string != STR_NULL) {
					str = wp->string;
				} else {
					temp[0] = wp->town_index;
					temp[1] = wp->town_cn + 1;
					str = wp->town_cn == 0 ? STR_WAYPOINTNAME_CITY : STR_WAYPOINTNAME_CITY_SERIAL;
				}
				buff = GetStringWithArgs(buff, str, temp, last);
				break;
			}

			case SCC_STATION_NAME: { // {STATION}
				const Station* st = GetStation(GetInt32(&argv));

				if (!IsValidStation(st)) { // station doesn't exist anymore
					buff = GetStringWithArgs(buff, STR_UNKNOWN_DESTINATION, NULL, last);
				} else {
					int32 temp[2];
					temp[0] = st->town->townnametype;
					temp[1] = st->town->townnameparts;
					buff = GetStringWithArgs(buff, st->string_id, temp, last);
				}
				break;
			}

			case SCC_TOWN_NAME: { // {TOWN}
				const Town* t = GetTown(GetInt32(&argv));
				int32 temp[1];

				assert(IsValidTown(t));

				temp[0] = t->townnameparts;
				buff = GetStringWithArgs(buff, t->townnametype, temp, last);
				break;
			}

			case SCC_CURRENCY_64: { // {CURRENCY64}
				buff = FormatGenericCurrency(buff, _currency, GetInt64(&argv), false, last);
				break;
			}

			case SCC_SETCASE: { // {SETCASE}
				// This is a pseudo command, it's outputted when someone does {STRING.ack}
				// The modifier is added to all subsequent GetStringWithArgs that accept the modifier.
				modifier = (byte)*str++ << 24;
				break;
			}

			case SCC_SWITCH_CASE: { // {Used to implement case switching}
				// <0x9E> <NUM CASES> <CASE1> <LEN1> <STRING1> <CASE2> <LEN2> <STRING2> <CASE3> <LEN3> <STRING3> <STRINGDEFAULT>
				// Each LEN is printed using 2 bytes in big endian order.
				uint num = (byte)*str++;
				while (num) {
					if ((byte)str[0] == casei) {
						// Found the case, adjust str pointer and continue
						str += 3;
						break;
					}
					// Otherwise skip to the next case
					str += 3 + (str[1] << 8) + str[2];
					num--;
				}
				break;
			}

			default:
				if (buff + Utf8CharLen(b) < last) buff += Utf8Encode(buff, b);
				break;
		}
	}
	*buff = '\0';
	return buff;
}


static char *StationGetSpecialString(char *buff, int x, const char* last)
{
	if ((x & 0x01) && (buff + Utf8CharLen(SCC_TRAIN) < last)) buff += Utf8Encode(buff, SCC_TRAIN);
	if ((x & 0x02) && (buff + Utf8CharLen(SCC_LORRY) < last)) buff += Utf8Encode(buff, SCC_LORRY);
	if ((x & 0x04) && (buff + Utf8CharLen(SCC_BUS)   < last)) buff += Utf8Encode(buff, SCC_BUS);
	if ((x & 0x08) && (buff + Utf8CharLen(SCC_PLANE) < last)) buff += Utf8Encode(buff, SCC_PLANE);
	if ((x & 0x10) && (buff + Utf8CharLen(SCC_SHIP)  < last)) buff += Utf8Encode(buff, SCC_SHIP);
	*buff = '\0';
	return buff;
}

static char *GetSpecialTownNameString(char *buff, int ind, uint32 seed, const char* last)
{
	char name[512];

	_town_name_generators[ind](name, seed, lastof(name));
	return strecpy(buff, name, last);
}

static const char* const _silly_company_names[] = {
	"Bloggs Brothers",
	"Tiny Transport Ltd.",
	"Express Travel",
	"Comfy-Coach & Co.",
	"Crush & Bump Ltd.",
	"Broken & Late Ltd.",
	"Sam Speedy & Son",
	"Supersonic Travel",
	"Mike's Motors",
	"Lightning International",
	"Pannik & Loozit Ltd.",
	"Inter-City Transport",
	"Getout & Pushit Ltd."
};

static const char* const _surname_list[] = {
	"Adams",
	"Allan",
	"Baker",
	"Bigwig",
	"Black",
	"Bloggs",
	"Brown",
	"Campbell",
	"Gordon",
	"Hamilton",
	"Hawthorn",
	"Higgins",
	"Green",
	"Gribble",
	"Jones",
	"McAlpine",
	"MacDonald",
	"McIntosh",
	"Muir",
	"Murphy",
	"Nelson",
	"O'Donnell",
	"Parker",
	"Phillips",
	"Pilkington",
	"Quigley",
	"Sharkey",
	"Thomson",
	"Watkins"
};

static const char* const _silly_surname_list[] = {
	"Grumpy",
	"Dozy",
	"Speedy",
	"Nosey",
	"Dribble",
	"Mushroom",
	"Cabbage",
	"Sniffle",
	"Fishy",
	"Swindle",
	"Sneaky",
	"Nutkins"
};

static const char _initial_name_letters[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'P', 'R', 'S', 'T', 'W',
};

static char *GenAndCoName(char *buff, uint32 arg, const char* last)
{
	const char* const* base;
	uint num;

	if (_opt_ptr->landscape == LT_CANDY) {
		base = _silly_surname_list;
		num  = lengthof(_silly_surname_list);
	} else {
		base = _surname_list;
		num  = lengthof(_surname_list);
	}

	buff = strecpy(buff, base[num * GB(arg, 16, 8) >> 8], last);
	buff = strecpy(buff, " & Co.", last);

	return buff;
}

static char *GenPresidentName(char *buff, uint32 x, const char* last)
{
	char initial[] = "?. ";
	const char* const* base;
	uint num;
	uint i;

	initial[0] = _initial_name_letters[sizeof(_initial_name_letters) * GB(x, 0, 8) >> 8];
	buff = strecpy(buff, initial, last);

	i = (sizeof(_initial_name_letters) + 35) * GB(x, 8, 8) >> 8;
	if (i < sizeof(_initial_name_letters)) {
		initial[0] = _initial_name_letters[i];
		buff = strecpy(buff, initial, last);
	}

	if (_opt_ptr->landscape == LT_CANDY) {
		base = _silly_surname_list;
		num  = lengthof(_silly_surname_list);
	} else {
		base = _surname_list;
		num  = lengthof(_surname_list);
	}

	buff = strecpy(buff, base[num * GB(x, 16, 8) >> 8], last);

	return buff;
}

static char *GetSpecialPlayerNameString(char *buff, int ind, const int32 *argv, const char* last)
{
	switch (ind) {
		case 1: // not used
			return strecpy(buff, _silly_company_names[GetInt32(&argv) & 0xFFFF], last);

		case 2: // used for Foobar & Co company names
			return GenAndCoName(buff, GetInt32(&argv), last);

		case 3: // President name
			return GenPresidentName(buff, GetInt32(&argv), last);

		case 4: // song names
			return strecpy(buff, origin_songs_specs[GetInt32(&argv) - 1].song_name, last);
	}

	// town name?
	if (IS_INT_INSIDE(ind - 6, 0, SPECSTR_TOWNNAME_LAST-SPECSTR_TOWNNAME_START + 1)) {
		buff = GetSpecialTownNameString(buff, ind - 6, GetInt32(&argv), last);
		return strecpy(buff, " Transport", last);
	}

	// language name?
	if (IS_INT_INSIDE(ind, (SPECSTR_LANGUAGE_START - 0x70E4), (SPECSTR_LANGUAGE_END - 0x70E4) + 1)) {
		int i = ind - (SPECSTR_LANGUAGE_START - 0x70E4);
		return strecpy(buff,
			i == _dynlang.curr ? _langpack->own_name : _dynlang.ent[i].name, last);
	}

	// resolution size?
	if (IS_INT_INSIDE(ind, (SPECSTR_RESOLUTION_START - 0x70E4), (SPECSTR_RESOLUTION_END - 0x70E4) + 1)) {
		int i = ind - (SPECSTR_RESOLUTION_START - 0x70E4);
		buff += snprintf(
			buff, last - buff + 1, "%dx%d", _resolutions[i][0], _resolutions[i][1]
		);
		return buff;
	}

	// screenshot format name?
	if (IS_INT_INSIDE(ind, (SPECSTR_SCREENSHOT_START - 0x70E4), (SPECSTR_SCREENSHOT_END - 0x70E4) + 1)) {
		int i = ind - (SPECSTR_SCREENSHOT_START - 0x70E4);
		return strecpy(buff, GetScreenshotFormatDesc(i), last);
	}

	assert(0);
	return NULL;
}

// remap a string ID from the old format to the new format
StringID RemapOldStringID(StringID s)
{
	switch (s) {
		case 0x0006: return STR_SV_EMPTY;
		case 0x7000: return STR_SV_UNNAMED;
		case 0x70E4: return SPECSTR_PLAYERNAME_ENGLISH;
		case 0x70E9: return SPECSTR_PLAYERNAME_ENGLISH;
		case 0x8864: return STR_SV_TRAIN_NAME;
		case 0x902B: return STR_SV_ROADVEH_NAME;
		case 0x9830: return STR_SV_SHIP_NAME;
		case 0xA02F: return STR_SV_AIRCRAFT_NAME;

		default:
			if (IS_INT_INSIDE(s, 0x300F, 0x3030)) {
				return s - 0x300F + STR_SV_STNAME;
			} else {
				return s;
			}
	}
}

bool ReadLanguagePack(int lang_index)
{
	int tot_count, i;
	LanguagePack *lang_pack;
	size_t len;
	char **langpack_offs;
	char *s;

	{
		char *lang = str_fmt("%s%s", _paths.lang_dir, _dynlang.ent[lang_index].file);
		lang_pack = ReadFileToMem(lang, &len, 200000);
		free(lang);
	}
	if (lang_pack == NULL) return false;
	if (len < sizeof(LanguagePack) ||
			lang_pack->ident != TO_LE32(LANGUAGE_PACK_IDENT) ||
			lang_pack->version != TO_LE32(LANGUAGE_PACK_VERSION)) {
		free(lang_pack);
		return false;
	}

#if defined(TTD_BIG_ENDIAN)
	for (i = 0; i != 32; i++) {
		lang_pack->offsets[i] = ReadLE16Aligned(&lang_pack->offsets[i]);
	}
#endif

	tot_count = 0;
	for (i = 0; i != 32; i++) {
		uint num = lang_pack->offsets[i];
		_langtab_start[i] = tot_count;
		_langtab_num[i] = num;
		tot_count += num;
	}

	// Allocate offsets
	langpack_offs = malloc(tot_count * sizeof(*langpack_offs));

	// Fill offsets
	s = lang_pack->data;
	for (i = 0; i != tot_count; i++) {
		len = (byte)*s;
		*s++ = '\0'; // zero terminate the string before.
		if (len >= 0xC0) len = ((len & 0x3F) << 8) + (byte)*s++;
		langpack_offs[i] = s;
		s += len;
	}

	free(_langpack);
	_langpack = lang_pack;

	free(_langpack_offs);
	_langpack_offs = langpack_offs;

	ttd_strlcpy(_dynlang.curr_file, _dynlang.ent[lang_index].file, sizeof(_dynlang.curr_file));

	_dynlang.curr = lang_index;
	SetCurrentGrfLangID(_langpack->isocode);
	return true;
}

/** Determine the current charset based on the environment
 * First check some default values, after this one we passed ourselves
 * and if none exist return the value for $LANG
 * @param environment variable to check conditionally if default ones are not
 *        set. Pass NULL if you don't want additional checks.
 * @return return string containing current charset, or NULL if not-determinable */
const char *GetCurrentLocale(const char *param)
{
	const char *env;

	env = getenv("LANGUAGE");
	if (env != NULL) return env;

	env = getenv("LC_ALL");
	if (env != NULL) return env;

	if (param != NULL) {
		env = getenv(param);
		if (env != NULL) return env;
	}

	return getenv("LANG");
}

static int CDECL LanguageCompareFunc(const void *a, const void *b)
{
	return strcmp(*(const char* const *)a, *(const char* const *)b);
}

static int GetLanguageList(char **languages, int max)
{
	DIR *dir;
	struct dirent *dirent;
	int num = 0;

	dir = ttd_opendir(_paths.lang_dir);
	if (dir != NULL) {
		while ((dirent = readdir(dir)) != NULL) {
			const char *d_name = FS2OTTD(dirent->d_name);
			char *t = strrchr(d_name, '.');

			if (t != NULL && strcasecmp(t, ".lng") == 0) {
				languages[num++] = strdup(d_name);
				if (num == max) break;
			}
		}
		closedir(dir);
	}

	qsort(languages, num, sizeof(char*), LanguageCompareFunc);
	return num;
}

// make a list of the available language packs. put the data in _dynlang struct.
void InitializeLanguagePacks(void)
{
	DynamicLanguages *dl = &_dynlang;
	int i;
	int n;
	int m;
	int def;
	int def2;
	int fallback;
	LanguagePack hdr;
	FILE *in;
	char *files[MAX_LANG];
	const char* lang;

	lang = GetCurrentLocale("LC_MESSAGES");
	if (lang == NULL) lang = "en_GB";

	n = GetLanguageList(files, lengthof(files));

	def = -1;
	def2 = -1;
	fallback = 0;

	// go through the language files and make sure that they are valid.
	for (i = m = 0; i != n; i++) {
		size_t j;

		char *s = str_fmt("%s%s", _paths.lang_dir, files[i]);
		in = fopen(s, "rb");
		free(s);
		if (in == NULL ||
				(j = fread(&hdr, sizeof(hdr), 1, in), fclose(in), j) != 1 ||
				hdr.ident != TO_LE32(LANGUAGE_PACK_IDENT) ||
				hdr.version != TO_LE32(LANGUAGE_PACK_VERSION)) {
			free(files[i]);
			continue;
		}

		dl->ent[m].file = files[i];
		dl->ent[m].name = strdup(hdr.name);

		if (strcmp(hdr.isocode, "en_GB")  == 0) fallback = m;
		if (strncmp(hdr.isocode, lang, 2) == 0) def2 = m;
		if (strncmp(hdr.isocode, lang, 5) == 0) def = m;

		m++;
	}
	if (def == -1) def = (def2 != -1 ? def2 : fallback);

	if (m == 0)
		error(n == 0 ? "No available language packs" : "Invalid version of language packs");

	dl->num = m;
	for (i = 0; i != dl->num; i++) dl->dropdown[i] = SPECSTR_LANGUAGE_START + i;
	dl->dropdown[i] = INVALID_STRING_ID;

	for (i = 0; i != dl->num; i++)
		if (strcmp(dl->ent[i].file, dl->curr_file) == 0) {
			def = i;
			break;
		}

	if (!ReadLanguagePack(def))
		error("can't read language pack '%s'", dl->ent[def].file);
}
