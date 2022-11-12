#pragma once
#include "constants.h"
#include <deque> //used for faster river generation, and for pathfinding


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
	NONE, FISH, TEMPERATE, JUNGLE, TAIGA, GLADE
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

struct UI_Building {
	bool top_color = false;
	bool bot_color = false;

	short cost = 0;
	short type = 0;
	short max_type = 0;

	short sX = 0;
	short sY = 16;

	short anim_num = 1;
	short anim_speed = 1; //lower means faster

	UI_Building(short c, short t, bool tc = false, bool bc = false, short an = 1, short as = 1) {
		cost = c;
		type = t;
		anim_num = an;
		anim_speed = as;
		top_color = tc;
		bot_color = bc;
	}
};

enum BUILDINGS {
	NADA, FARM_ONE, FARM_TWO, FARM_THREE, WHEAT, HOUSE, BRIDGE, DOCK, WORKSHOP, WINDMILL, FORT
};

std::vector<UI_Building> initBuildings() {
	std::vector<UI_Building> buildings = {};

	buildings.push_back(UI_Building(1, 1)); //Farm
	buildings.push_back(UI_Building(3, 5, true)); //House
	buildings.push_back(UI_Building(4, 8)); //Bridge
	buildings.push_back(UI_Building(4, 11)); //Dock
	buildings.push_back(UI_Building(6, 9, false, true, 6, 8)); //Workshop
	buildings.push_back(UI_Building(8, 6, false, false, 6, 6)); //Windmill
	buildings.push_back(UI_Building(8, 7, false, true)); //Fort

	for (int i = 0; i < buildings.size(); i++) {
		buildings[i].sX = 16 * i;
		if (i == 0) {
			buildings[i].max_type = 4;
		}
		else {
			buildings[i].max_type = buildings[i].type;
		}
	}

	return buildings;
}


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
	bool naval = false;

	UI_Unit(short m, short h, short a, short d, short r, short c, bool n = false) {
		sY = 0;

		MP = m;
		HP = h;
		ATK = a;
		DEF = d;
		range = r;
		cost = c;
		naval = n;
	}
	
};

struct Entry {
	int turn = 1;
	float income = 1;
	int unit_value = 0;
};


std::vector<UI_Unit> initUnits() { //Initialize unit types
	std::vector<UI_Unit> units = {};

	//---------------------MP, HP,AT,DF,RG,COST
	units.push_back(UI_Unit(2, 4, 1, 1, 2, 1)); //1) Peasant Levy
	units.push_back(UI_Unit(2, 8, 3, 5, 1, 2)); //2) Spearmen
	units.push_back(UI_Unit(3, 4, 4, 4, 3, 4)); //3) Archers
	units.push_back(UI_Unit(2, 15, 7, 7, 1, 5)); //4) Heavy Infantry
	units.push_back(UI_Unit(5, 6, 4, 2, 3, 5)); //5) Cav Archer
	units.push_back(UI_Unit(6, 12, 5, 4, 1, 5)); //6) Light Cavalry
	units.push_back(UI_Unit(4, 20, 8, 4, 1, 8)); //7) Heavy Cavalry
	units.push_back(UI_Unit(4, 20, 8, 4, 1, 8)); //8) Commander
	units.push_back(UI_Unit(2, 12, 2, 2, 2, 8)); //9) Mayor
	units.push_back(UI_Unit(3, 6, 2, 2, 2, 2, true)); //10) fishing raft
	units.push_back(UI_Unit(4, 15, 3, 3, 3, 6, true)); //11) warship
	units.push_back(UI_Unit(4, 12, 8, 4, 1, 6, true)); //12) longboat

	for (int i = 0; i < units.size(); i++) {
		units[i].type = i + 1;
		units[i].sX = i * 16;
	}

	return units;
}

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

	int harvest = 0;

	char building = false;
	Unit unit;
};

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
	