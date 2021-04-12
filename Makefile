TARGET = openttd

# Enable network debuging or debug symbols
#PSP_NETDEBUG=1
#DEBUG_SYM=1

PRX_EXPORTS=exports.exp
BUILD_PRX = 1
PSP_FW_VERSION = 371

OBJS = ai/default/default.o ai/trolly/build.o ai/trolly/pathfinder.o ai/trolly/shared.o ai/trolly/trolly.o ai/ai.o aircraft_cmd.o aircraft_gui.o airport_gui.o airport.o aystar.o bridge_gui.o callback_table.o clear_cmd.o command.o console_cmds.o console.o currency.o debug.o dedicated.o depot.o disaster_cmd.o dock_gui.o driver.o dummy_land.o economy.o engine_gui.o engine.o fileio.o gfxinit.o gfx.o graph_gui.o industry_cmd.o industry_gui.o intro_gui.o landscape.o main_gui.o map.o md5.o mersenne.o minilzo.o misc_cmd.o misc_gui.o misc.o mixer.o music/null_m.o music_gui.o namegen.o network_client.o network_data.o network_gamelist.o network_gui.o network.o network_server.o network_udp.o newgrf.o news_gui.o npf.o oldloader.o openttd.o order_cmd.o order_gui.o pathfind.o player_gui.o players.o queue.o rail_cmd.o rail_gui.o rail.o road_cmd.o road_gui.o roadveh_cmd.o roadveh_gui.o saveload.o screenshot.o sdl.o settings_gui.o settings.o ship_cmd.o ship_gui.o signs.o smallmap_gui.o sound/null_s.o sound/psp_s.o sound.o spritecache.o station_cmd.o station_gui.o string.o strings.o subsidy_gui.o terraform_gui.o texteff.o thread.o tile.o town_cmd.o town_gui.o train_cmd.o train_gui.o tree_cmd.o tunnelbridge_cmd.o psp.o unmovable_cmd.o vehicle_gui.o vehicle.o  rev.o video/dedicated_v.o video/null_v.o video/sdl_v.o viewport.o water_cmd.o waypoint.o widget.o window.o oldpool.o newgrf_engine.o newgrf_station.o newgrf_cargo.o newgrf_config.o newgrf_gui.o newgrf_sound.o newgrf_spritegroup.o newgrf_text.o date.o depot_gui.o fios.o bmp.o bridge_map.o build_vehicle_gui.o elrail.o fontcache.o genworld.o helpers.o station_map.o genworld_gui.o road_map.o heightmap.o thread.o yapf/follow_track.o  yapf/yapf_common.o  yapf/yapf_rail.o  yapf/yapf_road.o  yapf/yapf_ship.o os_timer.o music.o tunnel_map.o tgp.o danzeff.o music/psp_timidity.o


INCDIR =
CFLAGS = -G0 -O2 -Wall -DWITH_REV -DWITH_ZLIB -DWITH_PNG -DWITH_SDL -DPSP -DNO_THREADS -DENABLE_NETWORK
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

ifdef PSP_NETDEBUG
CFLAGS += -DPSP_NETDEBUG
endif

ifdef DEBUG_SYM
CFLAGS += -g
endif

LIBDIR =
LIBS =
LDFLAGS =

# SDL
PSPBIN = $(PSPSDK)/../bin
CFLAGS += -I/usr/local/pspdev/psp/include/SDL

# FreeType
CFLAGS += -I/usr/local/pspdev/psp/include/freetype2 -DWITH_FREETYPE


LIBS += -L/usr/local/pspdev/psp/lib -L/usr/local/pspdev/psp/sdk/lib -lSDL_image -lSDL -lm -lpspgu -lpspaudio -lpng -lz -ljpeg -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspsdk -lc -lpspnet -lpspnet_inet -lpspnet_apctl -lpspnet_resolver -lpsputility -lpspuser -lpspkernel -lpsphprm -lpsppower -lpspwlan -lstdc++ -lfreetype

# Timdity
CFLAGS += -I/usr/local/pspdev/psp/include/libtimidity
LIBS += -lpspaudiolib -lpspaudio -ltimidity


EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Openttd
PSP_EBOOT_ICON = eboot/ICON0.PNG
PSP_EBOOT_ICON1 = NULL
PSP_EBOOT_UNKPNG = eboot/PIC0.PNG
PSP_EBOOT_PIC1 = eboot/PIC1.PNG
PSP_EBOOT_SND0 = eboot/SND0.AT3

PSPSDK=$(shell psp-config --pspsdk-path)
include build.mak
