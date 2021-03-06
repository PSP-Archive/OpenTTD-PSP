/* $Id: airport_movement.h 10279 2007-06-22 20:08:37Z rubidium $ */

#ifndef AIRPORT_MOVEMENT_H
#define AIRPORT_MOVEMENT_H


// state machine input struct (from external file, etc.)
// Finite sTate mAchine --> FTA
typedef struct AirportFTAbuildup {
	byte position; // the position that an airplane is at
	byte heading;  // the current orders (eg. TAKEOFF, HANGAR, ENDLANDING, etc.)
	uint32 block;  // the block this position is on on the airport (st->airport_flags)
	byte next;     // next position from this position
} AirportFTAbuildup;

///////////////////////////////////////////////////////////////////////
/////*********Movement Positions on Airports********************///////
// Country Airfield (small) 4x3
static const AirportMovingData _airport_moving_data_country[22] = {
	{   53,    3, AMED_EXACTPOS,                   3 }, // 00 In Hangar
	{   53,   27, 0,                               0 }, // 01 Taxi to right outside depot
	{   32,   23, AMED_EXACTPOS,                   7 }, // 02 Terminal 1
	{   10,   23, AMED_EXACTPOS,                   7 }, // 03 Terminal 2
	{   43,   37, 0,                               0 }, // 04 Going towards terminal 2
	{   24,   37, 0,                               0 }, // 05 Going towards terminal 2
	{   53,   37, 0,                               0 }, // 06 Going for takeoff
	{   61,   40, AMED_EXACTPOS,                   1 }, // 07 Taxi to start of runway (takeoff)
	{    3,   40, AMED_NOSPDCLAMP,                 0 }, // 08 Accelerate to end of runway
	{  -79,   40, AMED_NOSPDCLAMP | AMED_TAKEOFF,  0 }, // 09 Take off
	{  177,   40, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 10 Fly to landing position in air
	{   56,   40, AMED_NOSPDCLAMP | AMED_LAND,     0 }, // 11 Going down for land
	{    3,   40, AMED_NOSPDCLAMP | AMED_BRAKE,    0 }, // 12 Just landed, brake until end of runway
	{    7,   40, 0,                               0 }, // 13 Just landed, turn around and taxi 1 square
	{   53,   40, 0,                               0 }, // 14 Taxi from runway to crossing
	{  -31,  193, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 15 Fly around waiting for a landing spot (north-east)
	{    1,    1, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 16 Fly around waiting for a landing spot (north-west)
	{  257,    1, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 17 Fly around waiting for a landing spot (south-west)
	{  273,   49, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 18 Fly around waiting for a landing spot (south)
	{   44,   37, AMED_HELI_RAISE,                 0 }, // 19 Helicopter takeoff
	{   44,   40, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 20 In position above landing spot helicopter
	{   44,   40, AMED_HELI_LOWER,                 0 }, // 21 Helicopter landing
};

// Commuter Airfield (small) 5x4
static const AirportMovingData _airport_moving_data_commuter[37] = {
	{   69,    3, AMED_EXACTPOS,                   3 }, // 00 In Hangar
	{   72,   22, 0,                               0 }, // 01 Taxi to right outside depot
	{    8,   22, AMED_EXACTPOS,                   5 }, // 01 Taxi to right outside depot
	{   24,   36, AMED_EXACTPOS,                   3 }, // 03 Terminal 1
	{   40,   36, AMED_EXACTPOS,                   3 }, // 04 Terminal 2
	{   56,   36, AMED_EXACTPOS,                   3 }, // 05 Terminal 3
	{   40,    8, AMED_EXACTPOS,                   1 }, // 06 Helipad 1
	{   56,    8, AMED_EXACTPOS,                   1 }, // 07 Helipad 2
	{   24,   22, 0,                               5 }, // 08 Taxiing
	{   40,   22, 0,                               5 }, // 09 Taxiing
	{   56,   22, 0,                               5 }, // 10 Taxiing
	{   72,   40, 0,                               3 }, // 11 Airport OUTWAY
	{   72,   54, AMED_EXACTPOS,                   1 }, // 12 Accelerate to end of runway
	{    7,   54, AMED_NOSPDCLAMP,                 0 }, // 13 Release control of runway, for smoother movement
	{    5,   54, AMED_NOSPDCLAMP,                 0 }, // 14 End of runway
	{  -79,   54, AMED_NOSPDCLAMP | AMED_TAKEOFF,  0 }, // 15 Take off
	{  145,   54, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 16 Fly to landing position in air
	{   73,   54, AMED_NOSPDCLAMP | AMED_LAND,     0 }, // 17 Going down for land
	{    3,   54, AMED_NOSPDCLAMP | AMED_BRAKE,    0 }, // 18 Just landed, brake until end of runway
	{   12,   54, 0,                               7 }, // 19 Just landed, turn around and taxi
	{    8,   32, 0,                               7 }, // 20 Taxi from runway to crossing
	{  -31,  149, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 21 Fly around waiting for a landing spot (north-east)
	{    1,    6, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 22 Fly around waiting for a landing spot (north-west)
	{  193,    6, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 23 Fly around waiting for a landing spot (south-west)
	{  225,   81, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 24 Fly around waiting for a landing spot (south)
	// Helicopter
	{   80,    0, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 25 Bufferspace before helipad
	{   80,    0, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 26 Bufferspace before helipad
	{   32,    8, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 27 Get in position for Helipad1
	{   48,    8, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 28 Get in position for Helipad2
	{   32,    8, AMED_HELI_LOWER,                 0 }, // 29 Land at Helipad1
	{   48,    8, AMED_HELI_LOWER,                 0 }, // 30 Land at Helipad2
	{   32,    8, AMED_HELI_RAISE,                 0 }, // 31 Takeoff Helipad1
	{   48,    8, AMED_HELI_RAISE,                 0 }, // 32 Takeoff Helipad2
	{   64,   22, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 33 Go to position for Hangarentrance in air
	{   64,   22, AMED_HELI_LOWER,                 0 }, // 34 Land in front of hangar
	{   40,    8, AMED_EXACTPOS,                   0 }, // pre-helitakeoff helipad 1
	{   56,    8, AMED_EXACTPOS,                   0 }, // pre-helitakeoff helipad 2
};

// City Airport (large) 6x6
static const AirportMovingData _airport_moving_data_town[25] = {
	{   85,    3, AMED_EXACTPOS,                   3 }, // 00 In Hangar
	{   85,   27, 0,                               0 }, // 01 Taxi to right outside depot
	{   26,   41, AMED_EXACTPOS,                   5 }, // 02 Terminal 1
	{   56,   20, AMED_EXACTPOS,                   3 }, // 03 Terminal 2
	{   38,    8, AMED_EXACTPOS,                   5 }, // 04 Terminal 3
	{   65,    6, 0,                               0 }, // 05 Taxi to right in infront of terminal 2/3
	{   80,   27, 0,                               0 }, // 06 Taxiway terminals 2-3
	{   44,   63, 0,                               0 }, // 07 Taxi to Airport center
	{   58,   71, 0,                               0 }, // 08 Towards takeoff
	{   72,   85, 0,                               0 }, // 09 Taxi to runway (takeoff)
	{   89,   85, AMED_EXACTPOS,                   1 }, // 10 Taxi to start of runway (takeoff)
	{    3,   85, AMED_NOSPDCLAMP,                 0 }, // 11 Accelerate to end of runway
	{  -79,   85, AMED_NOSPDCLAMP | AMED_TAKEOFF,  0 }, // 12 Take off
	{  177,   85, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 13 Fly to landing position in air
	{   89,   85, AMED_NOSPDCLAMP | AMED_LAND,     0 }, // 14 Going down for land
	{    3,   85, AMED_NOSPDCLAMP | AMED_BRAKE,    0 }, // 15 Just landed, brake until end of runway
	{   20,   87, 0,                               0 }, // 16 Just landed, turn around and taxi 1 square
	{   36,   71, 0,                               0 }, // 17 Taxi from runway to crossing
	{  -31,  193, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 18 Fly around waiting for a landing spot (north-east)
	{    1,    1, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 19 Fly around waiting for a landing spot (north-west)
	{  257,    1, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 20 Fly around waiting for a landing spot (south-west)
	{  273,   49, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 21 Fly around waiting for a landing spot (south)
	{   44,   63, AMED_HELI_RAISE,                 0 }, // 22 Helicopter takeoff
	{   28,   74, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 23 In position above landing spot helicopter
	{   28,   74, AMED_HELI_LOWER,                 0 }, // 24 Helicopter landing
};

// Metropolitan Airport (metropolitan) - 2 runways
static const AirportMovingData _airport_moving_data_metropolitan[27] = {
	{   85,    3, AMED_EXACTPOS,                   3 }, // 00 In Hangar
	{   85,   27, 0,                               0 }, // 01 Taxi to right outside depot
	{   26,   41, AMED_EXACTPOS,                   5 }, // 02 Terminal 1
	{   56,   20, AMED_EXACTPOS,                   3 }, // 03 Terminal 2
	{   38,    8, AMED_EXACTPOS,                   5 }, // 04 Terminal 3
	{   65,    6, 0,                               0 }, // 05 Taxi to right in infront of terminal 2/3
	{   70,   33, 0,                               0 }, // 06 Taxiway terminals 2-3
	{   44,   58, 0,                               0 }, // 07 Taxi to Airport center
	{   72,   58, 0,                               0 }, // 08 Towards takeoff
	{   72,   69, 0,                               0 }, // 09 Taxi to runway (takeoff)
	{   89,   69, AMED_EXACTPOS,                   1 }, // 10 Taxi to start of runway (takeoff)
	{    3,   69, AMED_NOSPDCLAMP,                 0 }, // 11 Accelerate to end of runway
	{  -79,   69, AMED_NOSPDCLAMP | AMED_TAKEOFF,  0 }, // 12 Take off
	{  177,   85, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 13 Fly to landing position in air
	{   89,   85, AMED_NOSPDCLAMP | AMED_LAND,     0 }, // 14 Going down for land
	{    3,   85, AMED_NOSPDCLAMP | AMED_BRAKE,    0 }, // 15 Just landed, brake until end of runway
	{   21,   85, 0,                               0 }, // 16 Just landed, turn around and taxi 1 square
	{   21,   69, 0,                               0 }, // 17 On Runway-out taxiing to In-Way
	{   21,   54, AMED_EXACTPOS,                   5 }, // 18 Taxi from runway to crossing
	{  -31,  193, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 19 Fly around waiting for a landing spot (north-east)
	{    1,    1, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 20 Fly around waiting for a landing spot (north-west)
	{  257,    1, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 21 Fly around waiting for a landing spot (south-west)
	{  273,   49, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 22 Fly around waiting for a landing spot (south)
	{   44,   58, 0,                               0 }, // 23 Helicopter takeoff spot on ground (to clear airport sooner)
	{   44,   63, AMED_HELI_RAISE,                 0 }, // 24 Helicopter takeoff
	{   15,   54, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 25 Get in position above landing spot helicopter
	{   15,   54, AMED_HELI_LOWER,                 0 }, // 26 Helicopter landing
};

// International Airport (international) - 2 runways, 6 terminals, dedicated helipod
static const AirportMovingData _airport_moving_data_international[51] = {
	{    7,   55, AMED_EXACTPOS,                   3 }, // 00 In Hangar 1
	{  100,   21, AMED_EXACTPOS,                   3 }, // 01 In Hangar 2
	{    7,   70, 0,                               0 }, // 02 Taxi to right outside depot
	{  100,   36, 0,                               0 }, // 03 Taxi to right outside depot
	{   38,   70, AMED_EXACTPOS,                   5 }, // 04 Terminal 1
	{   38,   54, AMED_EXACTPOS,                   5 }, // 05 Terminal 2
	{   38,   38, AMED_EXACTPOS,                   5 }, // 06 Terminal 3
	{   70,   70, AMED_EXACTPOS,                   1 }, // 07 Terminal 4
	{   70,   54, AMED_EXACTPOS,                   1 }, // 08 Terminal 5
	{   70,   38, AMED_EXACTPOS,                   1 }, // 09 Terminal 6
	{  104,   71, AMED_EXACTPOS,                   1 }, // 10 Helipad 1
	{  104,   55, AMED_EXACTPOS,                   1 }, // 11 Helipad 2
	{   22,   87, 0,                               0 }, // 12 Towards Terminals 4/5/6, Helipad 1/2
	{   60,   87, 0,                               0 }, // 13 Towards Terminals 4/5/6, Helipad 1/2
	{   66,   87, 0,                               0 }, // 14 Towards Terminals 4/5/6, Helipad 1/2
	{   86,   87, AMED_EXACTPOS,                   7 }, // 15 Towards Terminals 4/5/6, Helipad 1/2
	{   86,   70, 0,                               0 }, // 16 In Front of Terminal 4 / Helipad 1
	{   86,   54, 0,                               0 }, // 17 In Front of Terminal 5 / Helipad 2
	{   86,   38, 0,                               0 }, // 18 In Front of Terminal 6
	{   86,   22, 0,                               0 }, // 19 Towards Terminals Takeoff (Taxiway)
	{   66,   22, 0,                               0 }, // 20 Towards Terminals Takeoff (Taxiway)
	{   60,   22, 0,                               0 }, // 21 Towards Terminals Takeoff (Taxiway)
	{   38,   22, 0,                               0 }, // 22 Towards Terminals Takeoff (Taxiway)
	{   22,   70, 0,                               0 }, // 23 In Front of Terminal 1
	{   22,   58, 0,                               0 }, // 24 In Front of Terminal 2
	{   22,   38, 0,                               0 }, // 25 In Front of Terminal 3
	{   22,   22, AMED_EXACTPOS,                   7 }, // 26 Going for Takeoff
	{   22,    6, 0,                               0 }, // 27 On Runway-out, prepare for takeoff
	{    3,    6, AMED_EXACTPOS,                   5 }, // 28 Accelerate to end of runway
	{   60,    6, AMED_NOSPDCLAMP,                 0 }, // 29 Release control of runway, for smoother movement
	{  105,    6, AMED_NOSPDCLAMP,                 0 }, // 30 End of runway
	{  190,    6, AMED_NOSPDCLAMP | AMED_TAKEOFF,  0 }, // 31 Take off
	{  193,  104, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 32 Fly to landing position in air
	{  105,  104, AMED_NOSPDCLAMP | AMED_LAND,     0 }, // 33 Going down for land
	{    3,  104, AMED_NOSPDCLAMP | AMED_BRAKE,    0 }, // 34 Just landed, brake until end of runway
	{   12,  104, AMED_SLOWTURN,                   0 }, // 35 Just landed, turn around and taxi 1 square
	{    7,   84, 0,                               0 }, // 36 Taxi from runway to crossing
	{  -31,  209, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 37 Fly around waiting for a landing spot (north-east)
	{    1,    6, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 38 Fly around waiting for a landing spot (north-west)
	{  273,    6, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 39 Fly around waiting for a landing spot (south-west)
	{  305,   81, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 40 Fly around waiting for a landing spot (south)
	// Helicopter
	{  128,   80, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 41 Bufferspace before helipad
	{  128,   80, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 42 Bufferspace before helipad
	{   96,   71, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 43 Get in position for Helipad1
	{   96,   55, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 44 Get in position for Helipad2
	{   96,   71, AMED_HELI_LOWER,                 0 }, // 45 Land at Helipad1
	{   96,   55, AMED_HELI_LOWER,                 0 }, // 46 Land at Helipad2
	{  104,   71, AMED_HELI_RAISE,                 0 }, // 47 Takeoff Helipad1
	{  104,   55, AMED_HELI_RAISE,                 0 }, // 48 Takeoff Helipad2
	{  104,   32, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 49 Go to position for Hangarentrance in air
	{  104,   32, AMED_HELI_LOWER,                 0} // 50 Land in HANGAR2_AREA to go to hangar
};

// Intercontinental Airport - 4 runways, 8 terminals, 2 dedicated helipads
static const AirportMovingData _airport_moving_data_intercontinental[77] = {
	{    7,   87, AMED_EXACTPOS,                   3 }, // 00 In Hangar 1
	{  135,   72, AMED_EXACTPOS,                   3 }, // 01 In Hangar 2
	{    7,  104, 0,                               0 }, // 02 Taxi to right outside depot 1
	{  135,   88, 0, 0 }, // 03 Taxi to right outside depot 2
	{   56,  120, AMED_EXACTPOS,                   6 }, // 04 Terminal 1
	{   56,  104, AMED_EXACTPOS,                   5 }, // 05 Terminal 2
	{   56,   88, AMED_EXACTPOS,                   5 }, // 06 Terminal 3
	{   56,   72, AMED_EXACTPOS,                   5 }, // 07 Terminal 4
	{   88,  120, AMED_EXACTPOS,                   0 }, // 08 Terminal 5
	{   88,  104, AMED_EXACTPOS,                   1 }, // 09 Terminal 6
	{   88,   88, AMED_EXACTPOS,                   1 }, // 10 Terminal 7
	{   88,   72, AMED_EXACTPOS,                   1 }, // 11 Terminal 8
	{   88,   56, AMED_EXACTPOS,                   3 }, // 12 Helipad 1
	{   72,   56, AMED_EXACTPOS,                   1 }, // 13 Helipad 2
	{   40,  136, 0,                               0 }, // 14 Term group 2 enter 1 a
	{   56,  136, 0,                               0 }, // 15 Term group 2 enter 1 b
	{   88,  136, 0,                               0 }, // 16 Term group 2 enter 2 a
	{  104,  136, 0,                               0 }, // 17 Term group 2 enter 2 b
	{  104,  120, 0,                               0 }, // 18 Term group 2 - opp term 5
	{  104,  104, 0,                               0 }, // 19 Term group 2 - opp term 6 & exit2
	{  104,   88, 0,                               0 }, // 20 Term group 2 - opp term 7 & hangar area 2
	{  104,   72, 0,                               0 }, // 21 Term group 2 - opp term 8
	{  104,   56, 0,                               0 }, // 22 Taxi Term group 2 exit a
	{  104,   40, 0,                               0 }, // 23 Taxi Term group 2 exit b
	{   56,   40, 0,                               0 }, // 24 Term group 2 exit 2a
	{   40,   40, 0,                               0 }, // 25 Term group 2 exit 2b
	{   40,  120, 0,                               0 }, // 26 Term group 1 - opp term 1
	{   40,  104, 0,                               0 }, // 27 Term group 1 - opp term 2 & hangar area 1
	{   40,   88, 0,                               0 }, // 28 Term group 1 - opp term 3
	{   40,   72, 0,                               0 }, // 29 Term group 1 - opp term 4
	{   18,   72, 0,                               7 }, // 30 Outway 1
	{    8,   40, 0,                               7 }, // 31 Airport OUTWAY
	{    8,   24, AMED_EXACTPOS,                   5 }, // 32 Accelerate to end of runway
	{  119,   24, AMED_NOSPDCLAMP,                 0 }, // 33 Release control of runway, for smoother movement
	{  117,   24, AMED_NOSPDCLAMP,                 0 }, // 34 End of runway
	{  197,   24, AMED_NOSPDCLAMP | AMED_TAKEOFF,  0 }, // 35 Take off
	{  254,   84, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 36 Flying to landing position in air
	{  117,  168, AMED_NOSPDCLAMP | AMED_LAND,     0 }, // 37 Going down for land
	{    3,  168, AMED_NOSPDCLAMP | AMED_BRAKE,    0 }, // 38 Just landed, brake until end of runway
	{    8,  168, 0,                               0 }, // 39 Just landed, turn around and taxi
	{    8,  144, 0,                               7 }, // 40 Taxi from runway
	{    8,  128, 0,                               7 }, // 41 Taxi from runway
	{    8,  120, AMED_EXACTPOS,                   5 }, // 42 Airport entrance
	{   56,  344, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 43 Fly around waiting for a landing spot (north-east)
	{ -200,   88, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 44 Fly around waiting for a landing spot (north-west)
	{   56, -168, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 45 Fly around waiting for a landing spot (south-west)
	{  312,   88, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 46 Fly around waiting for a landing spot (south)
	// Helicopter
	{   96,   40, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 47 Bufferspace before helipad
	{   96,   40, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 48 Bufferspace before helipad
	{   82,   54, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 49 Get in position for Helipad1
	{   64,   56, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 50 Get in position for Helipad2
	{   81,   55, AMED_HELI_LOWER,                 0 }, // 51 Land at Helipad1
	{   64,   56, AMED_HELI_LOWER,                 0 }, // 52 Land at Helipad2
	{   80,   56, AMED_HELI_RAISE,                 0 }, // 53 Takeoff Helipad1
	{   64,   56, AMED_HELI_RAISE,                 0 }, // 54 Takeoff Helipad2
	{  136,   96, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 55 Go to position for Hangarentrance in air
	{  136,   96, AMED_HELI_LOWER,                 0 }, // 56 Land in front of hangar2
	{  126,  104, 0,                               3 }, // 57 Outway 2
	{  136,  136, 0,                               1 }, // 58 Airport OUTWAY 2
	{  136,  152, AMED_EXACTPOS,                   1 }, // 59 Accelerate to end of runway2
	{   16,  152, AMED_NOSPDCLAMP,                 0 }, // 60 Release control of runway2, for smoother movement
	{   20,  152, AMED_NOSPDCLAMP,                 0 }, // 61 End of runway2
	{  -56,  152, AMED_NOSPDCLAMP | AMED_TAKEOFF,  0 }, // 62 Take off2
	{   24,    8, AMED_NOSPDCLAMP | AMED_LAND,     0 }, // 63 Going down for land2
	{  136,    8, AMED_NOSPDCLAMP | AMED_BRAKE,    0 }, // 64 Just landed, brake until end of runway2in
	{  136,    8, 0,                               0 }, // 65 Just landed, turn around and taxi
	{  136,   24, 0,                               3 }, // 66 Taxi from runway 2in
	{  136,   40, 0,                               3 }, // 67 Taxi from runway 2in
	{  136,   56, AMED_EXACTPOS,                   1 }, // 68 Airport entrance2
	{  -56,    8, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 69 Fly to landing position in air2
	{   88,   40, 0,                               0 }, // 70 Taxi Term group 2 exit - opp heli1
	{   72,   40, 0,                               0 }, // 71 Taxi Term group 2 exit - opp heli2
	{   88,   57, AMED_EXACTPOS,                   3 }, // 72 pre-helitakeoff helipad 1
	{   71,   56, AMED_EXACTPOS,                   1 }, // 73 pre-helitakeoff helipad 2
	{    8,  120, AMED_HELI_RAISE,                 0 }, // 74 Helitakeoff outside depot 1
	{  136,  104, AMED_HELI_RAISE,                 0 }, // 75 Helitakeoff outside depot 2
	{  197,  168, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0} // 76 Fly to landing position in air1
};


// Heliport (heliport)
static const AirportMovingData _airport_moving_data_heliport[9] = {
	{    5,    9, AMED_EXACTPOS,                   1 }, // 0 - At heliport terminal
	{    2,    9, AMED_HELI_RAISE,                 0 }, // 1 - Take off (play sound)
	{   -3,    9, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 2 - In position above landing spot helicopter
	{   -3,    9, AMED_HELI_LOWER,                 0 }, // 3 - Land
	{    2,    9, 0,                               0 }, // 4 - Goto terminal on ground
	{  -31,   59, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 5 - Circle #1 (north-east)
	{  -31,  -49, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 6 - Circle #2 (north-west)
	{   49,  -49, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 7 - Circle #3 (south-west)
	{   70,    9, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 8 - Circle #4 (south)
};

// HeliDepot 2x2 (heliport)
static const AirportMovingData _airport_moving_data_helidepot[18] = {
	{   24,    4, AMED_EXACTPOS,                   1 }, // 0 - At depot
	{   24,   28, 0,                               0 }, // 1 Taxi to right outside depot
	{    5,   38, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 2 Flying
	{  -15,  -15, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 3 - Circle #1 (north-east)
	{  -15,  -49, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 4 - Circle #2 (north-west)
	{   49,  -49, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 5 - Circle #3 (south-west)
	{   49,  -15, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 6 - Circle #4 (south-east)
	{    8,   32, AMED_NOSPDCLAMP | AMED_SLOWTURN, 7 }, // 7 - PreHelipad
	{    8,   32, AMED_NOSPDCLAMP | AMED_SLOWTURN, 7 }, // 8 - Helipad
	{    8,   16, AMED_NOSPDCLAMP | AMED_SLOWTURN, 7 }, // 9 - Land
	{    8,   16, AMED_HELI_LOWER,                 7 }, // 10 - Land
	{    8,   24, AMED_HELI_RAISE,                 0 }, // 11 - Take off (play sound)
	{   32,   24, AMED_NOSPDCLAMP | AMED_SLOWTURN, 7 }, // 12 Air to above hangar area
	{   32,   24, AMED_HELI_LOWER,                 7 }, // 13 Taxi to right outside depot
	{    8,   24, AMED_EXACTPOS,                   7 }, // 14 - on helipad1
	{   24,   28, AMED_HELI_RAISE,                 0 }, // 15 Takeoff right outside depot
	{    8,   24, AMED_HELI_RAISE,                 5 }, // 16 - Take off (play sound)
	{    8,   24, AMED_SLOWTURN | AMED_EXACTPOS,   2 }, // 17 - turn on helipad1 for takeoff
};

// HeliDepot 2x2 (heliport)
static const AirportMovingData _airport_moving_data_helistation[33] = {
	{    8,    3, AMED_EXACTPOS,                   3 }, // 00 In Hangar2
	{    8,   22, 0,                               0 }, // 01 outside hangar 2
	{  116,   24, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 02 Fly to landing position in air
	{   14,   22, AMED_HELI_RAISE,                 0 }, // 03 Helitakeoff outside hangar1(play sound)
	{   24,   22, 0,                               0 }, // 04 taxiing
	{   40,   22, 0,                               0 }, // 05 taxiing
	{   40,    8, AMED_EXACTPOS,                   1 }, // 06 Helipad 1
	{   56,    8, AMED_EXACTPOS,                   1 }, // 07 Helipad 2
	{   56,   24, AMED_EXACTPOS,                   1 }, // 08 Helipad 3
	{   40,    8, AMED_EXACTPOS,                   0 }, // 09 pre-helitakeoff helipad 1
	{   56,    8, AMED_EXACTPOS,                   0 }, // 10 pre-helitakeoff helipad 2
	{   56,   24, AMED_EXACTPOS,                   0 }, // 11 pre-helitakeoff helipad 3
	{   32,    8, AMED_HELI_RAISE,                 0 }, // 12 Takeoff Helipad1
	{   48,    8, AMED_HELI_RAISE,                 0 }, // 13 Takeoff Helipad2
	{   48,   24, AMED_HELI_RAISE,                 0 }, // 14 Takeoff Helipad3
	{   84,   24, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 15 Bufferspace before helipad
	{   68,   24, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 16 Bufferspace before helipad
	{   32,    8, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 17 Get in position for Helipad1
	{   48,    8, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 18 Get in position for Helipad2
	{   48,   24, AMED_NOSPDCLAMP | AMED_SLOWTURN, 1 }, // 19 Get in position for Helipad3
	{   40,    8, AMED_HELI_LOWER,                 0 }, // 20 Land at Helipad1
	{   48,    8, AMED_HELI_LOWER,                 0 }, // 21 Land at Helipad2
	{   48,   24, AMED_HELI_LOWER,                 0 }, // 22 Land at Helipad3
	{    0,   22, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 23 Go to position for Hangarentrance in air
	{    0,   22, AMED_HELI_LOWER,                 0 }, // 24 Land in front of hangar
	{  148,   -8, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 25 Fly around waiting for a landing spot (south-east)
	{  148,    8, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 26 Fly around waiting for a landing spot (south-west)
	{  132,   24, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 27 Fly around waiting for a landing spot (south-west)
	{  100,   24, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 28 Fly around waiting for a landing spot (north-east)
	{   84,    8, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 29 Fly around waiting for a landing spot (south-east)
	{   84,   -8, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 30 Fly around waiting for a landing spot (south-west)
	{  100,  -24, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 31 Fly around waiting for a landing spot (north-west)
	{  132,  -24, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 32 Fly around waiting for a landing spot (north-east)
};

// Oilrig
static const AirportMovingData _airport_moving_data_oilrig[9] = {
	{   31,    9, AMED_EXACTPOS,                   1 }, // 0 - At oilrig terminal
	{   28,    9, AMED_HELI_RAISE,                 0 }, // 1 - Take off (play sound)
	{   23,    9, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 2 - In position above landing spot helicopter
	{   23,    9, AMED_HELI_LOWER,                 0 }, // 3 - Land
	{   28,    9, 0,                               0 }, // 4 - Goto terminal on ground
	{  -31,   69, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 5 - circle #1 (north-east)
	{  -31,  -49, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 6 - circle #2 (north-west)
	{   69,  -49, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 7 - circle #3 (south-west)
	{   70,    9, AMED_NOSPDCLAMP | AMED_SLOWTURN, 0 }, // 8 - circle #4 (south)
};

///////////////////////////////////////////////////////////////////////
/////**********Movement Machine on Airports*********************///////
/* First element of terminals array tells us how many depots there are (to know size of array)
 * this may be changed later when airports are moved to external file  */
static const TileIndexDiffC _airport_depots_country[] = {{3, 0}};
static const byte _airport_terminal_country[] = {1, 2};
static const AirportFTAbuildup _airport_fta_country[] = {
	{  0, HANGAR, NOTHING_block, 1 },
	{  1, 255, AIRPORT_BUSY_block, 0 }, { 1, HANGAR, 0, 0 }, { 1, TERM1, TERM1_block, 2 }, { 1, TERM2, 0, 4 }, { 1, HELITAKEOFF, 0, 19 }, { 1, 0, 0, 6 },
	{  2, TERM1, TERM1_block, 1 },
	{  3, TERM2, TERM2_block, 5 },
	{  4, 255, AIRPORT_BUSY_block, 0 }, { 4, TERM2, 0, 5 }, { 4, HANGAR, 0, 1 }, { 4, TAKEOFF, 0, 6 }, { 4, HELITAKEOFF, 0, 1 },
	{  5, 255, AIRPORT_BUSY_block, 0 }, { 5, TERM2, TERM2_block, 3 }, { 5, 0, 0, 4 },
	{  6, 0, AIRPORT_BUSY_block, 7 },
	// takeoff
	{  7, TAKEOFF, AIRPORT_BUSY_block, 8 },
	{  8, STARTTAKEOFF, NOTHING_block, 9 },
	{  9, ENDTAKEOFF, NOTHING_block, 0 },
	// landing
	{ 10, FLYING, NOTHING_block, 15 }, { 10, LANDING, 0, 11 }, { 10, HELILANDING, 0, 20 },
	{ 11, LANDING, AIRPORT_BUSY_block, 12 },
	{ 12, 0, AIRPORT_BUSY_block, 13 },
	{ 13, ENDLANDING, AIRPORT_BUSY_block, 14 }, { 13, TERM2, 0, 5 }, { 13, 0, 0, 14 },
	{ 14, 0, AIRPORT_BUSY_block, 1 },
	// In air
	{ 15, 0, NOTHING_block, 16 },
	{ 16, 0, NOTHING_block, 17 },
	{ 17, 0, NOTHING_block, 18 },
	{ 18, 0, NOTHING_block, 10 },
	{ 19, HELITAKEOFF, NOTHING_block, 0 },
	{ 20, HELILANDING, AIRPORT_BUSY_block, 21 },
	{ 21, HELIENDLANDING, AIRPORT_BUSY_block, 1 },
	{ MAX_ELEMENTS, 0, 0, 0 } // end marker. DO NOT REMOVE
};

static const TileIndexDiffC _airport_depots_commuter[] = { { 4, 0 } };
static const byte _airport_terminal_commuter[] = { 1, 3 };
static const byte _airport_helipad_commuter[] = { 1, 2 };
static const AirportFTAbuildup _airport_fta_commuter[] = {
	{  0, HANGAR, NOTHING_block, 1 }, { 0, HELITAKEOFF, HELIPAD2_block, 1 }, { 0, 0, 0, 1 },
	{  1, 255, TAXIWAY_BUSY_block, 0 }, { 1, HANGAR, 0, 0 }, { 1, TAKEOFF, 0, 11 }, { 1, TERM1, TAXIWAY_BUSY_block, 10 }, { 1, TERM2, TAXIWAY_BUSY_block, 10 }, { 1, TERM3, TAXIWAY_BUSY_block, 10 }, { 1, HELIPAD1, TAXIWAY_BUSY_block, 10 }, { 1, HELIPAD2, TAXIWAY_BUSY_block, 10 }, { 1, HELITAKEOFF, TAXIWAY_BUSY_block, 10 }, { 1, 0, 0, 0 },
	{  2, 255, AIRPORT_ENTRANCE_block, 2 }, { 2, HANGAR, 0, 8 }, { 2, TERM1, 0, 8 }, { 2, TERM2, 0, 8 }, { 2, TERM3, 0, 8 }, { 2, HELIPAD1, 0, 8 }, { 2, HELIPAD2, 0, 8 }, { 2, HELITAKEOFF, 0, 8 }, { 2, 0, 0, 2 },
	{  3, TERM1, TERM1_block, 8 }, { 3, HANGAR, 0, 8 }, { 3, TAKEOFF, 0, 8 }, { 3, 0, 0, 3 },
	{  4, TERM2, TERM2_block, 9 }, { 4, HANGAR, 0, 9 }, { 4, TAKEOFF, 0, 9 }, { 4, 0, 0, 4 },
	{  5, TERM3, TERM3_block, 10 }, { 5, HANGAR, 0, 10 }, { 5, TAKEOFF, 0, 10 }, { 5, 0, 0, 5 },
	{  6, HELIPAD1, HELIPAD1_block, 6 }, { 6, HANGAR, TAXIWAY_BUSY_block, 9 }, { 6, HELITAKEOFF, 0, 35 },
	{  7, HELIPAD2, HELIPAD2_block, 7 }, { 7, HANGAR, TAXIWAY_BUSY_block, 10 }, { 7, HELITAKEOFF, 0, 36 },
	{  8, 255, TAXIWAY_BUSY_block, 8 }, { 8, TAKEOFF, TAXIWAY_BUSY_block, 9 }, { 8, HANGAR, TAXIWAY_BUSY_block, 9 }, { 8, TERM1, TERM1_block, 3 }, { 8, 0, TAXIWAY_BUSY_block, 9 },
	{  9, 255, TAXIWAY_BUSY_block, 9 }, { 9, TAKEOFF, TAXIWAY_BUSY_block, 10 }, { 9, HANGAR, TAXIWAY_BUSY_block, 10 }, { 9, TERM2, TERM2_block, 4 }, { 9, HELIPAD1, HELIPAD1_block, 6 }, { 9, HELITAKEOFF, HELIPAD1_block, 6 }, { 9, TERM1, TAXIWAY_BUSY_block, 8 }, { 9, 0, TAXIWAY_BUSY_block, 10 },
	{ 10, 255, TAXIWAY_BUSY_block, 10 }, { 10, TERM3, TERM3_block, 5 }, { 10, HELIPAD1, 0, 9 }, { 10, HELIPAD2, HELIPAD2_block, 7 }, { 10, HELITAKEOFF, HELIPAD2_block, 7 }, { 10, TAKEOFF, TAXIWAY_BUSY_block, 1 }, { 10, HANGAR, TAXIWAY_BUSY_block, 1 }, { 10, 0, TAXIWAY_BUSY_block, 9 },
	{ 11, 0, OUT_WAY_block, 12 },
	// takeoff
	{ 12, TAKEOFF, RUNWAY_IN_OUT_block, 13 },
	{ 13, 0, RUNWAY_IN_OUT_block, 14 },
	{ 14, STARTTAKEOFF, RUNWAY_IN_OUT_block, 15 },
	{ 15, ENDTAKEOFF, NOTHING_block, 0 },
	// landing
	{ 16, FLYING, NOTHING_block, 21 }, { 16, LANDING, IN_WAY_block, 17 }, { 16, HELILANDING, 0, 25 },
	{ 17, LANDING, RUNWAY_IN_OUT_block, 18 },
	{ 18, 0, RUNWAY_IN_OUT_block, 19 },
	{ 19, 0, RUNWAY_IN_OUT_block, 20 },
	{ 20, ENDLANDING, IN_WAY_block, 2 },
	// In Air
	{ 21, 0, NOTHING_block, 22 },
	{ 22, 0, NOTHING_block, 23 },
	{ 23, 0, NOTHING_block, 24 },
	{ 24, 0, NOTHING_block, 16 },
	// Helicopter -- stay in air in special place as a buffer to choose from helipads
	{ 25, HELILANDING, PRE_HELIPAD_block, 26 },
	{ 26, HELIENDLANDING, PRE_HELIPAD_block, 26 }, { 26, HELIPAD1, 0, 27 }, { 26, HELIPAD2, 0, 28 }, { 26, HANGAR, 0, 33 },
	{ 27, 0, NOTHING_block, 29 }, //helipad1 approach
	{ 28, 0, NOTHING_block, 30 },
	// landing
	{ 29, 255, NOTHING_block, 0 }, { 29, HELIPAD1, HELIPAD1_block, 6 },
	{ 30, 255, NOTHING_block, 0 }, { 30, HELIPAD2, HELIPAD2_block, 7 },
	// Helicopter -- takeoff
	{ 31, HELITAKEOFF, NOTHING_block, 0 },
	{ 32, HELITAKEOFF, NOTHING_block, 0 },
	{ 33, 0, TAXIWAY_BUSY_block, 34 }, // need to go to hangar when waiting in air
	{ 34, 0, TAXIWAY_BUSY_block, 1 },
	{ 35, 0, HELIPAD1_block, 31 },
	{ 36, 0, HELIPAD2_block, 32 },
	{ MAX_ELEMENTS, 0, 0, 0 } // end marker. DO NOT REMOVE
};

static const TileIndexDiffC _airport_depots_city[] = { { 5, 0 } };
static const byte _airport_terminal_city[] = { 1, 3 };
static const AirportFTAbuildup _airport_fta_city[] = {
	{  0, HANGAR, NOTHING_block, 1 }, { 0, TAKEOFF, OUT_WAY_block, 1 }, { 0, 0, 0, 1 },
	{  1, 255, TAXIWAY_BUSY_block, 0 }, { 1, HANGAR, 0, 0 }, { 1, TERM2, 0, 6 }, { 1, TERM3, 0, 6 }, { 1, 0, 0, 7 }, // for all else, go to 7
	{  2, TERM1, TERM1_block, 7 }, { 2, TAKEOFF, OUT_WAY_block, 7 }, { 2, 0, 0, 7 },
	{  3, TERM2, TERM2_block, 5 }, { 3, TAKEOFF, OUT_WAY_block, 5 }, { 3, 0, 0, 5 },
	{  4, TERM3, TERM3_block, 5 }, { 4, TAKEOFF, OUT_WAY_block, 5 }, { 4, 0, 0, 5 },
	{  5, 255, TAXIWAY_BUSY_block, 0 }, { 5, TERM2, TERM2_block, 3 }, { 5, TERM3, TERM3_block, 4 }, { 5, 0, 0, 6 },
	{  6, 255, TAXIWAY_BUSY_block, 0 }, { 6, TERM2, 0, 5 }, { 6, TERM3, 0, 5 }, { 6, HANGAR, 0, 1 }, { 6, 0, 0, 7 },
	{  7, 255, TAXIWAY_BUSY_block, 0 }, { 7, TERM1, TERM1_block, 2 }, { 7, TAKEOFF, OUT_WAY_block, 8 }, { 7, HELITAKEOFF, 0, 22 }, { 7, HANGAR, 0, 1 }, { 7, 0, 0, 6 },
	{  8, 0, OUT_WAY_block, 9 },
	{  9, 0, RUNWAY_IN_OUT_block, 10 },
	// takeoff
	{ 10, TAKEOFF, RUNWAY_IN_OUT_block, 11 },
	{ 11, STARTTAKEOFF, NOTHING_block, 12 },
	{ 12, ENDTAKEOFF, NOTHING_block, 0 },
	// landing
	{ 13, FLYING, NOTHING_block, 18 }, { 13, LANDING, 0, 14 }, { 13, HELILANDING, 0, 23 },
	{ 14, LANDING, RUNWAY_IN_OUT_block, 15 },
	{ 15, 0, RUNWAY_IN_OUT_block, 16 },
	{ 16, 0, RUNWAY_IN_OUT_block, 17 },
	{ 17, ENDLANDING, IN_WAY_block, 7 },
	// In Air
	{ 18, 0, NOTHING_block, 19 },
	{ 19, 0, NOTHING_block, 20 },
	{ 20, 0, NOTHING_block, 21 },
	{ 21, 0, NOTHING_block, 13 },
	// helicopter
	{ 22, HELITAKEOFF, NOTHING_block, 0 },
	{ 23, HELILANDING, IN_WAY_block, 24 },
	{ 24, HELIENDLANDING, IN_WAY_block, 17 },
	{ MAX_ELEMENTS, 0, 0, 0 } // end marker. DO NOT REMOVE
};

static const TileIndexDiffC _airport_depots_metropolitan[] = { { 5, 0 } };
static const byte _airport_terminal_metropolitan[] = { 1, 3 };
static const AirportFTAbuildup _airport_fta_metropolitan[] = {
	{  0, HANGAR, NOTHING_block, 1 },
	{  1, 255, TAXIWAY_BUSY_block, 0 }, { 1, HANGAR, 0, 0 }, { 1, TERM2, 0, 6 }, { 1, TERM3, 0, 6 }, { 1, 0, 0, 7 }, // for all else, go to 7
	{  2, TERM1, TERM1_block, 7 },
	{  3, TERM2, TERM2_block, 5 },
	{  4, TERM3, TERM3_block, 5 },
	{  5, 255, TAXIWAY_BUSY_block, 0 }, { 5, TERM2, TERM2_block, 3 }, { 5, TERM3, TERM3_block, 4 }, { 5, 0, 0, 6 },
	{  6, 255, TAXIWAY_BUSY_block, 0 }, { 6, TERM2, 0, 5 }, { 6, TERM3, 0, 5 }, { 6, HANGAR, 0, 1 }, { 6, 0, 0, 7 },
	{  7, 255, TAXIWAY_BUSY_block, 0 }, { 7, TERM1, TERM1_block, 2 }, { 7, TAKEOFF, 0, 8 }, { 7, HELITAKEOFF, 0, 23 }, { 7, HANGAR, 0, 1 }, { 7, 0, 0, 6 },
	{  8, 0, OUT_WAY_block, 9 },
	{  9, 0, RUNWAY_OUT_block, 10 },
	// takeoff
	{ 10, TAKEOFF, RUNWAY_OUT_block, 11 },
	{ 11, STARTTAKEOFF, NOTHING_block, 12 },
	{ 12, ENDTAKEOFF, NOTHING_block, 0 },
	// landing
	{ 13, FLYING, NOTHING_block, 19 }, { 13, LANDING, 0, 14 }, { 13, HELILANDING, 0, 25 },
	{ 14, LANDING, RUNWAY_IN_block, 15 },
	{ 15, 0, RUNWAY_IN_block, 16 },
	{ 16, 255, RUNWAY_IN_block, 0 }, { 16, ENDLANDING, IN_WAY_block, 17 },
	{ 17, 255, RUNWAY_OUT_block, 0 }, { 17, ENDLANDING, IN_WAY_block, 18 },
	{ 18, ENDLANDING, IN_WAY_block, 7 },
	// In Air
	{ 19, 0, NOTHING_block, 20 },
	{ 20, 0, NOTHING_block, 21 },
	{ 21, 0, NOTHING_block, 22 },
	{ 22, 0, NOTHING_block, 13 },
	// helicopter
	{ 23, 0, NOTHING_block, 24 },
	{ 24, HELITAKEOFF, NOTHING_block, 0 },
	{ 25, HELILANDING, IN_WAY_block, 26 },
	{ 26, HELIENDLANDING, IN_WAY_block, 18 },
	{ MAX_ELEMENTS, 0, 0, 0 } // end marker. DO NOT REMOVE
};

static const TileIndexDiffC _airport_depots_international[] = { { 0, 3 }, { 6, 1 } };
static const byte _airport_terminal_international[] = { 2, 3, 3 };
static const byte _airport_helipad_international[] = { 1, 2 };
static const AirportFTAbuildup _airport_fta_international[] = {
	{  0, HANGAR, NOTHING_block, 2 }, { 0, 255, TERM_GROUP1_block, 0 }, { 0, 255, TERM_GROUP2_ENTER1_block, 1 }, { 0, HELITAKEOFF, HELIPAD1_block, 2 }, { 0, 0, 0, 2 },
	{  1, HANGAR, NOTHING_block, 3 }, { 1, 255, HANGAR2_AREA_block, 1 }, { 1, HELITAKEOFF, HELIPAD2_block, 3 }, { 1, 0, 0, 3 },
	{  2, 255, AIRPORT_ENTRANCE_block, 0 }, { 2, HANGAR, 0, 0 }, { 2, TERM4, 0, 12 }, { 2, TERM5, 0, 12 }, { 2, TERM6, 0, 12 }, { 2, HELIPAD1, 0, 12 }, { 2, HELIPAD2, 0, 12 }, { 2, HELITAKEOFF, 0, 12 }, { 2, 0, 0, 23 },
	{  3, 255, HANGAR2_AREA_block, 0 }, { 3, HANGAR, 0, 1 }, { 3, 0, 0, 18 },
	{  4, TERM1, TERM1_block, 23 }, { 4, HANGAR, AIRPORT_ENTRANCE_block, 23 }, { 4, 0, 0, 23 },
	{  5, TERM2, TERM2_block, 24 }, { 5, HANGAR, AIRPORT_ENTRANCE_block, 24 }, { 5, 0, 0, 24 },
	{  6, TERM3, TERM3_block, 25 }, { 6, HANGAR, AIRPORT_ENTRANCE_block, 25 }, { 6, 0, 0, 25 },
	{  7, TERM4, TERM4_block, 16 }, { 7, HANGAR, HANGAR2_AREA_block, 16 }, { 7, 0, 0, 16 },
	{  8, TERM5, TERM5_block, 17 }, { 8, HANGAR, HANGAR2_AREA_block, 17 }, { 8, 0, 0, 17 },
	{  9, TERM6, TERM6_block, 18 }, { 9, HANGAR, HANGAR2_AREA_block, 18 }, { 9, 0, 0, 18 },
	{ 10, HELIPAD1, HELIPAD1_block, 10 }, { 10, HANGAR, HANGAR2_AREA_block, 16 }, { 10, HELITAKEOFF, 0, 47 },
	{ 11, HELIPAD2, HELIPAD2_block, 11 }, { 11, HANGAR, HANGAR2_AREA_block, 17 }, { 11, HELITAKEOFF, 0, 48 },
	{ 12, 0, TERM_GROUP2_ENTER1_block, 13 },
	{ 13, 0, TERM_GROUP2_ENTER1_block, 14 },
	{ 14, 0, TERM_GROUP2_ENTER2_block, 15 },
	{ 15, 0, TERM_GROUP2_ENTER2_block, 16 },
	{ 16, 255, TERM_GROUP2_block, 0 }, { 16, TERM4, TERM4_block, 7 }, { 16, HELIPAD1, HELIPAD1_block, 10 }, { 16, HELITAKEOFF, HELIPAD1_block, 10 }, { 16, 0, 0, 17 },
	{ 17, 255, TERM_GROUP2_block, 0 }, { 17, TERM5, TERM5_block, 8 }, { 17, TERM4, 0, 16 }, { 17, HELIPAD1, 0, 16 }, { 17, HELIPAD2, HELIPAD2_block, 11 }, { 17, HELITAKEOFF, HELIPAD2_block, 11 }, { 17, 0, 0, 18 },
	{ 18, 255, TERM_GROUP2_block, 0 }, { 18, TERM6, TERM6_block, 9 }, { 18, TAKEOFF, 0, 19 }, { 18, HANGAR, HANGAR2_AREA_block, 3 }, { 18, 0, 0, 17 },
	{ 19, 0, TERM_GROUP2_EXIT1_block, 20 },
	{ 20, 0, TERM_GROUP2_EXIT1_block, 21 },
	{ 21, 0, TERM_GROUP2_EXIT2_block, 22 },
	{ 22, 0, TERM_GROUP2_EXIT2_block, 26 },
	{ 23, 255, TERM_GROUP1_block, 0 }, { 23, TERM1, TERM1_block, 4 }, { 23, HANGAR, AIRPORT_ENTRANCE_block, 2 }, { 23, 0, 0, 24 },
	{ 24, 255, TERM_GROUP1_block, 0 }, { 24, TERM2, TERM2_block, 5 }, { 24, TERM1, 0, 23 }, { 24, HANGAR, 0, 23 }, { 24, 0, 0, 25 },
	{ 25, 255, TERM_GROUP1_block, 0 }, { 25, TERM3, TERM3_block, 6 }, { 25, TAKEOFF, 0, 26 }, { 25, 0, 0, 24 },
	{ 26, 255, TAXIWAY_BUSY_block, 0 }, { 26, TAKEOFF, 0, 27 }, { 26, 0, 0, 25 },
	{ 27, 0, OUT_WAY_block, 28 },
	// takeoff
	{ 28, TAKEOFF, OUT_WAY_block, 29 },
	{ 29, 0, RUNWAY_OUT_block, 30 },
	{ 30, STARTTAKEOFF, NOTHING_block, 31 },
	{ 31, ENDTAKEOFF, NOTHING_block, 0 },
	// landing
	{ 32, FLYING, NOTHING_block, 37 }, { 32, LANDING, 0, 33 }, { 32, HELILANDING, 0, 41 },
	{ 33, LANDING, RUNWAY_IN_block, 34 },
	{ 34, 0, RUNWAY_IN_block, 35 },
	{ 35, 0, RUNWAY_IN_block, 36 },
	{ 36, ENDLANDING, IN_WAY_block, 36 }, { 36, 255, TERM_GROUP1_block, 0 }, { 36, 255, TERM_GROUP2_ENTER1_block, 1 }, { 36, TERM4, 0, 12 }, { 36, TERM5, 0, 12 }, { 36, TERM6, 0, 12 }, { 36, 0, 0, 2 },
	// In Air
	{ 37, 0, NOTHING_block, 38 },
	{ 38, 0, NOTHING_block, 39 },
	{ 39, 0, NOTHING_block, 40 },
	{ 40, 0, NOTHING_block, 32 },
	// Helicopter -- stay in air in special place as a buffer to choose from helipads
	{ 41, HELILANDING, PRE_HELIPAD_block, 42 },
	{ 42, HELIENDLANDING, PRE_HELIPAD_block, 42 }, { 42, HELIPAD1, 0, 43 }, { 42, HELIPAD2, 0, 44 }, { 42, HANGAR, 0, 49 },
	{ 43, 0, NOTHING_block, 45 },
	{ 44, 0, NOTHING_block, 46 },
	// landing
	{ 45, 255, NOTHING_block, 0 }, { 45, HELIPAD1, HELIPAD1_block, 10 },
	{ 46, 255, NOTHING_block, 0 }, { 46, HELIPAD2, HELIPAD2_block, 11 },
	// Helicopter -- takeoff
	{ 47, HELITAKEOFF, NOTHING_block, 0 },
	{ 48, HELITAKEOFF, NOTHING_block, 0 },
	{ 49, 0, HANGAR2_AREA_block, 50 }, // need to go to hangar when waiting in air
	{ 50, 0, HANGAR2_AREA_block, 3 },
	{ MAX_ELEMENTS, 0, 0, 0 } // end marker. DO NOT REMOVE
};

// intercontinental
static const TileIndexDiffC _airport_depots_intercontinental[] = { { 0, 5 }, { 8, 4 } };
static const byte _airport_terminal_intercontinental[] = { 2, 4, 4 };
static const byte _airport_helipad_intercontinental[] = { 1, 2 };
static const AirportFTAbuildup _airport_fta_intercontinental[] = {
	{  0, HANGAR, NOTHING_block, 2 }, { 0, 255, HANGAR1_AREA_block | TERM_GROUP1_block, 0 }, { 0, 255, HANGAR1_AREA_block | TERM_GROUP1_block, 1 }, { 0, TAKEOFF, HANGAR1_AREA_block | TERM_GROUP1_block, 2 }, { 0, 0, 0, 2 },
	{  1, HANGAR, NOTHING_block, 3 }, { 1, 255, HANGAR2_AREA_block, 1 }, { 1, 255, HANGAR2_AREA_block, 0 }, { 1, 0, 0, 3 },
	{  2, 255, HANGAR1_AREA_block, 0 }, { 2, 255, TERM_GROUP1_block, 0 }, { 2, 255, TERM_GROUP1_block, 1 }, { 2, HANGAR, 0, 0 }, { 2, TAKEOFF, TERM_GROUP1_block, 27 }, { 2, TERM5, 0, 26 }, { 2, TERM6, 0, 26 }, { 2, TERM7, 0, 26 }, { 2, TERM8, 0, 26 }, { 2, HELIPAD1, 0, 26 }, { 2, HELIPAD2, 0, 26 }, { 2, HELITAKEOFF, 0, 74 }, { 2, 0, 0, 27 },
	{  3, 255, HANGAR2_AREA_block, 0 }, { 3, HANGAR, 0, 1 }, { 3, HELITAKEOFF, 0, 75 }, { 3, 0, 0, 20 },
	{  4, TERM1, TERM1_block, 26 }, { 4, HANGAR, HANGAR1_AREA_block | TERM_GROUP1_block, 26 }, { 4, 0, 0, 26 },
	{  5, TERM2, TERM2_block, 27 }, { 5, HANGAR, HANGAR1_AREA_block | TERM_GROUP1_block, 27 }, { 5, 0, 0, 27 },
	{  6, TERM3, TERM3_block, 28 }, { 6, HANGAR, HANGAR1_AREA_block | TERM_GROUP1_block, 28 }, { 6, 0, 0, 28 },
	{  7, TERM4, TERM4_block, 29 }, { 7, HANGAR, HANGAR1_AREA_block | TERM_GROUP1_block, 29 }, { 7, 0, 0, 29 },
	{  8, TERM5, TERM5_block, 18 }, { 8, HANGAR, HANGAR2_AREA_block, 18 }, { 8, 0, 0, 18 },
	{  9, TERM6, TERM6_block, 19 }, { 9, HANGAR, HANGAR2_AREA_block, 19 }, { 9, 0, 0, 19 },
	{ 10, TERM7, TERM7_block, 20 }, { 10, HANGAR, HANGAR2_AREA_block, 20 }, { 10, 0, 0, 20 },
	{ 11, TERM8, TERM8_block, 21 }, { 11, HANGAR, HANGAR2_AREA_block, 21 }, { 11, 0, 0, 21 },
	{ 12, HELIPAD1, HELIPAD1_block, 12 }, { 12, HANGAR, 0, 70 }, { 12, HELITAKEOFF, 0, 72 },
	{ 13, HELIPAD2, HELIPAD2_block, 13 }, { 13, HANGAR, 0, 71 }, { 13, HELITAKEOFF, 0, 73 },
	{ 14, 0, TERM_GROUP2_ENTER1_block, 15 },
	{ 15, 0, TERM_GROUP2_ENTER1_block, 16 },
	{ 16, 0, TERM_GROUP2_ENTER2_block, 17 },
	{ 17, 0, TERM_GROUP2_ENTER2_block, 18 },
	{ 18, 255, TERM_GROUP2_block, 0 }, { 18, TERM5, TERM5_block, 8 }, { 18, TAKEOFF, 0, 19 }, { 18, HELITAKEOFF, HELIPAD1_block, 19 }, { 18, 0, TERM_GROUP2_EXIT1_block, 19 },
	{ 19, 255, TERM_GROUP2_block, 0 }, { 19, TERM6, TERM6_block, 9 }, { 19, TERM5, 0, 18 }, { 19, TAKEOFF, 0, 57 }, { 19, HELITAKEOFF, HELIPAD1_block, 20 }, { 19, 0, TERM_GROUP2_EXIT1_block, 20 }, // add exit to runway out 2
	{ 20, 255, TERM_GROUP2_block, 0 }, { 20, TERM7, TERM7_block, 10 }, { 20, TERM5, 0, 19 }, { 20, TERM6, 0, 19 }, { 20, HANGAR, HANGAR2_AREA_block, 3 }, { 20, TAKEOFF, 0, 19 }, { 20, 0, TERM_GROUP2_EXIT1_block, 21 },
	{ 21, 255, TERM_GROUP2_block, 0 }, { 21, TERM8, TERM8_block, 11 }, { 21, HANGAR, HANGAR2_AREA_block, 20 }, { 21, TERM5, 0, 20 }, { 21, TERM6, 0, 20 }, { 21, TERM7, 0, 20 }, { 21, TAKEOFF, 0, 20 }, { 21, 0, TERM_GROUP2_EXIT1_block, 22 },
	{ 22, 255, TERM_GROUP2_block, 0 }, { 22, HANGAR, 0, 21 }, { 22, TERM5, 0, 21 }, { 22, TERM6, 0, 21 }, { 22, TERM7, 0, 21 }, { 22, TERM8, 0, 21 }, { 22, TAKEOFF, 0, 21 }, { 22, 0, 0, 23 },
	{ 23, 0, TERM_GROUP2_EXIT1_block, 70 },
	{ 24, 0, TERM_GROUP2_EXIT2_block, 25 },
	{ 25, 255, TERM_GROUP2_EXIT2_block, 0 }, { 25, HANGAR, HANGAR1_AREA_block | TERM_GROUP1_block, 29 }, { 25, 0, 0, 29 },
	{ 26, 255, TERM_GROUP1_block, 0 }, { 26, TERM1, TERM1_block, 4 }, { 26, HANGAR, HANGAR1_AREA_block, 27 }, { 26, TERM5, TERM_GROUP2_ENTER1_block, 14 }, { 26, TERM6, TERM_GROUP2_ENTER1_block, 14 }, { 26, TERM7, TERM_GROUP2_ENTER1_block, 14 }, { 26, TERM8, TERM_GROUP2_ENTER1_block, 14 }, { 26, HELIPAD1, TERM_GROUP2_ENTER1_block, 14 }, { 26, HELIPAD2, TERM_GROUP2_ENTER1_block, 14 }, { 26, HELITAKEOFF, TERM_GROUP2_ENTER1_block, 14 }, { 26, 0, 0, 27 },
	{ 27, 255, TERM_GROUP1_block, 0 }, { 27, TERM2, TERM2_block, 5 }, { 27, HANGAR, HANGAR1_AREA_block, 2 }, { 27, TERM1, 0, 26 }, { 27, TERM5, 0, 26 }, { 27, TERM6, 0, 26 }, { 27, TERM7, 0, 26 }, { 27, TERM8, 0, 26 }, { 27, HELIPAD1, 0, 14 }, { 27, HELIPAD2, 0, 14 }, { 27, 0, 0, 28 },
	{ 28, 255, TERM_GROUP1_block, 0 }, { 28, TERM3, TERM3_block, 6 }, { 28, HANGAR, HANGAR1_AREA_block, 27 }, { 28, TERM1, 0, 27 }, { 28, TERM2, 0, 27 }, { 28, TERM4, 0, 29 }, { 28, TERM5, 0, 14 }, { 28, TERM6, 0, 14 }, { 28, TERM7, 0, 14 }, { 28, TERM8, 0, 14 }, { 28, HELIPAD1, 0, 14 }, { 28, HELIPAD2, 0, 14 }, { 28, 0, 0, 29 },
	{ 29, 255, TERM_GROUP1_block, 0 }, { 29, TERM4, TERM4_block, 7 }, { 29, HANGAR, HANGAR1_AREA_block, 27 }, { 29, TAKEOFF, 0, 30 }, { 29, 0, 0, 28 },
	{ 30, 0, OUT_WAY_block2, 31 },
	{ 31, 0, OUT_WAY_block, 32 },
	// takeoff
	{ 32, TAKEOFF, RUNWAY_OUT_block, 33 },
	{ 33, 0, RUNWAY_OUT_block, 34 },
	{ 34, STARTTAKEOFF, NOTHING_block, 35 },
	{ 35, ENDTAKEOFF, NOTHING_block, 0 },
	// landing
	{ 36, 0, 0, 0 },
	{ 37, LANDING, RUNWAY_IN_block, 38 },
	{ 38, 0, RUNWAY_IN_block, 39 },
	{ 39, 0, RUNWAY_IN_block, 40 },
	{ 40, ENDLANDING, RUNWAY_IN_block, 41 },
	{ 41, 0, IN_WAY_block, 42 },
	{ 42, 255, IN_WAY_block, 0 }, { 42, 255, TERM_GROUP1_block, 0 }, { 42, 255, TERM_GROUP1_block, 1 }, { 42, HANGAR, 0, 2 }, { 42, 0, 0, 26 },
	// In Air
	{ 43, 0, 0, 44 },
	{ 44, FLYING, 0, 45 }, { 44, HELILANDING, 0, 47 }, { 44, LANDING, 0, 69 }, { 44, 0, 0, 45 },
	{ 45, 0, 0, 46 },
	{ 46, FLYING, 0, 43 }, { 46, LANDING, 0, 76 }, { 46, 0, 0, 43 },
	// Helicopter -- stay in air in special place as a buffer to choose from helipads
	{ 47, HELILANDING, PRE_HELIPAD_block, 48 },
	{ 48, HELIENDLANDING, PRE_HELIPAD_block, 48 }, { 48, HELIPAD1, 0, 49 }, { 48, HELIPAD2, 0, 50 }, { 48, HANGAR, 0, 55 },
	{ 49, 0, NOTHING_block, 51 },
	{ 50, 0, NOTHING_block, 52 },
	// landing
	{ 51, 255, NOTHING_block, 0 }, { 51, HELIPAD1, HELIPAD1_block, 12 }, { 51, HANGAR, 0, 55 }, { 51, 0, 0, 12 },
	{ 52, 255, NOTHING_block, 0 }, { 52, HELIPAD2, HELIPAD2_block, 13 }, { 52, HANGAR, 0, 55 }, { 52, 0, 0, 13 },
	// Helicopter -- takeoff
	{ 53, HELITAKEOFF, NOTHING_block, 0 },
	{ 54, HELITAKEOFF, NOTHING_block, 0 },
	{ 55, 0, HANGAR2_AREA_block, 56 }, // need to go to hangar when waiting in air
	{ 56, 0, HANGAR2_AREA_block, 3 },
	// runway 2 out support
	{ 57, 255, OUT_WAY2_block, 0 }, { 57, TAKEOFF, 0, 58 }, { 57, 0, 0, 58 },
	{ 58, 0, OUT_WAY2_block, 59 },
	{ 59, TAKEOFF, RUNWAY_OUT2_block, 60 }, // takeoff
	{ 60, 0, RUNWAY_OUT2_block, 61 },
	{ 61, STARTTAKEOFF, NOTHING_block, 62 },
	{ 62, ENDTAKEOFF, NOTHING_block, 0 },
	// runway 2 in support
	{ 63, LANDING, RUNWAY_IN2_block, 64 },
	{ 64, 0, RUNWAY_IN2_block, 65 },
	{ 65, 0, RUNWAY_IN2_block, 66 },
	{ 66, ENDLANDING, RUNWAY_IN2_block, 0 }, { 66, 255, 0, 1 }, { 66, 255, 0, 0 }, { 66, 0, 0, 67 },
	{ 67, 0, IN_WAY2_block, 68 },
	{ 68, 255, IN_WAY2_block, 0 }, { 68, 255, TERM_GROUP2_block, 1 }, { 68, 255, TERM_GROUP1_block, 0 }, { 68, HANGAR, HANGAR2_AREA_block, 22 }, { 68, 0, 0, 22 },
	{ 69, 255, RUNWAY_IN2_block, 0 }, { 69, 0, RUNWAY_IN2_block, 63 },
	{ 70, 255, TERM_GROUP2_EXIT1_block, 0 }, { 70, HELIPAD1, HELIPAD1_block, 12 }, { 70, HELITAKEOFF, HELIPAD1_block, 12 }, { 70, 0, 0, 71 },
	{ 71, 255, TERM_GROUP2_EXIT1_block, 0 }, { 71, HELIPAD2, HELIPAD2_block, 13 }, { 71, HELITAKEOFF, HELIPAD1_block, 12 }, { 71, 0, 0, 24 },
	{ 72, 0, HELIPAD1_block, 53 },
	{ 73, 0, HELIPAD2_block, 54 },
	{ 74, HELITAKEOFF, NOTHING_block, 0 },
	{ 75, HELITAKEOFF, NOTHING_block, 0 },
	{ 76, 255, RUNWAY_IN_block, 0 }, { 76, 0, RUNWAY_IN_block, 37 },
	{ MAX_ELEMENTS, 0, 0, 0 } // end marker. DO NOT REMOVE
};


// heliports, oilrigs don't have depots
static const byte _airport_helipad_heliport_oilrig[] = { 1, 1 };
static const AirportFTAbuildup _airport_fta_heliport_oilrig[] = {
	{ 0, HELIPAD1, HELIPAD1_block, 1 },
	{ 1, HELITAKEOFF, NOTHING_block, 0 }, // takeoff
	{ 2, 255, AIRPORT_BUSY_block, 0 }, { 2, HELILANDING, 0, 3 }, { 2, HELITAKEOFF, 0, 1 },
	{ 3, HELILANDING, AIRPORT_BUSY_block, 4 },
	{ 4, HELIENDLANDING, AIRPORT_BUSY_block, 4 }, { 4, HELIPAD1, HELIPAD1_block, 0 }, { 4, HELITAKEOFF, 0, 2 },
	// In Air
	{ 5, 0, NOTHING_block, 6 },
	{ 6, 0, NOTHING_block, 7 },
	{ 7, 0, NOTHING_block, 8 },
	{ 8, FLYING, NOTHING_block, 5 }, { 8, HELILANDING, HELIPAD1_block, 2 }, // landing
	{ MAX_ELEMENTS, 0, 0, 0 } // end marker. DO NOT REMOVE
};

// helidepots
static const TileIndexDiffC _airport_depots_helidepot[] = { { 1, 0 } };
static const byte _airport_helipad_helidepot[] = { 1, 1 };
static const AirportFTAbuildup _airport_fta_helidepot[] = {
	{  0, HANGAR, NOTHING_block, 1 },
	{  1, 255, HANGAR2_AREA_block, 0 }, { 1, HANGAR, 0, 0 }, { 1, HELIPAD1, HELIPAD1_block, 14 }, { 1, HELITAKEOFF, 0, 15 }, { 1, 0, 0, 0 },
	{  2, FLYING, NOTHING_block, 3 }, { 2, HELILANDING, PRE_HELIPAD_block, 7 }, { 2, HANGAR, 0, 12 }, { 2, HELITAKEOFF, NOTHING_block, 16 },
	// In Air
	{  3, 0, NOTHING_block, 4 },
	{  4, 0, NOTHING_block, 5 },
	{  5, 0, NOTHING_block, 6 },
	{  6, 0, NOTHING_block, 2 },
	// Helicopter -- stay in air in special place as a buffer to choose from helipads
	{  7, HELILANDING, PRE_HELIPAD_block, 8 },
	{  8, HELIENDLANDING, PRE_HELIPAD_block, 8 }, { 8, HELIPAD1, 0, 9 }, { 8, HANGAR, 0, 12 }, { 8, 0, 0, 2 },
	{  9, 0, NOTHING_block, 10 },
	// landing
	{ 10, 255, NOTHING_block, 10 }, { 10, HELIPAD1, HELIPAD1_block, 14 }, { 10, HANGAR, 0, 1 }, { 10, 0, 0, 14 },
	// Helicopter -- takeoff
	{ 11, HELITAKEOFF, NOTHING_block, 0 },
	{ 12, 0, HANGAR2_AREA_block, 13 }, // need to go to hangar when waiting in air
	{ 13, 0, HANGAR2_AREA_block, 1 },
	{ 14, HELIPAD1, HELIPAD1_block, 14 }, { 14, HANGAR, 0, 1 }, { 14, HELITAKEOFF, 0, 17 },
	{ 15, HELITAKEOFF, NOTHING_block, 0 }, // takeoff outside depot
	{ 16, HELITAKEOFF, 0, 14 },
	{ 17, 0, NOTHING_block, 11 },
	{ MAX_ELEMENTS, 0, 0, 0 } // end marker. DO NOT REMOVE
};

// helistation
static const TileIndexDiffC _airport_depots_helistation[] = { { 0, 0 } };
static const byte _airport_helipad_helistation[] = { 1, 3 };
static const AirportFTAbuildup _airport_fta_helistation[] = {
	{  0, HANGAR, NOTHING_block, 8 },    { 0, HELIPAD1, 0, 1 }, { 0, HELIPAD2, 0, 1 }, { 0, HELIPAD3, 0, 1 }, { 0, HELITAKEOFF, 0, 1 }, { 0, 0, 0, 0 },
	{  1, 255, HANGAR2_AREA_block, 0 },  { 1, HANGAR, 0, 0 }, { 1, HELITAKEOFF, 0, 3 }, { 1, 0, 0, 4 },
	// landing
	{  2, FLYING, NOTHING_block, 28 },   { 2, HELILANDING, 0, 15 }, { 2, 0, 0, 28 },
	// helicopter side
	{  3, HELITAKEOFF, NOTHING_block, 0 }, // helitakeoff outside hangar2
	{  4, 255, TAXIWAY_BUSY_block, 0 },  { 4, HANGAR, HANGAR2_AREA_block, 1 }, { 4, HELITAKEOFF, 0, 1 }, { 4, 0, 0, 5 },
	{  5, 255, TAXIWAY_BUSY_block, 0 },  { 5, HELIPAD1, HELIPAD1_block, 6 }, { 5, HELIPAD2, HELIPAD2_block, 7 }, { 5, HELIPAD3, HELIPAD3_block, 8 }, { 5, 0, 0, 4 },
	{  6, HELIPAD1, HELIPAD1_block, 5 }, { 6, HANGAR, HANGAR2_AREA_block, 5 }, { 6, HELITAKEOFF, 0, 9 }, { 6, 0, 0, 6 },
	{  7, HELIPAD2, HELIPAD2_block, 5 }, { 7, HANGAR, HANGAR2_AREA_block, 5 }, { 7, HELITAKEOFF, 0, 10 }, { 7, 0, 0, 7 },
	{  8, HELIPAD3, HELIPAD3_block, 5 }, { 8, HANGAR, HANGAR2_AREA_block, 5 }, { 8, HELITAKEOFF, 0, 11 }, { 8, 0, 0, 8 },
	{  9, 0, HELIPAD1_block, 12 },
	{ 10, 0, HELIPAD2_block, 13 },
	{ 11, 0, HELIPAD3_block, 14 },
	{ 12, HELITAKEOFF, NOTHING_block, 0 },
	{ 13, HELITAKEOFF, NOTHING_block, 0 },
	{ 14, HELITAKEOFF, NOTHING_block, 0 },
	// heli - in flight moves
	{ 15, HELILANDING, PRE_HELIPAD_block, 16 },
	{ 16, HELIENDLANDING, PRE_HELIPAD_block, 16 }, { 16, HELIPAD1, 0, 17 }, { 16, HELIPAD2, 0, 18 }, { 16, HELIPAD3, 0, 19 }, { 16, HANGAR, 0, 23 },
	{ 17, 0, NOTHING_block, 20 },
	{ 18, 0, NOTHING_block, 21 },
	{ 19, 0, NOTHING_block, 22 },
	// heli landing
	{ 20, 255, NOTHING_block, 0 }, { 20, HELIPAD1, HELIPAD1_block, 6 }, { 20, HANGAR, 0, 23 }, { 20, 0, 0, 6 },
	{ 21, 255, NOTHING_block, 0 }, { 21, HELIPAD2, HELIPAD2_block, 7 }, { 21, HANGAR, 0, 23 }, { 21, 0, 0, 7 },
	{ 22, 255, NOTHING_block, 0 }, { 22, HELIPAD3, HELIPAD3_block, 8 }, { 22, HANGAR, 0, 23 }, { 22, 0, 0, 8 },
	{ 23, 0, HANGAR2_AREA_block, 24 }, // need to go to helihangar when waiting in air
	{ 24, 0, HANGAR2_AREA_block, 1 },
	{ 25, 0, NOTHING_block, 26 },
	{ 26, 0, NOTHING_block, 27 },
	{ 27, 0, NOTHING_block, 2 },
	{ 28, 0, NOTHING_block, 29 },
	{ 29, 0, NOTHING_block, 30 },
	{ 30, 0, NOTHING_block, 31 },
	{ 31, 0, NOTHING_block, 32 },
	{ 32, 0, NOTHING_block, 25 },
	{ MAX_ELEMENTS, 0, 0, 0 } // end marker. DO NOT REMOVE
};


static const AirportMovingData * const _airport_moving_datas[] = {
	_airport_moving_data_country,           // Country Airfield (small) 4x3
	_airport_moving_data_town,              // City Airport (large) 6x6
	_airport_moving_data_heliport,          // Heliport
	_airport_moving_data_metropolitan,      // Metropolitain Airport (large) - 2 runways
	_airport_moving_data_international,     // International Airport (xlarge) - 2 runways
	_airport_moving_data_commuter,          // Commuter Airfield (small) 5x4
	_airport_moving_data_helidepot,         // Helidepot
	_airport_moving_data_intercontinental,  // Intercontinental Airport (xxlarge) - 4 runways
	_airport_moving_data_helistation,       // Helistation
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	_airport_moving_data_oilrig             // Oilrig
};

#endif /* AIRPORT_MOVEMENT_H */
