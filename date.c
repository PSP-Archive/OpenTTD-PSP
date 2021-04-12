/* $Id: date.c 7948 2007-01-07 12:06:34Z Darkvater $ */

#include "stdafx.h"
#include "openttd.h"
#include "date.h"
#include "variables.h"
#include "macros.h"
#include "vehicle.h"
#include "network.h"
#include "network_data.h"
#include "network_server.h"
#include "functions.h"
#include "currency.h"

Year      _cur_year;
Month     _cur_month;
Date      _date;
DateFract _date_fract;


void SetDate(Date date)
{
	YearMonthDay ymd;

	_date = date;
	ConvertDateToYMD(date, &ymd);
	_cur_year = ymd.year;
	_cur_month = ymd.month;
#ifdef ENABLE_NETWORK
	_network_last_advertise_frame = 0;
	_network_need_advertise = true;
#endif /* ENABLE_NETWORK */
}

#define M(a, b) ((a << 5) | b)
static const uint16 _month_date_from_year_day[] = {
	M( 0, 1), M( 0, 2), M( 0, 3), M( 0, 4), M( 0, 5), M( 0, 6), M( 0, 7), M( 0, 8), M( 0, 9), M( 0, 10), M( 0, 11), M( 0, 12), M( 0, 13), M( 0, 14), M( 0, 15), M( 0, 16), M( 0, 17), M( 0, 18), M( 0, 19), M( 0, 20), M( 0, 21), M( 0, 22), M( 0, 23), M( 0, 24), M( 0, 25), M( 0, 26), M( 0, 27), M( 0, 28), M( 0, 29), M( 0, 30), M( 0, 31),
	M( 1, 1), M( 1, 2), M( 1, 3), M( 1, 4), M( 1, 5), M( 1, 6), M( 1, 7), M( 1, 8), M( 1, 9), M( 1, 10), M( 1, 11), M( 1, 12), M( 1, 13), M( 1, 14), M( 1, 15), M( 1, 16), M( 1, 17), M( 1, 18), M( 1, 19), M( 1, 20), M( 1, 21), M( 1, 22), M( 1, 23), M( 1, 24), M( 1, 25), M( 1, 26), M( 1, 27), M( 1, 28), M( 1, 29),
	M( 2, 1), M( 2, 2), M( 2, 3), M( 2, 4), M( 2, 5), M( 2, 6), M( 2, 7), M( 2, 8), M( 2, 9), M( 2, 10), M( 2, 11), M( 2, 12), M( 2, 13), M( 2, 14), M( 2, 15), M( 2, 16), M( 2, 17), M( 2, 18), M( 2, 19), M( 2, 20), M( 2, 21), M( 2, 22), M( 2, 23), M( 2, 24), M( 2, 25), M( 2, 26), M( 2, 27), M( 2, 28), M( 2, 29), M( 2, 30), M( 2, 31),
	M( 3, 1), M( 3, 2), M( 3, 3), M( 3, 4), M( 3, 5), M( 3, 6), M( 3, 7), M( 3, 8), M( 3, 9), M( 3, 10), M( 3, 11), M( 3, 12), M( 3, 13), M( 3, 14), M( 3, 15), M( 3, 16), M( 3, 17), M( 3, 18), M( 3, 19), M( 3, 20), M( 3, 21), M( 3, 22), M( 3, 23), M( 3, 24), M( 3, 25), M( 3, 26), M( 3, 27), M( 3, 28), M( 3, 29), M( 3, 30),
	M( 4, 1), M( 4, 2), M( 4, 3), M( 4, 4), M( 4, 5), M( 4, 6), M( 4, 7), M( 4, 8), M( 4, 9), M( 4, 10), M( 4, 11), M( 4, 12), M( 4, 13), M( 4, 14), M( 4, 15), M( 4, 16), M( 4, 17), M( 4, 18), M( 4, 19), M( 4, 20), M( 4, 21), M( 4, 22), M( 4, 23), M( 4, 24), M( 4, 25), M( 4, 26), M( 4, 27), M( 4, 28), M( 4, 29), M( 4, 30), M( 4, 31),
	M( 5, 1), M( 5, 2), M( 5, 3), M( 5, 4), M( 5, 5), M( 5, 6), M( 5, 7), M( 5, 8), M( 5, 9), M( 5, 10), M( 5, 11), M( 5, 12), M( 5, 13), M( 5, 14), M( 5, 15), M( 5, 16), M( 5, 17), M( 5, 18), M( 5, 19), M( 5, 20), M( 5, 21), M( 5, 22), M( 5, 23), M( 5, 24), M( 5, 25), M( 5, 26), M( 5, 27), M( 5, 28), M( 5, 29), M( 5, 30),
	M( 6, 1), M( 6, 2), M( 6, 3), M( 6, 4), M( 6, 5), M( 6, 6), M( 6, 7), M( 6, 8), M( 6, 9), M( 6, 10), M( 6, 11), M( 6, 12), M( 6, 13), M( 6, 14), M( 6, 15), M( 6, 16), M( 6, 17), M( 6, 18), M( 6, 19), M( 6, 20), M( 6, 21), M( 6, 22), M( 6, 23), M( 6, 24), M( 6, 25), M( 6, 26), M( 6, 27), M( 6, 28), M( 6, 29), M( 6, 30), M( 6, 31),
	M( 7, 1), M( 7, 2), M( 7, 3), M( 7, 4), M( 7, 5), M( 7, 6), M( 7, 7), M( 7, 8), M( 7, 9), M( 7, 10), M( 7, 11), M( 7, 12), M( 7, 13), M( 7, 14), M( 7, 15), M( 7, 16), M( 7, 17), M( 7, 18), M( 7, 19), M( 7, 20), M( 7, 21), M( 7, 22), M( 7, 23), M( 7, 24), M( 7, 25), M( 7, 26), M( 7, 27), M( 7, 28), M( 7, 29), M( 7, 30), M( 7, 31),
	M( 8, 1), M( 8, 2), M( 8, 3), M( 8, 4), M( 8, 5), M( 8, 6), M( 8, 7), M( 8, 8), M( 8, 9), M( 8, 10), M( 8, 11), M( 8, 12), M( 8, 13), M( 8, 14), M( 8, 15), M( 8, 16), M( 8, 17), M( 8, 18), M( 8, 19), M( 8, 20), M( 8, 21), M( 8, 22), M( 8, 23), M( 8, 24), M( 8, 25), M( 8, 26), M( 8, 27), M( 8, 28), M( 8, 29), M( 8, 30),
	M( 9, 1), M( 9, 2), M( 9, 3), M( 9, 4), M( 9, 5), M( 9, 6), M( 9, 7), M( 9, 8), M( 9, 9), M( 9, 10), M( 9, 11), M( 9, 12), M( 9, 13), M( 9, 14), M( 9, 15), M( 9, 16), M( 9, 17), M( 9, 18), M( 9, 19), M( 9, 20), M( 9, 21), M( 9, 22), M( 9, 23), M( 9, 24), M( 9, 25), M( 9, 26), M( 9, 27), M( 9, 28), M( 9, 29), M( 9, 30), M( 9, 31),
	M(10, 1), M(10, 2), M(10, 3), M(10, 4), M(10, 5), M(10, 6), M(10, 7), M(10, 8), M(10, 9), M(10, 10), M(10, 11), M(10, 12), M(10, 13), M(10, 14), M(10, 15), M(10, 16), M(10, 17), M(10, 18), M(10, 19), M(10, 20), M(10, 21), M(10, 22), M(10, 23), M(10, 24), M(10, 25), M(10, 26), M(10, 27), M(10, 28), M(10, 29), M(10, 30),
	M(11, 1), M(11, 2), M(11, 3), M(11, 4), M(11, 5), M(11, 6), M(11, 7), M(11, 8), M(11, 9), M(11, 10), M(11, 11), M(11, 12), M(11, 13), M(11, 14), M(11, 15), M(11, 16), M(11, 17), M(11, 18), M(11, 19), M(11, 20), M(11, 21), M(11, 22), M(11, 23), M(11, 24), M(11, 25), M(11, 26), M(11, 27), M(11, 28), M(11, 29), M(11, 30), M(11, 31),
};
#undef M

enum {
	ACCUM_JAN = 0,
	ACCUM_FEB = ACCUM_JAN + 31,
	ACCUM_MAR = ACCUM_FEB + 29,
	ACCUM_APR = ACCUM_MAR + 31,
	ACCUM_MAY = ACCUM_APR + 30,
	ACCUM_JUN = ACCUM_MAY + 31,
	ACCUM_JUL = ACCUM_JUN + 30,
	ACCUM_AUG = ACCUM_JUL + 31,
	ACCUM_SEP = ACCUM_AUG + 31,
	ACCUM_OCT = ACCUM_SEP + 30,
	ACCUM_NOV = ACCUM_OCT + 31,
	ACCUM_DEC = ACCUM_NOV + 30,
};

static const uint16 _accum_days_for_month[] = {
	ACCUM_JAN, ACCUM_FEB, ACCUM_MAR, ACCUM_APR,
	ACCUM_MAY, ACCUM_JUN, ACCUM_JUL, ACCUM_AUG,
	ACCUM_SEP, ACCUM_OCT, ACCUM_NOV, ACCUM_DEC,
};

static inline bool IsLeapYear(Year yr)
{
	return yr % 4 == 0 && (yr % 100 != 0 || yr % 400 == 0);
}

/**
 * Converts a Date to a Year, Month & Day.
 * @param date the date to convert from
 * @param ymd  the year, month and day to write to
 */
void ConvertDateToYMD(Date date, YearMonthDay *ymd)
{
	/*
	 * Year determination in multiple steps to account for leap
	 * years. First do the large steps, then the smaller ones.
	 */

	/* There are 97 leap years in 400 years */
	Year yr = 400 * (date / (365 * 400 + 97));
	int rem = date % (365 * 400 + 97);
	uint16 x;

	if (rem >= 365 * 100 + 25) {
		/* There are 25 leap years in the first 100 years after
		 * every 400th year, as every 400th year is a leap year */
		yr  += 100;
		rem -= 365 * 100 + 25;

		/* There are 24 leap years in the next couple of 100 years */
		yr += 100 * (rem / (365 * 100 + 24));
		rem = (rem % (365 * 100 + 24));
	}

	if (!IsLeapYear(yr) && rem >= 365 * 4) {
		/* The first 4 year of the century are not always a leap year */
		yr  += 4;
		rem -= 365 * 4;
	}

	/* There is 1 leap year every 4 years */
	yr += 4 * (rem / (365 * 4 + 1));
	rem = rem % (365 * 4 + 1);

	/* The last (max 3) years to account for; the first one
	 * can be, but is not necessarily a leap year */
	while (rem >= (IsLeapYear(yr) ? 366 : 365)) {
		rem -= IsLeapYear(yr) ? 366 : 365;
		yr++;
	}

	/* Skip the 29th of February in non-leap years */
	if (!IsLeapYear(yr) && rem >= ACCUM_MAR - 1) rem++;

	ymd->year = yr;

	x = _month_date_from_year_day[rem];
	ymd->month = x >> 5;
	ymd->day = x & 0x1F;
}

/**
 * Converts a tupe of Year, Month and Day to a Date.
 * @param year  is a number between 0..MAX_YEAR
 * @param month is a number between 0..11
 * @param day   is a number between 1..31
 */
Date ConvertYMDToDate(Year year, Month month, Day day)
{
	/*
	 * Each passed leap year adds one day to the 'day count'.
	 *
	 * A special case for the year 0 as no year has been passed,
	 * but '(year - 1) / 4' does not yield '-1' to counteract the
	 * '+1' at the end of the formula as divisions round to zero.
	 */
	int nr_of_leap_years = (year == 0) ? 0 : ((year - 1) / 4 - (year - 1) / 100 + (year - 1) / 400 + 1);

	/* Day-offset in a leap year */
	int days = _accum_days_for_month[month] + day - 1;

	/* Account for the missing of the 29th of February in non-leap years */
	if (!IsLeapYear(year) && days >= ACCUM_MAR) days--;

	return year * 365 + nr_of_leap_years + days;
}

/** Functions used by the IncreaseDate function */

extern void OnNewDay_Train(Vehicle *v);
extern void OnNewDay_RoadVeh(Vehicle *v);
extern void OnNewDay_Aircraft(Vehicle *v);
extern void OnNewDay_Ship(Vehicle *v);
static void OnNewDay_EffectVehicle(Vehicle *v) { /* empty */ }
extern void OnNewDay_DisasterVehicle(Vehicle *v);

typedef void OnNewVehicleDayProc(Vehicle *v);

static OnNewVehicleDayProc * _on_new_vehicle_day_proc[] = {
	OnNewDay_Train,
	OnNewDay_RoadVeh,
	OnNewDay_Ship,
	OnNewDay_Aircraft,
	OnNewDay_EffectVehicle,
	OnNewDay_DisasterVehicle,
};

extern void WaypointsDailyLoop(void);
extern void TextMessageDailyLoop(void);
extern void EnginesDailyLoop(void);
extern void DisasterDailyLoop(void);

extern void PlayersMonthlyLoop(void);
extern void EnginesMonthlyLoop(void);
extern void TownsMonthlyLoop(void);
extern void IndustryMonthlyLoop(void);
extern void StationMonthlyLoop(void);

extern void PlayersYearlyLoop(void);
extern void TrainsYearlyLoop(void);
extern void RoadVehiclesYearlyLoop(void);
extern void AircraftYearlyLoop(void);
extern void ShipsYearlyLoop(void);

extern void ShowEndGameChart(void);


static const Month _autosave_months[] = {
	 0, // never
	 1, // every month
	 3, // every 3 months
	 6, // every 6 months
	12, // every 12 months
};

/**
 * Runs the day_proc for every DAY_TICKS vehicle starting at daytick.
 */
static void RunVehicleDayProc(uint daytick)
{
	uint total = GetMaxVehicleIndex() + 1;
	uint i;

	for (i = daytick; i < total; i += DAY_TICKS) {
		Vehicle *v = GetVehicle(i);

		if (IsValidVehicle(v)) _on_new_vehicle_day_proc[v->type - 0x10](v);
	}
}

void IncreaseDate(void)
{
	YearMonthDay ymd;

	if (_game_mode == GM_MENU) {
		_tick_counter++;
		return;
	}

	RunVehicleDayProc(_date_fract);

	/* increase day, and check if a new day is there? */
	_tick_counter++;

	_date_fract++;
	if (_date_fract < DAY_TICKS) return;
	_date_fract = 0;

	/* yeah, increase day counter and call various daily loops */
	_date++;

	TextMessageDailyLoop();

	DisasterDailyLoop();
	WaypointsDailyLoop();

	if (_game_mode != GM_MENU) {
		InvalidateWindowWidget(WC_STATUS_BAR, 0, 0);
		EnginesDailyLoop();
	}

	/* check if we entered a new month? */
	ConvertDateToYMD(_date, &ymd);
	if (ymd.month == _cur_month) return;
	_cur_month = ymd.month;

	/* yes, call various monthly loops */
	if (_game_mode != GM_MENU) {
		if (_opt.autosave != 0 && (_cur_month % _autosave_months[_opt.autosave]) == 0) {
			_do_autosave = true;
			RedrawAutosave();
		}

		PlayersMonthlyLoop();
		EnginesMonthlyLoop();
		TownsMonthlyLoop();
		IndustryMonthlyLoop();
		StationMonthlyLoop();
		if (_network_server) NetworkServerMonthlyLoop();
	}

	/* check if we entered a new year? */
	if (ymd.year == _cur_year) return;
	_cur_year = ymd.year;

	/* yes, call various yearly loops */
	PlayersYearlyLoop();
	TrainsYearlyLoop();
	RoadVehiclesYearlyLoop();
	AircraftYearlyLoop();
	ShipsYearlyLoop();
	if (_network_server) NetworkServerYearlyLoop();

	/* check if we reached end of the game */
	if (_cur_year == _patches.ending_year) {
			ShowEndGameChart();
	/* check if we reached the maximum year, decrement dates by a year */
	} else if (_cur_year == MAX_YEAR + 1) {
		Vehicle *v;
		uint days_this_year;

		_cur_year--;
		days_this_year = IsLeapYear(_cur_year) ? 366 : 365;
		_date -= days_this_year;
		FOR_ALL_VEHICLES(v) v->date_of_last_service -= days_this_year;

		/* Because the _date wraps here, and text-messages expire by game-days, we have to clean out
		 *  all of them if the date is set back, else those messages will hang for ever */
		InitTextMessage();
	}

	if (_patches.auto_euro) CheckSwitchToEuro();
}
