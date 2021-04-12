/* $Id: currency.h 9471 2007-03-26 00:20:22Z belugas $ */

#ifndef CURRENCY_H
#define CURRENCY_H

enum {
	CF_NOEURO = 0,
	CF_ISEURO = 1,
	NUM_CURRENCY = 28,
	CUSTOM_CURRENCY_ID = NUM_CURRENCY - 1
};

typedef struct {
	uint16 rate;
	char separator;
	Year to_euro;
	char prefix[16];
	char suffix[16];
	/**
	 * The currency symbol is represented by two possible values, prefix and suffix
	 * Usage of one or the other is determined by symbol_pos.
	 * 0 = prefix
	 * 1 = suffix
	 * 2 = both : Special case only for custom currency.
	 *            It is not a spec from Newgrf,
	 *            rather a way to let users do what they want with custom curency
	 */
	byte symbol_pos;
	StringID name;
} CurrencySpec;


extern CurrencySpec _currency_specs[NUM_CURRENCY];

// XXX small hack, but makes the rest of the code a bit nicer to read
#define _custom_currency (_currency_specs[CUSTOM_CURRENCY_ID])
#define _currency ((const CurrencySpec*)&_currency_specs[_opt_ptr->currency])

uint GetMaskOfAllowedCurrencies(void);
void CheckSwitchToEuro(void);
void ResetCurrencies(bool preserve_custom);
StringID* BuildCurrencyDropdown(void);
byte GetNewgrfCurrencyIdConverted(byte grfcurr_id);

#endif /* CURRENCY_H */
