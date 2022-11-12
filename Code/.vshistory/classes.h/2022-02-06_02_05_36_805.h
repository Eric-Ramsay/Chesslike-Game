#pragma once
#include "constants.h"
#include <deque> //used for faster river generation, and for pathfinding


enum BUILDING {
	HOUSE, FARM, STONE_FORT, PATROLS, LUMBER_MILL, FORAGERS, WATCH_TOWER, BAZAAR, ORCHARD, GRANARY, BARRACKS, OVENS, EMPTY
};

enum RES {
	POP, WOOD, STONE, FOOD, LEVY, OPINION, GOLD, POWER, TECH
};

enum FORMAT {
	LEFT, RIGHT, CENTER
};

enum UNIT {
	NOTHING, PEASANT, SPEARMAN, ARCHER, SWORDSMAN, CAVALRY_ARCHER, LIGHT_CAVALRY, HEAVY_CAVALRY, COMMANDER
};

enum TERRAIN {
	SHALLOW, OCEAN, RIVER, LAKE, GRASS, STEPPE, SAVANNA, MEADOW, DESERT, TUNDRA, ICE, BOG, COLD_DESERT
};
enum ELEVATION {
	WATER, FLAT, HILL, MOUNTAIN
};

enum FOREST {
	NONE, TEMPERATE, JUNGLE, TAIGA, GLADE
};

enum Stats {
	DMG, SKIRMISH, ARMOR, HEALTH
};

struct C {
	int x;
	int y;
	C(int x1, int y1) {
		x = x1;
		y = y1;
	}
};

struct Spot {
	int x;
	int y;
	short parent = -1; //Used only for pathfinding.
	float g = -1; //Distance to start. pathfinding.
	float h = -1; //Distance to end.   pathfinding.

	Spot(int x1 = 0, int y1 = 0) {
		x = x1;
		y = y1;
	}

	Spot(C coord) {
		x = coord.x;
		y = coord.y;
	}
	bool operator==(const Spot rhs) {
		return this->x == rhs.x && this->y == rhs.y;
	}
	bool operator!=(const Spot rhs) {
		return this->x != rhs.x || this->y != rhs.y;
	}
};



struct Box {
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	Box() {};
	Box(int x1, int y1, int width, int height) {
		x = x1;
		y = y1;
		w = width;
		h = height;
	}
};

struct UI_Unit {
	short type = 0;

	short sX = 0;
	short sY = 0;

	short MP = 1;
	short HP = 3;
	short ATK = 1;
	short DEF = 1;
	short range = 1;
	short cost = 1;

	UI_Unit(short m, short h, short a, short d, short r, short c) {
		sY = 0;

		MP = m;
		HP = h;
		ATK = a;
		DEF = d;
		range = r;
		cost = c;
	}
	
};
struct Unit {
	char owner = 0;
	char type = 0;

	char HP = 100;
	char MP = 0;
};



struct Tile {
	char owner = 0;

	TERRAIN type = GRASS;
	ELEVATION elev = FLAT;
	FOREST forest = NONE;

	char building = false;
	Unit unit;
};


bool canBuyBuilding(std::vector<std::vector<Tile>>& map, int x, int y, int index) {
	bool s = false;
	int cost = 1;
	if (index == 1) { //House
		cost = 3;
	}
	else if (index == 2) { //Windmill
		cost = 8;
	}
	else if (index == 3) { //Fort
		cost = 8;
	}
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			if ((map[safeC(y + i)][safeC(x + j)].unit.type == 1 || map[safeC(y + i)][safeC(x + j)].unit.type == 9) && map[safeC(y + i)][safeC(x + j)].unit.owner == p.turn) {
				s = true;
				i = 2; j = 2;
			}
		}
	}
	s = s && (map[y][x].building == 0 || (GetKey(olc::Key::SHIFT).bHeld && map[y][x].owner == p.turn));
	return s && map[y][x].elev >= FLAT && map[y][x].forest == NONE && map[y][x].elev != MOUNTAIN && p.gold >= cost;
}




struct MapTile {
	float heat = 50; // 0 to 100
	float rain = 150; //0 to 1000
	char type = 'g';
	float elevation = 25; //0 to 100
	char forest = 'e';
	float forestChance = 0;
	int id[3] = { -1, -1, -1 };
	char it = -1;
	int checked = -1;
	int ways[4] = { 0, 0, 0, 0 };
};

struct Player {
	int gold = 6;
	char turn = 0;

	std::vector<C> units = {};
	std::vector<C> buildings = {};

	int max_pop = 6;
	bool started = false; //Whether you get a free commander
};
	