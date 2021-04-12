/* $Id: dock_gui.c 11074 2007-09-09 20:52:54Z rubidium $ */

#include "stdafx.h"
#include "openttd.h"
#include "table/sprites.h"
#include "table/strings.h"
#include "functions.h"
#include "map.h"
#include "window.h"
#include "station.h"
#include "gui.h"
#include "viewport.h"
#include "gfx.h"
#include "sound.h"
#include "command.h"
#include "variables.h"

static void ShowBuildDockStationPicker(void);
static void ShowBuildDocksDepotPicker(void);

static Axis _ship_depot_direction;

void CcBuildDocks(bool success, TileIndex tile, uint32 p1, uint32 p2)
{
	if (success) {
		SndPlayTileFx(SND_02_SPLAT, tile);
		ResetObjectToPlace();
	}
}

void CcBuildCanal(bool success, TileIndex tile, uint32 p1, uint32 p2)
{
	if (success) SndPlayTileFx(SND_02_SPLAT, tile);
}


static void PlaceDocks_Dock(TileIndex tile)
{
	DoCommandP(tile, 0, 0, CcBuildDocks, CMD_BUILD_DOCK | CMD_MSG(STR_9802_CAN_T_BUILD_DOCK_HERE));
}

static void PlaceDocks_Depot(TileIndex tile)
{
	DoCommandP(tile, _ship_depot_direction, 0, CcBuildDocks, CMD_BUILD_SHIP_DEPOT | CMD_MSG(STR_3802_CAN_T_BUILD_SHIP_DEPOT));
}

static void PlaceDocks_Buoy(TileIndex tile)
{
	DoCommandP(tile, 0, 0, CcBuildDocks, CMD_BUILD_BUOY | CMD_MSG(STR_9835_CAN_T_POSITION_BUOY_HERE));
}

static void PlaceDocks_DemolishArea(TileIndex tile)
{
	VpStartPlaceSizing(tile, VPM_X_AND_Y | GUI_PlaceProc_DemolishArea);
}

static void PlaceDocks_BuildCanal(TileIndex tile)
{
	VpStartPlaceSizing(tile, VPM_X_OR_Y);
}

static void PlaceDocks_BuildLock(TileIndex tile)
{
	DoCommandP(tile, 0, 0, CcBuildDocks, CMD_BUILD_LOCK | CMD_MSG(STR_CANT_BUILD_LOCKS));
}


enum {
	DTW_CANAL    = 3,
	DTW_LOCK     = 4,
	DTW_DEMOLISH = 6,
	DTW_DEPOT    = 7,
	DTW_STATION  = 8,
	DTW_BUOY     = 9
};


static void BuildDocksClick_Canal(Window *w)
{
	HandlePlacePushButton(w, DTW_CANAL, SPR_CURSOR_CANAL, 1, PlaceDocks_BuildCanal);
}

static void BuildDocksClick_Lock(Window *w)
{
	HandlePlacePushButton(w, DTW_LOCK, SPR_CURSOR_LOCK, 1, PlaceDocks_BuildLock);
}

static void BuildDocksClick_Demolish(Window *w)
{
	HandlePlacePushButton(w, DTW_DEMOLISH, ANIMCURSOR_DEMOLISH, 1, PlaceDocks_DemolishArea);
}

static void BuildDocksClick_Depot(Window *w)
{
	if (HandlePlacePushButton(w, DTW_DEPOT, SPR_CURSOR_SHIP_DEPOT, 1, PlaceDocks_Depot)) ShowBuildDocksDepotPicker();
}

static void BuildDocksClick_Dock(Window *w)
{
	if (HandlePlacePushButton(w, DTW_STATION, SPR_CURSOR_DOCK, 3, PlaceDocks_Dock)) ShowBuildDockStationPicker();
}

static void BuildDocksClick_Buoy(Window *w)
{
	HandlePlacePushButton(w, DTW_BUOY, SPR_CURSOR_BOUY, 1, PlaceDocks_Buoy);
}

static void BuildDocksClick_Landscaping(Window *w)
{
	ShowTerraformToolbar();
}

typedef void OnButtonClick(Window *w);
static OnButtonClick * const _build_docks_button_proc[] = {
	BuildDocksClick_Canal,
	BuildDocksClick_Lock,
	NULL,
	BuildDocksClick_Demolish,
	BuildDocksClick_Depot,
	BuildDocksClick_Dock,
	BuildDocksClick_Buoy,
	BuildDocksClick_Landscaping,
};

static void BuildDocksToolbWndProc(Window *w, WindowEvent *e)
{
	switch (e->event) {
	case WE_PAINT:
		DrawWindowWidgets(w);
		break;

	case WE_CLICK:
		if (e->we.click.widget - 3 >= 0 && e->we.click.widget != 5) _build_docks_button_proc[e->we.click.widget - 3](w);
		break;

	case WE_KEYPRESS:
		switch (e->we.keypress.keycode) {
			case '1': BuildDocksClick_Canal(w); break;
			case '2': BuildDocksClick_Lock(w); break;
			case '3': BuildDocksClick_Demolish(w); break;
			case '4': BuildDocksClick_Depot(w); break;
			case '5': BuildDocksClick_Dock(w); break;
			case '6': BuildDocksClick_Buoy(w); break;
			case 'l': BuildDocksClick_Landscaping(w); break;
			default:  return;
		}
		break;

	case WE_PLACE_OBJ:
		_place_proc(e->we.place.tile);
		break;

	case WE_PLACE_DRAG: {
		VpSelectTilesWithMethod(e->we.place.pt.x, e->we.place.pt.y, e->we.place.userdata);
		return;
	}

	case WE_PLACE_MOUSEUP:
		if (e->we.place.pt.x != -1) {
			if ((e->we.place.userdata & 0xF) == VPM_X_AND_Y) { // dragged actions
				GUIPlaceProcDragXY(e);
			} else if (e->we.place.userdata == VPM_X_OR_Y) {
				DoCommandP(e->we.place.tile, e->we.place.starttile, 0, CcBuildCanal, CMD_BUILD_CANAL | CMD_MSG(STR_CANT_BUILD_CANALS));
			}
		}
		break;

	case WE_ABORT_PLACE_OBJ:
		RaiseWindowButtons(w);

		w = FindWindowById(WC_BUILD_STATION, 0);
		if (w != NULL) WP(w,def_d).close = true;

		w = FindWindowById(WC_BUILD_DEPOT, 0);
		if (w != NULL) WP(w,def_d).close = true;
		break;

	case WE_PLACE_PRESIZE: {
		TileIndex tile_from;
		TileIndex tile_to;

		tile_from = tile_to = e->we.place.tile;
		switch (GetTileSlope(tile_from, NULL)) {
			case SLOPE_SW: tile_to += TileDiffXY(-1,  0); break;
			case SLOPE_SE: tile_to += TileDiffXY( 0, -1); break;
			case SLOPE_NW: tile_to += TileDiffXY( 0,  1); break;
			case SLOPE_NE: tile_to += TileDiffXY( 1,  0); break;
			default: break;
		}
		VpSetPresizeRange(tile_from, tile_to);
	} break;

	case WE_DESTROY:
		if (_patches.link_terraform_toolbar) DeleteWindowById(WC_SCEN_LAND_GEN, 0);
		break;
	}
}

static const Widget _build_docks_toolb_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,     7,     0,    10,     0,    13, STR_00C5,                   STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,     7,    11,   145,     0,    13, STR_9801_DOCK_CONSTRUCTION, STR_018C_WINDOW_TITLE_DRAG_THIS},
{  WWT_STICKYBOX,   RESIZE_NONE,     7,   146,   157,     0,    13, 0x0,                        STR_STICKY_BUTTON},
{     WWT_IMGBTN,   RESIZE_NONE,     7,     0,    21,    14,    35, SPR_IMG_BUILD_CANAL,        STR_BUILD_CANALS_TIP},
{     WWT_IMGBTN,   RESIZE_NONE,     7,    22,    43,    14,    35, SPR_IMG_BUILD_LOCK,         STR_BUILD_LOCKS_TIP},

{      WWT_PANEL,   RESIZE_NONE,     7,    44,    47,    14,    35, 0x0,                        STR_NULL},

{     WWT_IMGBTN,   RESIZE_NONE,     7,    48,    69,    14,    35, SPR_IMG_DYNAMITE,           STR_018D_DEMOLISH_BUILDINGS_ETC},
{     WWT_IMGBTN,   RESIZE_NONE,     7,    70,    91,    14,    35, SPR_IMG_SHIP_DEPOT,         STR_981E_BUILD_SHIP_DEPOT_FOR_BUILDING},
{     WWT_IMGBTN,   RESIZE_NONE,     7,    92,   113,    14,    35, SPR_IMG_SHIP_DOCK,          STR_981D_BUILD_SHIP_DOCK},
{     WWT_IMGBTN,   RESIZE_NONE,     7,   114,   135,    14,    35, SPR_IMG_BOUY,               STR_9834_POSITION_BUOY_WHICH_CAN},
{     WWT_IMGBTN,   RESIZE_NONE,     7,   136,   157,    14,    35, SPR_IMG_LANDSCAPING,        STR_LANDSCAPING_TOOLBAR_TIP},
{   WIDGETS_END},
};

static const WindowDesc _build_docks_toolbar_desc = {
	WDP_ALIGN_TBR, 22, 158, 36,
	WC_BUILD_TOOLBAR, 0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_STICKY_BUTTON,
	_build_docks_toolb_widgets,
	BuildDocksToolbWndProc
};

void ShowBuildDocksToolbar(void)
{
	if (!IsValidPlayer(_current_player)) return;

	DeleteWindowById(WC_BUILD_TOOLBAR, 0);
	AllocateWindowDesc(&_build_docks_toolbar_desc);
	if (_patches.link_terraform_toolbar) ShowTerraformToolbar();
}

static void BuildDockStationWndProc(Window *w, WindowEvent *e)
{
	switch (e->event) {
	case WE_CREATE: LowerWindowWidget(w, _station_show_coverage + 3); break;

	case WE_PAINT: {
		int rad = (_patches.modified_catchment) ? CA_DOCK : 4;

		if (WP(w,def_d).close) return;
		DrawWindowWidgets(w);

		if (_station_show_coverage) {
			SetTileSelectBigSize(-rad, -rad, 2 * rad, 2 * rad);
		} else {
			SetTileSelectSize(1, 1);
		}

		DrawStationCoverageAreaText(4, 50, (uint)-1, rad);
		break;
	}

	case WE_CLICK:
		switch (e->we.click.widget) {
			case 3:
			case 4:
				RaiseWindowWidget(w, _station_show_coverage + 3);
				_station_show_coverage = e->we.click.widget - 3;
				LowerWindowWidget(w, _station_show_coverage + 3);
				SndPlayFx(SND_15_BEEP);
				SetWindowDirty(w);
				break;
		}
		break;

	case WE_MOUSELOOP:
		if (WP(w,def_d).close) {
			DeleteWindow(w);
			return;
		}

		CheckRedrawStationCoverage(w);
		break;

	case WE_DESTROY:
		if (!WP(w,def_d).close) ResetObjectToPlace();
		break;
	}
}

static const Widget _build_dock_station_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,     7,     0,    10,     0,    13, STR_00C5,                         STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,     7,    11,   147,     0,    13, STR_3068_DOCK,                    STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,   RESIZE_NONE,     7,     0,   147,    14,    74, 0x0,                              STR_NULL},
{    WWT_TEXTBTN,   RESIZE_NONE,    14,    14,    73,    30,    40, STR_02DB_OFF,                     STR_3065_DON_T_HIGHLIGHT_COVERAGE},
{    WWT_TEXTBTN,   RESIZE_NONE,    14,    74,   133,    30,    40, STR_02DA_ON,                      STR_3064_HIGHLIGHT_COVERAGE_AREA},
{      WWT_LABEL,   RESIZE_NONE,     7,     0,   147,    17,    30, STR_3066_COVERAGE_AREA_HIGHLIGHT, STR_NULL},
{   WIDGETS_END},
};

static const WindowDesc _build_dock_station_desc = {
	WDP_AUTO, WDP_AUTO, 148, 75,
	WC_BUILD_STATION, WC_BUILD_TOOLBAR,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_dock_station_widgets,
	BuildDockStationWndProc
};

static void ShowBuildDockStationPicker(void)
{
	AllocateWindowDesc(&_build_dock_station_desc);
}

static void UpdateDocksDirection(void)
{
	if (_ship_depot_direction != AXIS_X) {
		SetTileSelectSize(1, 2);
	} else {
		SetTileSelectSize(2, 1);
	}
}

static void BuildDocksDepotWndProc(Window *w, WindowEvent *e)
{
	switch (e->event) {
	case WE_CREATE: LowerWindowWidget(w, _ship_depot_direction + 3); break;

	case WE_PAINT:
		DrawWindowWidgets(w);

		DrawShipDepotSprite(67, 35, 0);
		DrawShipDepotSprite(35, 51, 1);
		DrawShipDepotSprite(135, 35, 2);
		DrawShipDepotSprite(167, 51, 3);
		return;

	case WE_CLICK: {
		switch (e->we.click.widget) {
		case 3:
		case 4:
			RaiseWindowWidget(w, _ship_depot_direction + 3);
			_ship_depot_direction = e->we.click.widget - 3;
			LowerWindowWidget(w, _ship_depot_direction + 3);
			SndPlayFx(SND_15_BEEP);
			UpdateDocksDirection();
			SetWindowDirty(w);
			break;
		}
	} break;

	case WE_MOUSELOOP:
		if (WP(w,def_d).close) DeleteWindow(w);
		break;

	case WE_DESTROY:
		if (!WP(w,def_d).close) ResetObjectToPlace();
		break;
	}
}

static const Widget _build_docks_depot_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,     7,     0,    10,     0,    13, STR_00C5,                        STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,   RESIZE_NONE,     7,    11,   203,     0,    13, STR_3800_SHIP_DEPOT_ORIENTATION, STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,   RESIZE_NONE,     7,     0,   203,    14,    85, 0x0,                             STR_NULL},
{      WWT_PANEL,   RESIZE_NONE,    14,     3,   100,    17,    82, 0x0,                             STR_3803_SELECT_SHIP_DEPOT_ORIENTATION},
{      WWT_PANEL,   RESIZE_NONE,    14,   103,   200,    17,    82, 0x0,                             STR_3803_SELECT_SHIP_DEPOT_ORIENTATION},
{   WIDGETS_END},
};

static const WindowDesc _build_docks_depot_desc = {
	WDP_AUTO, WDP_AUTO, 204, 86,
	WC_BUILD_DEPOT, WC_BUILD_TOOLBAR,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_docks_depot_widgets,
	BuildDocksDepotWndProc
};


static void ShowBuildDocksDepotPicker(void)
{
	AllocateWindowDesc(&_build_docks_depot_desc);
	UpdateDocksDirection();
}


void InitializeDockGui(void)
{
	_ship_depot_direction = AXIS_X;
}
