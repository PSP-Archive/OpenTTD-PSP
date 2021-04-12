/* $Id: network_gamelist.c 8625 2007-02-08 10:19:03Z rubidium $ */

#ifdef ENABLE_NETWORK

#include "stdafx.h"
#include "debug.h"
#include "network_data.h"
#include "newgrf_config.h"
#include "network_udp.h"

/** Should we stop/contiue requerying of offline servers? */
static bool _stop_requerying = false;

// This file handles the GameList
// Also, it handles the request to a server for data about the server

/** Add a new item to the linked gamelist. If the IP and Port match
 * return the existing item instead of adding it again
 * @param ip the IP-address (inet_addr) of the to-be added item
 * @param port the port the server is running on
 * @return a point to the newly added or already existing item */
NetworkGameList *NetworkGameListAddItem(uint32 ip, uint16 port)
{
	NetworkGameList *item, *prev_item;

	prev_item = NULL;
	for (item = _network_game_list; item != NULL; item = item->next) {
		if (item->ip == ip && item->port == port) return item;
		prev_item = item;
	}

	item = malloc(sizeof(*item));
	memset(item, 0, sizeof(*item));
	item->next = NULL;
	item->ip = ip;
	item->port = port;

	if (prev_item == NULL) {
		_network_game_list = item;
	} else {
		prev_item->next = item;
	}
	DEBUG(net, 4) ("[NET][GameList] Added server to list");

	UpdateNetworkGameWindow(false);
	_stop_requerying = false;

	return item;
}

/** Remove an item from the gamelist linked list
 * @param remove pointer to the item to be removed */
void NetworkGameListRemoveItem(NetworkGameList *remove)
{
	NetworkGameList *item, *prev_item;

	prev_item = NULL;
	for (item = _network_game_list; item != NULL; item = item->next) {
		if (remove == item) {
			if (prev_item == NULL) {
				_network_game_list = remove->next;
			} else {
				prev_item->next = remove->next;
			}

			/* Remove GRFConfig information */
			ClearGRFConfigList(&remove->info.grfconfig);
			free(remove);
			remove = NULL;

			DEBUG(net, 4) ("[NET][GameList] Removed server from list");
			UpdateNetworkGameWindow(false);
			return;
		}
		prev_item = item;
	}
}

enum {
	MAX_GAME_LIST_REQUERY_COUNT =  5,
	REQUERY_EVERY_X_GAMELOOPS   = 60,
};

/** Requeries the (game) servers we have not gotten a reply from */
void NetworkGameListRequery(void)
{
	static uint8 requery_cnt = 0;
	struct in_addr ip;
	NetworkGameList *item;

	if (_stop_requerying || ++requery_cnt < REQUERY_EVERY_X_GAMELOOPS) return;

	requery_cnt = 0;
	_stop_requerying = true;

	for (item = _network_game_list; item != NULL; item = item->next) {
		uint8 retries;

		if (item->online || item->retries >= MAX_GAME_LIST_REQUERY_COUNT) continue;

		ip.s_addr = item->ip;

		/* item gets mostly zeroed by NetworkUDPQueryServer */
		retries = item->retries;
		NetworkUDPQueryServer(inet_ntoa(ip), item->port);
		item->retries = retries + 1;

		_stop_requerying = false;
	}

}

#endif /* ENABLE_NETWORK */
