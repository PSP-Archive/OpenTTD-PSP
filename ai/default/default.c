/* $Id: default.c 10952 2007-08-20 15:17:24Z glx $ */

#include "../../stdafx.h"
#include "../../openttd.h"
#include "../../aircraft.h"
#include "../../bridge_map.h"
#include "../../functions.h"
#include "../../map.h"
#include "../../rail_map.h"
#include "../../road_map.h"
#include "../../roadveh.h"
#include "../../station_map.h"
#include "../../tile.h"
#include "../../player.h"
#include "../../tunnel_map.h"
#include "../../vehicle.h"
#include "../../engine.h"
#include "../../command.h"
#include "../../town.h"
#include "../../industry.h"
#include "../../station.h"
#include "../../pathfind.h"
#include "../../economy.h"
#include "../../airport.h"
#include "../../depot.h"
#include "../../variables.h"
#include "../../bridge.h"
#include "../../date.h"
#include "default.h"

// remove some day perhaps?
static uint _ai_service_interval;

typedef void AiStateAction(Player *p);

enum {
	AIS_0                            =  0,
	AIS_1                            =  1,
	AIS_VEH_LOOP                     =  2,
	AIS_VEH_CHECK_REPLACE_VEHICLE    =  3,
	AIS_VEH_DO_REPLACE_VEHICLE       =  4,
	AIS_WANT_NEW_ROUTE               =  5,
	AIS_BUILD_DEFAULT_RAIL_BLOCKS    =  6,
	AIS_BUILD_RAIL                   =  7,
	AIS_BUILD_RAIL_VEH               =  8,
	AIS_DELETE_RAIL_BLOCKS           =  9,
	AIS_BUILD_DEFAULT_ROAD_BLOCKS    = 10,
	AIS_BUILD_ROAD                   = 11,
	AIS_BUILD_ROAD_VEHICLES          = 12,
	AIS_DELETE_ROAD_BLOCKS           = 13,
	AIS_AIRPORT_STUFF                = 14,
	AIS_BUILD_DEFAULT_AIRPORT_BLOCKS = 15,
	AIS_BUILD_AIRCRAFT_VEHICLES      = 16,
	AIS_CHECK_SHIP_STUFF             = 17,
	AIS_BUILD_DEFAULT_SHIP_BLOCKS    = 18,
	AIS_DO_SHIP_STUFF                = 19,
	AIS_SELL_VEHICLE                 = 20,
	AIS_REMOVE_STATION               = 21,
	AIS_REMOVE_TRACK                 = 22,
	AIS_REMOVE_SINGLE_RAIL_TILE      = 23
};


#include "../../table/ai_rail.h"

static byte GetRailTrackStatus(TileIndex tile)
{
	uint32 r = GetTileTrackStatus(tile, TRANSPORT_RAIL);
	return (byte) (r | r >> 8);
}


static void AiCase0(Player *p)
{
	p->ai.state = AIS_REMOVE_TRACK;
	p->ai.state_counter = 0;
}

static void AiCase1(Player *p)
{
	p->ai.cur_veh = NULL;
	p->ai.state = AIS_VEH_LOOP;
}

static void AiStateVehLoop(Player *p)
{
	Vehicle *v;
	uint index;

	index = (p->ai.cur_veh == NULL) ? 0 : p->ai.cur_veh->index + 1;

	FOR_ALL_VEHICLES_FROM(v, index) {
		if (v->owner != _current_player) continue;

		if ((v->type == VEH_Train && v->subtype == 0) ||
				v->type == VEH_Road ||
				(v->type == VEH_Aircraft && v->subtype <= 2) ||
				v->type == VEH_Ship) {
			/* replace engine? */
			if (v->type == VEH_Train && v->engine_type < 3 &&
					(_price.build_railvehicle >> 3) < p->player_money) {
				p->ai.state = AIS_VEH_CHECK_REPLACE_VEHICLE;
				p->ai.cur_veh = v;
				return;
			}

			/* not profitable? */
			if (v->age >= 730 &&
					v->profit_last_year < _price.station_value * 5 &&
					v->profit_this_year < _price.station_value * 5) {
				p->ai.state_counter = 0;
				p->ai.state = AIS_SELL_VEHICLE;
				p->ai.cur_veh = v;
				return;
			}

			/* not reliable? */
			if (v->age >= v->max_age || (
						v->age != 0 &&
						GetEngine(v->engine_type)->reliability < 35389
					)) {
				p->ai.state = AIS_VEH_CHECK_REPLACE_VEHICLE;
				p->ai.cur_veh = v;
				return;
			}
		}
	}

	p->ai.state = AIS_WANT_NEW_ROUTE;
	p->ai.state_counter = 0;
}

static EngineID AiChooseTrainToBuild(RailType railtype, int32 money, byte flag, TileIndex tile)
{
	EngineID best_veh_index = INVALID_ENGINE;
	byte best_veh_score = 0;
	int32 ret;
	EngineID i;

	for (i = 0; i < NUM_TRAIN_ENGINES; i++) {
		const RailVehicleInfo *rvi = RailVehInfo(i);
		const Engine* e = GetEngine(i);

		if (!IsCompatibleRail(e->railtype, railtype) ||
				rvi->flags & RVI_WAGON ||
				(rvi->flags & RVI_MULTIHEAD && flag & 1) ||
				!HASBIT(e->player_avail, _current_player) ||
				e->reliability < 0x8A3D) {
			continue;
		}

		ret = DoCommand(tile, i, 0, 0, CMD_BUILD_RAIL_VEHICLE);
		if (!CmdFailed(ret) && ret <= money && rvi->ai_rank >= best_veh_score) {
			best_veh_score = rvi->ai_rank;
			best_veh_index = i;
		}
	}

	return best_veh_index;
}

static EngineID AiChooseRoadVehToBuild(CargoID cargo, int32 money, TileIndex tile)
{
	EngineID best_veh_index = INVALID_ENGINE;
	int32 best_veh_rating = 0;
	EngineID i = ROAD_ENGINES_INDEX;
	EngineID end = i + NUM_ROAD_ENGINES;

	for (; i != end; i++) {
		const RoadVehicleInfo *rvi = RoadVehInfo(i);
		const Engine* e = GetEngine(i);
		int32 rating;
		int32 ret;

		if (!HASBIT(e->player_avail, _current_player) || e->reliability < 0x8A3D) {
			continue;
		}

		/* Skip vehicles which can't take our cargo type */
		if (rvi->cargo_type != cargo && !CanRefitTo(i, cargo)) continue;

		/* Rate and compare the engine by speed & capacity */
		rating = rvi->max_speed * rvi->capacity;
		if (rating <= best_veh_rating) continue;

		ret = DoCommand(tile, i, 0, 0, CMD_BUILD_ROAD_VEH);
		if (CmdFailed(ret)) continue;

		/* Add the cost of refitting */
		if (rvi->cargo_type != cargo) ret += GetRefitCost(i);
		if (ret > money) continue;

		best_veh_rating = rating;
		best_veh_index = i;
	}

	return best_veh_index;
}

static EngineID AiChooseAircraftToBuild(int32 money, byte flag)
{
	EngineID best_veh_index = INVALID_ENGINE;
	int32 best_veh_cost = 0;
	EngineID i;

	for (i = AIRCRAFT_ENGINES_INDEX; i != AIRCRAFT_ENGINES_INDEX + NUM_AIRCRAFT_ENGINES; i++) {
		const Engine* e = GetEngine(i);
		int32 ret;

		if (!HASBIT(e->player_avail, _current_player) || e->reliability < 0x8A3D) {
			continue;
		}

		if ((AircraftVehInfo(i)->subtype & AIR_CTOL) != flag) continue;

		ret = DoCommand(0, i, 0, DC_QUERY_COST, CMD_BUILD_AIRCRAFT);
		if (!CmdFailed(ret) && ret <= money && ret >= best_veh_cost) {
			best_veh_cost = ret;
			best_veh_index = i;
		}
	}

	return best_veh_index;
}

static int32 AiGetBasePrice(const Player* p)
{
	int32 base = _price.station_value;

	// adjust base price when more expensive vehicles are available
	switch (p->ai.railtype_to_use) {
		default: NOT_REACHED();
		case RAILTYPE_RAIL:     break;
		case RAILTYPE_ELECTRIC: break;
		case RAILTYPE_MONO:     base = (base * 3) >> 1; break;
		case RAILTYPE_MAGLEV:   base *= 2; break;
	}

	return base;
}

static EngineID AiChooseRoadVehToReplaceWith(const Player* p, const Vehicle* v)
{
	int32 avail_money = p->player_money + v->value;
	return AiChooseRoadVehToBuild(v->cargo_type, avail_money, v->tile);
}

static EngineID AiChooseAircraftToReplaceWith(const Player* p, const Vehicle* v)
{
	int32 avail_money = p->player_money + v->value;
	return AiChooseAircraftToBuild(
		avail_money, AircraftVehInfo(v->engine_type)->subtype & AIR_CTOL
	);
}

static EngineID AiChooseTrainToReplaceWith(const Player* p, const Vehicle* v)
{
	int32 avail_money = p->player_money + v->value;
	const Vehicle* u = v;
	int num = 0;

	while (++num, u->next != NULL) {
		u = u->next;
	}

	// XXX: check if a wagon
	return AiChooseTrainToBuild(v->u.rail.railtype, avail_money, 0, v->tile);
}

static EngineID AiChooseShipToReplaceWith(const Player* p, const Vehicle* v)
{
	/* Ships are not implemented in this (broken) AI */
	return INVALID_ENGINE;
}

static void AiHandleGotoDepot(Player *p, int cmd)
{
	if (p->ai.cur_veh->current_order.type != OT_GOTO_DEPOT)
		DoCommand(0, p->ai.cur_veh->index, 0, DC_EXEC, cmd);

	if (++p->ai.state_counter <= 1387) {
		p->ai.state = AIS_VEH_DO_REPLACE_VEHICLE;
		return;
	}

	if (p->ai.cur_veh->current_order.type == OT_GOTO_DEPOT) {
		p->ai.cur_veh->current_order.type = OT_DUMMY;
		p->ai.cur_veh->current_order.flags = 0;
		InvalidateWindow(WC_VEHICLE_VIEW, p->ai.cur_veh->index);
	}
}

static void AiRestoreVehicleOrders(Vehicle *v, BackuppedOrders *bak)
{
	uint i;

	for (i = 0; bak->order[i].type != OT_NOTHING; i++) {
		if (!DoCommandP(0, v->index + (i << 16), PackOrder(&bak->order[i]), NULL, CMD_INSERT_ORDER | CMD_NO_TEST_IF_IN_NETWORK))
			break;
	}
}

static void AiHandleReplaceTrain(Player *p)
{
	const Vehicle* v = p->ai.cur_veh;
	BackuppedOrders orderbak[1];
	EngineID veh;

	// wait until the vehicle reaches the depot.
	if (!IsTileDepotType(v->tile, TRANSPORT_RAIL) || v->u.rail.track != 0x80 || !(v->vehstatus&VS_STOPPED)) {
		AiHandleGotoDepot(p, CMD_SEND_TRAIN_TO_DEPOT);
		return;
	}

	veh = AiChooseTrainToReplaceWith(p, v);
	if (veh != INVALID_ENGINE) {
		TileIndex tile;

		BackupVehicleOrders(v, orderbak);
		tile = v->tile;

		if (!CmdFailed(DoCommand(0, v->index, 2, DC_EXEC, CMD_SELL_RAIL_WAGON)) &&
				!CmdFailed(DoCommand(tile, veh, 0, DC_EXEC, CMD_BUILD_RAIL_VEHICLE))) {
			VehicleID veh = _new_vehicle_id;
			AiRestoreVehicleOrders(GetVehicle(veh), orderbak);
			DoCommand(0, veh, 0, DC_EXEC, CMD_START_STOP_TRAIN);

			DoCommand(0, veh, _ai_service_interval, DC_EXEC, CMD_CHANGE_SERVICE_INT);
		}
	}
}

static void AiHandleReplaceRoadVeh(Player *p)
{
	const Vehicle* v = p->ai.cur_veh;
	BackuppedOrders orderbak[1];
	EngineID veh;

	if (!IsRoadVehInDepotStopped(v)) {
		AiHandleGotoDepot(p, CMD_SEND_ROADVEH_TO_DEPOT);
		return;
	}

	veh = AiChooseRoadVehToReplaceWith(p, v);
	if (veh != INVALID_ENGINE) {
		TileIndex tile;

		BackupVehicleOrders(v, orderbak);
		tile = v->tile;

		if (!CmdFailed(DoCommand(0, v->index, 0, DC_EXEC, CMD_SELL_ROAD_VEH)) &&
				!CmdFailed(DoCommand(tile, veh, 0, DC_EXEC, CMD_BUILD_ROAD_VEH))) {
			VehicleID veh = _new_vehicle_id;

			AiRestoreVehicleOrders(GetVehicle(veh), orderbak);
			DoCommand(0, veh, 0, DC_EXEC, CMD_START_STOP_ROADVEH);
			DoCommand(0, veh, _ai_service_interval, DC_EXEC, CMD_CHANGE_SERVICE_INT);
		}
	}
}

static void AiHandleReplaceAircraft(Player *p)
{
	const Vehicle* v = p->ai.cur_veh;
	BackuppedOrders orderbak[1];
	EngineID veh;

	if (!IsAircraftInHangarStopped(v)) {
		AiHandleGotoDepot(p, CMD_SEND_AIRCRAFT_TO_HANGAR);
		return;
	}

	veh = AiChooseAircraftToReplaceWith(p, v);
	if (veh != INVALID_ENGINE) {
		TileIndex tile;

		BackupVehicleOrders(v, orderbak);
		tile = v->tile;

		if (!CmdFailed(DoCommand(0, v->index, 0, DC_EXEC, CMD_SELL_AIRCRAFT)) &&
				!CmdFailed(DoCommand(tile, veh, 0, DC_EXEC, CMD_BUILD_AIRCRAFT))) {
			VehicleID veh = _new_vehicle_id;
			AiRestoreVehicleOrders(GetVehicle(veh), orderbak);
			DoCommand(0, veh, 0, DC_EXEC, CMD_START_STOP_AIRCRAFT);

			DoCommand(0, veh, _ai_service_interval, DC_EXEC, CMD_CHANGE_SERVICE_INT);
		}
	}
}

static void AiHandleReplaceShip(Player *p)
{
	/* Ships are not implemented in this (broken) AI */
}

typedef EngineID CheckReplaceProc(const Player* p, const Vehicle* v);

static CheckReplaceProc* const _veh_check_replace_proc[] = {
	AiChooseTrainToReplaceWith,
	AiChooseRoadVehToReplaceWith,
	AiChooseShipToReplaceWith,
	AiChooseAircraftToReplaceWith,
};

typedef void DoReplaceProc(Player *p);
static DoReplaceProc* const _veh_do_replace_proc[] = {
	AiHandleReplaceTrain,
	AiHandleReplaceRoadVeh,
	AiHandleReplaceShip,
	AiHandleReplaceAircraft
};

static void AiStateCheckReplaceVehicle(Player *p)
{
	const Vehicle* v = p->ai.cur_veh;

	if (!IsValidVehicle(v) ||
			v->owner != _current_player ||
			v->type > VEH_Ship ||
			_veh_check_replace_proc[v->type - VEH_Train](p, v) == INVALID_ENGINE) {
		p->ai.state = AIS_VEH_LOOP;
	} else {
		p->ai.state_counter = 0;
		p->ai.state = AIS_VEH_DO_REPLACE_VEHICLE;
	}
}

static void AiStateDoReplaceVehicle(Player *p)
{
	const Vehicle* v = p->ai.cur_veh;

	p->ai.state = AIS_VEH_LOOP;
	// vehicle is not owned by the player anymore, something went very wrong.
	if (!IsValidVehicle(v) || v->owner != _current_player) return;
	_veh_do_replace_proc[v->type - VEH_Train](p);
}

typedef struct FoundRoute {
	int distance;
	CargoID cargo;
	void *from;
	void *to;
} FoundRoute;

static Town *AiFindRandomTown(void)
{
	return GetRandomTown();
}

static Industry *AiFindRandomIndustry(void)
{
	int num = RandomRange(GetMaxIndustryIndex());
	if (IsValidIndustry(GetIndustry(num))) return GetIndustry(num);

	return NULL;
}

static void AiFindSubsidyIndustryRoute(FoundRoute *fr)
{
	uint i;
	CargoID cargo;
	const Subsidy* s;
	Industry* from;
	TileIndex to_xy;

	// initially error
	fr->distance = -1;

	// Randomize subsidy index..
	i = RandomRange(lengthof(_subsidies) * 3);
	if (i >= lengthof(_subsidies)) return;

	s = &_subsidies[i];

	// Don't want passengers or mail
	cargo = s->cargo_type;
	if (cargo == CT_INVALID ||
			cargo == CT_PASSENGERS ||
			cargo == CT_MAIL ||
			s->age > 7) {
		return;
	}
	fr->cargo = cargo;

	fr->from = from = GetIndustry(s->from);

	if (cargo == CT_GOODS || cargo == CT_FOOD) {
		Town* to_tow = GetTown(s->to);

		if (to_tow->population < (cargo == CT_FOOD ? 200U : 900U)) return; // error
		fr->to = to_tow;
		to_xy = to_tow->xy;
	} else {
		Industry* to_ind = GetIndustry(s->to);

		fr->to = to_ind;
		to_xy = to_ind->xy;
	}

	fr->distance = DistanceManhattan(from->xy, to_xy);
}

static void AiFindSubsidyPassengerRoute(FoundRoute *fr)
{
	uint i;
	const Subsidy* s;
	Town *from,*to;

	// initially error
	fr->distance = -1;

	// Randomize subsidy index..
	i = RandomRange(lengthof(_subsidies) * 3);
	if (i >= lengthof(_subsidies)) return;

	s = &_subsidies[i];

	// Only want passengers
	if (s->cargo_type != CT_PASSENGERS || s->age > 7) return;
	fr->cargo = s->cargo_type;

	fr->from = from = GetTown(s->from);
	fr->to = to = GetTown(s->to);

	// They must be big enough
	if (from->population < 400 || to->population < 400) return;

	fr->distance = DistanceManhattan(from->xy, to->xy);
}

static void AiFindRandomIndustryRoute(FoundRoute *fr)
{
	Industry* i;
	uint32 r;
	CargoID cargo;

	// initially error
	fr->distance = -1;

	r = Random();

	// pick a source
	fr->from = i = AiFindRandomIndustry();
	if (i == NULL) return;

	// pick a random produced cargo
	cargo = i->produced_cargo[0];
	if (r & 1 && i->produced_cargo[1] != CT_INVALID) cargo = i->produced_cargo[1];

	fr->cargo = cargo;

	// don't allow passengers
	if (cargo == CT_INVALID || cargo == CT_PASSENGERS) return;

	if (cargo != CT_GOODS && cargo != CT_FOOD) {
		// pick a dest, and see if it can receive
		Industry* i2 = AiFindRandomIndustry();

		if (i2 == NULL || i == i2 || (
					i2->accepts_cargo[0] != cargo &&
					i2->accepts_cargo[1] != cargo &&
					i2->accepts_cargo[2] != cargo)
				) {
			return;
		}

		fr->to = i2;
		fr->distance = DistanceManhattan(i->xy, i2->xy);
	} else {
		// pick a dest town, and see if it's big enough
		Town* t = AiFindRandomTown();

		if (t == NULL || t->population < (cargo == CT_FOOD ? 200U : 900U)) return;

		fr->to = t;
		fr->distance = DistanceManhattan(i->xy, t->xy);
	}
}

static void AiFindRandomPassengerRoute(FoundRoute *fr)
{
	Town* source;
	Town* dest;

	// initially error
	fr->distance = -1;

	fr->from = source = AiFindRandomTown();
	if (source == NULL || source->population < 400) return;

	fr->to = dest = AiFindRandomTown();
	if (dest == NULL || source == dest || dest->population < 400) return;

	fr->distance = DistanceManhattan(source->xy, dest->xy);
}

// Warn: depends on 'xy' being the first element in both Town and Industry
#define GET_TOWN_OR_INDUSTRY_TILE(p) (((Town*)(p))->xy)

static bool AiCheckIfRouteIsGood(Player *p, FoundRoute *fr, byte bitmask)
{
	TileIndex from_tile, to_tile;
	Station *st;
	int dist;
	uint same_station = 0;

	// Make sure distance to closest station is < 37 pixels.
	from_tile = GET_TOWN_OR_INDUSTRY_TILE(fr->from);
	to_tile = GET_TOWN_OR_INDUSTRY_TILE(fr->to);

	dist = 0xFFFF;
	FOR_ALL_STATIONS(st) {
		int cur;

		if (st->owner != _current_player) continue;
		cur = DistanceMax(from_tile, st->xy);
		if (cur < dist) dist = cur;
		cur = DistanceMax(to_tile, st->xy);
		if (cur < dist) dist = cur;
		if (to_tile == from_tile && st->xy == to_tile) same_station++;
	}

	// To prevent the AI from building ten busstations in the same town, do some calculations
	//  For each road or airport station, we want 350 of population!
	if ((bitmask == 2 || bitmask == 4) &&
			same_station > 2 &&
			((Town*)fr->from)->population < same_station * 350) {
		return false;
	}

	if (dist != 0xFFFF && dist > 37) return false;

	if (p->ai.route_type_mask != 0 &&
			!(p->ai.route_type_mask & bitmask) &&
			!CHANCE16(1, 5)) {
		return false;
	}

	if (fr->cargo == CT_PASSENGERS || fr->cargo == CT_MAIL) {
		const Town* from = fr->from;
		const Town* to   = fr->to;

		if (from->pct_pass_transported > 0x99 ||
				to->pct_pass_transported > 0x99) {
			return false;
		}

		// Make sure it has a reasonably good rating
		if (from->ratings[_current_player] < -100 ||
				to->ratings[_current_player] < -100) {
			return false;
		}
	} else {
		const Industry* i = (const Industry*)fr->from;

		if (i->pct_transported[fr->cargo != i->produced_cargo[0]] > 0x99 ||
				i->total_production[fr->cargo != i->produced_cargo[0]] == 0) {
			return false;
		}
	}

	p->ai.route_type_mask |= bitmask;
	return true;
}

static byte AiGetDirectionBetweenTiles(TileIndex a, TileIndex b)
{
	byte i = (TileX(a) < TileX(b)) ? 1 : 0;
	if (TileY(a) >= TileY(b)) i ^= 3;
	return i;
}

static TileIndex AiGetPctTileBetween(TileIndex a, TileIndex b, byte pct)
{
	return TileXY(
		TileX(a) + ((TileX(b) - TileX(a)) * pct >> 8),
		TileY(a) + ((TileY(b) - TileY(a)) * pct >> 8)
	);
}

static void AiWantLongIndustryRoute(Player *p)
{
	int i;
	FoundRoute fr;

	i = 60;
	for (;;) {
		// look for one from the subsidy list
		AiFindSubsidyIndustryRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 60, 90 + 1)) break;

		// try a random one
		AiFindRandomIndustryRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 60, 90 + 1)) break;

		// only test 60 times
		if (--i == 0) return;
	}

	if (!AiCheckIfRouteIsGood(p, &fr, 1)) return;

	// Fill the source field
	p->ai.dst.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.to);
	p->ai.src.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.from);

	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 9;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.unk6 = 1;
	p->ai.src.unk7 = 0;
	p->ai.src.buildcmd_a = 0x24;
	p->ai.src.buildcmd_b = 0xFF;
	p->ai.src.direction = AiGetDirectionBetweenTiles(
		p->ai.src.spec_tile,
		p->ai.dst.spec_tile
	);
	p->ai.src.cargo = fr.cargo | 0x80;

	// Fill the dest field

	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 9;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.unk6 = 1;
	p->ai.dst.unk7 = 0;
	p->ai.dst.buildcmd_a = 0x34;
	p->ai.dst.buildcmd_b = 0xFF;
	p->ai.dst.direction = AiGetDirectionBetweenTiles(
		p->ai.dst.spec_tile,
		p->ai.src.spec_tile
	);
	p->ai.dst.cargo = fr.cargo;

	// Fill middle field 1
	p->ai.mid1.spec_tile = AiGetPctTileBetween(
		p->ai.src.spec_tile,
		p->ai.dst.spec_tile,
		0x55
	);
	p->ai.mid1.use_tile = 0;
	p->ai.mid1.rand_rng = 6;
	p->ai.mid1.cur_building_rule = 0xFF;
	p->ai.mid1.unk6 = 2;
	p->ai.mid1.unk7 = 1;
	p->ai.mid1.buildcmd_a = 0x30;
	p->ai.mid1.buildcmd_b = 0xFF;
	p->ai.mid1.direction = p->ai.src.direction;
	p->ai.mid1.cargo = fr.cargo;

	// Fill middle field 2
	p->ai.mid2.spec_tile = AiGetPctTileBetween(
		p->ai.src.spec_tile,
		p->ai.dst.spec_tile,
		0xAA
	);
	p->ai.mid2.use_tile = 0;
	p->ai.mid2.rand_rng = 6;
	p->ai.mid2.cur_building_rule = 0xFF;
	p->ai.mid2.unk6 = 2;
	p->ai.mid2.unk7 = 1;
	p->ai.mid2.buildcmd_a = 0xFF;
	p->ai.mid2.buildcmd_b = 0xFF;
	p->ai.mid2.direction = p->ai.dst.direction;
	p->ai.mid2.cargo = fr.cargo;

	// Fill common fields
	p->ai.cargo_type = fr.cargo;
	p->ai.num_wagons = 3;
	p->ai.build_kind = 2;
	p->ai.num_build_rec = 4;
	p->ai.num_loco_to_build = 2;
	p->ai.num_want_fullload = 2;
	p->ai.wagon_list[0] = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;

	p->ai.state = AIS_BUILD_DEFAULT_RAIL_BLOCKS;
	p->ai.state_mode = -1;
	p->ai.state_counter = 0;
	p->ai.timeout_counter = 0;
}

static void AiWantMediumIndustryRoute(Player *p)
{
	int i;
	FoundRoute fr;

	i = 60;
	for (;;) {
		// look for one from the subsidy list
		AiFindSubsidyIndustryRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 40, 60 + 1)) break;

		// try a random one
		AiFindRandomIndustryRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 40, 60 + 1)) break;

		// only test 60 times
		if (--i == 0) return;
	}

	if (!AiCheckIfRouteIsGood(p, &fr, 1)) return;

	// Fill the source field
	p->ai.src.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.from);
	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 9;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.unk6 = 1;
	p->ai.src.unk7 = 0;
	p->ai.src.buildcmd_a = 0x10;
	p->ai.src.buildcmd_b = 0xFF;
	p->ai.src.direction = AiGetDirectionBetweenTiles(
		GET_TOWN_OR_INDUSTRY_TILE(fr.from),
		GET_TOWN_OR_INDUSTRY_TILE(fr.to)
	);
	p->ai.src.cargo = fr.cargo | 0x80;

	// Fill the dest field
	p->ai.dst.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.to);
	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 9;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.unk6 = 1;
	p->ai.dst.unk7 = 0;
	p->ai.dst.buildcmd_a = 0xFF;
	p->ai.dst.buildcmd_b = 0xFF;
	p->ai.dst.direction = AiGetDirectionBetweenTiles(
		GET_TOWN_OR_INDUSTRY_TILE(fr.to),
		GET_TOWN_OR_INDUSTRY_TILE(fr.from)
	);
	p->ai.dst.cargo = fr.cargo;

	// Fill common fields
	p->ai.cargo_type = fr.cargo;
	p->ai.num_wagons = 3;
	p->ai.build_kind = 1;
	p->ai.num_build_rec = 2;
	p->ai.num_loco_to_build = 1;
	p->ai.num_want_fullload = 1;
	p->ai.wagon_list[0] = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;
	p->ai.state = AIS_BUILD_DEFAULT_RAIL_BLOCKS;
	p->ai.state_mode = -1;
	p->ai.state_counter = 0;
	p->ai.timeout_counter = 0;
}

static void AiWantShortIndustryRoute(Player *p)
{
	int i;
	FoundRoute fr;

	i = 60;
	for (;;) {
		// look for one from the subsidy list
		AiFindSubsidyIndustryRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 15, 40 + 1)) break;

		// try a random one
		AiFindRandomIndustryRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 15, 40 + 1)) break;

		// only test 60 times
		if (--i == 0) return;
	}

	if (!AiCheckIfRouteIsGood(p, &fr, 1)) return;

	// Fill the source field
	p->ai.src.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.from);
	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 9;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.unk6 = 1;
	p->ai.src.unk7 = 0;
	p->ai.src.buildcmd_a = 0x10;
	p->ai.src.buildcmd_b = 0xFF;
	p->ai.src.direction = AiGetDirectionBetweenTiles(
		GET_TOWN_OR_INDUSTRY_TILE(fr.from),
		GET_TOWN_OR_INDUSTRY_TILE(fr.to)
	);
	p->ai.src.cargo = fr.cargo | 0x80;

	// Fill the dest field
	p->ai.dst.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.to);
	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 9;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.unk6 = 1;
	p->ai.dst.unk7 = 0;
	p->ai.dst.buildcmd_a = 0xFF;
	p->ai.dst.buildcmd_b = 0xFF;
	p->ai.dst.direction = AiGetDirectionBetweenTiles(
		GET_TOWN_OR_INDUSTRY_TILE(fr.to),
		GET_TOWN_OR_INDUSTRY_TILE(fr.from)
	);
	p->ai.dst.cargo = fr.cargo;

	// Fill common fields
	p->ai.cargo_type = fr.cargo;
	p->ai.num_wagons = 2;
	p->ai.build_kind = 1;
	p->ai.num_build_rec = 2;
	p->ai.num_loco_to_build = 1;
	p->ai.num_want_fullload = 1;
	p->ai.wagon_list[0] = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;
	p->ai.state = AIS_BUILD_DEFAULT_RAIL_BLOCKS;
	p->ai.state_mode = -1;
	p->ai.state_counter = 0;
	p->ai.timeout_counter = 0;
}

static void AiWantMailRoute(Player *p)
{
	int i;
	FoundRoute fr;

	i = 60;
	for (;;) {
		// look for one from the subsidy list
		AiFindSubsidyPassengerRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 60, 110 + 1)) break;

		// try a random one
		AiFindRandomPassengerRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 60, 110 + 1)) break;

		// only test 60 times
		if (--i == 0) return;
	}

	fr.cargo = CT_MAIL;
	if (!AiCheckIfRouteIsGood(p, &fr, 1)) return;

	// Fill the source field
	p->ai.src.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.from);
	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 7;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.unk6 = 1;
	p->ai.src.unk7 = 0;
	p->ai.src.buildcmd_a = 0x24;
	p->ai.src.buildcmd_b = 0xFF;
	p->ai.src.direction = AiGetDirectionBetweenTiles(
		GET_TOWN_OR_INDUSTRY_TILE(fr.from),
		GET_TOWN_OR_INDUSTRY_TILE(fr.to)
	);
	p->ai.src.cargo = fr.cargo;

	// Fill the dest field
	p->ai.dst.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.to);
	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 7;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.unk6 = 1;
	p->ai.dst.unk7 = 0;
	p->ai.dst.buildcmd_a = 0x34;
	p->ai.dst.buildcmd_b = 0xFF;
	p->ai.dst.direction = AiGetDirectionBetweenTiles(
		GET_TOWN_OR_INDUSTRY_TILE(fr.to),
		GET_TOWN_OR_INDUSTRY_TILE(fr.from)
	);
	p->ai.dst.cargo = fr.cargo;

	// Fill middle field 1
	p->ai.mid1.spec_tile = AiGetPctTileBetween(
		GET_TOWN_OR_INDUSTRY_TILE(fr.from),
		GET_TOWN_OR_INDUSTRY_TILE(fr.to),
		0x55
	);
	p->ai.mid1.use_tile = 0;
	p->ai.mid1.rand_rng = 6;
	p->ai.mid1.cur_building_rule = 0xFF;
	p->ai.mid1.unk6 = 2;
	p->ai.mid1.unk7 = 1;
	p->ai.mid1.buildcmd_a = 0x30;
	p->ai.mid1.buildcmd_b = 0xFF;
	p->ai.mid1.direction = p->ai.src.direction;
	p->ai.mid1.cargo = fr.cargo;

	// Fill middle field 2
	p->ai.mid2.spec_tile = AiGetPctTileBetween(
		GET_TOWN_OR_INDUSTRY_TILE(fr.from),
		GET_TOWN_OR_INDUSTRY_TILE(fr.to),
		0xAA
	);
	p->ai.mid2.use_tile = 0;
	p->ai.mid2.rand_rng = 6;
	p->ai.mid2.cur_building_rule = 0xFF;
	p->ai.mid2.unk6 = 2;
	p->ai.mid2.unk7 = 1;
	p->ai.mid2.buildcmd_a = 0xFF;
	p->ai.mid2.buildcmd_b = 0xFF;
	p->ai.mid2.direction = p->ai.dst.direction;
	p->ai.mid2.cargo = fr.cargo;

	// Fill common fields
	p->ai.cargo_type = fr.cargo;
	p->ai.num_wagons = 3;
	p->ai.build_kind = 2;
	p->ai.num_build_rec = 4;
	p->ai.num_loco_to_build = 2;
	p->ai.num_want_fullload = 0;
	p->ai.wagon_list[0] = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;
	p->ai.state = AIS_BUILD_DEFAULT_RAIL_BLOCKS;
	p->ai.state_mode = -1;
	p->ai.state_counter = 0;
	p->ai.timeout_counter = 0;
}

static void AiWantPassengerRoute(Player *p)
{
	int i;
	FoundRoute fr;

	i = 60;
	for (;;) {
		// look for one from the subsidy list
		AiFindSubsidyPassengerRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 0, 55 + 1)) break;

		// try a random one
		AiFindRandomPassengerRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 0, 55 + 1)) break;

		// only test 60 times
		if (--i == 0) return;
	}

	fr.cargo = CT_PASSENGERS;
	if (!AiCheckIfRouteIsGood(p, &fr, 1)) return;

	// Fill the source field
	p->ai.src.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.from);
	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 7;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.unk6 = 1;
	p->ai.src.unk7 = 0;
	p->ai.src.buildcmd_a = 0x10;
	p->ai.src.buildcmd_b = 0xFF;
	p->ai.src.direction = AiGetDirectionBetweenTiles(
		GET_TOWN_OR_INDUSTRY_TILE(fr.from),
		GET_TOWN_OR_INDUSTRY_TILE(fr.to)
	);
	p->ai.src.cargo = fr.cargo;

	// Fill the dest field
	p->ai.dst.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.to);
	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 7;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.unk6 = 1;
	p->ai.dst.unk7 = 0;
	p->ai.dst.buildcmd_a = 0xFF;
	p->ai.dst.buildcmd_b = 0xFF;
	p->ai.dst.direction = AiGetDirectionBetweenTiles(
		GET_TOWN_OR_INDUSTRY_TILE(fr.to),
		GET_TOWN_OR_INDUSTRY_TILE(fr.from)
	);
	p->ai.dst.cargo = fr.cargo;

	// Fill common fields
	p->ai.cargo_type = fr.cargo;
	p->ai.num_wagons = 2;
	p->ai.build_kind = 1;
	p->ai.num_build_rec = 2;
	p->ai.num_loco_to_build = 1;
	p->ai.num_want_fullload = 0;
	p->ai.wagon_list[0] = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;
	p->ai.state = AIS_BUILD_DEFAULT_RAIL_BLOCKS;
	p->ai.state_mode = -1;
	p->ai.state_counter = 0;
	p->ai.timeout_counter = 0;
}

static void AiWantTrainRoute(Player *p)
{
	uint16 r = GB(Random(), 0, 16);

	p->ai.railtype_to_use = GetBestRailtype(p);

	if (r > 0xD000) {
		AiWantLongIndustryRoute(p);
	} else if (r > 0x6000) {
		AiWantMediumIndustryRoute(p);
	} else if (r > 0x1000) {
		AiWantShortIndustryRoute(p);
	} else if (r > 0x800) {
		AiWantPassengerRoute(p);
	} else {
		AiWantMailRoute(p);
	}
}

static void AiWantLongRoadIndustryRoute(Player *p)
{
	int i;
	FoundRoute fr;

	i = 60;
	for (;;) {
		// look for one from the subsidy list
		AiFindSubsidyIndustryRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 35, 55 + 1)) break;

		// try a random one
		AiFindRandomIndustryRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 35, 55 + 1)) break;

		// only test 60 times
		if (--i == 0) return;
	}

	if (!AiCheckIfRouteIsGood(p, &fr, 2)) return;

	// Fill the source field
	p->ai.src.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.from);
	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 9;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.buildcmd_a = 1;
	p->ai.src.direction = 0;
	p->ai.src.cargo = fr.cargo | 0x80;

	// Fill the dest field
	p->ai.dst.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.to);
	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 9;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.buildcmd_a = 0xFF;
	p->ai.dst.direction = 0;
	p->ai.dst.cargo = fr.cargo;

	// Fill common fields
	p->ai.cargo_type = fr.cargo;
	p->ai.num_build_rec = 2;
	p->ai.num_loco_to_build = 5;
	p->ai.num_want_fullload = 5;

//	p->ai.loco_id = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;

	p->ai.state = AIS_BUILD_DEFAULT_ROAD_BLOCKS;
	p->ai.state_mode = -1;
	p->ai.state_counter = 0;
	p->ai.timeout_counter = 0;
}

static void AiWantMediumRoadIndustryRoute(Player *p)
{
	int i;
	FoundRoute fr;

	i = 60;
	for (;;) {
		// look for one from the subsidy list
		AiFindSubsidyIndustryRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 15, 40 + 1)) break;

		// try a random one
		AiFindRandomIndustryRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 15, 40 + 1)) break;

		// only test 60 times
		if (--i == 0) return;
	}

	if (!AiCheckIfRouteIsGood(p, &fr, 2)) return;

	// Fill the source field
	p->ai.src.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.from);
	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 9;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.buildcmd_a = 1;
	p->ai.src.direction = 0;
	p->ai.src.cargo = fr.cargo | 0x80;

	// Fill the dest field
	p->ai.dst.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.to);
	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 9;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.buildcmd_a = 0xFF;
	p->ai.dst.direction = 0;
	p->ai.dst.cargo = fr.cargo;

	// Fill common fields
	p->ai.cargo_type = fr.cargo;
	p->ai.num_build_rec = 2;
	p->ai.num_loco_to_build = 3;
	p->ai.num_want_fullload = 3;

//	p->ai.loco_id = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;

	p->ai.state = AIS_BUILD_DEFAULT_ROAD_BLOCKS;
	p->ai.state_mode = -1;
	p->ai.state_counter = 0;
	p->ai.timeout_counter = 0;
}

static void AiWantLongRoadPassengerRoute(Player *p)
{
	int i;
	FoundRoute fr;

	i = 60;
	for (;;) {
		// look for one from the subsidy list
		AiFindSubsidyPassengerRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 55, 180 + 1)) break;

		// try a random one
		AiFindRandomPassengerRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 55, 180 + 1)) break;

		// only test 60 times
		if (--i == 0) return;
	}

	fr.cargo = CT_PASSENGERS;

	if (!AiCheckIfRouteIsGood(p, &fr, 2)) return;

	// Fill the source field
	p->ai.src.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.to);
	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 10;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.buildcmd_a = 1;
	p->ai.src.direction = 0;
	p->ai.src.cargo = CT_PASSENGERS;

	// Fill the dest field
	p->ai.dst.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.from);
	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 10;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.buildcmd_a = 0xFF;
	p->ai.dst.direction = 0;
	p->ai.dst.cargo = CT_PASSENGERS;

	// Fill common fields
	p->ai.cargo_type = CT_PASSENGERS;
	p->ai.num_build_rec = 2;
	p->ai.num_loco_to_build = 4;
	p->ai.num_want_fullload = 0;

//	p->ai.loco_id = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;

	p->ai.state = AIS_BUILD_DEFAULT_ROAD_BLOCKS;
	p->ai.state_mode = -1;
	p->ai.state_counter = 0;
	p->ai.timeout_counter = 0;
}

static void AiWantPassengerRouteInsideTown(Player *p)
{
	int i;
	FoundRoute fr;
	Town *t;

	i = 60;
	for (;;) {
		// Find a town big enough
		t = AiFindRandomTown();
		if (t != NULL && t->population >= 700) break;

		// only test 60 times
		if (--i == 0) return;
	}

	fr.cargo = CT_PASSENGERS;
	fr.from = fr.to = t;

	if (!AiCheckIfRouteIsGood(p, &fr, 2)) return;

	// Fill the source field
	p->ai.src.spec_tile = t->xy;
	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 10;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.buildcmd_a = 1;
	p->ai.src.direction = 0;
	p->ai.src.cargo = CT_PASSENGERS;

	// Fill the dest field
	p->ai.dst.spec_tile = t->xy;
	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 10;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.buildcmd_a = 0xFF;
	p->ai.dst.direction = 0;
	p->ai.dst.cargo = CT_PASSENGERS;

	// Fill common fields
	p->ai.cargo_type = CT_PASSENGERS;
	p->ai.num_build_rec = 2;
	p->ai.num_loco_to_build = 2;
	p->ai.num_want_fullload = 0;

//	p->ai.loco_id = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;

	p->ai.state = AIS_BUILD_DEFAULT_ROAD_BLOCKS;
	p->ai.state_mode = -1;
	p->ai.state_counter = 0;
	p->ai.timeout_counter = 0;
}

static void AiWantRoadRoute(Player *p)
{
	uint16 r = GB(Random(), 0, 16);

	if (r > 0x4000) {
		AiWantLongRoadIndustryRoute(p);
	} else if (r > 0x2000) {
		AiWantMediumRoadIndustryRoute(p);
	} else if (r > 0x1000) {
		AiWantLongRoadPassengerRoute(p);
	} else {
		AiWantPassengerRouteInsideTown(p);
	}
}

static void AiWantPassengerAircraftRoute(Player *p)
{
	FoundRoute fr;
	int i;

	i = 60;
	for (;;) {
		// look for one from the subsidy list
		AiFindSubsidyPassengerRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 0, 95 + 1)) break;

		// try a random one
		AiFindRandomPassengerRoute(&fr);
		if (IS_INT_INSIDE(fr.distance, 0, 95 + 1)) break;

		// only test 60 times
		if (--i == 0) return;
	}

	fr.cargo = CT_PASSENGERS;
	if (!AiCheckIfRouteIsGood(p, &fr, 4)) return;


	// Fill the source field
	p->ai.src.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.to);
	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 12;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.cargo = fr.cargo;

	// Fill the dest field
	p->ai.dst.spec_tile = GET_TOWN_OR_INDUSTRY_TILE(fr.from);
	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 12;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.cargo = fr.cargo;

	// Fill common fields
	p->ai.cargo_type = fr.cargo;
	p->ai.build_kind = 0;
	p->ai.num_build_rec = 2;
	p->ai.num_loco_to_build = 1;
	p->ai.num_want_fullload = 1;
//	p->ai.loco_id = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;

	p->ai.state = AIS_AIRPORT_STUFF;
	p->ai.timeout_counter = 0;
}

static void AiWantOilRigAircraftRoute(Player *p)
{
	int i;
	FoundRoute fr;
	Town *t;
	Industry *in;

	i = 60;
	for (;;) {
		// Find a town
		t = AiFindRandomTown();
		if (t != NULL) {
			// Find a random oil rig industry
			in = AiFindRandomIndustry();
			if (in != NULL && in->type == IT_OIL_RIG) {
				if (DistanceManhattan(t->xy, in->xy) < 60)
					break;
			}
		}

		// only test 60 times
		if (--i == 0) return;
	}

	fr.cargo = CT_PASSENGERS;
	fr.from = fr.to = t;

	if (!AiCheckIfRouteIsGood(p, &fr, 4)) return;

	// Fill the source field
	p->ai.src.spec_tile = t->xy;
	p->ai.src.use_tile = 0;
	p->ai.src.rand_rng = 12;
	p->ai.src.cur_building_rule = 0xFF;
	p->ai.src.cargo = CT_PASSENGERS;

	// Fill the dest field
	p->ai.dst.spec_tile = in->xy;
	p->ai.dst.use_tile = 0;
	p->ai.dst.rand_rng = 5;
	p->ai.dst.cur_building_rule = 0xFF;
	p->ai.dst.cargo = CT_PASSENGERS;

	// Fill common fields
	p->ai.cargo_type = CT_PASSENGERS;
	p->ai.build_kind = 1;
	p->ai.num_build_rec = 2;
	p->ai.num_loco_to_build = 1;
	p->ai.num_want_fullload = 0;
//	p->ai.loco_id = INVALID_VEHICLE;
	p->ai.order_list_blocks[0] = 0;
	p->ai.order_list_blocks[1] = 1;
	p->ai.order_list_blocks[2] = 255;

	p->ai.state = AIS_AIRPORT_STUFF;
	p->ai.timeout_counter = 0;
}

static void AiWantAircraftRoute(Player *p)
{
	uint16 r = (uint16)Random();

	if (r >= 0x2AAA || _date < 0x3912 + DAYS_TILL_ORIGINAL_BASE_YEAR) {
		AiWantPassengerAircraftRoute(p);
	} else {
		AiWantOilRigAircraftRoute(p);
	}
}



static void AiStateWantNewRoute(Player *p)
{
	uint16 r;
	int i;

	if (p->player_money < AiGetBasePrice(p) * 500) {
		p->ai.state = AIS_0;
		return;
	}

	i = 200;
	for (;;) {
		r = (uint16)Random();

		if (_patches.ai_disable_veh_train &&
				_patches.ai_disable_veh_roadveh &&
				_patches.ai_disable_veh_aircraft &&
				_patches.ai_disable_veh_ship) {
			return;
		}

		if (r < 0x7626) {
			if (_patches.ai_disable_veh_train) continue;
			AiWantTrainRoute(p);
		} else if (r < 0xC4EA) {
			if (_patches.ai_disable_veh_roadveh) continue;
			AiWantRoadRoute(p);
		} else if (r < 0xD89B) {
			if (_patches.ai_disable_veh_aircraft) continue;
			AiWantAircraftRoute(p);
		} else {
			/* Ships are not implemented in this (broken) AI */
		}

		// got a route?
		if (p->ai.state != AIS_WANT_NEW_ROUTE) break;

		// time out?
		if (--i == 0) {
			if (++p->ai.state_counter == 556) p->ai.state = AIS_0;
			break;
		}
	}
}

static bool AiCheckTrackResources(TileIndex tile, const AiDefaultBlockData *p, byte cargo)
{
	uint rad = (_patches.modified_catchment) ? CA_TRAIN : 4;

	for (; p->mode != 4; p++) {
		AcceptedCargo values;
		TileIndex tile2;
		uint w;
		uint h;

		if (p->mode != 1) continue;

		tile2 = TILE_ADD(tile, ToTileIndexDiff(p->tileoffs));
		w = GB(p->attr, 1, 3);
		h = GB(p->attr, 4, 3);

		if (p->attr & 1) uintswap(w, h);

		if (cargo & 0x80) {
			GetProductionAroundTiles(values, tile2, w, h, rad);
			return values[cargo & 0x7F] != 0;
		} else {
			GetAcceptanceAroundTiles(values, tile2, w, h, rad);
			if (!(values[cargo] & ~7))
				return false;
			if (cargo != CT_MAIL)
				return true;
			return !!((values[cargo]>>1) & ~7);
		}
	}

	return true;
}

static int32 AiDoBuildDefaultRailTrack(TileIndex tile, const AiDefaultBlockData* p, RailType railtype, byte flag)
{
	int32 ret;
	int32 total_cost = 0;
	Town *t = NULL;
	int rating = 0;
	int i,j,k;

	for (;;) {
		// This will seldomly overflow for valid reasons. Mask it to be on the safe side.
		uint c = TILE_MASK(tile + ToTileIndexDiff(p->tileoffs));

		_cleared_town = NULL;

		if (p->mode < 2) {
			if (p->mode == 0) {
				// Depot
				ret = DoCommand(c, railtype, p->attr, flag | DC_AUTO | DC_NO_WATER | DC_AI_BUILDING, CMD_BUILD_TRAIN_DEPOT);
			} else {
				// Station
				ret = DoCommand(c, (p->attr&1) | (p->attr>>4)<<8 | (p->attr>>1&7)<<16, railtype, flag | DC_AUTO | DC_NO_WATER | DC_AI_BUILDING, CMD_BUILD_RAILROAD_STATION);
			}

			if (CmdFailed(ret)) return CMD_ERROR;
			total_cost += ret;

clear_town_stuff:;
			if (_cleared_town != NULL) {
				if (t != NULL && t != _cleared_town)
					return CMD_ERROR;
				t = _cleared_town;
				rating += _cleared_town_rating;
			}
		} else if (p->mode == 2) {
			// Rail
			if (IsTileType(c, MP_RAILWAY)) return CMD_ERROR;

			j = p->attr;
			k = 0;

			// Build the rail
			for (i = 0; i != 6; i++, j >>= 1) {
				if (j&1) {
					k = i;
					ret = DoCommand(c, railtype, i, flag | DC_AUTO | DC_NO_WATER, CMD_BUILD_SINGLE_RAIL);
					if (CmdFailed(ret)) return CMD_ERROR;
					total_cost += ret;
				}
			}

			/* signals too? */
			if (j & 3) {
				/* XXX - we need to check manually whether we can build a signal if DC_EXEC is
				   not set because the rail has not actually been built */
				if (!IsTileType(c, MP_RAILWAY)) return CMD_ERROR;

				if (flag & DC_EXEC) {
					j = 4 - j;
					do {
						ret = DoCommand(c, k, 0, flag, CMD_BUILD_SIGNALS);
					} while (--j);
				} else {
					ret = _price.build_signals;
				}
				if (CmdFailed(ret)) return CMD_ERROR;
				total_cost += ret;
			}
		} else if (p->mode == 3) {
			//Clear stuff and then build single rail.
			if (GetTileSlope(c, NULL) != SLOPE_FLAT) return CMD_ERROR;
			ret = DoCommand(c, 0, 0, flag | DC_AUTO | DC_NO_WATER | DC_AI_BUILDING, CMD_LANDSCAPE_CLEAR);
			if (CmdFailed(ret)) return CMD_ERROR;
			total_cost += ret + _price.build_rail;

			if (flag & DC_EXEC) {
				DoCommand(c, railtype, p->attr&1, flag | DC_AUTO | DC_NO_WATER | DC_AI_BUILDING, CMD_BUILD_SINGLE_RAIL);
			}

			goto clear_town_stuff;
		} else {
			// Unk
			break;
		}

		p++;
	}

	if (!(flag & DC_EXEC)) {
		if (t != NULL && rating > t->ratings[_current_player]) {
			return CMD_ERROR;
		}
	}

	return total_cost;
}

// Returns rule and cost
static int AiBuildDefaultRailTrack(TileIndex tile, byte p0, byte p1, byte p2, byte p3, byte dir, byte cargo, RailType railtype, int32* cost)
{
	int i;
	const AiDefaultRailBlock *p;

	for (i = 0; (p = _default_rail_track_data[i]) != NULL; i++) {
		if (p->p0 == p0 && p->p1 == p1 && p->p2 == p2 && p->p3 == p3 &&
				(p->dir == 0xFF || p->dir == dir || ((p->dir - 1) & 3) == dir)) {
			*cost = AiDoBuildDefaultRailTrack(tile, p->data, railtype, DC_NO_TOWN_RATING);
			if (!CmdFailed(*cost) && AiCheckTrackResources(tile, p->data, cargo))
				return i;
		}
	}

	return -1;
}

static const byte _terraform_up_flags[] = {
	14, 13, 12, 11,
	10,  9,  8,  7,
	 6,  5,  4,  3,
	 2,  1,  0,  1,
	 2,  1,  4,  1,
	 2,  1,  8,  1,
	 2,  1,  4,  2,
	 2,  1
};

static const byte _terraform_down_flags[] = {
	1,  2, 3,  4,
	5,  6, 1,  8,
	9, 10, 8, 12,
	4,  2, 0,  0,
	1,  2, 3,  4,
	5,  6, 2,  8,
	9, 10, 1, 12,
	8,  4
};

static void AiDoTerraformLand(TileIndex tile, int dir, int unk, int mode)
{
	PlayerID old_player;
	uint32 r;
	Slope slope;
	uint h;

	old_player = _current_player;
	_current_player = OWNER_NONE;

	r = Random();

	unk &= (int)r;

	do {
		tile = TILE_MASK(tile + TileOffsByDiagDir(dir));

		r >>= 2;
		if (r & 2) {
			dir++;
			if (r & 1) dir -= 2;
		}
		dir &= 3;
	} while (--unk >= 0);

	slope = GetTileSlope(tile, &h);

	if (slope != SLOPE_FLAT) {
		if (mode > 0 || (mode == 0 && !(r & 0xC))) {
			// Terraform up
			DoCommand(tile, _terraform_up_flags[slope - 1], 1,
				DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_TERRAFORM_LAND);
		} else if (h != 0) {
			// Terraform down
			DoCommand(tile, _terraform_down_flags[slope - 1], 0,
				DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_TERRAFORM_LAND);
		}
	}

	_current_player = old_player;
}

static void AiStateBuildDefaultRailBlocks(Player *p)
{
	uint i;
	int j;
	AiBuildRec *aib;
	int rule;
	int32 cost;

	// time out?
	if (++p->ai.timeout_counter == 1388) {
		p->ai.state = AIS_DELETE_RAIL_BLOCKS;
		return;
	}

	// do the following 8 times
	for (i = 0; i < 8; i++) {
		// check if we can build the default track
		aib = &p->ai.src;
		j = p->ai.num_build_rec;
		do {
			// this item has already been built?
			if (aib->cur_building_rule != 255) continue;

			// adjust the coordinate randomly,
			// to make sure that we find a position.
			aib->use_tile = AdjustTileCoordRandomly(aib->spec_tile, aib->rand_rng);

			// check if the track can be build there.
			rule = AiBuildDefaultRailTrack(aib->use_tile,
				p->ai.build_kind, p->ai.num_wagons,
				aib->unk6, aib->unk7,
				aib->direction, aib->cargo,
				p->ai.railtype_to_use,
				&cost
			);

			if (rule == -1) {
				// cannot build, terraform after a while
				if (p->ai.state_counter >= 600) {
					AiDoTerraformLand(aib->use_tile, Random()&3, 3, (int8)p->ai.state_mode);
				}
				// also try the other terraform direction
				if (++p->ai.state_counter >= 1000) {
					p->ai.state_counter = 0;
					p->ai.state_mode = -p->ai.state_mode;
				}
			} else if (CheckPlayerHasMoney(cost)) {
				int32 r;
				// player has money, build it.
				aib->cur_building_rule = rule;

				r = AiDoBuildDefaultRailTrack(
					aib->use_tile,
					_default_rail_track_data[rule]->data,
					p->ai.railtype_to_use,
					DC_EXEC | DC_NO_TOWN_RATING
				);
				assert(!CmdFailed(r));
			}
		} while (++aib,--j);
	}

	// check if we're done with all of them
	aib = &p->ai.src;
	j = p->ai.num_build_rec;
	do {
		if (aib->cur_building_rule == 255) return;
	} while (++aib,--j);

	// yep, all are done. switch state to the rail building state.
	p->ai.state = AIS_BUILD_RAIL;
	p->ai.state_mode = 255;
}

static TileIndex AiGetEdgeOfDefaultRailBlock(byte rule, TileIndex tile, byte cmd, int *dir)
{
	const AiDefaultBlockData *p = _default_rail_track_data[rule]->data;

	while (p->mode != 3 || !((--cmd) & 0x80)) p++;

	return tile + ToTileIndexDiff(p->tileoffs) - TileOffsByDiagDir(*dir = p->attr);
}

typedef struct AiRailPathFindData {
	TileIndex tile;
	TileIndex tile2;
	int count;
	bool flag;
} AiRailPathFindData;

static bool AiEnumFollowTrack(TileIndex tile, AiRailPathFindData *a, int track, uint length, byte *state)
{
	if (a->flag) return true;

	if (length > 20 || tile == a->tile) {
		a->flag = true;
		return true;
	}

	if (DistanceMax(tile, a->tile2) < 4) a->count++;

	return false;
}

static bool AiDoFollowTrack(const Player* p)
{
	AiRailPathFindData arpfd;

	arpfd.tile = p->ai.start_tile_a;
	arpfd.tile2 = p->ai.cur_tile_a;
	arpfd.flag = false;
	arpfd.count = 0;
	FollowTrack(p->ai.cur_tile_a + TileOffsByDiagDir(p->ai.cur_dir_a), 0x2000 | TRANSPORT_RAIL, p->ai.cur_dir_a^2,
		(TPFEnumProc*)AiEnumFollowTrack, NULL, &arpfd);
	return arpfd.count > 8;
}

typedef struct AiRailFinder {
	TileIndex final_tile;
	byte final_dir;
	byte depth;
	byte recursive_mode;
	byte cur_best_dir;
	byte best_dir;
	byte cur_best_depth;
	byte best_depth;
	uint cur_best_dist;
	const byte *best_ptr;
	uint best_dist;
	TileIndex cur_best_tile, best_tile;
	TileIndex bridge_end_tile;
	Player *player;
} AiRailFinder;

static const byte _ai_table_15[4][8] = {
	{0, 0, 4, 3, 3, 1, 128 + 0, 64},
	{1, 1, 2, 0, 4, 2, 128 + 1, 65},
	{0, 2, 2, 3, 5, 1, 128 + 2, 66},
	{1, 3, 5, 0, 3, 2, 128 + 3, 67}
};

static const byte _dir_table_1[] = { 3, 9, 12, 6};
static const byte _dir_table_2[] = {12, 6,  3, 9};


static bool AiIsTileBanned(const Player* p, TileIndex tile, byte val)
{
	int i;

	for (i = 0; i != p->ai.banned_tile_count; i++) {
		if (p->ai.banned_tiles[i] == tile && p->ai.banned_val[i] == val) {
			return true;
		}
	}
	return false;
}

static void AiBanTile(Player* p, TileIndex tile, byte val)
{
	uint i;

	for (i = lengthof(p->ai.banned_tiles) - 1; i != 0; i--) {
		p->ai.banned_tiles[i] = p->ai.banned_tiles[i - 1];
		p->ai.banned_val[i] = p->ai.banned_val[i - 1];
	}

	p->ai.banned_tiles[0] = tile;
	p->ai.banned_val[0] = val;

	if (p->ai.banned_tile_count != lengthof(p->ai.banned_tiles)) {
		p->ai.banned_tile_count++;
	}
}

static void AiBuildRailRecursive(AiRailFinder *arf, TileIndex tile, int dir);

static bool AiCheckRailPathBetter(AiRailFinder *arf, const byte *p)
{
	bool better = false;

	if (arf->recursive_mode < 1) {
		// Mode is 0. This means destination has not been found yet.
		// If the found path is shorter than the current one, remember it.
		if (arf->cur_best_dist < arf->best_dist) {
			arf->best_dir = arf->cur_best_dir;
			arf->best_dist = arf->cur_best_dist;
			arf->best_ptr = p;
			arf->best_tile = arf->cur_best_tile;
			better = true;
		}
	} else if (arf->recursive_mode > 1) {
		// Mode is 2.
		if (arf->best_dist != 0 || arf->cur_best_depth < arf->best_depth) {
			arf->best_depth = arf->cur_best_depth;
			arf->best_dist = 0;
			arf->best_ptr = p;
			arf->best_tile = 0;
			better = true;
		}
	}
	arf->recursive_mode = 0;
	arf->cur_best_dist = (uint)-1;
	arf->cur_best_depth = 0xff;

	return better;
}

static inline void AiCheckBuildRailBridgeHere(AiRailFinder *arf, TileIndex tile, const byte *p)
{
	Slope tileh;
	uint z;
	bool flag;

	int dir2 = p[0] & 3;

	tileh = GetTileSlope(tile, &z);
	if (tileh == _dir_table_1[dir2] || (tileh == SLOPE_FLAT && z != 0)) {
		TileIndex tile_new = tile;

		// Allow bridges directly over bottom tiles
		flag = z == 0;
		for (;;) {
			TileType type;

			if ((TileIndexDiff)tile_new < -TileOffsByDiagDir(dir2)) return; // Wraping around map, no bridge possible!
			tile_new = TILE_MASK(tile_new + TileOffsByDiagDir(dir2));
			type = GetTileType(tile_new);

			if (type == MP_CLEAR || type == MP_TREES || GetTileSlope(tile_new, NULL) != SLOPE_FLAT) {
				if (!flag) return;
				break;
			}
			if (type != MP_WATER && type != MP_RAILWAY && type != MP_STREET) return;
			flag = true;
		}

		// Is building a (rail)bridge possible at this place (type doesn't matter)?
		if (CmdFailed(DoCommand(tile_new, tile, 0 | arf->player->ai.railtype_to_use << 8, DC_AUTO, CMD_BUILD_BRIDGE))) {
			return;
		}
		AiBuildRailRecursive(arf, tile_new, dir2);

		// At the bottom depth, check if the new path is better than the old one.
		if (arf->depth == 1) {
			if (AiCheckRailPathBetter(arf, p)) arf->bridge_end_tile = tile_new;
		}
	}
}

static inline void AiCheckBuildRailTunnelHere(AiRailFinder *arf, TileIndex tile, const byte *p)
{
	uint z;

	if (GetTileSlope(tile, &z) == _dir_table_2[p[0] & 3] && z != 0) {
		int32 cost = DoCommand(tile, arf->player->ai.railtype_to_use, 0, DC_AUTO, CMD_BUILD_TUNNEL);

		if (!CmdFailed(cost) && cost <= (arf->player->player_money>>4)) {
			AiBuildRailRecursive(arf, _build_tunnel_endtile, p[0]&3);
			if (arf->depth == 1) AiCheckRailPathBetter(arf, p);
		}
	}
}


static void AiBuildRailRecursive(AiRailFinder *arf, TileIndex tile, int dir)
{
	const byte *p;

	tile = TILE_MASK(tile + TileOffsByDiagDir(dir));

	// Reached destination?
	if (tile == arf->final_tile) {
		if (arf->final_dir != (dir^2)) {
			if (arf->recursive_mode != 2) arf->recursive_mode = 1;
		} else if (arf->recursive_mode != 2) {
			arf->recursive_mode = 2;
			arf->cur_best_depth = arf->depth;
		} else {
			if (arf->depth < arf->cur_best_depth) arf->cur_best_depth = arf->depth;
		}
		return;
	}

	// Depth too deep?
	if (arf->depth >= 4) {
		uint dist = DistanceMaxPlusManhattan(tile, arf->final_tile);

		if (dist < arf->cur_best_dist) {
			// Store the tile that is closest to the final position.
			arf->cur_best_depth = arf->depth;
			arf->cur_best_dist = dist;
			arf->cur_best_tile = tile;
			arf->cur_best_dir = dir;
		}
		return;
	}

	// Increase recursion depth
	arf->depth++;

	// Grab pointer to list of stuff that is possible to build
	p = _ai_table_15[dir];

	// Try to build a single rail in all directions.
	if (GetTileZ(tile) == 0) {
		p += 6;
	} else {
		do {
			// Make sure the tile is not in the list of banned tiles and that a rail can be built here.
			if (!AiIsTileBanned(arf->player, tile, p[0]) &&
					!CmdFailed(DoCommand(tile, arf->player->ai.railtype_to_use, p[0], DC_AUTO | DC_NO_WATER | DC_NO_RAIL_OVERLAP, CMD_BUILD_SINGLE_RAIL))) {
				AiBuildRailRecursive(arf, tile, p[1]);
			}

			// At the bottom depth?
			if (arf->depth == 1) AiCheckRailPathBetter(arf, p);

			p += 2;
		} while (!(p[0]&0x80));
	}

	AiCheckBuildRailBridgeHere(arf, tile, p);
	AiCheckBuildRailTunnelHere(arf, tile, p+1);

	arf->depth--;
}


static const byte _dir_table_3[]= {0x25, 0x2A, 0x19, 0x16};

static void AiBuildRailConstruct(Player *p)
{
	AiRailFinder arf;
	int i;

	// Check too much lookahead?
	if (AiDoFollowTrack(p)) {
		p->ai.state_counter = (Random()&0xE)+6; // Destruct this amount of blocks
		p->ai.state_mode = 1; // Start destruct

		// Ban this tile and don't reach it for a while.
		AiBanTile(p, p->ai.cur_tile_a, FindFirstBit(GetRailTrackStatus(p->ai.cur_tile_a)));
		return;
	}

	// Setup recursive finder and call it.
	arf.player = p;
	arf.final_tile = p->ai.cur_tile_b;
	arf.final_dir = p->ai.cur_dir_b;
	arf.depth = 0;
	arf.recursive_mode = 0;
	arf.best_ptr = NULL;
	arf.cur_best_dist = (uint)-1;
	arf.cur_best_depth = 0xff;
	arf.best_dist = (uint)-1;
	arf.best_depth = 0xff;
	arf.cur_best_tile = 0;
	arf.best_tile = 0;
	AiBuildRailRecursive(&arf, p->ai.cur_tile_a, p->ai.cur_dir_a);

	// Reached destination?
	if (arf.recursive_mode == 2 && arf.cur_best_depth == 0) {
		p->ai.state_mode = 255;
		return;
	}

	// Didn't find anything to build?
	if (arf.best_ptr == NULL) {
		// Terraform some
		for (i = 0; i != 5; i++) {
			AiDoTerraformLand(p->ai.cur_tile_a, p->ai.cur_dir_a, 3, 0);
		}

		if (++p->ai.state_counter == 21) {
			p->ai.state_counter = 40;
			p->ai.state_mode = 1;

			// Ban this tile
			AiBanTile(p, p->ai.cur_tile_a, FindFirstBit(GetRailTrackStatus(p->ai.cur_tile_a)));
		}
		return;
	}

	p->ai.cur_tile_a += TileOffsByDiagDir(p->ai.cur_dir_a);

	if (arf.best_ptr[0] & 0x80) {
		int i;
		int32 bridge_len = GetBridgeLength(arf.bridge_end_tile, p->ai.cur_tile_a);

		/* Figure out which (rail)bridge type to build
		 * start with best bridge, then go down to worse and worse bridges
		 * unnecessary to check for worst bridge (i=0), since AI will always build
		 * that. AI is so fucked up that fixing this small thing will probably not
		 * solve a thing
		 */
		for (i = MAX_BRIDGES - 1; i != 0; i--) {
			if (CheckBridge_Stuff(i, bridge_len)) {
				int32 cost = DoCommand(arf.bridge_end_tile, p->ai.cur_tile_a, i | (p->ai.railtype_to_use << 8), DC_AUTO, CMD_BUILD_BRIDGE);
				if (!CmdFailed(cost) && cost < (p->player_money >> 5)) break;
			}
		}

		// Build it
		DoCommand(arf.bridge_end_tile, p->ai.cur_tile_a, i | (p->ai.railtype_to_use << 8), DC_AUTO | DC_EXEC, CMD_BUILD_BRIDGE);

		p->ai.cur_tile_a = arf.bridge_end_tile;
		p->ai.state_counter = 0;
	} else if (arf.best_ptr[0] & 0x40) {
		// tunnel
		DoCommand(p->ai.cur_tile_a, p->ai.railtype_to_use, 0, DC_AUTO | DC_EXEC, CMD_BUILD_TUNNEL);
		p->ai.cur_tile_a = _build_tunnel_endtile;
		p->ai.state_counter = 0;
	} else {
		// rail
		p->ai.cur_dir_a = arf.best_ptr[1];
		DoCommand(p->ai.cur_tile_a, p->ai.railtype_to_use, arf.best_ptr[0],
			DC_EXEC | DC_AUTO | DC_NO_WATER | DC_NO_RAIL_OVERLAP, CMD_BUILD_SINGLE_RAIL);
		p->ai.state_counter = 0;
	}

	if (arf.best_tile != 0) {
		for (i = 0; i != 2; i++) {
			AiDoTerraformLand(arf.best_tile, arf.best_dir, 3, 0);
		}
	}
}

static bool AiRemoveTileAndGoForward(Player *p)
{
	byte b;
	int bit;
	const byte *ptr;
	TileIndex tile = p->ai.cur_tile_a;
	TileIndex tilenew;

	if (IsTileType(tile, MP_TUNNELBRIDGE)) {
		if (IsTunnel(tile)) {
			// Clear the tunnel and continue at the other side of it.
			if (CmdFailed(DoCommand(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR)))
				return false;
			p->ai.cur_tile_a = TILE_MASK(_build_tunnel_endtile - TileOffsByDiagDir(p->ai.cur_dir_a));
			return true;
		}

		if (IsBridgeRamp(tile)) {
			// Check if the bridge points in the right direction.
			// This is not really needed the first place AiRemoveTileAndGoForward is called.
			if (DiagDirToAxis(GetBridgeRampDirection(tile)) != (p->ai.cur_dir_a & 1U)) return false;

			tile = GetOtherBridgeEnd(tile);

			tilenew = TILE_MASK(tile - TileOffsByDiagDir(p->ai.cur_dir_a));
			// And clear the bridge.
			if (CmdFailed(DoCommand(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR)))
				return false;
			p->ai.cur_tile_a = tilenew;
			return true;
		}
	}

	// Find the railtype at the position. Quit if no rail there.
	b = GetRailTrackStatus(tile) & _dir_table_3[p->ai.cur_dir_a];
	if (b == 0) return false;

	// Convert into a bit position that CMD_REMOVE_SINGLE_RAIL expects.
	bit = FindFirstBit(b);

	// Then remove and signals if there are any.
	if (IsTileType(tile, MP_RAILWAY) &&
			GetRailTileType(tile) == RAIL_TILE_SIGNALS) {
		DoCommand(tile, 0, 0, DC_EXEC, CMD_REMOVE_SIGNALS);
	}

	// And also remove the rail.
	if (CmdFailed(DoCommand(tile, 0, bit, DC_EXEC, CMD_REMOVE_SINGLE_RAIL)))
		return false;

	// Find the direction at the other edge of the rail.
	ptr = _ai_table_15[p->ai.cur_dir_a ^ 2];
	while (ptr[0] != bit) ptr += 2;
	p->ai.cur_dir_a = ptr[1] ^ 2;

	// And then also switch tile.
	p->ai.cur_tile_a = TILE_MASK(p->ai.cur_tile_a - TileOffsByDiagDir(p->ai.cur_dir_a));

	return true;
}


static void AiBuildRailDestruct(Player *p)
{
	// Decrease timeout.
	if (!--p->ai.state_counter) {
		p->ai.state_mode = 2;
		p->ai.state_counter = 0;
	}

	// Don't do anything if the destination is already reached.
	if (p->ai.cur_tile_a == p->ai.start_tile_a) return;

	AiRemoveTileAndGoForward(p);
}


static void AiBuildRail(Player *p)
{
	switch (p->ai.state_mode) {
		case 0: // Construct mode, build new rail.
			AiBuildRailConstruct(p);
			break;

		case 1: // Destruct mode, destroy the rail currently built.
			AiBuildRailDestruct(p);
			break;

		case 2: {
			uint i;

			// Terraform some and then try building again.
			for (i = 0; i != 4; i++) {
				AiDoTerraformLand(p->ai.cur_tile_a, p->ai.cur_dir_a, 3, 0);
			}

			if (++p->ai.state_counter == 4) {
				p->ai.state_counter = 0;
				p->ai.state_mode = 0;
			}
		}

		default: break;
	}
}

static void AiStateBuildRail(Player *p)
{
	int num;
	AiBuildRec *aib;
	byte cmd;
	TileIndex tile;
	int dir;

	// time out?
	if (++p->ai.timeout_counter == 1388) {
		p->ai.state = AIS_DELETE_RAIL_BLOCKS;
		return;
	}

	// Currently building a rail between two points?
	if (p->ai.state_mode != 255) {
		AiBuildRail(p);

		// Alternate between edges
		swap_tile(&p->ai.start_tile_a, &p->ai.start_tile_b);
		swap_tile(&p->ai.cur_tile_a, &p->ai.cur_tile_b);
		swap_byte(&p->ai.start_dir_a, &p->ai.start_dir_b);
		swap_byte(&p->ai.cur_dir_a, &p->ai.cur_dir_b);
		return;
	}

	// Now, find two new points to build between
	num = p->ai.num_build_rec;
	aib = &p->ai.src;

	for (;;) {
		cmd = aib->buildcmd_a;
		aib->buildcmd_a = 255;
		if (cmd != 255) break;

		cmd = aib->buildcmd_b;
		aib->buildcmd_b = 255;
		if (cmd != 255) break;

		aib++;
		if (--num == 0) {
			p->ai.state = AIS_BUILD_RAIL_VEH;
			p->ai.state_counter = 0; // timeout
			return;
		}
	}

	// Find first edge to build from.
	tile = AiGetEdgeOfDefaultRailBlock(aib->cur_building_rule, aib->use_tile, cmd&3, &dir);
	p->ai.start_tile_a = tile;
	p->ai.cur_tile_a = tile;
	p->ai.start_dir_a = dir;
	p->ai.cur_dir_a = dir;
	DoCommand(TILE_MASK(tile + TileOffsByDiagDir(dir)), 0, (dir&1)?1:0, DC_EXEC, CMD_REMOVE_SINGLE_RAIL);

	assert(TILE_MASK(tile) != 0xFF00);

	// Find second edge to build to
	aib = (&p->ai.src) + ((cmd >> 4)&0xF);
	tile = AiGetEdgeOfDefaultRailBlock(aib->cur_building_rule, aib->use_tile, (cmd>>2)&3, &dir);
	p->ai.start_tile_b = tile;
	p->ai.cur_tile_b = tile;
	p->ai.start_dir_b = dir;
	p->ai.cur_dir_b = dir;
	DoCommand(TILE_MASK(tile + TileOffsByDiagDir(dir)), 0, (dir&1)?1:0, DC_EXEC, CMD_REMOVE_SINGLE_RAIL);

	assert(TILE_MASK(tile) != 0xFF00);

	// And setup state.
	p->ai.state_mode = 2;
	p->ai.state_counter = 0;
	p->ai.banned_tile_count = 0;
}

static StationID AiGetStationIdByDef(TileIndex tile, int id)
{
	const AiDefaultBlockData *p = _default_rail_track_data[id]->data;
	while (p->mode != 1) p++;
	return GetStationIndex(TILE_ADD(tile, ToTileIndexDiff(p->tileoffs)));
}

static EngineID AiFindBestWagon(CargoID cargo, RailType railtype)
{
	EngineID best_veh_index = INVALID_ENGINE;
	EngineID i;
	uint16 best_capacity = 0;
	uint16 best_speed    = 0;
	uint speed;

	for (i = 0; i < NUM_TRAIN_ENGINES; i++) {
		const RailVehicleInfo *rvi = RailVehInfo(i);
		const Engine* e = GetEngine(i);

		if (!IsCompatibleRail(e->railtype, railtype) ||
				!(rvi->flags & RVI_WAGON) ||
				!HASBIT(e->player_avail, _current_player)) {
			continue;
		}

		if (rvi->cargo_type != cargo) continue;

		/* max_speed of 0 indicates no speed limit */
		speed = rvi->max_speed == 0 ? 0xFFFF : rvi->max_speed;

		if (rvi->capacity >= best_capacity && speed >= best_speed) {
			best_capacity = rvi->capacity;
			best_speed    = best_speed;
			best_veh_index = i;
		}
	}

	return best_veh_index;
}

static void AiStateBuildRailVeh(Player *p)
{
	const AiDefaultBlockData *ptr;
	TileIndex tile;
	EngineID veh;
	int i;
	CargoID cargo;
	int32 cost;
	Vehicle *v;
	VehicleID loco_id;

	ptr = _default_rail_track_data[p->ai.src.cur_building_rule]->data;
	while (ptr->mode != 0) ptr++;

	tile = TILE_ADD(p->ai.src.use_tile, ToTileIndexDiff(ptr->tileoffs));


	cargo = p->ai.cargo_type;
	for (i = 0;;) {
		if (p->ai.wagon_list[i] == INVALID_VEHICLE) {
			veh = AiFindBestWagon(cargo, p->ai.railtype_to_use);
			/* veh will return INVALID_ENGINE if no suitable wagon is available.
			 * We shall treat this in the same way as having no money */
			if (veh == INVALID_ENGINE) goto handle_nocash;
			cost = DoCommand(tile, veh, 0, DC_EXEC, CMD_BUILD_RAIL_VEHICLE);
			if (CmdFailed(cost)) goto handle_nocash;
			p->ai.wagon_list[i] = _new_vehicle_id;
			p->ai.wagon_list[i + 1] = INVALID_VEHICLE;
			return;
		}
		if (cargo == CT_MAIL) cargo = CT_PASSENGERS;
		if (++i == p->ai.num_wagons * 2 - 1) break;
	}

	// Which locomotive to build?
	veh = AiChooseTrainToBuild(p->ai.railtype_to_use, p->player_money, cargo != CT_PASSENGERS ? 1 : 0, tile);
	if (veh == INVALID_ENGINE) {
handle_nocash:
		// after a while, if AI still doesn't have cash, get out of this block by selling the wagons.
		if (++p->ai.state_counter == 1000) {
			for (i = 0; p->ai.wagon_list[i] != INVALID_VEHICLE; i++) {
				cost = DoCommand(tile, p->ai.wagon_list[i], 0, DC_EXEC, CMD_SELL_RAIL_WAGON);
				assert(!CmdFailed(cost));
			}
			p->ai.state = AIS_0;
		}
		return;
	}

	// Try to build the locomotive
	cost = DoCommand(tile, veh, 0, DC_EXEC, CMD_BUILD_RAIL_VEHICLE);
	assert(!CmdFailed(cost));
	loco_id = _new_vehicle_id;

	// Sell a vehicle if the train is double headed.
	v = GetVehicle(loco_id);
	if (v->next != NULL) {
		i = p->ai.wagon_list[p->ai.num_wagons * 2 - 2];
		p->ai.wagon_list[p->ai.num_wagons * 2 - 2] = INVALID_VEHICLE;
		DoCommand(tile, i, 0, DC_EXEC, CMD_SELL_RAIL_WAGON);
	}

	// Move the wagons onto the train
	for (i = 0; p->ai.wagon_list[i] != INVALID_VEHICLE; i++) {
		DoCommand(tile, p->ai.wagon_list[i] | (loco_id << 16), 0, DC_EXEC, CMD_MOVE_RAIL_VEHICLE);
	}

	for (i = 0; p->ai.order_list_blocks[i] != 0xFF; i++) {
		const AiBuildRec* aib = &p->ai.src + p->ai.order_list_blocks[i];
		bool is_pass = (
			p->ai.cargo_type == CT_PASSENGERS ||
			p->ai.cargo_type == CT_MAIL ||
			(_opt.landscape == LT_NORMAL && p->ai.cargo_type == CT_VALUABLES)
		);
		Order order;

		order.type = OT_GOTO_STATION;
		order.flags = 0;
		order.dest = AiGetStationIdByDef(aib->use_tile, aib->cur_building_rule);

		if (!is_pass && i == 1) order.flags |= OF_UNLOAD;
		if (p->ai.num_want_fullload != 0 && (is_pass || i == 0))
			order.flags |= OF_FULL_LOAD;

		DoCommand(0, loco_id + (i << 16), PackOrder(&order), DC_EXEC, CMD_INSERT_ORDER);
	}

	DoCommand(0, loco_id, 0, DC_EXEC, CMD_START_STOP_TRAIN);

	DoCommand(0, loco_id, _ai_service_interval, DC_EXEC, CMD_CHANGE_SERVICE_INT);

	if (p->ai.num_want_fullload != 0) p->ai.num_want_fullload--;

	if (--p->ai.num_loco_to_build != 0) {
//		p->ai.loco_id = INVALID_VEHICLE;
		p->ai.wagon_list[0] = INVALID_VEHICLE;
	} else {
		p->ai.state = AIS_0;
	}
}

static void AiStateDeleteRailBlocks(Player *p)
{
	const AiBuildRec* aib = &p->ai.src;
	uint num = p->ai.num_build_rec;

	do {
		const AiDefaultBlockData* b;

		if (aib->cur_building_rule == 255) continue;
		for (b = _default_rail_track_data[aib->cur_building_rule]->data; b->mode != 4; b++) {
			DoCommand(TILE_ADD(aib->use_tile, ToTileIndexDiff(b->tileoffs)), 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
		}
	} while (++aib,--num);

	p->ai.state = AIS_0;
}

static bool AiCheckRoadResources(TileIndex tile, const AiDefaultBlockData *p, byte cargo)
{
	uint values[NUM_CARGO];
	int rad;

	if (_patches.modified_catchment) {
		rad = CA_TRUCK; // Same as CA_BUS at the moment?
	} else { // change that at some point?
		rad = 4;
	}

	for (;; p++) {
		if (p->mode == 4) {
			return true;
		} else if (p->mode == 1) {
			TileIndex tile2 = TILE_ADD(tile, ToTileIndexDiff(p->tileoffs));

			if (cargo & 0x80) {
				GetProductionAroundTiles(values, tile2, 1, 1, rad);
				return values[cargo & 0x7F] != 0;
			} else {
				GetAcceptanceAroundTiles(values, tile2, 1, 1, rad);
				return (values[cargo]&~7) != 0;
			}
		}
	}
}

static bool _want_road_truck_station;
static int32 AiDoBuildDefaultRoadBlock(TileIndex tile, const AiDefaultBlockData *p, byte flag);

// Returns rule and cost
static int AiFindBestDefaultRoadBlock(TileIndex tile, byte direction, byte cargo, int32 *cost)
{
	int i;
	const AiDefaultRoadBlock *p;

	_want_road_truck_station = (cargo & 0x7F) != CT_PASSENGERS;

	for (i = 0; (p = _road_default_block_data[i]) != NULL; i++) {
		if (p->dir == direction) {
			*cost = AiDoBuildDefaultRoadBlock(tile, p->data, 0);
			if (!CmdFailed(*cost) && AiCheckRoadResources(tile, p->data, cargo))
				return i;
		}
	}

	return -1;
}

static int32 AiDoBuildDefaultRoadBlock(TileIndex tile, const AiDefaultBlockData *p, byte flag)
{
	int32 ret;
	int32 total_cost = 0;
	Town *t = NULL;
	int rating = 0;
	int roadflag = 0;

	for (;p->mode != 4;p++) {
		TileIndex c = TILE_MASK(tile + ToTileIndexDiff(p->tileoffs));

		_cleared_town = NULL;

		if (p->mode == 2) {
			if (IsTileType(c, MP_STREET) &&
					GetRoadTileType(c) == ROAD_TILE_NORMAL &&
					(GetRoadBits(c) & p->attr) != 0) {
				roadflag |= 2;

				// all bits are already built?
				if ((GetRoadBits(c) & p->attr) == p->attr) continue;
			}

			ret = DoCommand(c, p->attr, 0, flag | DC_AUTO | DC_NO_WATER, CMD_BUILD_ROAD);
			if (CmdFailed(ret)) return CMD_ERROR;
			total_cost += ret;

			continue;
		}

		if (p->mode == 0) {
			// Depot
			ret = DoCommand(c, p->attr, 0, flag | DC_AUTO | DC_NO_WATER | DC_AI_BUILDING, CMD_BUILD_ROAD_DEPOT);
			goto clear_town_stuff;
		} else if (p->mode == 1) {
			if (_want_road_truck_station) {
				// Truck station
				ret = DoCommand(c, p->attr, RS_TRUCK, flag | DC_AUTO | DC_NO_WATER | DC_AI_BUILDING, CMD_BUILD_ROAD_STOP);
			} else {
				// Bus station
				ret = DoCommand(c, p->attr, RS_BUS, flag | DC_AUTO | DC_NO_WATER | DC_AI_BUILDING, CMD_BUILD_ROAD_STOP);
			}
clear_town_stuff:;

			if (CmdFailed(ret)) return CMD_ERROR;
			total_cost += ret;

			if (_cleared_town != NULL) {
				if (t != NULL && t != _cleared_town) return CMD_ERROR;
				t = _cleared_town;
				rating += _cleared_town_rating;
			}
		} else if (p->mode == 3) {
			if (flag & DC_EXEC) continue;

			if (GetTileSlope(c, NULL) != SLOPE_FLAT) return CMD_ERROR;

			if (!IsTileType(c, MP_STREET) || GetRoadTileType(c) != ROAD_TILE_NORMAL) {
				ret = DoCommand(c, 0, 0, flag | DC_AUTO | DC_NO_WATER | DC_AI_BUILDING, CMD_LANDSCAPE_CLEAR);
				if (CmdFailed(ret)) return CMD_ERROR;
			}

		}
	}

	if (!_want_road_truck_station && !(roadflag & 2)) return CMD_ERROR;

	if (!(flag & DC_EXEC)) {
		if (t != NULL && rating > t->ratings[_current_player]) return CMD_ERROR;
	}
	return total_cost;
}

// Make sure the blocks are not too close to each other
static bool AiCheckBlockDistances(Player *p, TileIndex tile)
{
	const AiBuildRec* aib = &p->ai.src;
	uint num = p->ai.num_build_rec;

	do {
		if (aib->cur_building_rule != 255) {
			if (DistanceManhattan(aib->use_tile, tile) < 9) return false;
		}
	} while (++aib, --num);

	return true;
}


static void AiStateBuildDefaultRoadBlocks(Player *p)
{
	uint i;
	int j;
	AiBuildRec *aib;
	int rule;
	int32 cost;

	// time out?
	if (++p->ai.timeout_counter == 1388) {
		p->ai.state = AIS_DELETE_RAIL_BLOCKS;
		return;
	}

	// do the following 8 times
	for (i = 0; i != 8; i++) {
		// check if we can build the default track
		aib = &p->ai.src;
		j = p->ai.num_build_rec;
		do {
			// this item has already been built?
			if (aib->cur_building_rule != 255) continue;

			// adjust the coordinate randomly,
			// to make sure that we find a position.
			aib->use_tile = AdjustTileCoordRandomly(aib->spec_tile, aib->rand_rng);

			// check if the road can be built there.
			rule = AiFindBestDefaultRoadBlock(
				aib->use_tile, aib->direction, aib->cargo, &cost
			);

			if (rule == -1) {
				// cannot build, terraform after a while
				if (p->ai.state_counter >= 600) {
					AiDoTerraformLand(aib->use_tile, Random()&3, 3, (int8)p->ai.state_mode);
				}
				// also try the other terraform direction
				if (++p->ai.state_counter >= 1000) {
					p->ai.state_counter = 0;
					p->ai.state_mode = -p->ai.state_mode;
				}
			} else if (CheckPlayerHasMoney(cost) && AiCheckBlockDistances(p,aib->use_tile)) {
				int32 r;

				// player has money, build it.
				aib->cur_building_rule = rule;

				r = AiDoBuildDefaultRoadBlock(
					aib->use_tile,
					_road_default_block_data[rule]->data,
					DC_EXEC | DC_NO_TOWN_RATING
				);
				assert(!CmdFailed(r));
			}
		} while (++aib,--j);
	}

	// check if we're done with all of them
	aib = &p->ai.src;
	j = p->ai.num_build_rec;
	do {
		if (aib->cur_building_rule == 255) return;
	} while (++aib,--j);

	// yep, all are done. switch state to the rail building state.
	p->ai.state = AIS_BUILD_ROAD;
	p->ai.state_mode = 255;
}

typedef struct {
	TileIndex final_tile;
	byte final_dir;
	byte depth;
	byte recursive_mode;
	byte cur_best_dir;
	byte best_dir;
	byte cur_best_depth;
	byte best_depth;
	uint cur_best_dist;
	const byte *best_ptr;
	uint best_dist;
	TileIndex cur_best_tile, best_tile;
	TileIndex bridge_end_tile;
	Player *player;
} AiRoadFinder;

typedef struct AiRoadEnum {
	TileIndex dest;
	TileIndex best_tile;
	int best_track;
	uint best_dist;
} AiRoadEnum;

static const byte _dir_by_track[] = {
	0, 1, 0, 1, 2, 1,
	0, 0,
	2, 3, 3, 2, 3, 0,
};

static void AiBuildRoadRecursive(AiRoadFinder *arf, TileIndex tile, int dir);

static bool AiCheckRoadPathBetter(AiRoadFinder *arf, const byte *p)
{
	bool better = false;

	if (arf->recursive_mode < 1) {
		// Mode is 0. This means destination has not been found yet.
		// If the found path is shorter than the current one, remember it.
		if (arf->cur_best_dist < arf->best_dist ||
			(arf->cur_best_dist == arf->best_dist && arf->cur_best_depth < arf->best_depth)) {
			arf->best_depth = arf->cur_best_depth;
			arf->best_dist = arf->cur_best_dist;
			arf->best_dir = arf->cur_best_dir;
			arf->best_ptr = p;
			arf->best_tile = arf->cur_best_tile;
			better = true;
		}
	} else if (arf->recursive_mode > 1) {
		// Mode is 2.
		if (arf->best_dist != 0 || arf->cur_best_depth < arf->best_depth) {
			arf->best_depth = arf->cur_best_depth;
			arf->best_dist = 0;
			arf->best_ptr = p;
			arf->best_tile = 0;
			better = true;
		}
	}
	arf->recursive_mode = 0;
	arf->cur_best_dist = (uint)-1;
	arf->cur_best_depth = 0xff;

	return better;
}


static bool AiEnumFollowRoad(TileIndex tile, AiRoadEnum *a, int track, uint length, byte *state)
{
	uint dist = DistanceManhattan(tile, a->dest);

	if (dist <= a->best_dist) {
		TileIndex tile2 = TILE_MASK(tile + TileOffsByDiagDir(_dir_by_track[track]));

		if (IsTileType(tile2, MP_STREET) && GetRoadTileType(tile2) == ROAD_TILE_NORMAL) {
			a->best_dist = dist;
			a->best_tile = tile;
			a->best_track = track;
		}
	}

	return false;
}

static const uint16 _ai_road_table_and[4] = {
	0x1009,
	0x16,
	0x520,
	0x2A00,
};

static bool AiCheckRoadFinished(Player *p)
{
	AiRoadEnum are;
	TileIndex tile;
	int dir = p->ai.cur_dir_a;
	uint32 bits;
	int i;

	are.dest = p->ai.cur_tile_b;
	tile = TILE_MASK(p->ai.cur_tile_a + TileOffsByDiagDir(dir));

	if (IsRoadStopTile(tile) || IsTileDepotType(tile, TRANSPORT_ROAD)) return false;
	bits = GetTileTrackStatus(tile, TRANSPORT_ROAD) & _ai_road_table_and[dir];
	if (bits == 0) return false;

	are.best_dist = (uint)-1;

	for_each_bit(i, bits) {
		FollowTrack(tile, 0x3000 | TRANSPORT_ROAD, _dir_by_track[i], (TPFEnumProc*)AiEnumFollowRoad, NULL, &are);
	}

	if (DistanceManhattan(tile, are.dest) <= are.best_dist) return false;

	if (are.best_dist == 0) return true;

	p->ai.cur_tile_a = are.best_tile;
	p->ai.cur_dir_a = _dir_by_track[are.best_track];
	return false;
}


static bool AiBuildRoadHelper(TileIndex tile, int flags, int type)
{
	static const RoadBits _road_bits[] = {
		ROAD_X,
		ROAD_Y,
		ROAD_NW | ROAD_NE,
		ROAD_SW | ROAD_SE,
		ROAD_NW | ROAD_SW,
		ROAD_SE | ROAD_NE
	};
	return !CmdFailed(DoCommand(tile, _road_bits[type], 0, flags, CMD_BUILD_ROAD));
}

static inline void AiCheckBuildRoadBridgeHere(AiRoadFinder *arf, TileIndex tile, const byte *p)
{
	Slope tileh;
	uint z;
	bool flag;

	int dir2 = p[0] & 3;

	tileh = GetTileSlope(tile, &z);
	if (tileh == _dir_table_1[dir2] || (tileh == SLOPE_FLAT && z != 0)) {
		TileIndex tile_new = tile;

		// Allow bridges directly over bottom tiles
		flag = z == 0;
		for (;;) {
			TileType type;

			if ((TileIndexDiff)tile_new < -TileOffsByDiagDir(dir2)) return; // Wraping around map, no bridge possible!
			tile_new = TILE_MASK(tile_new + TileOffsByDiagDir(dir2));
			type = GetTileType(tile_new);

			if (type == MP_CLEAR || type == MP_TREES || GetTileSlope(tile, NULL) != SLOPE_FLAT) {
				// Allow a bridge if either we have a tile that's water, rail or street,
				// or if we found an up tile.
				if (!flag) return;
				break;
			}
			if (type != MP_WATER && type != MP_RAILWAY && type != MP_STREET) return;
			flag = true;
		}

		// Is building a (rail)bridge possible at this place (type doesn't matter)?
		if (CmdFailed(DoCommand(tile_new, tile, 0x8000, DC_AUTO, CMD_BUILD_BRIDGE)))
			return;
		AiBuildRoadRecursive(arf, tile_new, dir2);

		// At the bottom depth, check if the new path is better than the old one.
		if (arf->depth == 1) {
			if (AiCheckRoadPathBetter(arf, p)) arf->bridge_end_tile = tile_new;
		}
	}
}

static inline void AiCheckBuildRoadTunnelHere(AiRoadFinder *arf, TileIndex tile, const byte *p)
{
	uint z;

	if (GetTileSlope(tile, &z) == _dir_table_2[p[0] & 3] && z != 0) {
		int32 cost = DoCommand(tile, 0x200, 0, DC_AUTO, CMD_BUILD_TUNNEL);

		if (!CmdFailed(cost) && cost <= (arf->player->player_money>>4)) {
			AiBuildRoadRecursive(arf, _build_tunnel_endtile, p[0]&3);
			if (arf->depth == 1)  AiCheckRoadPathBetter(arf, p);
		}
	}
}



static void AiBuildRoadRecursive(AiRoadFinder *arf, TileIndex tile, int dir)
{
	const byte *p;

	tile = TILE_MASK(tile + TileOffsByDiagDir(dir));

	// Reached destination?
	if (tile == arf->final_tile) {
		if ((arf->final_dir^2) == dir) {
			arf->recursive_mode = 2;
			arf->cur_best_depth = arf->depth;
		}
		return;
	}

	// Depth too deep?
	if (arf->depth >= 4) {
		uint dist = DistanceMaxPlusManhattan(tile, arf->final_tile);
		if (dist < arf->cur_best_dist) {
			// Store the tile that is closest to the final position.
			arf->cur_best_dist = dist;
			arf->cur_best_tile = tile;
			arf->cur_best_dir = dir;
			arf->cur_best_depth = arf->depth;
		}
		return;
	}

	// Increase recursion depth
	arf->depth++;

	// Grab pointer to list of stuff that is possible to build
	p = _ai_table_15[dir];

	// Try to build a single rail in all directions.
	if (GetTileZ(tile) == 0) {
		p += 6;
	} else {
		do {
			// Make sure that a road can be built here.
			if (AiBuildRoadHelper(tile, DC_AUTO | DC_NO_WATER | DC_AI_BUILDING, p[0])) {
				AiBuildRoadRecursive(arf, tile, p[1]);
			}

			// At the bottom depth?
			if (arf->depth == 1) AiCheckRoadPathBetter(arf, p);

			p += 2;
		} while (!(p[0] & 0x80));
	}

	AiCheckBuildRoadBridgeHere(arf, tile, p);
	AiCheckBuildRoadTunnelHere(arf, tile, p+1);

	arf->depth--;
}


static void AiBuildRoadConstruct(Player *p)
{
	AiRoadFinder arf;
	int i;
	TileIndex tile;

	// Reached destination?
	if (AiCheckRoadFinished(p)) {
		p->ai.state_mode = 255;
		return;
	}

	// Setup recursive finder and call it.
	arf.player = p;
	arf.final_tile = p->ai.cur_tile_b;
	arf.final_dir = p->ai.cur_dir_b;
	arf.depth = 0;
	arf.recursive_mode = 0;
	arf.best_ptr = NULL;
	arf.cur_best_dist = (uint)-1;
	arf.cur_best_depth = 0xff;
	arf.best_dist = (uint)-1;
	arf.best_depth =  0xff;
	arf.cur_best_tile = 0;
	arf.best_tile = 0;
	AiBuildRoadRecursive(&arf, p->ai.cur_tile_a, p->ai.cur_dir_a);

	// Reached destination?
	if (arf.recursive_mode == 2 && arf.cur_best_depth == 0) {
		p->ai.state_mode = 255;
		return;
	}

	// Didn't find anything to build?
	if (arf.best_ptr == NULL) {
		// Terraform some
do_some_terraform:
		for (i = 0; i != 5; i++)
			AiDoTerraformLand(p->ai.cur_tile_a, p->ai.cur_dir_a, 3, 0);

		if (++p->ai.state_counter == 21) {
			p->ai.state_mode = 1;

			p->ai.cur_tile_a = TILE_MASK(p->ai.cur_tile_a + TileOffsByDiagDir(p->ai.cur_dir_a));
			p->ai.cur_dir_a ^= 2;
			p->ai.state_counter = 0;
		}
		return;
	}

	tile = TILE_MASK(p->ai.cur_tile_a + TileOffsByDiagDir(p->ai.cur_dir_a));

	if (arf.best_ptr[0]&0x80) {
		int i;
		int32 bridge_len;
		p->ai.cur_tile_a = arf.bridge_end_tile;
		bridge_len = GetBridgeLength(tile, p->ai.cur_tile_a); // tile

		/* Figure out what (road)bridge type to build
		 * start with best bridge, then go down to worse and worse bridges
		 * unnecessary to check for worse bridge (i=0), since AI will always build that.
		 *AI is so fucked up that fixing this small thing will probably not solve a thing
		 */
		for (i = 10; i != 0; i--) {
			if (CheckBridge_Stuff(i, bridge_len)) {
				int32 cost = DoCommand(tile, p->ai.cur_tile_a, i + (0x80 << 8), DC_AUTO, CMD_BUILD_BRIDGE);
				if (!CmdFailed(cost) && cost < (p->player_money >> 5)) break;
			}
		}

		// Build it
		DoCommand(tile, p->ai.cur_tile_a, i + (0x80 << 8), DC_AUTO | DC_EXEC, CMD_BUILD_BRIDGE);

		p->ai.state_counter = 0;
	} else if (arf.best_ptr[0]&0x40) {
		// tunnel
		DoCommand(tile, 0x200, 0, DC_AUTO | DC_EXEC, CMD_BUILD_TUNNEL);
		p->ai.cur_tile_a = _build_tunnel_endtile;
		p->ai.state_counter = 0;
	} else {
		// road
		if (!AiBuildRoadHelper(tile, DC_EXEC | DC_AUTO | DC_NO_WATER | DC_AI_BUILDING, arf.best_ptr[0]))
			goto do_some_terraform;

		p->ai.cur_dir_a = arf.best_ptr[1];
		p->ai.cur_tile_a = tile;
		p->ai.state_counter = 0;
	}

	if (arf.best_tile != 0) {
		for (i = 0; i != 2; i++)
			AiDoTerraformLand(arf.best_tile, arf.best_dir, 3, 0);
	}
}


static void AiBuildRoad(Player *p)
{
	if (p->ai.state_mode < 1) {
		// Construct mode, build new road.
		AiBuildRoadConstruct(p);
	} else if (p->ai.state_mode == 1) {
		// Destruct mode, not implemented for roads.
		p->ai.state_mode = 2;
		p->ai.state_counter = 0;
	} else if (p->ai.state_mode == 2) {
		uint i;

		// Terraform some and then try building again.
		for (i = 0; i != 4; i++) {
			AiDoTerraformLand(p->ai.cur_tile_a, p->ai.cur_dir_a, 3, 0);
		}

		if (++p->ai.state_counter == 4) {
			p->ai.state_counter = 0;
			p->ai.state_mode = 0;
		}
	}
}

static TileIndex AiGetRoadBlockEdge(byte rule, TileIndex tile, int *dir)
{
	const AiDefaultBlockData *p = _road_default_block_data[rule]->data;
	while (p->mode != 1) p++;
	*dir = p->attr;
	return TILE_ADD(tile, ToTileIndexDiff(p->tileoffs));
}


static void AiStateBuildRoad(Player *p)
{
	int num;
	AiBuildRec *aib;
	byte cmd;
	TileIndex tile;
	int dir;

	// time out?
	if (++p->ai.timeout_counter == 1388) {
		p->ai.state = AIS_DELETE_ROAD_BLOCKS;
		return;
	}

	// Currently building a road between two points?
	if (p->ai.state_mode != 255) {
		AiBuildRoad(p);

		// Alternate between edges
		swap_tile(&p->ai.start_tile_a, &p->ai.start_tile_b);
		swap_tile(&p->ai.cur_tile_a, &p->ai.cur_tile_b);
		swap_byte(&p->ai.start_dir_a, &p->ai.start_dir_b);
		swap_byte(&p->ai.cur_dir_a, &p->ai.cur_dir_b);

		return;
	}

	// Now, find two new points to build between
	num = p->ai.num_build_rec;
	aib = &p->ai.src;

	for (;;) {
		cmd = aib->buildcmd_a;
		aib->buildcmd_a = 255;
		if (cmd != 255) break;

		aib++;
		if (--num == 0) {
			p->ai.state = AIS_BUILD_ROAD_VEHICLES;
			return;
		}
	}

	// Find first edge to build from.
	tile = AiGetRoadBlockEdge(aib->cur_building_rule, aib->use_tile, &dir);
	p->ai.start_tile_a = tile;
	p->ai.cur_tile_a = tile;
	p->ai.start_dir_a = dir;
	p->ai.cur_dir_a = dir;

	// Find second edge to build to
	aib = (&p->ai.src) + (cmd&0xF);
	tile = AiGetRoadBlockEdge(aib->cur_building_rule, aib->use_tile, &dir);
	p->ai.start_tile_b = tile;
	p->ai.cur_tile_b = tile;
	p->ai.start_dir_b = dir;
	p->ai.cur_dir_b = dir;

	// And setup state.
	p->ai.state_mode = 2;
	p->ai.state_counter = 0;
	p->ai.banned_tile_count = 0;
}

static StationID AiGetStationIdFromRoadBlock(TileIndex tile, int id)
{
	const AiDefaultBlockData *p = _road_default_block_data[id]->data;
	while (p->mode != 1) p++;
	return GetStationIndex(TILE_ADD(tile, ToTileIndexDiff(p->tileoffs)));
}

static void AiStateBuildRoadVehicles(Player *p)
{
	const AiDefaultBlockData *ptr;
	TileIndex tile;
	VehicleID loco_id;
	EngineID veh;
	uint i;

	ptr = _road_default_block_data[p->ai.src.cur_building_rule]->data;
	for (; ptr->mode != 0; ptr++) {}
	tile = TILE_ADD(p->ai.src.use_tile, ToTileIndexDiff(ptr->tileoffs));

	veh = AiChooseRoadVehToBuild(p->ai.cargo_type, p->player_money, tile);
	if (veh == INVALID_ENGINE) {
		p->ai.state = AIS_0;
		return;
	}

	if (CmdFailed(DoCommand(tile, veh, 0, DC_EXEC, CMD_BUILD_ROAD_VEH))) return;

	loco_id = _new_vehicle_id;

	if (GetVehicle(loco_id)->cargo_type != p->ai.cargo_type) {
		/* Cargo type doesn't match, so refit it */
		if (CmdFailed(DoCommand(tile, loco_id, p->ai.cargo_type, DC_EXEC, CMD_REFIT_ROAD_VEH))) {
			/* Refit failed... sell the vehicle */
			DoCommand(tile, loco_id, 0, DC_EXEC, CMD_SELL_ROAD_VEH);
			return;
		}
	}

	for (i = 0; p->ai.order_list_blocks[i] != 0xFF; i++) {
		const AiBuildRec* aib = &p->ai.src + p->ai.order_list_blocks[i];
		bool is_pass = (
			p->ai.cargo_type == CT_PASSENGERS ||
			p->ai.cargo_type == CT_MAIL ||
			(_opt.landscape == LT_NORMAL && p->ai.cargo_type == CT_VALUABLES)
		);
		Order order;

		order.type = OT_GOTO_STATION;
		order.flags = 0;
		order.dest = AiGetStationIdFromRoadBlock(aib->use_tile, aib->cur_building_rule);

		if (!is_pass && i == 1) order.flags |= OF_UNLOAD;
		if (p->ai.num_want_fullload != 0 && (is_pass || i == 0))
			order.flags |= OF_FULL_LOAD;

		DoCommand(0, loco_id + (i << 16), PackOrder(&order), DC_EXEC, CMD_INSERT_ORDER);
	}

	DoCommand(0, loco_id, 0, DC_EXEC, CMD_START_STOP_ROADVEH);
	DoCommand(0, loco_id, _ai_service_interval, DC_EXEC, CMD_CHANGE_SERVICE_INT);

	if (p->ai.num_want_fullload != 0) p->ai.num_want_fullload--;
	if (--p->ai.num_loco_to_build == 0) p->ai.state = AIS_0;
}

static void AiStateDeleteRoadBlocks(Player *p)
{
	const AiBuildRec* aib = &p->ai.src;
	uint num = p->ai.num_build_rec;

	do {
		const AiDefaultBlockData* b;

		if (aib->cur_building_rule == 255) continue;
		for (b = _road_default_block_data[aib->cur_building_rule]->data; b->mode != 4; b++) {
			if (b->mode > 1) continue;
			DoCommand(TILE_ADD(aib->use_tile, ToTileIndexDiff(b->tileoffs)), 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
		}
	} while (++aib,--num);

	p->ai.state = AIS_0;
}


static void AiStateAirportStuff(Player *p)
{
	const Station* st;
	byte acc_planes;
	int i;
	AiBuildRec *aib;
	byte rule;

	// Here we look for an airport we could use instead of building a new
	// one. If we find such an aiport for any waypoint,
	// AiStateBuildDefaultAirportBlocks() will kindly skip that one when
	// building the waypoints.

	i = 0;
	do {
		// We do this all twice - once for the source (town in the case
		// of oilrig route) and then for the destination (oilrig in the
		// case of oilrig route).
		aib = &p->ai.src + i;

		FOR_ALL_STATIONS(st) {
			// Is this an airport?
			if (!(st->facilities & FACIL_AIRPORT)) continue;

			// Do we own the airport? (Oilrigs aren't owned, though.)
			if (st->owner != OWNER_NONE && st->owner != _current_player) continue;

			acc_planes = GetAirport(st->airport_type)->acc_planes;

			// Dismiss heliports, unless we are checking an oilrig.
			if (acc_planes == HELICOPTERS_ONLY && (p->ai.build_kind != 1 || i != 1))
				continue;

			// Dismiss country airports if we are doing the other
			// endpoint of an oilrig route.
			if (acc_planes == AIRCRAFT_ONLY && (p->ai.build_kind == 1 && i == 0))
				continue;

			// Dismiss airports too far away.
			if (DistanceMax(st->airport_tile, aib->spec_tile) > aib->rand_rng)
				continue;

			// It's ideal airport, let's take it!

			/* XXX: This part is utterly broken - rule should
			 * contain number of the rule appropriate for the
			 * airport type (country, town, ...), see
			 * _airport_default_block_data (rule is just an index
			 * in this array). But the only difference between the
			 * currently existing two rules (rule 0 - town and rule
			 * 1 - country) is the attr field which is used only
			 * when building new airports - and that's irrelevant
			 * for us. So using just about any rule will suffice
			 * here for now (some of the new airport types would be
			 * broken because they will probably need different
			 * tileoff values etc), no matter that
			 * IsHangarTile() makes no sense. --pasky */
			if (acc_planes == HELICOPTERS_ONLY) {
				/* Heliports should have maybe own rulesets but
				 * OTOH we don't want AI to pick them up when
				 * looking for a suitable airport type to build.
				 * So any of rules 0 or 1 would do for now. The
				 * original rule number was 2 but that's a bug
				 * because we have no such rule. */
				rule = 1;
			} else {
				rule = IsHangarTile(st->airport_tile);
			}

			aib->cur_building_rule = rule;
			aib->use_tile = st->airport_tile;
			break;
		}
	} while (++i != p->ai.num_build_rec);

	p->ai.state = AIS_BUILD_DEFAULT_AIRPORT_BLOCKS;
	p->ai.state_mode = 255;
	p->ai.state_counter = 0;
}

static int32 AiDoBuildDefaultAirportBlock(TileIndex tile, const AiDefaultBlockData *p, byte flag)
{
	int32 total_cost = 0, ret;

	for (; p->mode == 0; p++) {
		if (!HASBIT(_avail_aircraft, p->attr)) return CMD_ERROR;
		ret = DoCommand(TILE_MASK(tile + ToTileIndexDiff(p->tileoffs)), p->attr,0,flag | DC_AUTO | DC_NO_WATER,CMD_BUILD_AIRPORT);
		if (CmdFailed(ret)) return CMD_ERROR;
		total_cost += ret;
	}

	return total_cost;
}

static bool AiCheckAirportResources(TileIndex tile, const AiDefaultBlockData *p, byte cargo)
{
	uint values[NUM_CARGO];
	int rad;

	if (_patches.modified_catchment) {
		rad = CA_AIR_LARGE; // I Have NFI what airport the
	} else { // AI is going to build here
		rad = 4;
	}

	for (; p->mode == 0; p++) {
		TileIndex tile2 = TILE_ADD(tile, ToTileIndexDiff(p->tileoffs));
		const AirportFTAClass* airport = GetAirport(p->attr);
		uint w = airport->size_x;
		uint h = airport->size_y;

		if (cargo & 0x80) {
			GetProductionAroundTiles(values, tile2, w, h, rad);
			return values[cargo & 0x7F] != 0;
		} else {
			GetAcceptanceAroundTiles(values, tile2, w, h, rad);
			return values[cargo] >= 8;
		}
	}
	return true;
}

static int AiFindBestDefaultAirportBlock(TileIndex tile, byte cargo, byte heli, int32 *cost)
{
	const AiDefaultBlockData *p;
	uint i;

	for (i = 0; (p = _airport_default_block_data[i]) != NULL; i++) {
		// If we are doing a helicopter service, avoid building
		// airports where they can't land.
		if (heli && GetAirport(p->attr)->acc_planes == AIRCRAFT_ONLY) continue;

		*cost = AiDoBuildDefaultAirportBlock(tile, p, 0);
		if (!CmdFailed(*cost) && AiCheckAirportResources(tile, p, cargo))
			return i;
	}
	return -1;
}

static void AiStateBuildDefaultAirportBlocks(Player *p)
{
	int i, j;
	AiBuildRec *aib;
	int rule;
	int32 cost;

	// time out?
	if (++p->ai.timeout_counter == 1388) {
		p->ai.state = AIS_0;
		return;
	}

	// do the following 8 times
	i = 8;
	do {
		// check if we can build the default
		aib = &p->ai.src;
		j = p->ai.num_build_rec;
		do {
			// this item has already been built?
			if (aib->cur_building_rule != 255) continue;

			// adjust the coordinate randomly,
			// to make sure that we find a position.
			aib->use_tile = AdjustTileCoordRandomly(aib->spec_tile, aib->rand_rng);

			// check if the aircraft stuff can be built there.
			rule = AiFindBestDefaultAirportBlock(aib->use_tile, aib->cargo, p->ai.build_kind, &cost);

//			SetRedErrorSquare(aib->use_tile);

			if (rule == -1) {
				// cannot build, terraform after a while
				if (p->ai.state_counter >= 600) {
					AiDoTerraformLand(aib->use_tile, Random()&3, 3, (int8)p->ai.state_mode);
				}
				// also try the other terraform direction
				if (++p->ai.state_counter >= 1000) {
					p->ai.state_counter = 0;
					p->ai.state_mode = -p->ai.state_mode;
				}
			} else if (CheckPlayerHasMoney(cost) && AiCheckBlockDistances(p,aib->use_tile)) {
				// player has money, build it.
				int32 r;

				aib->cur_building_rule = rule;

				r = AiDoBuildDefaultAirportBlock(
					aib->use_tile,
					_airport_default_block_data[rule],
					DC_EXEC | DC_NO_TOWN_RATING
				);
				assert(!CmdFailed(r));
			}
		} while (++aib,--j);
	} while (--i);

	// check if we're done with all of them
	aib = &p->ai.src;
	j = p->ai.num_build_rec;
	do {
		if (aib->cur_building_rule == 255) return;
	} while (++aib,--j);

	// yep, all are done. switch state.
	p->ai.state = AIS_BUILD_AIRCRAFT_VEHICLES;
}

static StationID AiGetStationIdFromAircraftBlock(TileIndex tile, int id)
{
	const AiDefaultBlockData *p = _airport_default_block_data[id];
	while (p->mode != 1) p++;
	return GetStationIndex(TILE_ADD(tile, ToTileIndexDiff(p->tileoffs)));
}

static void AiStateBuildAircraftVehicles(Player *p)
{
	const AiDefaultBlockData *ptr;
	TileIndex tile;
	EngineID veh;
	int i;
	VehicleID loco_id;

	ptr = _airport_default_block_data[p->ai.src.cur_building_rule];
	for (; ptr->mode != 0; ptr++) {}

	tile = TILE_ADD(p->ai.src.use_tile, ToTileIndexDiff(ptr->tileoffs));

	veh = AiChooseAircraftToBuild(p->player_money, p->ai.build_kind != 0 ? 0 : AIR_CTOL);
	if (veh == INVALID_ENGINE) return;

	/* XXX - Have the AI pick the hangar terminal in an airport. Eg get airport-type
	 * and offset to the FIRST depot because the AI picks the st->xy tile */
	tile += ToTileIndexDiff(GetAirport(GetStationByTile(tile)->airport_type)->airport_depots[0]);
	if (CmdFailed(DoCommand(tile, veh, 0, DC_EXEC, CMD_BUILD_AIRCRAFT))) return;
	loco_id = _new_vehicle_id;

	for (i = 0; p->ai.order_list_blocks[i] != 0xFF; i++) {
		AiBuildRec *aib = (&p->ai.src) + p->ai.order_list_blocks[i];
		bool is_pass = (p->ai.cargo_type == CT_PASSENGERS || p->ai.cargo_type == CT_MAIL);
		Order order;

		order.type = OT_GOTO_STATION;
		order.flags = 0;
		order.dest = AiGetStationIdFromAircraftBlock(aib->use_tile, aib->cur_building_rule);

		if (!is_pass && i == 1) order.flags |= OF_UNLOAD;
		if (p->ai.num_want_fullload != 0 && (is_pass || i == 0))
			order.flags |= OF_FULL_LOAD;

		DoCommand(0, loco_id + (i << 16), PackOrder(&order), DC_EXEC, CMD_INSERT_ORDER);
	}

	DoCommand(0, loco_id, 0, DC_EXEC, CMD_START_STOP_AIRCRAFT);

	DoCommand(0, loco_id, _ai_service_interval, DC_EXEC, CMD_CHANGE_SERVICE_INT);

	if (p->ai.num_want_fullload != 0) p->ai.num_want_fullload--;

	if (--p->ai.num_loco_to_build == 0) p->ai.state = AIS_0;
}

static void AiStateCheckShipStuff(Player *p)
{
	/* Ships are not implemented in this (broken) AI */
}

static void AiStateBuildDefaultShipBlocks(Player *p)
{
	/* Ships are not implemented in this (broken) AI */
}

static void AiStateDoShipStuff(Player *p)
{
	/* Ships are not implemented in this (broken) AI */
}

static void AiStateSellVeh(Player *p)
{
	Vehicle *v = p->ai.cur_veh;

	if (v->owner == _current_player) {
		if (v->type == VEH_Train) {

			if (!IsTileDepotType(v->tile, TRANSPORT_RAIL) || v->u.rail.track != 0x80 || !(v->vehstatus&VS_STOPPED)) {
				if (v->current_order.type != OT_GOTO_DEPOT)
					DoCommand(0, v->index, 0, DC_EXEC, CMD_SEND_TRAIN_TO_DEPOT);
				goto going_to_depot;
			}

			// Sell whole train
			DoCommand(v->tile, v->index, 1, DC_EXEC, CMD_SELL_RAIL_WAGON);

		} else if (v->type == VEH_Road) {
			if (!IsRoadVehInDepotStopped(v)) {
				if (v->current_order.type != OT_GOTO_DEPOT)
					DoCommand(0, v->index, 0, DC_EXEC, CMD_SEND_ROADVEH_TO_DEPOT);
				goto going_to_depot;
			}

			DoCommand(0, v->index, 0, DC_EXEC, CMD_SELL_ROAD_VEH);
		} else if (v->type == VEH_Aircraft) {
			if (!IsAircraftInHangarStopped(v)) {
				if (v->current_order.type != OT_GOTO_DEPOT)
					DoCommand(0, v->index, 0, DC_EXEC, CMD_SEND_AIRCRAFT_TO_HANGAR);
				goto going_to_depot;
			}

			DoCommand(0, v->index, 0, DC_EXEC, CMD_SELL_AIRCRAFT);
		} else if (v->type == VEH_Ship) {
			/* Ships are not implemented in this (broken) AI */
		}
	}

	goto return_to_loop;
going_to_depot:;
	if (++p->ai.state_counter <= 832) return;

	if (v->current_order.type == OT_GOTO_DEPOT) {
		v->current_order.type = OT_DUMMY;
		v->current_order.flags = 0;
		InvalidateWindow(WC_VEHICLE_VIEW, v->index);
	}
return_to_loop:;
	p->ai.state = AIS_VEH_LOOP;
}

static void AiStateRemoveStation(Player *p)
{
	// Remove stations that aren't in use by any vehicle
	byte *in_use;
	const Order *ord;
	const Station *st;
	TileIndex tile;

	// Go to this state when we're done.
	p->ai.state = AIS_1;

	// Get a list of all stations that are in use by a vehicle
	in_use = malloc(GetMaxStationIndex() + 1);
	memset(in_use, 0, GetMaxStationIndex() + 1);
	FOR_ALL_ORDERS(ord) {
		if (ord->type == OT_GOTO_STATION) in_use[ord->dest] = 1;
	}

	// Go through all stations and delete those that aren't in use
	FOR_ALL_STATIONS(st) {
		if (st->owner == _current_player && !in_use[st->index] &&
				( (st->bus_stops != NULL && (tile = st->bus_stops->xy) != 0) ||
					(st->truck_stops != NULL && (tile = st->truck_stops->xy)) != 0 ||
					(tile = st->train_tile) != 0 ||
					(tile = st->dock_tile) != 0 ||
					(tile = st->airport_tile) != 0)) {
			DoCommand(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
		}
	}

	free(in_use);
}

static void AiRemovePlayerRailOrRoad(Player *p, TileIndex tile)
{
	TrackBits rails;

	if (IsTileType(tile, MP_RAILWAY)) {
		if (!IsTileOwner(tile, _current_player)) return;

		if (IsPlainRailTile(tile)) {
is_rail_crossing:;
			rails = GetRailTrackStatus(tile);

			if (rails == TRACK_BIT_HORZ || rails == TRACK_BIT_VERT) return;

			if (rails & TRACK_BIT_3WAY_NE) {
pos_0:
				if ((GetRailTrackStatus(TILE_MASK(tile - TileDiffXY(1, 0))) & TRACK_BIT_3WAY_SW) == 0) {
					p->ai.cur_dir_a = 0;
					p->ai.cur_tile_a = tile;
					p->ai.state = AIS_REMOVE_SINGLE_RAIL_TILE;
					return;
				}
			}

			if (rails & TRACK_BIT_3WAY_SE) {
pos_1:
				if ((GetRailTrackStatus(TILE_MASK(tile + TileDiffXY(0, 1))) & TRACK_BIT_3WAY_NW) == 0) {
					p->ai.cur_dir_a = 1;
					p->ai.cur_tile_a = tile;
					p->ai.state = AIS_REMOVE_SINGLE_RAIL_TILE;
					return;
				}
			}

			if (rails & TRACK_BIT_3WAY_SW) {
pos_2:
				if ((GetRailTrackStatus(TILE_MASK(tile + TileDiffXY(1, 0))) & TRACK_BIT_3WAY_NE) == 0) {
					p->ai.cur_dir_a = 2;
					p->ai.cur_tile_a = tile;
					p->ai.state = AIS_REMOVE_SINGLE_RAIL_TILE;
					return;
				}
			}

			if (rails & TRACK_BIT_3WAY_NW) {
pos_3:
				if ((GetRailTrackStatus(TILE_MASK(tile - TileDiffXY(0, 1))) & TRACK_BIT_3WAY_SE) == 0) {
					p->ai.cur_dir_a = 3;
					p->ai.cur_tile_a = tile;
					p->ai.state = AIS_REMOVE_SINGLE_RAIL_TILE;
					return;
				}
			}
		} else {
			static const byte _depot_bits[] = {0x19,0x16,0x25,0x2A};

			DiagDirection dir = GetRailDepotDirection(tile);

			if (GetRailTrackStatus(tile + TileOffsByDiagDir(dir)) & _depot_bits[dir])
				return;

			DoCommand(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
		}
	} else if (IsTileType(tile, MP_STREET)) {
		if (!IsTileOwner(tile, _current_player)) return;

		if (IsLevelCrossing(tile)) goto is_rail_crossing;

		if (GetRoadTileType(tile) == ROAD_TILE_DEPOT) {
			DiagDirection dir;
			TileIndex t;

			// Check if there are any stations around.
			t = tile + TileDiffXY(-1, 0);
			if (IsTileType(t, MP_STATION) && IsTileOwner(t, _current_player)) return;

			t = tile + TileDiffXY(1, 0);
			if (IsTileType(t, MP_STATION) && IsTileOwner(t, _current_player)) return;

			t = tile + TileDiffXY(0, -1);
			if (IsTileType(t, MP_STATION) && IsTileOwner(t, _current_player)) return;

			t = tile + TileDiffXY(0, 1);
			if (IsTileType(t, MP_STATION) && IsTileOwner(t, _current_player)) return;

			dir = GetRoadDepotDirection(tile);

			DoCommand(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
			DoCommand(
				TILE_MASK(tile + TileOffsByDiagDir(dir)),
				DiagDirToRoadBits(ReverseDiagDir(dir)),
				0,
				DC_EXEC,
				CMD_REMOVE_ROAD);
		}
	} else if (IsTileType(tile, MP_TUNNELBRIDGE)) {
		if (!IsTileOwner(tile, _current_player) ||
				!IsBridge(tile) ||
				!IsBridgeRamp(tile) ||
				GetBridgeTransportType(tile) != TRANSPORT_RAIL) {
			return;
		}

		rails = 0;

		switch (GetBridgeRampDirection(tile)) {
			default:
			case DIAGDIR_NE: goto pos_2;
			case DIAGDIR_SE: goto pos_3;
			case DIAGDIR_SW: goto pos_0;
			case DIAGDIR_NW: goto pos_1;
		}
	}
}

static void AiStateRemoveTrack(Player *p)
{
	/* Was 1000 for standard 8x8 maps. */
	int num = MapSizeX() * 4;

	do {
		TileIndex tile = ++p->ai.state_counter;

		// Iterated all tiles?
		if (tile >= MapSize()) {
			p->ai.state = AIS_REMOVE_STATION;
			return;
		}

		// Remove player stuff in that tile
		AiRemovePlayerRailOrRoad(p, tile);
		if (p->ai.state != AIS_REMOVE_TRACK) return;
	} while (--num);
}

static void AiStateRemoveSingleRailTile(Player *p)
{
	// Remove until we can't remove more.
	if (!AiRemoveTileAndGoForward(p)) p->ai.state = AIS_REMOVE_TRACK;
}

static AiStateAction * const _ai_actions[] = {
	AiCase0,
	AiCase1,
	AiStateVehLoop,
	AiStateCheckReplaceVehicle,
	AiStateDoReplaceVehicle,
	AiStateWantNewRoute,

	AiStateBuildDefaultRailBlocks,
	AiStateBuildRail,
	AiStateBuildRailVeh,
	AiStateDeleteRailBlocks,

	AiStateBuildDefaultRoadBlocks,
	AiStateBuildRoad,
	AiStateBuildRoadVehicles,
	AiStateDeleteRoadBlocks,

	AiStateAirportStuff,
	AiStateBuildDefaultAirportBlocks,
	AiStateBuildAircraftVehicles,

	AiStateCheckShipStuff,
	AiStateBuildDefaultShipBlocks,
	AiStateDoShipStuff,

	AiStateSellVeh,
	AiStateRemoveStation,
	AiStateRemoveTrack,

	AiStateRemoveSingleRailTile
};

extern void ShowBuyCompanyDialog(uint player);

static void AiHandleTakeover(Player *p)
{
	if (p->bankrupt_timeout != 0) {
		p->bankrupt_timeout -= 8;
		if (p->bankrupt_timeout > 0) return;
		p->bankrupt_timeout = 0;
		DeleteWindowById(WC_BUY_COMPANY, _current_player);
		if (IsLocalPlayer()) {
			AskExitToGameMenu();
			return;
		}
		if (IsHumanPlayer(_current_player)) return;
	}

	if (p->bankrupt_asked == 255) return;

	{
		uint asked = p->bankrupt_asked;
		Player *pp, *best_pl = NULL;
		int32 best_val = -1;
		uint old_p;

		// Ask the guy with the highest performance hist.
		FOR_ALL_PLAYERS(pp) {
			if (pp->is_active &&
					!(asked&1) &&
					pp->bankrupt_asked == 0 &&
					best_val < pp->old_economy[1].performance_history) {
				best_val = pp->old_economy[1].performance_history;
				best_pl = pp;
			}
			asked>>=1;
		}

		// Asked all players?
		if (best_val == -1) {
			p->bankrupt_asked = 255;
			return;
		}

		SETBIT(p->bankrupt_asked, best_pl->index);

		if (best_pl->index == _local_player) {
			p->bankrupt_timeout = 4440;
			ShowBuyCompanyDialog(_current_player);
			return;
		}
		if (IsHumanPlayer(best_pl->index)) return;

		// Too little money for computer to buy it?
		if (best_pl->player_money >> 1 >= p->bankrupt_value) {
			// Computer wants to buy it.
			old_p = _current_player;
			_current_player = best_pl->index;
			DoCommand(0, old_p, 0, DC_EXEC, CMD_BUY_COMPANY);
			_current_player = old_p;
		}
	}
}

static void AiAdjustLoan(const Player* p)
{
	int32 base = AiGetBasePrice(p);

	if (p->player_money > base * 1400) {
		// Decrease loan
		if (p->current_loan != 0) {
			DoCommand(0, 0, 0, DC_EXEC, CMD_DECREASE_LOAN);
		}
	} else if (p->player_money < base * 500) {
		// Increase loan
		if (p->current_loan < _economy.max_loan &&
				p->num_valid_stat_ent >= 2 &&
				-(p->old_economy[0].expenses+p->old_economy[1].expenses) < base * 60) {
			DoCommand(0, 0, 0, DC_EXEC, CMD_INCREASE_LOAN);
		}
	}
}

static void AiBuildCompanyHQ(Player *p)
{
	TileIndex tile;

	if (p->location_of_house == 0 &&
			p->last_build_coordinate != 0) {
		tile = AdjustTileCoordRandomly(p->last_build_coordinate, 8);
		DoCommand(tile, 0, 0, DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_BUILD_COMPANY_HQ);
	}
}


void AiDoGameLoop(Player *p)
{
	if (p->bankrupt_asked != 0) {
		AiHandleTakeover(p);
		return;
	}

	// Ugly hack to make sure the service interval of the AI is good, not looking
	//  to the patch-setting
	// Also, it takes into account the setting if the service-interval is in days
	//  or in %
	_ai_service_interval = _patches.servint_ispercent?80:180;

	if (IsHumanPlayer(_current_player)) return;

	AiAdjustLoan(p);
	AiBuildCompanyHQ(p);

#if 0
	{
		static byte old_state = 99;
		static bool hasdots = false;
		char *_ai_state_names[]={
			"AiCase0",
			"AiCase1",
			"AiStateVehLoop",
			"AiStateCheckReplaceVehicle",
			"AiStateDoReplaceVehicle",
			"AiStateWantNewRoute",
			"AiStateBuildDefaultRailBlocks",
			"AiStateBuildRail",
			"AiStateBuildRailVeh",
			"AiStateDeleteRailBlocks",
			"AiStateBuildDefaultRoadBlocks",
			"AiStateBuildRoad",
			"AiStateBuildRoadVehicles",
			"AiStateDeleteRoadBlocks",
			"AiStateAirportStuff",
			"AiStateBuildDefaultAirportBlocks",
			"AiStateBuildAircraftVehicles",
			"AiStateCheckShipStuff",
			"AiStateBuildDefaultShipBlocks",
			"AiStateDoShipStuff",
			"AiStateSellVeh",
			"AiStateRemoveStation",
			"AiStateRemoveTrack",
			"AiStateRemoveSingleRailTile"
		};

		if (p->ai.state != old_state) {
			if (hasdots)
				printf("\n");
			hasdots=false;
			printf("AiState: %s\n", _ai_state_names[old_state=p->ai.state]);
		} else {
			printf(".");
			hasdots=true;
		}
	}
#endif

	_ai_actions[p->ai.state](p);
}
