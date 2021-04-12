/* $Id: misc_gui.c 11074 2007-09-09 20:52:54Z rubidium $ */

#include "stdafx.h"
#include "openttd.h"
#include "hal.h"
#include "heightmap.h"
#include "debug.h"
#include "functions.h"
#include "newgrf.h"
#include "saveload.h"
#include "strings.h"
#include "table/sprites.h"
#include "table/strings.h"
#include "table/tree_land.h"
#include "map.h"
#include "window.h"
#include "gui.h"
#include "viewport.h"
#include "gfx.h"
#include "station.h"
#include "command.h"
#include "player.h"
#include "town.h"
#include "sound.h"
#include "network.h"
#include "string.h"
#include "variables.h"
#include "vehicle.h"
#include "train.h"
#include "tgp.h"
#include "settings.h"
#include "date.h"
#include "fileio.h"

#include "fios.h"
/* Variables to display file lists */
FiosItem *_fios_list;
int _saveload_mode;

extern void GenerateLandscape(byte mode);
extern void SwitchMode(int new_mode);

static bool _fios_path_changed;
static bool _savegame_sort_dirty;

enum {
	LAND_INFO_LINES          =   7,
	LAND_INFO_LINE_BUFF_SIZE = 512,
};

static char _landinfo_data[LAND_INFO_LINES][LAND_INFO_LINE_BUFF_SIZE];

static void LandInfoWndProc(Window *w, WindowEvent *e)
{
	if (e->event == WE_PAINT) {
		DrawWindowWidgets(w);

		DoDrawStringCentered(140, 16, _landinfo_data[0], 13);
		DoDrawStringCentered(140, 27, _landinfo_data[1], 0);
		DoDrawStringCentered(140, 38, _landinfo_data[2], 0);
		DoDrawStringCentered(140, 49, _landinfo_data[3], 0);
		DoDrawStringCentered(140, 60, _landinfo_data[4], 0);
		if (_landinfo_data[5][0] != '\0') DrawStringMultiCenter(140, 76, BindCString(_landinfo_data[5]), 276);
		if (_landinfo_data[6][0] != '\0') DoDrawStringCentered(140, 71, _landinfo_data[6], 0);
	}
}

static const Widget _land_info_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,    14,     0,    10,     0,    13, STR_00C5,                       STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,    14,    11,   279,     0,    13, STR_01A3_LAND_AREA_INFORMATION, STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,   RESIZE_NONE,    14,     0,   279,    14,    92, 0x0,                            STR_NULL},
{    WIDGETS_END},
};

static const WindowDesc _land_info_desc = {
	WDP_AUTO, WDP_AUTO, 280, 93,
	WC_LAND_INFO,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_land_info_widgets,
	LandInfoWndProc
};

static void Place_LandInfo(TileIndex tile)
{
	Player *p;
	Window *w;
	Town *t;
	int64 old_money;
	int64 costclear;
	AcceptedCargo ac;
	TileDesc td;
	StringID str;

	DeleteWindowById(WC_LAND_INFO, 0);

	w = AllocateWindowDesc(&_land_info_desc);
	WP(w, void_d).data = &_landinfo_data;

	p = GetPlayer(IsValidPlayer(_local_player) ? _local_player : 0);
	t = ClosestTownFromTile(tile, _patches.dist_local_authority);

	old_money = p->money64;
	p->money64 = p->player_money = 0x7fffffff;
	costclear = DoCommand(tile, 0, 0, 0, CMD_LANDSCAPE_CLEAR);
	p->money64 = old_money;
	UpdatePlayerMoney32(p);

	/* Because build_date is not set yet in every TileDesc, we make sure it is empty */
	td.build_date = 0;
	GetAcceptedCargo(tile, ac);
	GetTileDesc(tile, &td);

	SetDParam(0, td.dparam[0]);
	GetString(_landinfo_data[0], td.str, lastof(_landinfo_data[0]));

	SetDParam(0, STR_01A6_N_A);
	if (td.owner != OWNER_NONE && td.owner != OWNER_WATER) GetNameOfOwner(td.owner, tile);
	GetString(_landinfo_data[1], STR_01A7_OWNER, lastof(_landinfo_data[1]));

	str = STR_01A4_COST_TO_CLEAR_N_A;
	if (!CmdFailed(costclear)) {
		SetDParam(0, costclear);
		str = STR_01A5_COST_TO_CLEAR;
	}
	GetString(_landinfo_data[2], str, lastof(_landinfo_data[2]));

	snprintf(_userstring, lengthof(_userstring), "0x%.4X", tile);
	SetDParam(0, TileX(tile));
	SetDParam(1, TileY(tile));
	SetDParam(2, STR_SPEC_USERSTRING);
	GetString(_landinfo_data[3], STR_LANDINFO_COORDS, lastof(_landinfo_data[3]));

	SetDParam(0, STR_01A9_NONE);
	if (t != NULL && IsValidTown(t)) {
		SetDParam(0, STR_TOWN);
		SetDParam(1, t->index);
	}
	GetString(_landinfo_data[4], STR_01A8_LOCAL_AUTHORITY, lastof(_landinfo_data[4]));

	{
		int i;
		char *p = GetString(_landinfo_data[5], STR_01CE_CARGO_ACCEPTED, lastof(_landinfo_data[5]));
		bool found = false;

		for (i = 0; i < NUM_CARGO; ++i) {
			if (ac[i] > 0) {
				/* Add a comma between each item. */
				if (found) {
					*p++ = ',';
					*p++ = ' ';
				}
				found = true;

				/* If the accepted value is less than 8, show it in 1/8:ths */
				if (ac[i] < 8) {
					SetDParam(0, ac[i]);
					SetDParam(1, _cargoc.names_s[i]);
					p = GetString(p, STR_01D1_8, lastof(_landinfo_data[5]));
				} else {
					p = GetString(p, _cargoc.names_s[i], lastof(_landinfo_data[5]));
				}
			}
		}

		if (!found) _landinfo_data[5][0] = '\0';
	}

	if (td.build_date != 0) {
		SetDParam(0, td.build_date);
		GetString(_landinfo_data[6], STR_BUILD_DATE, lastof(_landinfo_data[6]));
	} else {
		_landinfo_data[6][0] = '\0';
	}

#if defined(_DEBUG)
#	define LANDINFOD_LEVEL 0
#else
#	define LANDINFOD_LEVEL 1
#endif
	DEBUG(misc, LANDINFOD_LEVEL) ("TILE: %#x (%i,%i)", tile, TileX(tile), TileY(tile));
	DEBUG(misc, LANDINFOD_LEVEL) ("type_height  = %#x", _m[tile].type_height);
	DEBUG(misc, LANDINFOD_LEVEL) ("m1           = %#x", _m[tile].m1);
	DEBUG(misc, LANDINFOD_LEVEL) ("m2           = %#x", _m[tile].m2);
	DEBUG(misc, LANDINFOD_LEVEL) ("m3           = %#x", _m[tile].m3);
	DEBUG(misc, LANDINFOD_LEVEL) ("m4           = %#x", _m[tile].m4);
	DEBUG(misc, LANDINFOD_LEVEL) ("m5           = %#x", _m[tile].m5);
	DEBUG(misc, LANDINFOD_LEVEL) ("extra        = %#x", _m[tile].extra);
#undef LANDINFOD_LEVEL
}

void PlaceLandBlockInfo(void)
{
	if (_cursor.sprite == SPR_CURSOR_QUERY) {
		ResetObjectToPlace();
	} else {
		_place_proc = Place_LandInfo;
		SetObjectToPlace(SPR_CURSOR_QUERY, 1, 1, 0);
	}
}

static const char *credits[] = {
	/*************************************************************************
	 *                      maximum length of string which fits in window   -^*/
	"Original design by Chris Sawyer",
	"Original graphics by Simon Foster",
	"",
	"The OpenTTD team (in alphabetical order):",
	"  Jean-Francois Claeys (Belugas) - In training, not yet specialized",
	"  Bjarni Corfitzen (Bjarni) - MacOSX port, coder",
	"  Matthijs Kooijman (blathijs) - Pathfinder-guru",
	"  Victor Fischer (Celestar) - Programming everywhere you need him to",
	"  Tamás Faragó (Darkvater) - Lead coder",
	"  Loïc Guilloux (glx) - In training, not yet specialized",
	"  Jaroslav Mazanec (KUDr) - YAPG (Yet Another Pathfinder God) ;)",
	"  Attila Bán (MiHaMiX) - WebTranslator, Nightlies, Wiki and bugtracker host",
	"  Owen Rudge (orudge) - Forum host, OS/2 port",
	"  Peter Nelson (peter1138) - Spiritual descendant from newgrf gods",
	"  Remko Bijker (Rubidium) - THE desync hunter",
	"  Christoph Mallon (Tron) - Programmer, code correctness police",
	"  Patric Stout (TrueLight) - Coder, network guru, SVN-, MS- and website host",
	"",
	"Retired Developers:",
	"  Ludvig Strigeus (ludde) - OpenTTD author, main coder (0.1 - 0.3.3)",
	"  Serge Paquet (vurlix) - Assistant project manager, coder (0.1 - 0.3.3)",
	"  Dominik Scherer (dominik81) - Lead programmer, GUI expert (0.3.0 - 0.3.6)",
	"",
	"Special thanks go out to:",
	"  Josef Drexler - For his great work on TTDPatch",
	"  Marcin Grzegorczyk - For his documentation of TTD internals",
	"  Petr Baudis (pasky) - Many patches, newgrf support",
	"  Stefan Meißner (sign_de) - For his work on the console",
	"  Simon Sasburg (HackyKid) - Many bugfixes he has blessed us with (and PBS)",
	"  Cian Duffy (MYOB) - BeOS port / manual writing",
	"  Christian Rosentreter (tokai) - MorphOS / AmigaOS port",
	"  Richard Kempton (richK) - additional airports, initial TGP implementation",
	"",
	"  Michael Blunck - Pre-Signals and Semaphores © 2003",
	"  George - Canal/Lock graphics © 2003-2004",
	"  Marcin Grzegorczyk - Foundations for Tracks on Slopes",
	"  All Translators - Who made OpenTTD a truly international game",
	"  Bug Reporters - Without whom OpenTTD would still be full of bugs!",
	"",
	"",
	"And last but not least:",
	"  Chris Sawyer - For an amazing game!"
};

static void AboutWindowProc(Window *w, WindowEvent *e)
{
	switch (e->event) {
	case WE_CREATE: /* Set up window counter and start position of scroller */
		WP(w, scroller_d).counter = 0;
		WP(w, scroller_d).height = w->height - 40;
		break;
	case WE_PAINT: {
		uint i;
		int y = WP(w, scroller_d).height;
		DrawWindowWidgets(w);

		// Show original copyright and revision version
		DrawStringCentered(210, 17, STR_00B6_ORIGINAL_COPYRIGHT, 0);
		DrawStringCentered(210, 17 + 10, STR_00B7_VERSION, 0);

		// Show all scrolling credits
		for (i = 0; i < lengthof(credits); i++) {
			if (y >= 50 && y < (w->height - 40)) {
				DoDrawString(credits[i], 10, y, 0x10);
			}
			y += 10;
		}

		// If the last text has scrolled start anew from the start
		if (y < 50) WP(w, scroller_d).height = w->height - 40;

		DoDrawStringCentered(210, w->height - 25, "Website: http://www.openttd.org", 16);
		DrawStringCentered(210, w->height - 15, STR_00BA_COPYRIGHT_OPENTTD, 0);
	}	break;
	case WE_MOUSELOOP: /* Timer to scroll the text and adjust the new top */
		if (WP(w, scroller_d).counter++ % 3 == 0) {
			WP(w, scroller_d).height--;
			SetWindowDirty(w);
		}
		break;
	}
}

static const Widget _about_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,    14,     0,    10,     0,    13, STR_00C5,         STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,    14,    11,   419,     0,    13, STR_015B_OPENTTD, STR_NULL},
{      WWT_PANEL,   RESIZE_NONE,    14,     0,   419,    14,   271, 0x0,              STR_NULL},
{      WWT_FRAME,   RESIZE_NONE,    14,     5,   414,    40,   245, STR_NULL,         STR_NULL},
{    WIDGETS_END},
};

static const WindowDesc _about_desc = {
	WDP_CENTER, WDP_CENTER, 420, 272,
	WC_GAME_OPTIONS,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_about_widgets,
	AboutWindowProc
};


void ShowAboutWindow(void)
{
	DeleteWindowById(WC_GAME_OPTIONS, 0);
	AllocateWindowDesc(&_about_desc);
}

static int _tree_to_plant;

static const uint32 _tree_sprites[] = {
	0x655, 0x663, 0x678, 0x62B, 0x647, 0x639, 0x64E, 0x632, 0x67F, 0x68D, 0x69B, 0x6A9,
	0x6AF, 0x6D2, 0x6D9, 0x6C4, 0x6CB, 0x6B6, 0x6BD, 0x6E0,
	0x72E, 0x734, 0x74A, 0x74F, 0x76B, 0x78F, 0x788, 0x77B, 0x75F, 0x774, 0x720, 0x797,
	0x79E, 0x7A5 | PALETTE_TO_GREEN, 0x7AC | PALETTE_TO_RED, 0x7B3, 0x7BA, 0x7C1 | PALETTE_TO_RED, 0x7C8 | PALETTE_TO_PALE_GREEN, 0x7CF | PALETTE_TO_YELLOW, 0x7D6 | PALETTE_TO_RED
};

static void BuildTreesWndProc(Window *w, WindowEvent *e)
{
	switch (e->event) {
	case WE_PAINT: {
		int x,y;
		int i, count;

		DrawWindowWidgets(w);

		WP(w,tree_d).base = i = _tree_base_by_landscape[_opt.landscape];
		WP(w,tree_d).count = count = _tree_count_by_landscape[_opt.landscape];

		x = 18;
		y = 54;
		do {
			DrawSprite(_tree_sprites[i], x, y);
			x += 35;
			if (!(++i & 3)) {
				x -= 35 * 4;
				y += 47;
			}
		} while (--count);
	} break;

	case WE_CLICK: {
		int wid = e->we.click.widget;

		switch (wid) {
		case 0:
			ResetObjectToPlace();
			break;

		case 3: case 4: case 5: case 6:
		case 7: case 8: case 9: case 10:
		case 11:case 12: case 13: case 14:
			if (wid - 3 >= WP(w,tree_d).count) break;

			if (HandlePlacePushButton(w, wid, SPR_CURSOR_TREE, 1, NULL))
				_tree_to_plant = WP(w,tree_d).base + wid - 3;
			break;

		case 15: // tree of random type.
			if (HandlePlacePushButton(w, 15, SPR_CURSOR_TREE, 1, NULL))
				_tree_to_plant = -1;
			break;

		case 16: /* place trees randomly over the landscape*/
			LowerWindowWidget(w, 16);
			w->flags4 |= 5 << WF_TIMEOUT_SHL;
			SndPlayFx(SND_15_BEEP);
			PlaceTreesRandomly();
			MarkWholeScreenDirty();
			break;
		}
	} break;

	case WE_PLACE_OBJ:
		VpStartPlaceSizing(e->we.place.tile, VPM_X_AND_Y_LIMITED);
		VpSetPlaceSizingLimit(20);
		break;

	case WE_PLACE_DRAG:
		VpSelectTilesWithMethod(e->we.place.pt.x, e->we.place.pt.y, e->we.place.userdata);
		return;

	case WE_PLACE_MOUSEUP:
		if (e->we.place.pt.x != -1) {
			DoCommandP(e->we.place.tile, _tree_to_plant, e->we.place.starttile, NULL,
				CMD_PLANT_TREE | CMD_MSG(STR_2805_CAN_T_PLANT_TREE_HERE));
		}
		break;

	case WE_TIMEOUT:
		RaiseWindowWidget(w, 16);
		break;

	case WE_ABORT_PLACE_OBJ:
		RaiseWindowButtons(w);
		break;
	}
}

static const Widget _build_trees_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,     7,     0,    10,     0,    13, STR_00C5,              STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,     7,    11,   142,     0,    13, STR_2802_TREES,        STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,   RESIZE_NONE,     7,     0,   142,    14,   170, 0x0,                   STR_NULL},
{      WWT_PANEL,   RESIZE_NONE,    14,     2,    35,    16,    61, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    37,    70,    16,    61, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    72,   105,    16,    61, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,   107,   140,    16,    61, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,     2,    35,    63,   108, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    37,    70,    63,   108, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    72,   105,    63,   108, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,   107,   140,    63,   108, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,     2,    35,   110,   155, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    37,    70,   110,   155, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    72,   105,   110,   155, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,   107,   140,   110,   155, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{    WWT_TEXTBTN,   RESIZE_NONE,    14,     2,   140,   157,   168, STR_TREES_RANDOM_TYPE, STR_TREES_RANDOM_TYPE_TIP},
{    WIDGETS_END},
};

static const WindowDesc _build_trees_desc = {
	WDP_AUTO, WDP_AUTO, 143, 171,
	WC_BUILD_TREES, WC_SCEN_LAND_GEN,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_trees_widgets,
	BuildTreesWndProc
};

static const Widget _build_trees_scen_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,     7,     0,    10,     0,    13, STR_00C5,              STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,     7,    11,   142,     0,    13, STR_2802_TREES,        STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,   RESIZE_NONE,     7,     0,   142,    14,   183, 0x0,                   STR_NULL},
{      WWT_PANEL,   RESIZE_NONE,    14,     2,    35,    16,    61, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    37,    70,    16,    61, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    72,   105,    16,    61, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,   107,   140,    16,    61, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,     2,    35,    63,   108, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    37,    70,    63,   108, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    72,   105,    63,   108, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,   107,   140,    63,   108, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,     2,    35,   110,   155, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    37,    70,   110,   155, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,    72,   105,   110,   155, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{      WWT_PANEL,   RESIZE_NONE,    14,   107,   140,   110,   155, 0x0,                   STR_280D_SELECT_TREE_TYPE_TO_PLANT},
{    WWT_TEXTBTN,   RESIZE_NONE,    14,     2,   140,   157,   168, STR_TREES_RANDOM_TYPE, STR_TREES_RANDOM_TYPE_TIP},
{    WWT_TEXTBTN,   RESIZE_NONE,    14,     2,   140,   170,   181, STR_028A_RANDOM_TREES, STR_028B_PLANT_TREES_RANDOMLY_OVER},
{    WIDGETS_END},
};

static const WindowDesc _build_trees_scen_desc = {
	WDP_AUTO, WDP_AUTO, 143, 184,
	WC_BUILD_TREES,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_trees_scen_widgets,
	BuildTreesWndProc
};


void ShowBuildTreesToolbar(void)
{
	if (!IsValidPlayer(_current_player)) return;
	AllocateWindowDescFront(&_build_trees_desc, 0);
}

void ShowBuildTreesScenToolbar(void)
{
	AllocateWindowDescFront(&_build_trees_scen_desc, 0);
}

static uint32 _errmsg_decode_params[20];
static StringID _errmsg_message_1, _errmsg_message_2;
static uint _errmsg_duration;


static const Widget _errmsg_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,     4,     0,    10,     0,    13, STR_00C5,         STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,     4,    11,   239,     0,    13, STR_00B2_MESSAGE, STR_NULL},
{      WWT_PANEL,   RESIZE_NONE,     4,     0,   239,    14,    45, 0x0,              STR_NULL},
{    WIDGETS_END},
};

static const Widget _errmsg_face_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,     4,     0,    10,     0,    13, STR_00C5,              STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,     4,    11,   333,     0,    13, STR_00B3_MESSAGE_FROM, STR_NULL},
{      WWT_PANEL,   RESIZE_NONE,     4,     0,   333,    14,   136, 0x0,                   STR_NULL},
{   WIDGETS_END},
};

static void ErrmsgWndProc(Window *w, WindowEvent *e)
{
	switch (e->event) {
	case WE_PAINT:
		COPY_IN_DPARAM(0, _errmsg_decode_params, lengthof(_errmsg_decode_params));
		DrawWindowWidgets(w);
		COPY_IN_DPARAM(0, _errmsg_decode_params, lengthof(_errmsg_decode_params));
		if (!IsWindowOfPrototype(w, _errmsg_face_widgets)) {
			DrawStringMultiCenter(
				120,
				(_errmsg_message_1 == INVALID_STRING_ID ? 25 : 15),
				_errmsg_message_2,
				238);
			if (_errmsg_message_1 != INVALID_STRING_ID)
				DrawStringMultiCenter(
					120,
					30,
					_errmsg_message_1,
					238);
		} else {
			const Player *p = GetPlayer(GetDParamX(_errmsg_decode_params,2));
			DrawPlayerFace(p->face, p->player_color, 2, 16);

			DrawStringMultiCenter(
				214,
				(_errmsg_message_1 == INVALID_STRING_ID ? 65 : 45),
				_errmsg_message_2,
				238);
			if (_errmsg_message_1 != INVALID_STRING_ID)
				DrawStringMultiCenter(
					214,
					90,
					_errmsg_message_1,
					238);
		}
		break;

	case WE_MOUSELOOP:
		if (_right_button_down) DeleteWindow(w);
		break;

	case WE_4:
		if (--_errmsg_duration == 0) DeleteWindow(w);
		break;

	case WE_DESTROY:
		SetRedErrorSquare(0);
		_switch_mode_errorstr = INVALID_STRING_ID;
		break;

	case WE_KEYPRESS:
		if (e->we.keypress.keycode == WKC_SPACE) {
			// Don't continue.
			e->we.keypress.cont = false;
			DeleteWindow(w);
		}
		break;
	}
}

void ShowErrorMessage(StringID msg_1, StringID msg_2, int x, int y)
{
	Window *w;
	const ViewPort *vp;
	Point pt;

	DeleteWindowById(WC_ERRMSG, 0);

	//assert(msg_2);
	if (msg_2 == 0) msg_2 = STR_EMPTY;

	_errmsg_message_1 = msg_1;
	_errmsg_message_2 = msg_2;
	COPY_OUT_DPARAM(_errmsg_decode_params, 0, lengthof(_errmsg_decode_params));
	_errmsg_duration = _patches.errmsg_duration;
	if (!_errmsg_duration) return;

	if (_errmsg_message_1 != STR_013B_OWNED_BY || GetDParamX(_errmsg_decode_params,2) >= 8) {

		if ( (x|y) != 0) {
			pt = RemapCoords2(x, y);
			vp = FindWindowById(WC_MAIN_WINDOW, 0)->viewport;

			// move x pos to opposite corner
			pt.x = ((pt.x - vp->virtual_left) >> vp->zoom) + vp->left;
			pt.x = (pt.x < (_screen.width >> 1)) ? _screen.width - 260 : 20;

			// move y pos to opposite corner
			pt.y = ((pt.y - vp->virtual_top) >> vp->zoom) + vp->top;
			pt.y = (pt.y < (_screen.height >> 1)) ? _screen.height - 80 : 100;

		} else {
			pt.x = (_screen.width - 240) >> 1;
			pt.y = (_screen.height - 46) >> 1;
		}
		w = AllocateWindow(pt.x, pt.y, 240, 46, ErrmsgWndProc, WC_ERRMSG, _errmsg_widgets);
	} else {
		if ( (x|y) != 0) {
			pt = RemapCoords2(x, y);
			vp = FindWindowById(WC_MAIN_WINDOW, 0)->viewport;
			pt.x = clamp(((pt.x - vp->virtual_left) >> vp->zoom) + vp->left - (334/2), 0, _screen.width - 334);
			pt.y = clamp(((pt.y - vp->virtual_top) >> vp->zoom) + vp->top - (137/2), 22, _screen.height - 137);
		} else {
			pt.x = (_screen.width - 334) >> 1;
			pt.y = (_screen.height - 137) >> 1;
		}
		w = AllocateWindow(pt.x, pt.y, 334, 137, ErrmsgWndProc, WC_ERRMSG, _errmsg_face_widgets);
	}

	w->desc_flags = WDF_STD_BTN | WDF_DEF_WIDGET;
}


void ShowEstimatedCostOrIncome(int32 cost, int x, int y)
{
	StringID msg = STR_0805_ESTIMATED_COST;

	if (cost < 0) {
		cost = -cost;
		msg = STR_0807_ESTIMATED_INCOME;
	}
	SetDParam(0, cost);
	ShowErrorMessage(INVALID_STRING_ID, msg, x, y);
}

void ShowCostOrIncomeAnimation(int x, int y, int z, int32 cost)
{
	StringID msg;
	Point pt = RemapCoords(x,y,z);

	msg = STR_0801_COST;
	if (cost < 0) {
		cost = -cost;
		msg = STR_0803_INCOME;
	}
	SetDParam(0, cost);
	AddTextEffect(msg, pt.x, pt.y, 0x250);
}

void ShowFeederIncomeAnimation(int x, int y, int z, int32 cost)
{
	Point pt = RemapCoords(x,y,z);

	SetDParam(0, cost);
	AddTextEffect(STR_FEEDER, pt.x, pt.y, 0x250);
}

static const Widget _tooltips_widgets[] = {
{      WWT_PANEL,   RESIZE_NONE,    14,     0,   199,     0,    31, 0x0, STR_NULL},
{   WIDGETS_END},
};


static void TooltipsWndProc(Window *w, WindowEvent *e)
{
	switch (e->event) {
		case WE_PAINT: {
			uint arg;
			GfxFillRect(0, 0, w->width - 1, w->height - 1, 0);
			GfxFillRect(1, 1, w->width - 2, w->height - 2, 0x44);

			for (arg = 0; arg < WP(w, tooltips_d).paramcount; arg++) {
				SetDParam(arg, WP(w, tooltips_d).params[arg]);
			}
			DrawStringMultiCenter((w->width >> 1), (w->height >> 1) - 5, WP(w, tooltips_d).string_id, 197);
			break;
		}

		case WE_MOUSELOOP:
			/* We can show tooltips while dragging tools. These are shown as long as
			 * we are dragging the tool. Normal tooltips work with rmb */
			if (WP(w, tooltips_d).paramcount == 0 ) {
				if (!_right_button_down) DeleteWindow(w);
			} else {
				if (!_left_button_down) DeleteWindow(w);
			}

			break;
	}
}

/** Shows a tooltip
* @param str String to be displayed
* @param params (optional) up to 5 pieces of additional information that may be
* added to a tooltip; currently only supports parameters of {NUM} (integer) */
void GuiShowTooltipsWithArgs(StringID str, uint paramcount, const uint32 params[])
{
	char buffer[512];
	BoundingRect br;
	Window *w;
	uint i;
	int x, y;

	DeleteWindowById(WC_TOOLTIPS, 0);

	/* We only show measurement tooltips with patch setting on */
	if (str == STR_NULL || (paramcount != 0 && !_patches.measure_tooltip)) return;

	for (i = 0; i != paramcount; i++) SetDParam(i, params[i]);
	GetString(buffer, str, lastof(buffer));

	br = GetStringBoundingBox(buffer);
	br.width += 6; br.height += 4; // increase slightly to have some space around the box

	/* Cut tooltip length to 200 pixels max, wrap to new line if longer */
	if (br.width > 200) {
		br.height += ((br.width - 4) / 176) * 10;
		br.width = 200;
	}

	/* Correctly position the tooltip position, watch out for window and cursor size
	 * Clamp value to below main toolbar and above statusbar. If tooltip would
	 * go below window, flip it so it is shown above the cursor */
	y = clamp(_cursor.pos.y + _cursor.size.y + _cursor.offs.y + 5, 22, _screen.height - 12);
	if (y + br.height > _screen.height - 12) y = _cursor.pos.y + _cursor.offs.y - br.height - 5;
	x = clamp(_cursor.pos.x - (br.width >> 1), 0, _screen.width - br.width);

	w = AllocateWindow(x, y, br.width, br.height, TooltipsWndProc, WC_TOOLTIPS, _tooltips_widgets);

	WP(w, tooltips_d).string_id = str;
	assert(sizeof(WP(w, tooltips_d).params[0]) == sizeof(params[0]));
	memcpy(WP(w, tooltips_d).params, params, sizeof(WP(w, tooltips_d).params[0]) * paramcount);
	WP(w, tooltips_d).paramcount = paramcount;

	w->flags4 &= ~WF_WHITE_BORDER_MASK; // remove white-border from tooltip
	w->widget[0].right = br.width;
	w->widget[0].bottom = br.height;
}


static void DrawStationCoverageText(const AcceptedCargo accepts,
	int str_x, int str_y, uint mask)
{
	char *b = _userstring;
	bool first = true;
	int i;

	b = InlineString(b, STR_000D_ACCEPTS);

	for (i = 0; i != NUM_CARGO; i++, mask >>= 1) {
		if (b >= lastof(_userstring) - 5) break;
		if (accepts[i] >= 8 && mask & 1) {
			if (first) {
				first = false;
			} else {
				/* Add a comma if this is not the first item */
				*b++ = ',';
				*b++ = ' ';
			}
			b = InlineString(b, _cargoc.names_s[i]);
		}
	}

	/* If first is still true then no cargo is accepted */
	if (first) b = InlineString(b, STR_00D0_NOTHING);

	*b = '\0';
	DrawStringMultiLine(str_x, str_y, STR_SPEC_USERSTRING, 144);
}

void DrawStationCoverageAreaText(int sx, int sy, uint mask, int rad) {
	TileIndex tile = TileVirtXY(_thd.pos.x, _thd.pos.y);
	AcceptedCargo accepts;
	if (tile < MapSize()) {
		GetAcceptanceAroundTiles(accepts, tile, _thd.size.x / TILE_SIZE, _thd.size.y / TILE_SIZE , rad);
		DrawStationCoverageText(accepts, sx, sy, mask);
	}
}

void CheckRedrawStationCoverage(const Window *w)
{
	if (_thd.dirty & 1) {
		_thd.dirty &= ~1;
		SetWindowDirty(w);
	}
}

void SetVScrollCount(Window *w, int num)
{
	w->vscroll.count = num;
	num -= w->vscroll.cap;
	if (num < 0) num = 0;
	if (num < w->vscroll.pos) w->vscroll.pos = num;
}

void SetVScroll2Count(Window *w, int num)
{
	w->vscroll2.count = num;
	num -= w->vscroll2.cap;
	if (num < 0) num = 0;
	if (num < w->vscroll2.pos) w->vscroll2.pos = num;
}

void SetHScrollCount(Window *w, int num)
{
	w->hscroll.count = num;
	num -= w->hscroll.cap;
	if (num < 0) num = 0;
	if (num < w->hscroll.pos) w->hscroll.pos = num;
}

/* Delete a character at the caret position in a text buf.
 * If backspace is set, delete the character before the caret,
 * else delete the character after it. */
static void DelChar(Textbuf *tb, bool backspace)
{
	WChar c;
	uint width;
	size_t len;
	char *s = tb->buf + tb->caretpos;

	if (backspace) s = Utf8PrevChar(s);

	len = Utf8Decode(&c, s);
	width = GetCharacterWidth(FS_NORMAL, c);

	tb->width  -= width;
	if (backspace) {
		tb->caretpos   -= len;
		tb->caretxoffs -= width;
	}

	/* Move the remaining characters over the marker */
	memmove(s, s + len, tb->length - (s - tb->buf) - len + 1);
	tb->length -= len;
}

/**
 * Delete a character from a textbuffer, either with 'Delete' or 'Backspace'
 * The character is delete from the position the caret is at
 * @param tb @Textbuf type to be changed
 * @param delmode Type of deletion, either @WKC_BACKSPACE or @WKC_DELETE
 * @return Return true on successfull change of Textbuf, or false otherwise
 */
bool DeleteTextBufferChar(Textbuf *tb, int delmode)
{
	if (delmode == WKC_BACKSPACE && tb->caretpos != 0) {
		DelChar(tb, true);
		return true;
	} else if (delmode == WKC_DELETE && tb->caretpos < tb->length) {
		DelChar(tb, false);
		return true;
	}

	return false;
}

/**
 * Delete every character in the textbuffer
 * @param tb @Textbuf buffer to be emptied
 */
void DeleteTextBufferAll(Textbuf *tb)
{
	memset(tb->buf, 0, tb->maxlength);
	tb->length = tb->width = 0;
	tb->caretpos = tb->caretxoffs = 0;
}

/**
 * Insert a character to a textbuffer. If maxwidth of the Textbuf is zero,
 * we don't care about the visual-length but only about the physical
 * length of the string
 * @param tb @Textbuf type to be changed
 * @param key Character to be inserted
 * @return Return true on successfull change of Textbuf, or false otherwise
 */
bool InsertTextBufferChar(Textbuf *tb, WChar key)
{
	const byte charwidth = GetCharacterWidth(FS_NORMAL, key);
	size_t len = Utf8CharLen(key);
	if (tb->length < (tb->maxlength - len) && (tb->maxwidth == 0 || tb->width + charwidth <= tb->maxwidth)) {
		memmove(tb->buf + tb->caretpos + len, tb->buf + tb->caretpos, tb->length - tb->caretpos + 1);
		Utf8Encode(tb->buf + tb->caretpos, key);
		tb->length += len;
		tb->width  += charwidth;

		tb->caretpos   += len;
		tb->caretxoffs += charwidth;
		return true;
	}
	return false;
}

/**
 * Handle text navigation with arrow keys left/right.
 * This defines where the caret will blink and the next characer interaction will occur
 * @param tb @Textbuf type where navigation occurs
 * @param navmode Direction in which navigation occurs @WKC_LEFT, @WKC_RIGHT, @WKC_END, @WKC_HOME
 * @return Return true on successfull change of Textbuf, or false otherwise
 */
bool MoveTextBufferPos(Textbuf *tb, int navmode)
{
	switch (navmode) {
	case WKC_LEFT:
		if (tb->caretpos != 0) {
			WChar c;
			const char *s = Utf8PrevChar(tb->buf + tb->caretpos);
			Utf8Decode(&c, s);
			tb->caretpos    = s - tb->buf; // -= (tb->buf + tb->caretpos - s)
			tb->caretxoffs -= GetCharacterWidth(FS_NORMAL, c);

			return true;
		}
		break;
	case WKC_RIGHT:
		if (tb->caretpos < tb->length) {
			WChar c;

			tb->caretpos   += Utf8Decode(&c, tb->buf + tb->caretpos);
			tb->caretxoffs += GetCharacterWidth(FS_NORMAL, c);

			return true;
		}
		break;
	case WKC_HOME:
		tb->caretpos = 0;
		tb->caretxoffs = 0;
		return true;
	case WKC_END:
		tb->caretpos = tb->length;
		tb->caretxoffs = tb->width;
		return true;
	}

	return false;
}

/**
 * Initialize the textbuffer by supplying it the buffer to write into
 * and the maximum length of this buffer
 * @param tb @Textbuf type which is getting initialized
 * @param buf the buffer that will be holding the data for input
 * @param maxlength maximum length in characters of this buffer
 * @param maxwidth maximum length in pixels of this buffer. If reached, buffer
 * cannot grow, even if maxlength would allow because there is space. A length
 * of zero '0' means the buffer is only restricted by maxlength */
void InitializeTextBuffer(Textbuf *tb, const char *buf, uint16 maxlength, uint16 maxwidth)
{
	tb->buf = (char*)buf;
	tb->maxlength = maxlength;
	tb->maxwidth  = maxwidth;
	tb->caret = true;
	UpdateTextBufferSize(tb);
}

/**
 * Update @Textbuf type with its actual physical character and screenlength
 * Get the count of characters in the string as well as the width in pixels.
 * Useful when copying in a larger amount of text at once
 * @param tb @Textbuf type which length is calculated
 */
void UpdateTextBufferSize(Textbuf *tb)
{
	const char *buf = tb->buf;
	WChar c = Utf8Consume(&buf);

	tb->width = 0;
	tb->length = 0;

	for (; c != '\0' && tb->length < (tb->maxlength - 1); c = Utf8Consume(&buf)) {
		tb->width += GetCharacterWidth(FS_NORMAL, c);
		tb->length += Utf8CharLen(c);
	}

	tb->caretpos = tb->length;
	tb->caretxoffs = tb->width;
}

int HandleEditBoxKey(Window *w, querystr_d *string, int wid, WindowEvent *e)
{
	e->we.keypress.cont = false;

	switch (e->we.keypress.keycode) {
	case WKC_ESC: return 2;
	case WKC_RETURN: case WKC_NUM_ENTER: return 1;
	case (WKC_CTRL | 'V'):
		if (InsertTextBufferClipboard(&string->text))
			InvalidateWidget(w, wid);
		break;
	case (WKC_CTRL | 'U'):
		DeleteTextBufferAll(&string->text);
		InvalidateWidget(w, wid);
		break;
	case WKC_BACKSPACE: case WKC_DELETE:
		if (DeleteTextBufferChar(&string->text, e->we.keypress.keycode))
			InvalidateWidget(w, wid);
		break;
	case WKC_LEFT: case WKC_RIGHT: case WKC_END: case WKC_HOME:
		if (MoveTextBufferPos(&string->text, e->we.keypress.keycode))
			InvalidateWidget(w, wid);
		break;
	default:
		if (IsValidChar(e->we.keypress.key, string->afilter)) {
			if (InsertTextBufferChar(&string->text, e->we.keypress.key)) {
				InvalidateWidget(w, wid);
			}
		} else { // key wasn't caught. Continue only if standard entry specified
			e->we.keypress.cont = (string->afilter == CS_ALPHANUMERAL);
		}
	}

	return 0;
}

bool HandleCaret(Textbuf *tb)
{
	/* caret changed? */
	bool b = !!(_caret_timer & 0x20);

	if (b != tb->caret) {
		tb->caret = b;
		return true;
	}
	return false;
}

void HandleEditBox(Window *w, querystr_d *string, int wid)
{
	if (HandleCaret(&string->text)) InvalidateWidget(w, wid);
}

void DrawEditBox(Window *w, querystr_d *string, int wid)
{
	DrawPixelInfo dpi, *old_dpi;
	int delta;
	const Widget *wi = &w->widget[wid];
	const Textbuf *tb = &string->text;

	/* Limit the drawing of the string inside the widget boundaries */
	if (!FillDrawPixelInfo(&dpi,
	      wi->left + 4,
	      wi->top + 1,
	      wi->right - wi->left - 4,
	      wi->bottom - wi->top - 1)
	) return;

	GfxFillRect(wi->left + 1, wi->top + 1, wi->right - 1, wi->bottom - 1, 215);

	old_dpi = _cur_dpi;
	_cur_dpi = &dpi;

	/* We will take the current widget length as maximum width, with a small
	 * space reserved at the end for the caret to show */
	delta = (wi->right - wi->left) - tb->width - 10;
	if (delta > 0) delta = 0;

	if (tb->caretxoffs + delta < 0) delta = -tb->caretxoffs;

	DoDrawString(tb->buf, delta, 0, 8);
	if (tb->caret) DoDrawString("_", tb->caretxoffs + delta, 0, 12);

	_cur_dpi = old_dpi;
}

static void QueryStringWndProc(Window *w, WindowEvent *e)
{
	static bool closed = false;
	switch (e->event) {
	case WE_CREATE:
		SETBIT(_no_scroll, SCROLL_EDIT);
		closed = false;
		break;

	case WE_PAINT:
		SetDParam(0, WP(w,querystr_d).caption);
		DrawWindowWidgets(w);

		DrawEditBox(w, &WP(w,querystr_d), 5);
		break;

	case WE_CLICK:
		switch (e->we.click.widget) {
		case 3: DeleteWindow(w); break;
		case 4:
press_ok:;
			if (WP(w, querystr_d).orig != NULL &&
					strcmp(WP(w, querystr_d).text.buf, WP(w, querystr_d).orig) == 0) {
				DeleteWindow(w);
			} else {
				char *buf = WP(w,querystr_d).text.buf;
				WindowClass wnd_class = WP(w,querystr_d).wnd_class;
				WindowNumber wnd_num = WP(w,querystr_d).wnd_num;
				Window *parent;

				// Mask the edit-box as closed, so we don't send out a CANCEL
				closed = true;

				DeleteWindow(w);

				parent = FindWindowById(wnd_class, wnd_num);
				if (parent != NULL) {
					WindowEvent e;
					e.event = WE_ON_EDIT_TEXT;
					e.we.edittext.str = buf;
					parent->wndproc(parent, &e);
				}
			}
			break;
		}
		break;

	case WE_MOUSELOOP: {
		if (!FindWindowById(WP(w,querystr_d).wnd_class, WP(w,querystr_d).wnd_num)) {
			DeleteWindow(w);
			return;
		}
		HandleEditBox(w, &WP(w, querystr_d), 5);
	} break;

	case WE_KEYPRESS: {
		switch (HandleEditBoxKey(w, &WP(w, querystr_d), 5, e)) {
		case 1: // Return
			goto press_ok;
		case 2: // Escape
			DeleteWindow(w);
			break;
		}
	} break;

	case WE_DESTROY:
		// If the window is not closed yet, it means it still needs to send a CANCEL
		if (!closed) {
			Window *parent = FindWindowById(WP(w,querystr_d).wnd_class, WP(w,querystr_d).wnd_num);
			if (parent != NULL) {
				WindowEvent e;
				e.event = WE_ON_EDIT_TEXT_CANCEL;
				parent->wndproc(parent, &e);
			}
		}
		CLRBIT(_no_scroll, SCROLL_EDIT);
		break;
	}
}

static const Widget _query_string_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,    14,     0,    10,     0,    13, STR_00C5,        STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,    14,    11,   259,     0,    13, STR_012D,        STR_NULL},
{      WWT_PANEL,   RESIZE_NONE,    14,     0,   259,    14,    29, 0x0,             STR_NULL},
{    WWT_TEXTBTN,   RESIZE_NONE,    14,     0,   129,    30,    41, STR_012E_CANCEL, STR_NULL},
{    WWT_TEXTBTN,   RESIZE_NONE,    14,   130,   259,    30,    41, STR_012F_OK,     STR_NULL},
{      WWT_PANEL,   RESIZE_NONE,    14,     2,   257,    16,    27, 0x0,             STR_NULL},
{   WIDGETS_END},
};

static const WindowDesc _query_string_desc = {
	190, 219, 260, 42,
	WC_QUERY_STRING,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_query_string_widgets,
	QueryStringWndProc
};

static char _edit_str_buf[64];
static char _orig_str_buf[lengthof(_edit_str_buf)];

void ShowQueryString(StringID str, StringID caption, uint maxlen, uint maxwidth, WindowClass window_class, WindowNumber window_number, CharSetFilter afilter)
{
	Window *w;
	uint realmaxlen = maxlen & ~0x1000;

	assert(realmaxlen < lengthof(_edit_str_buf));

	DeleteWindowById(WC_QUERY_STRING, 0);
	DeleteWindowById(WC_SAVELOAD, 0);

	w = AllocateWindowDesc(&_query_string_desc);

	GetString(_edit_str_buf, str, lastof(_edit_str_buf));
	_edit_str_buf[realmaxlen - 1] = '\0';

	if (maxlen & 0x1000) {
		WP(w, querystr_d).orig = NULL;
	} else {
		strecpy(_orig_str_buf, _edit_str_buf, lastof(_orig_str_buf));
		WP(w, querystr_d).orig = _orig_str_buf;
	}

	LowerWindowWidget(w, 5);
	WP(w, querystr_d).caption = caption;
	WP(w, querystr_d).wnd_class = window_class;
	WP(w, querystr_d).wnd_num = window_number;
	WP(w, querystr_d).afilter = afilter;
	InitializeTextBuffer(&WP(w, querystr_d).text, _edit_str_buf, realmaxlen, maxwidth);
}

static void QueryWndProc(Window *w, WindowEvent *e)
{
	switch (e->event) {
	case WE_PAINT:
		SetDParam(0, WP(w, query_d).caption);
		DrawWindowWidgets(w);

		DrawStringMultiCenter(90, 38, WP(w, query_d).message, 178);
		break;

	case WE_CLICK:
		switch (e->we.click.widget) {
		case 3:
		case 4:
			WP(w, query_d).calledback = true;
			if (WP(w, query_d).ok_cancel_callback != NULL) WP(w, query_d).ok_cancel_callback(e->we.click.widget == 4);
			DeleteWindow(w);
			break;
		}
		break;

	case WE_MOUSELOOP:
		if (!FindWindowById(WP(w, query_d).wnd_class, WP(w, query_d).wnd_num)) DeleteWindow(w);
		break;

	case WE_DESTROY:
		if (!WP(w, query_d).calledback && WP(w, query_d).ok_cancel_callback != NULL) WP(w, query_d).ok_cancel_callback(false);
		break;
	}
}

static const Widget _query_widgets[] = {
{ WWT_CLOSEBOX, RESIZE_NONE,  4,   0,  10,   0,  13, STR_00C5,        STR_018B_CLOSE_WINDOW},
{  WWT_CAPTION, RESIZE_NONE,  4,  11, 179,   0,  13, STR_012D,        STR_NULL},
{    WWT_PANEL, RESIZE_NONE,  4,   0, 179,  14,  91, 0x0,             STR_NULL},
{  WWT_TEXTBTN, RESIZE_NONE, 12,  25,  84,  72,  83, STR_012E_CANCEL, STR_NULL},
{  WWT_TEXTBTN, RESIZE_NONE, 12,  95, 154,  72,  83, STR_012F_OK,     STR_NULL},
{  WIDGETS_END },
};

static const WindowDesc _query_desc = {
	WDP_CENTER, WDP_CENTER, 180, 92,
	WC_OK_CANCEL_QUERY, 0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_query_widgets,
	QueryWndProc
};

void ShowQuery(StringID caption, StringID message, void (*ok_cancel_callback)(bool ok_clicked), WindowClass window_class, WindowNumber window_number)
{
	Window *w;

	DeleteWindowById(WC_OK_CANCEL_QUERY, 0);

	w = AllocateWindowDesc(&_query_desc);

	WP(w, query_d).caption            = caption;
	WP(w, query_d).message            = message;
	WP(w, query_d).wnd_class          = window_class;
	WP(w, query_d).wnd_num            = window_number;
	WP(w, query_d).ok_cancel_callback = ok_cancel_callback;
	WP(w, query_d).calledback         = false;
}

static const Widget _load_dialog_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,    14,     0,    10,     0,    13, STR_00C5,         STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,  RESIZE_RIGHT,    14,    11,   256,     0,    13, STR_NULL,         STR_018C_WINDOW_TITLE_DRAG_THIS},
{ WWT_PUSHTXTBTN,   RESIZE_NONE,    14,     0,   127,    14,    25, STR_SORT_BY_NAME, STR_SORT_ORDER_TIP},
{ WWT_PUSHTXTBTN,   RESIZE_NONE,    14,   128,   256,    14,    25, STR_SORT_BY_DATE, STR_SORT_ORDER_TIP},
{      WWT_PANEL,  RESIZE_RIGHT,    14,     0,   256,    26,    47, 0x0,              STR_NULL},
{      WWT_PANEL,     RESIZE_RB,    14,     0,   256,    48,   293, 0x0,              STR_NULL},
{ WWT_PUSHIMGBTN,     RESIZE_LR,    14,   245,   256,    48,    59, SPR_HOUSE_ICON,   STR_SAVELOAD_HOME_BUTTON},
{      WWT_INSET,     RESIZE_RB,    14,     2,   243,    50,   291, 0x0,              STR_400A_LIST_OF_DRIVES_DIRECTORIES},
{  WWT_SCROLLBAR,    RESIZE_LRB,    14,   245,   256,    60,   281, 0x0,              STR_0190_SCROLL_BAR_SCROLLS_LIST},
{  WWT_RESIZEBOX,   RESIZE_LRTB,    14,   245,   256,   282,   293, 0x0,              STR_RESIZE_BUTTON},
{   WIDGETS_END},
};

static const Widget _save_dialog_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,    14,     0,    10,     0,    13, STR_00C5,         STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,  RESIZE_RIGHT,    14,    11,   256,     0,    13, STR_NULL,         STR_018C_WINDOW_TITLE_DRAG_THIS},
{ WWT_PUSHTXTBTN,   RESIZE_NONE,    14,     0,   127,    14,    25, STR_SORT_BY_NAME, STR_SORT_ORDER_TIP},
{ WWT_PUSHTXTBTN,   RESIZE_NONE,    14,   128,   256,    14,    25, STR_SORT_BY_DATE, STR_SORT_ORDER_TIP},
{      WWT_PANEL,  RESIZE_RIGHT,    14,     0,   256,    26,    47, 0x0,              STR_NULL},
{      WWT_PANEL,     RESIZE_RB,    14,     0,   256,    48,   291, 0x0,              STR_NULL},
{ WWT_PUSHIMGBTN,     RESIZE_LR,    14,   245,   256,    48,    59, SPR_HOUSE_ICON,   STR_SAVELOAD_HOME_BUTTON},
{      WWT_INSET,     RESIZE_RB,    14,     2,   243,    50,   290, 0x0,              STR_400A_LIST_OF_DRIVES_DIRECTORIES},
{  WWT_SCROLLBAR,    RESIZE_LRB,    14,   245,   256,    60,   291, 0x0,              STR_0190_SCROLL_BAR_SCROLLS_LIST},
{      WWT_PANEL,    RESIZE_RTB,    14,     0,   256,   292,   307, 0x0,              STR_NULL},
{      WWT_PANEL,    RESIZE_RTB,    14,     2,   254,   294,   305, 0x0,              STR_400B_CURRENTLY_SELECTED_NAME},
{ WWT_PUSHTXTBTN,     RESIZE_TB,    14,     0,   127,   308,   319, STR_4003_DELETE,  STR_400C_DELETE_THE_CURRENTLY_SELECTED},
{ WWT_PUSHTXTBTN,     RESIZE_TB,    14,   128,   244,   308,   319, STR_4002_SAVE,    STR_400D_SAVE_THE_CURRENT_GAME_USING},
{  WWT_RESIZEBOX,   RESIZE_LRTB,    14,   245,   256,   308,   319, 0x0,              STR_RESIZE_BUTTON},
{   WIDGETS_END},
};

// Colors for fios types
const byte _fios_colors[] = {13, 9, 9, 6, 5, 6, 5, 6, 6, 8};

void BuildFileList(void)
{
	_fios_path_changed = true;
	FiosFreeSavegameList();

	switch (_saveload_mode) {
		case SLD_NEW_GAME:
		case SLD_LOAD_SCENARIO:
		case SLD_SAVE_SCENARIO:
			_fios_list = FiosGetScenarioList(_saveload_mode); break;
		case SLD_LOAD_HEIGHTMAP:
			_fios_list = FiosGetHeightmapList(_saveload_mode); break;

		default: _fios_list = FiosGetSavegameList(_saveload_mode); break;
	}
}

static void DrawFiosTexts(uint maxw)
{
	static const char *path = NULL;
	static StringID str = STR_4006_UNABLE_TO_READ_DRIVE;
	static uint32 tot = 0;

	if (_fios_path_changed) {
		str = FiosGetDescText(&path, &tot);
		_fios_path_changed = false;
	}

	if (str != STR_4006_UNABLE_TO_READ_DRIVE) SetDParam(0, tot);
	DrawString(2, 37, str, 0);
	DoDrawStringTruncated(path, 2, 27, 16, maxw);
}

static void MakeSortedSaveGameList(void)
{
	uint sort_start = 0;
	uint sort_end = 0;
	uint s_amount;
	int i;

	/* Directories are always above the files (FIOS_TYPE_DIR)
	 * Drives (A:\ (windows only) are always under the files (FIOS_TYPE_DRIVE)
	 * Only sort savegames/scenarios, not directories
	 */
	for (i = 0; i < _fios_num; i++) {
		switch (_fios_list[i].type) {
			case FIOS_TYPE_DIR:    sort_start++; break;
			case FIOS_TYPE_PARENT: sort_start++; break;
			case FIOS_TYPE_DRIVE:  sort_end++;   break;
		}
	}

	s_amount = _fios_num - sort_start - sort_end;
	if (s_amount > 0)
		qsort(_fios_list + sort_start, s_amount, sizeof(FiosItem), compare_FiosItems);
}

static void GenerateFileName(void)
{
	/* Check if we are not a specatator who wants to generate a name..
	    Let's use the name of player #0 for now. */
	const Player *p = GetPlayer(IsValidPlayer(_local_player) ? _local_player : 0);

	SetDParam(0, p->name_1);
	SetDParam(1, p->name_2);
	SetDParam(2, _date);
	GetString(_edit_str_buf, STR_4004, lastof(_edit_str_buf));
	SanitizeFilename(_edit_str_buf);
}

extern void StartupEngines(void);

static void SaveLoadDlgWndProc(Window *w, WindowEvent *e)
{
	static FiosItem o_dir;

	switch (e->event) {
	case WE_CREATE: { /* Set up OPENTTD button */
		o_dir.type = FIOS_TYPE_DIRECT;
		switch (_saveload_mode) {
			case SLD_SAVE_GAME:
			case SLD_LOAD_GAME:
				ttd_strlcpy(&o_dir.name[0], _paths.save_dir, sizeof(o_dir.name));
				break;

			case SLD_SAVE_SCENARIO:
			case SLD_LOAD_SCENARIO:
				ttd_strlcpy(&o_dir.name[0], _paths.scenario_dir, sizeof(o_dir.name));
				break;

			case SLD_LOAD_HEIGHTMAP:
				ttd_strlcpy(&o_dir.name[0], _paths.heightmap_dir, sizeof(o_dir.name));
				break;

			default:
				ttd_strlcpy(&o_dir.name[0], _paths.personal_dir, sizeof(o_dir.name));
		}
		break;
		}

	case WE_PAINT: {
		int pos;
		int y;

		SetVScrollCount(w, _fios_num);
		DrawWindowWidgets(w);
		DrawFiosTexts(w->width);

		if (_savegame_sort_dirty) {
			_savegame_sort_dirty = false;
			MakeSortedSaveGameList();
		}

		GfxFillRect(w->widget[7].left + 1, w->widget[7].top + 1, w->widget[7].right, w->widget[7].bottom, 0xD7);
		DoDrawString(
			_savegame_sort_order & SORT_DESCENDING ? DOWNARROW : UPARROW,
			_savegame_sort_order & SORT_BY_NAME ? w->widget[2].right - 9 : w->widget[3].right - 9,
			15, 16
		);

		y = w->widget[7].top + 1;
		for (pos = w->vscroll.pos; pos < _fios_num; pos++) {
			const FiosItem *item = _fios_list + pos;

			DoDrawStringTruncated(item->title, 4, y, _fios_colors[item->type], w->width - 18);
			y += 10;
			if (y >= w->vscroll.cap * 10 + w->widget[7].top + 1) break;
		}

		if (_saveload_mode == SLD_SAVE_GAME || _saveload_mode == SLD_SAVE_SCENARIO) {
			DrawEditBox(w, &WP(w,querystr_d), 10);
		}
		break;
	}

	case WE_CLICK:
		switch (e->we.click.widget) {
		case 2: /* Sort save names by name */
			_savegame_sort_order = (_savegame_sort_order == SORT_BY_NAME) ?
				SORT_BY_NAME | SORT_DESCENDING : SORT_BY_NAME;
			_savegame_sort_dirty = true;
			SetWindowDirty(w);
			break;

		case 3: /* Sort save names by date */
			_savegame_sort_order = (_savegame_sort_order == SORT_BY_DATE) ?
				SORT_BY_DATE | SORT_DESCENDING : SORT_BY_DATE;
			_savegame_sort_dirty = true;
			SetWindowDirty(w);
			break;

		case 6: /* OpenTTD 'button', jumps to OpenTTD directory */
			FiosBrowseTo(&o_dir);
			SetWindowDirty(w);
			BuildFileList();
			break;

		case 7: { /* Click the listbox */
			int y = (e->we.click.pt.y - w->widget[e->we.click.widget].top - 1) / 10;
			char *name;
			const FiosItem *file;

			if (y < 0 || (y += w->vscroll.pos) >= w->vscroll.count) return;

			file = _fios_list + y;

			name = FiosBrowseTo(file);
			if (name != NULL) {
				if (_saveload_mode == SLD_LOAD_GAME || _saveload_mode == SLD_LOAD_SCENARIO) {
					_switch_mode = (_game_mode == GM_EDITOR) ? SM_LOAD_SCENARIO : SM_LOAD;

					SetFiosType(file->type);
					ttd_strlcpy(_file_to_saveload.name, name, sizeof(_file_to_saveload.name));
					ttd_strlcpy(_file_to_saveload.title, file->title, sizeof(_file_to_saveload.title));

					DeleteWindow(w);
				} else if (_saveload_mode == SLD_LOAD_HEIGHTMAP) {
					SetFiosType(file->type);
					ttd_strlcpy(_file_to_saveload.name, name, sizeof(_file_to_saveload.name));
					ttd_strlcpy(_file_to_saveload.title, file->title, sizeof(_file_to_saveload.title));

					DeleteWindow(w);
					ShowHeightmapLoad();
				} else {
					// SLD_SAVE_GAME, SLD_SAVE_SCENARIO copy clicked name to editbox
					ttd_strlcpy(WP(w, querystr_d).text.buf, file->title, WP(w, querystr_d).text.maxlength);
					UpdateTextBufferSize(&WP(w, querystr_d).text);
					InvalidateWidget(w, 10);
				}
			} else {
				// Changed directory, need repaint.
				SetWindowDirty(w);
				BuildFileList();
			}
			break;
		}

		case 11: case 12: /* Delete, Save game */
			break;
		}
		break;
	case WE_MOUSELOOP:
		if (_saveload_mode == SLD_SAVE_GAME || _saveload_mode == SLD_SAVE_SCENARIO) {
			HandleEditBox(w, &WP(w, querystr_d), 10);
		}
		break;
	case WE_KEYPRESS:
		if (e->we.keypress.keycode == WKC_ESC) {
			DeleteWindow(w);
			return;
		}

		if (_saveload_mode == SLD_SAVE_GAME || _saveload_mode == SLD_SAVE_SCENARIO) {
			if (HandleEditBoxKey(w, &WP(w, querystr_d), 10, e) == 1) /* Press Enter */
					HandleButtonClick(w, 12);
		}
		break;
	case WE_TIMEOUT:
		/* This test protects against using widgets 11 and 12 which are only available
		 * in those two saveload mode  */
		if (!(_saveload_mode == SLD_SAVE_GAME || _saveload_mode == SLD_SAVE_SCENARIO)) break;

		if (IsWindowWidgetLowered(w, 11)) { /* Delete button clicked */
			if (!FiosDelete(WP(w,querystr_d).text.buf)) {
				ShowErrorMessage(INVALID_STRING_ID, STR_4008_UNABLE_TO_DELETE_FILE, 0, 0);
			} else {
				BuildFileList();
				/* Reset file name to current date on successfull delete */
				if (_saveload_mode == SLD_SAVE_GAME) GenerateFileName();
			}

			UpdateTextBufferSize(&WP(w, querystr_d).text);
			SetWindowDirty(w);
		} else if (IsWindowWidgetLowered(w, 12)) { /* Save button clicked */
			_switch_mode = SM_SAVE;
			FiosMakeSavegameName(_file_to_saveload.name, WP(w,querystr_d).text.buf, sizeof(_file_to_saveload.name));

			/* In the editor set up the vehicle engines correctly (date might have changed) */
			if (_game_mode == GM_EDITOR) StartupEngines();
		}
		break;
	case WE_DESTROY:
		// pause is only used in single-player, non-editor mode, non menu mode
		if (!_networking && _game_mode != GM_EDITOR && _game_mode != GM_MENU) {
			DoCommandP(0, 0, 0, NULL, CMD_PAUSE);
		}
		FiosFreeSavegameList();
		CLRBIT(_no_scroll, SCROLL_SAVE);
		break;
	case WE_RESIZE: {
		/* Widget 2 and 3 have to go with halve speed, make it so obiwan */
		uint diff = e->we.sizing.diff.x / 2;
		w->widget[2].right += diff;
		w->widget[3].left  += diff;
		w->widget[3].right += e->we.sizing.diff.x;

		/* Same for widget 11 and 12 in save-dialog */
		if (_saveload_mode == SLD_SAVE_GAME || _saveload_mode == SLD_SAVE_SCENARIO) {
			w->widget[11].right += diff;
			w->widget[12].left  += diff;
			w->widget[12].right += e->we.sizing.diff.x;
		}

		w->vscroll.cap += e->we.sizing.diff.y / 10;
		} break;
	}
}

static const WindowDesc _load_dialog_desc = {
	WDP_CENTER, WDP_CENTER, 257, 294,
	WC_SAVELOAD,0,
	WDF_STD_TOOLTIPS | WDF_DEF_WIDGET | WDF_STD_BTN | WDF_UNCLICK_BUTTONS | WDF_RESIZABLE,
	_load_dialog_widgets,
	SaveLoadDlgWndProc,
};

static const WindowDesc _save_dialog_desc = {
	WDP_CENTER, WDP_CENTER, 257, 320,
	WC_SAVELOAD,0,
	WDF_STD_TOOLTIPS | WDF_DEF_WIDGET | WDF_STD_BTN | WDF_UNCLICK_BUTTONS | WDF_RESIZABLE,
	_save_dialog_widgets,
	SaveLoadDlgWndProc,
};

void ShowSaveLoadDialog(int mode)
{
	static const StringID saveload_captions[] = {
		STR_4001_LOAD_GAME,
		STR_0298_LOAD_SCENARIO,
		STR_4000_SAVE_GAME,
		STR_0299_SAVE_SCENARIO,
		STR_4011_LOAD_HEIGHTMAP,
	};

	Window *w;
	const WindowDesc *sld = &_save_dialog_desc;


	SetObjectToPlace(SPR_CURSOR_ZZZ, 0, 0, 0);
	DeleteWindowById(WC_QUERY_STRING, 0);
	DeleteWindowById(WC_SAVELOAD, 0);

	_saveload_mode = mode;
	SETBIT(_no_scroll, SCROLL_SAVE);

	switch (mode) {
		case SLD_SAVE_GAME:     GenerateFileName(); break;
		case SLD_SAVE_SCENARIO: strcpy(_edit_str_buf, "UNNAMED"); break;
		default:                sld = &_load_dialog_desc; break;
	}

	assert((uint)mode < lengthof(saveload_captions));
	w = AllocateWindowDesc(sld);
	w->widget[1].data = saveload_captions[mode];
	w->vscroll.cap = 24;
	w->resize.step_width = 2;
	w->resize.step_height = 10;
	w->resize.height = w->height - 14 * 10; // Minimum of 10 items
	LowerWindowWidget(w, 7);

	WP(w, querystr_d).afilter = CS_ALPHANUMERAL;
	InitializeTextBuffer(&WP(w, querystr_d).text, _edit_str_buf, lengthof(_edit_str_buf), 240);

	// pause is only used in single-player, non-editor mode, non-menu mode. It
	// will be unpaused in the WE_DESTROY event handler.
	if (_game_mode != GM_MENU && !_networking && _game_mode != GM_EDITOR) {
		DoCommandP(0, 1, 0, NULL, CMD_PAUSE);
	}

	BuildFileList();

#ifdef PSP
    switch (mode) {
        case SLD_SAVE_GAME:
            ResizeWindow(w, 0, -86);
            w->vscroll.cap += -86 / 10;
            w->top += 35;
            break;
        case SLD_SAVE_SCENARIO:
            ResizeWindow(w, 0, -86);
            w->vscroll.cap += -86 / 10;
            w->top += 43;
            break;
        default:
            ResizeWindow(w, 0, -60);
            w->vscroll.cap += -60 / 10;
            w->top += 30;
    }
#endif
	ResetObjectToPlace();
}

void RedrawAutosave(void)
{
	SetWindowDirty(FindWindowById(WC_STATUS_BAR, 0));
}

void SetFiosType(const byte fiostype)
{
	switch (fiostype) {
		case FIOS_TYPE_FILE:
		case FIOS_TYPE_SCENARIO:
			_file_to_saveload.mode = SL_LOAD;
			break;

		case FIOS_TYPE_OLDFILE:
		case FIOS_TYPE_OLD_SCENARIO:
			_file_to_saveload.mode = SL_OLD_LOAD;
			break;

#ifdef WITH_PNG
		case FIOS_TYPE_PNG:
			_file_to_saveload.mode = SL_PNG;
			break;
#endif /* WITH_PNG */

		case FIOS_TYPE_BMP:
			_file_to_saveload.mode = SL_BMP;
			break;

		default:
			_file_to_saveload.mode = SL_INVALID;
			break;
	}
}

static int32 ClickMoneyCheat(int32 p1, int32 p2)
{
		DoCommandP(0, -10000000, 0, NULL, CMD_MONEY_CHEAT);
		return true;
}

// p1 player to set to, p2 is -1 or +1 (down/up)
static int32 ClickChangePlayerCheat(int32 p1, int32 p2)
{
	while (IsValidPlayer((PlayerID)p1)) {
		if (_players[p1].is_active) {
			SetLocalPlayer((PlayerID)p1);

			MarkWholeScreenDirty();
			return _local_player;
		}
		p1 += p2;
	}

	return _local_player;
}

// p1 -1 or +1 (down/up)
static int32 ClickChangeClimateCheat(int32 p1, int32 p2)
{
	if (p1 == -1) p1 = 3;
	if (p1 ==  4) p1 = 0;
	_opt.landscape = p1;
	ReloadNewGRFData();
	return _opt.landscape;
}

extern void EnginesMonthlyLoop(void);

// p2 1 (increase) or -1 (decrease)
static int32 ClickChangeDateCheat(int32 p1, int32 p2)
{
	YearMonthDay ymd;
	ConvertDateToYMD(_date, &ymd);

	if ((ymd.year == MIN_YEAR && p2 == -1) || (ymd.year == MAX_YEAR && p2 == 1)) return _cur_year;

	SetDate(ConvertYMDToDate(_cur_year + p2, ymd.month, ymd.day));
	EnginesMonthlyLoop();
	SetWindowDirty(FindWindowById(WC_STATUS_BAR, 0));
	return _cur_year;
}

typedef int32 CheckButtonClick(int32, int32);

enum ce_flags {CE_CLICK = 1 << 0};

typedef byte ce_flags;

typedef struct CheatEntry {
	VarType type;          // type of selector
	ce_flags flags;        // selector flags
	StringID str;          // string with descriptive text
	void *variable;        // pointer to the variable
	bool *been_used;       // has this cheat been used before?
	CheckButtonClick *proc;// procedure
	int16 min, max;        // range for spinbox setting
} CheatEntry;

static const CheatEntry _cheats_ui[] = {
	{SLE_BOOL,CE_CLICK, STR_CHEAT_MONEY,          &_cheats.money.value,           &_cheats.money.been_used,           &ClickMoneyCheat,         0,  0},
	{SLE_UINT8,      0, STR_CHEAT_CHANGE_PLAYER,  &_local_player,                 &_cheats.switch_player.been_used,   &ClickChangePlayerCheat,  0, 11},
	{SLE_BOOL,       0, STR_CHEAT_EXTRA_DYNAMITE, &_cheats.magic_bulldozer.value, &_cheats.magic_bulldozer.been_used, NULL,                     0,  0},
	{SLE_BOOL,       0, STR_CHEAT_CROSSINGTUNNELS,&_cheats.crossing_tunnels.value,&_cheats.crossing_tunnels.been_used,NULL,                     0,  0},
	{SLE_BOOL,       0, STR_CHEAT_BUILD_IN_PAUSE, &_cheats.build_in_pause.value,  &_cheats.build_in_pause.been_used,  NULL,                     0,  0},
	{SLE_BOOL,       0, STR_CHEAT_NO_JETCRASH,    &_cheats.no_jetcrash.value,     &_cheats.no_jetcrash.been_used,     NULL,                     0,  0},
	{SLE_BOOL,       0, STR_CHEAT_SETUP_PROD,     &_cheats.setup_prod.value,      &_cheats.setup_prod.been_used,      NULL,                     0,  0},
	{SLE_UINT8,      0, STR_CHEAT_SWITCH_CLIMATE, &_opt.landscape,                &_cheats.switch_climate.been_used,  &ClickChangeClimateCheat,-1,  4},
	{SLE_INT32,      0, STR_CHEAT_CHANGE_DATE,    &_cur_year,                     &_cheats.change_date.been_used,     &ClickChangeDateCheat,   -1,  1},
};


static const Widget _cheat_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,    14,     0,    10,     0,    13, STR_00C5,   STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,    14,    11,   399,     0,    13, STR_CHEATS, STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,   RESIZE_NONE,    14,     0,   399,    14,   169, 0x0,        STR_NULL},
{      WWT_PANEL,   RESIZE_NONE,    14,     0,   399,    14,   169, 0x0,        STR_CHEATS_TIP},
{   WIDGETS_END},
};

extern void DrawPlayerIcon(PlayerID pid, int x, int y);

static void CheatsWndProc(Window *w, WindowEvent *e)
{
	switch (e->event) {
	case WE_PAINT: {
		int clk = WP(w,def_d).data_1;
		int x, y;
		int i;

		DrawWindowWidgets(w);

		DrawStringMultiCenter(200, 25, STR_CHEATS_WARNING, 350);

		x = 0;
		y = 45;

		for (i = 0; i != lengthof(_cheats_ui); i++) {
			const CheatEntry *ce = &_cheats_ui[i];

			DrawSprite((*ce->been_used) ? SPR_BOX_CHECKED : SPR_BOX_EMPTY, x + 5, y + 2);

			switch (ce->type) {
			case SLE_BOOL: {
				bool on = (*(bool*)ce->variable);

				if (ce->flags & CE_CLICK) {
					DrawFrameRect(x + 20, y + 1, x + 30 + 9, y + 9, 0, (clk - (i * 2) == 1) ? FR_LOWERED : 0);
					if (i == 0) { // XXX - hack/hack for first element which is increase money. Told ya it's a mess
						SetDParam64(0, 10000000);
					} else {
						SetDParam(0, false);
					}
				} else {
					DrawFrameRect(x + 20, y + 1, x + 30 + 9, y + 9, on ? 6 : 4, on ? FR_LOWERED : 0);
					SetDParam(0, on ? STR_CONFIG_PATCHES_ON : STR_CONFIG_PATCHES_OFF);
				}
			} break;
			default: {
				int32 val = (int32)ReadValue(ce->variable, ce->type);
				char buf[512];

				/* Draw [<][>] boxes for settings of an integer-type */
				DrawArrowButtons(x + 20, y, 3, clk - (i * 2), true, true);

				switch (ce->str) {
				/* Display date for change date cheat */
				case STR_CHEAT_CHANGE_DATE: SetDParam(0, _date); break;
				/* Draw colored flag for change player cheat */
				case STR_CHEAT_CHANGE_PLAYER:
					SetDParam(0, val);
					GetString(buf, STR_CHEAT_CHANGE_PLAYER, lastof(buf));
					DrawPlayerIcon(_current_player, 60 + GetStringBoundingBox(buf).width, y + 2);
					break;
				/* Set correct string for switch climate cheat */
				case STR_CHEAT_SWITCH_CLIMATE: val += STR_TEMPERATE_LANDSCAPE;
				/* Fallthrough */
				default: SetDParam(0, val);
				}
			} break;
			}

			DrawString(50, y + 1, ce->str, 0);

			y += 12;
		}
		break;
	}

	case WE_CLICK: {
			const CheatEntry *ce;
			uint btn = (e->we.click.pt.y - 46) / 12;
			int32 value, oldvalue;
			uint x = e->we.click.pt.x;

			// not clicking a button?
			if (!IS_INT_INSIDE(x, 20, 40) || btn >= lengthof(_cheats_ui)) break;

			ce = &_cheats_ui[btn];
			oldvalue = value = (int32)ReadValue(ce->variable, ce->type);

			*ce->been_used = true;

			switch (ce->type) {
			case SLE_BOOL:
				if (ce->flags & CE_CLICK) WP(w,def_d).data_1 = btn * 2 + 1;
				value ^= 1;
				if (ce->proc != NULL) ce->proc(value, 0);
				break;
			default: {
				/* Add a dynamic step-size to the scroller. In a maximum of
				 * 50-steps you should be able to get from min to max */
				uint16 step = ((ce->max - ce->min) / 20);
				if (step == 0) step = 1;

				/* Increase or decrease the value and clamp it to extremes */
				value += (x >= 30) ? step : -step;
				value = clamp(value, ce->min, ce->max);

				// take whatever the function returns
				value = ce->proc(value, (x >= 30) ? 1 : -1);

				if (value != oldvalue) {
					WP(w,def_d).data_1 = btn * 2 + 1 + ((x >= 30) ? 1 : 0);
				}
			} break;
			}

			if (value != oldvalue) {
				WriteValue(ce->variable, ce->type, (int64)value);
				SetWindowDirty(w);
			}

			w->flags4 |= 5 << WF_TIMEOUT_SHL;

			SetWindowDirty(w);
		}
		break;
	case WE_TIMEOUT:
		WP(w,def_d).data_1 = 0;
		SetWindowDirty(w);
		break;
	}
}

static const WindowDesc _cheats_desc = {
	240, 22, 400, 170,
	WC_CHEATS,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_cheat_widgets,
	CheatsWndProc
};


void ShowCheatWindow(void)
{
	DeleteWindowById(WC_CHEATS, 0);
	AllocateWindowDesc(&_cheats_desc);
}
