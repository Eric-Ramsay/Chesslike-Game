#pragma once

enum FORMAT {
	LEFT, RIGHT, CENTER
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

struct Policy {
	bool active = false;
	int sX = 0;
	int sY = 0;
	int cost = 0;
	std::string name = "name";
	std::string description = "text";

	Policy(int c, std::string n, std::string desc) {
		cost = c;
		name = n;
		description = desc;
	}
};

enum POLICY_NAME {
	HOUSING, RAFTS, LEVY, PIKES, RANGERS, HARDY, LOGGING, HUSBANDRY, HILLFORTS, MEDICINE, BARRACKS, VOLUNTEERS, DONKEYS, FORTIFICATIONS
};
std::vector<Policy> initPolicies() {
	std::vector<Policy> policies = {};
	
	policies.push_back(Policy(2, "Efficient Housing", "Houses, Forts, and Piers provide *GREEN*2x*GREEN* population space."));
	policies.push_back(Policy(4, "Lightweight Rafts", "Fishing Ships are 1 gold cheaper and have +2 MP."));
	policies.push_back(Policy(3, "Forced Levy", "Slain peasants don't give enemies *GOLD*gold*GOLD*. Workers/Spears *GREEN*no upkeep*GREEN*"));
	policies.push_back(Policy(2, "Pikes", "Spearmen have *GREEN*+2*GREEN* HP and *GREEN*+1*GREEN* ATK."));
	policies.push_back(Policy(2, "Rangers", "No *GREEN*movement*GREEN* penalties from *GREEN*forests*GREEN* or *GREEN*elevation*GREEN*"));
	policies.push_back(Policy(1, "Hardy Folk", "Workers have *GREEN*8*GREEN* Max HP and *GREEN*2*GREEN* ATK"));
	policies.push_back(Policy(2, "Advanced Logging", "No *GOLD*gold cost*GOLD* for building over *GREEN*forests*GREEN*"));
	policies.push_back(Policy(1, "Equine Husbandry", "Cavalry units *GREEN*+1*GREEN* Movement."));
	policies.push_back(Policy(1, "Hillforts", "Units with an *GREEN*elevation advantage*GREEN* have *GREEN*+1*GREEN* defense."));
	policies.push_back(Policy(2, "Field Medicine", "Each turn, you can *GREEN*Heal*GREEN* one unit for *GREEN*5*GREEN* HP"));
	policies.push_back(Policy(2, "Barracks", "Units on forts and piers pay *GREEN*no Upkeep*GREEN*"));
	policies.push_back(Policy(2, "Volunteers", "You can *GREEN*recruit*GREEN* units on forts and piers"));
	policies.push_back(Policy(2, "Donkey Husbandry", "Merchants have *GREEN*+1*GREEN* MP and *GREEN*+2*GREEN* HP."));
	policies.push_back(Policy(2, "Fortifications", "All buildings have *GREEN*+3*GREEN* HP."));

	for (int i = 0; i < policies.size(); i++) {
		policies[i].sX = 16 * (i % 12);
		policies[i].sY = 144 + (16 * (i/12));
	}

	return policies;
}

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

	short HP = 0;
	short loot = 0;

	short cost = 0;
	short type = 0;
	short max_type = 0;

	short sX = 0;
	short sY = 16;

	short anim_num = 1;
	short anim_speed = 1; //lower means faster

	UI_Building(short c, short t, short h, bool tc = false, bool bc = false, short an = 1, short as = 1) {
		cost = c;
		type = t;
		HP = h;
		anim_num = an;
		anim_speed = as;
		top_color = tc;
		bot_color = bc;
	}
};

enum BUILDINGS {
	FARM, STAKES, HOUSE, BRIDGE, CANAL, PIER, DEPOT, WORKSHOP, WINDMILL, FORT, OFFICE
};

std::vector<UI_Building> initBuildings() {
	std::vector<UI_Building> buildings = {};
	buildings.push_back(UI_Building(1, 1, 5)); //Farm 0
	buildings.push_back(UI_Building(2, 13, 10, false, true)); //Stakes 1
	buildings.push_back(UI_Building(3, 5, 10, true)); //House 2
	buildings.push_back(UI_Building(3, 12, 10)); //Bridge 3
	buildings.push_back(UI_Building(3, 8, 10)); //Canal 4
	buildings.push_back(UI_Building(3, 11, 8, true)); //Pier 5
	buildings.push_back(UI_Building(5, 14, 15, false, true)); //Trade Depot 6
	buildings.push_back(UI_Building(6, 9, 15, false, true, 6, 8)); //Workshop 7
	buildings.push_back(UI_Building(8, 6, 15, false, true, 6, 6)); //Windmill 8
	buildings.push_back(UI_Building(8, 7, 30, false, true)); //Fort 9
	buildings.push_back(UI_Building(8, 10, 15, false, true)); //Office 10

	for (int i = 0; i < buildings.size(); i++) {
		buildings[i].sX = 16 * i;
		buildings[i].loot = 1 + buildings[i].cost / 2.5;
		if (i == FARM) { //multi type buidlings need to have their multiple types coded here
			buildings[i].max_type = 4;
		}
		else {
			buildings[i].max_type = buildings[i].type;
		}
	}

	return buildings;
}


struct UI_Unit {
	std::string name = "";
	short type = 0;

	short sX = 0;
	short sY = 0;

	bool can_buy = true;
	bool active = false;

	short MP = 1;
	short HP = 3;
	short ATK = 1;
	short DEF = 1;
	short range = 1;
	short cost = 1;
	bool naval = false;

	UI_Unit(std::string nam, short m, short h, short a, short d, short r, short c, bool n = false, bool cb = true) {
		sY = 0;
		name = nam;
		MP = m;
		HP = h;
		ATK = a;
		DEF = d;
		range = r;
		cost = c;
		naval = n;
		can_buy = cb;
	}
	
};

struct Entry {
	int turn = 1;
	float income = 1;
	int unit_value = 0;
};

enum UNITS {
	NO_UNIT, PEASANT, SPEARMAN, AXEMAN, HEAVY_INFANTRY, ARCHER, SKIRMISHER, LIGHT_CAVALRY, HEAVY_CAVALRY, CAVALRY_ARCHER,
	COMMANDER, MAYOR, FISHING_RAFT, WARSHIP, TRIREME, MAGE, MERCHANT, MANGONEL, PIKEMAN
};

std::vector<UI_Unit> initUnits() { //Initialize unit types
	std::vector<UI_Unit> units = {};

	//---------------------MP, HP,AT,DF,RG,COST
	units.push_back(UI_Unit("Peasant", 2, 4, 1, 1, 1, 1)); //1) Peasant Levy
	units[0].active = true;
	units.push_back(UI_Unit("Spearman", 2, 8, 3, 5, 1, 2)); //2) Spearmen
	units.push_back(UI_Unit("Axeman", 3, 8, 5, 3, 1, 3)); //3) axeman
	units.push_back(UI_Unit("Heavy Infantry", 2, 15, 8, 8, 1, 5)); //4) Heavy Infantry
	units.push_back(UI_Unit("Archer", 3, 4, 4, 4, 3, 4)); //5) Archers
	units.push_back(UI_Unit("Skirmisher", 4, 6, 6, 2, 2, 3)); //6) skirmisher
	units.push_back(UI_Unit("Light Cavalry", 6, 12, 5, 4, 1, 5)); //7) Light Cavalry
	units.push_back(UI_Unit("Heavy Cavalry", 4, 20, 8, 4, 1, 8)); //8) Heavy Cavalry
	units.push_back(UI_Unit("Cavalry Archer", 5, 6, 4, 2, 3, 5)); //9) Cav Archer

	units.push_back(UI_Unit("Commander", 4, 20, 8, 4, 1, 8, false, false)); //10) Commander
	units.push_back(UI_Unit("Mayor", 2, 12, 2, 2, 2, 8, false, false)); //11) Mayor

	units.push_back(UI_Unit("Fishing Raft", 3, 6, 2, 2, 2, 2, true)); //12) fishing raft
	units.push_back(UI_Unit("Warship", 4, 12, 3, 3, 3, 6, true)); //13) warship
	units.push_back(UI_Unit("Trireme", 5, 12, 8, 4, 1, 6, true)); //14) trireme
	
	units.push_back(UI_Unit("Mage", 2, 8, 4, 4, 4, 8)); //14) mage

	units.push_back(UI_Unit("Merchant", 2, 12, 3, 3, 2, 4)); //14) merchant
	units.push_back(UI_Unit("Mangonel", 2, 20, 8, 0, 4, 6)); //14) mangonel
	units.push_back(UI_Unit("Pikeman", 2, 10, 4, 5, 1, 2, false, false)); //15) pikemen

	for (int i = 0; i < units.size(); i++) {
		units[i].type = i + 1;
		units[i].sX = 16 + i * 16;
	}

	return units;
}

struct Unit {
	char owner = 0;
	char type = 0;

	char HP = 100;
	char MP = 0;
	char gold = 0;
};



struct Tile {
	char owner = 0;

	TERRAIN type = GRASS;
	ELEVATION elev = FLAT;
	FOREST forest = NONE;

	int harvest = 0;

	char building = 0;
	char HP = 0;
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
	float upkeep = 0;
	float debt = 0;

	std::vector<C> units = {};
	std::vector<C> buildings = {};

	int max_pop = 6;
	int max_buildings = 12;
	bool started = false; //Whether you get a free commander

	std::vector<bool> policies = {};
	Player() {
		for (int i = 0; i < 
	}
};

struct UI_Player {
	int kills = 0;
	int units = 0;
	int gold_earned = 0;
	int perk_key = 0;
};

	