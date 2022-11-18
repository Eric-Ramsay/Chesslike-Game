#define OLC_PGE_APPLICATION
#define NOMINMAX
#define MINIAUDIO_IMPLEMENTATION

//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include <algorithm>
namespace Gdiplus
{
	using std::min;
	using std::max;
};

#include "server.h"
#include "constants.h"
#include "classes.h"
#include "functions.h"

#include <string>
#include <iostream>
#include <fstream>

#include "miniaudio.h"


//SERVER SETTINGS -------------------------------------
bool connected = true;
std::string IP = "";
int port = 1234;
SOCKET sock;
//-----------------------------------------------------

//SOUND VARIABLES---------------------------------------
//SoLoud::Soloud audio; // SoLoud engine
ma_engine audio;
//------------------------------------------------------

class Terrain : public olc::PixelGameEngine
{
public:
	Terrain()
	{
		sAppName = "Terra Engine";
	}
private:
	std::vector<olc::Pixel> colors = { WHITE, olc::Pixel(166, 43, 58), olc::Pixel(16, 138, 231), olc::Pixel(39, 213, 216), olc::Pixel(76, 182, 92), olc::Pixel(231, 228, 0),  olc::Pixel(255, 140, 0) , olc::Pixel(114, 71, 255) , olc::Pixel(220, 0, 213) };
	std::vector<std::vector<Tile>> map;

	//SERVER RELATED STUFF / Server variables
	std::deque<std::string> buffer = {}; //a buffer of msgs to send to the server
	int turn = 0; //variable telling whose turn it is
	int harvest = 1; // used to prevent fish from being harvested more than  once per turn

	// SPRITES AND DECALS ---------------------
	olc::Sprite* terrain_s;
	olc::Sprite* flat_s;
	olc::Sprite* armySprites_s;
	olc::Sprite* font_s;
	olc::Sprite* icons_s;
	olc::Sprite* buildings_s;

	olc::Decal* terrain;
	olc::Decal* flat;
	olc::Decal* armySprites;
	olc::Decal* font;
	olc::Decal* icons;
	olc::Decal* building_sprites;

	 
	//Audio Management -------------------------------------
	enum SFX {
		BOW_DRAW, BOW_FIRE, KILL, ATTACK, BUILD, UNIT_PLACE, CHIMES, FISH_SPLASH, MARCH, FOREST_BUILD, SHIP_MOVE, FIREBALL, CLINK, BUILDING_HIT, BUILDING_DESTROY
	};
	std::vector<const char *> music = { "./Sounds/song1.mp3", "./Sounds/song2.mp3" };
	std::vector<const char *> sfx = {
		"./Sounds/bow_draw.mp3",
		"./Sounds/bow_fire.mp3",
		"./Sounds/kill.mp3",
		"./Sounds/attack.mp3",
		"./Sounds/build.mp3",
		"./Sounds/unit_place.mp3",
		"./Sounds/chimes.mp3",
		"./Sounds/fish_splash.mp3",
		"./Sounds/march.mp3",
		"./Sounds/forest_build.mp3",
		"./Sounds/ship_move.mp3",
		"./Sounds/fireball.mp3",
		"./Sounds/clink.mp3",
		"./Sounds/building_hit.mp3",
		"./Sounds/building_destroy.mp3"
	};
	bool audio_init = false;
	int music_index = 0;
	ma_sound sound;

	//Audio Manager
	void handleAudio() {
		static bool started = false;
		if (!audio_init) {
			audio_init = true;
			ma_engine_init(NULL, &audio);
		}
		else if (drawMap) {
			if (!started || sound.atEnd) { //handle music, most of this is useless error checking, 
				started = true;
				ma_sound_uninit(&sound);
				ma_result res = ma_sound_init_from_file(&audio, music[music_index], MA_SOUND_FLAG_ASYNC, NULL, NULL, &sound);
				if (res == MA_SUCCESS) {
					ma_sound_set_volume(&sound, (float)playMusic);
					res = ma_sound_start(&sound);
					music_index = (1 + music_index) % music.size();
				}
			}
		}
	}
	void playSound(SFX s) {
		ma_engine_play_sound(&audio, sfx[s], NULL);
	}
	//End of Audio Management ------------------------------
	
	//PLAYER
	Player p;
	std::vector<Player> players = {};

	//Map Making Variables
	bool MM_active = false;
	int MM_selected = 0;
	std::deque<Tile> MM_Tile = {Tile(), Tile(), Tile()};

	//Game Loop Variables
	float fTargetFrameTime = 1.0f / 60.0; // Virtual FPS of 60fps
	float fAccumulatedTime = 0.0f;
	int timer = 0;

	//Used for Dragging things when buying units/buildings, or moving units on the map
	C tH = C(0, 0);
	C tS = C(0, 0);
	C drag = C(0, 0);
	C crown = C(0, 0);
	C heal = C(0, 0);
	bool can_heal = false;
	bool dragging_heal = false;
	bool dragging_crown = false;
	bool dragging_unit = false;
	bool buying_unit = false;
	bool buying_building = false;

	//List of units, buildings, and policies
	std::vector<UI_Unit> units;
	std::vector<UI_Building> buildings;
	std::vector<Policy> policies;
	std::vector<int> UI_Units = {};

	//General UI variables
	bool unit_UI = 0;
	bool building_UI = false;
	int scrolling = -1;
	int unit_selected = 0;
	int building_selected = 0;
	bool attacking_unit = false;
	int yView = MAPSIZE / 2;
	int xView = MAPSIZE / 2;
	int dYView = 0;
	int dXView = 0;
	bool verify_map = false;
	bool drawMap = false;
	int makeCiv = 0;
	int zoom = 32;
	bool playMusic = true;

	//UI bounding boxes, so that player doesnt accidentally click behind UI 
	Box unit_bounds = Box(2, 4 + 15 * 3, 79 * 3, 129 * 3);
	Box building_bounds = Box(2, 6 + 144 * 3, 105 * 3, 34 * 3);
	Box unit_sel_bounds = Box(ScreenWidth() - (2 + 31*3), 2, 15*3, 15*3);
	Box building_sel_bounds = Box(ScreenWidth() - (2 + 15 * 3), 2, 15*3, 15*3);
	Box gold_bounds = Box(2, 2, 43 * 3, 15 * 3);
	Box pop_bounds = Box(2 + 44 * 3, 2, 43 * 3, 15 * 3);
	Box commander_bounds = Box(ScreenWidth() - (2 + 17 * 3), (2 + 17 * 3), 17 * 3, 24 * 3);
	Box end_turn = Box(ScreenWidth() - (2 + 16 * 2), ScreenHeight() - (2 + 16 * 2), 32, 32);


public:

	bool OnUserCreate() override { //Runs once when game starts	
		srand(time(NULL));

		//Initialize Sprites and Decals
		flat_s = new olc::Sprite("./Sprites/flat.png"); flat = new olc::Decal(flat_s);
		terrain_s = new olc::Sprite("./Sprites/terrain types.png");  terrain = new olc::Decal(terrain_s);
		icons_s = new olc::Sprite("./Sprites/icons.png"); icons = new olc::Decal(icons_s);
		buildings_s = new olc::Sprite("./Sprites/buildings.png"); building_sprites = new olc::Decal(buildings_s);
		font_s = new olc::Sprite("./Sprites/font.png");	font = new olc::Decal(font_s);
		armySprites_s = new olc::Sprite("./Sprites/units.png"); armySprites = new olc::Decal(armySprites_s);

		//Initialize units and buildings
		units = initUnits();
		buildings = initBuildings();
		policies = initPolicies();
		
		building_bounds.w = (2 + 17 * buildings.size()) * 3.0;
		unit_sel_bounds = Box(ScreenWidth() - (2 + 31 * 3), 2, 15 * 3, 15 * 3);
		building_sel_bounds = Box(ScreenWidth() - (2 + 15 * 3), 2, 15 * 3, 15 * 3);
		commander_bounds = Box(ScreenWidth() - (2 + 17 * 3), (2 + 17 * 3), 17 * 3, 24 * 3);
		end_turn = Box(ScreenWidth() - (2 + 16 * 2), ScreenHeight() - (2 + 16 * 2), 32, 32);

		Clear(olc::BLANK);
		serverInit();

		for (int i = 0; i < 9; i++) {
			players.push_back(Player());
		}

		return true;
	}
	bool OnUserUpdate(float fElapsedTime) override {
		//Main Game Loop Function, Checks I/O, runs Draw Functions
		fAccumulatedTime += fElapsedTime;
		static bool started = false;
		if (!started) {
			started = true;
			fAccumulatedTime = 0.0;
		}
		if (fAccumulatedTime >= fTargetFrameTime) {
			fAccumulatedTime -= fTargetFrameTime;
			fElapsedTime = fTargetFrameTime;
			if (timer == 0) {
				if (!drawMap) {
					sendData("m");
				}
			}
			timer++;
			if (timer == 500) {
				timer = 0;
			}
			if (!GetKey(olc::Key::SHIFT).bHeld) {
				if (GetKey(olc::Key::LEFT).bHeld || GetKey(olc::Key::A).bHeld) { //Pan Left
					dXView -= zoom/8;
					if (zoom < 16) {
						xView = safeC(xView - 1);
					}
				}
				else if (GetKey(olc::Key::RIGHT).bHeld || GetKey(olc::Key::D).bHeld) { //Pan Right
					dXView += zoom / 8;
					if (zoom < 16) {
						xView = safeC(xView + 1);
					}
				}
				if (GetKey(olc::Key::UP).bHeld || GetKey(olc::Key::W).bHeld) { //Pan Up
					dYView -= zoom / 8;
					if (zoom < 16) {
						yView = safeC(yView - 1);
					}
				}
				else if (GetKey(olc::Key::DOWN).bHeld || GetKey(olc::Key::S).bHeld) { //Pan Down
					dYView += zoom / 8;
					if (zoom < 16) {
						yView = safeC(yView + 1);
					}
				}
				if (GetKey(olc::Key::X).bHeld) {
					if (zoom >= ((ScreenHeight() / MAPSIZE)) && zoom >= 2) {
						zoom--;
					}
				}
				else if (GetKey(olc::Key::Z).bHeld) {
					if (zoom <= 63) {
						zoom++;
					}
				}
			}
			if (GetMouseWheel() < 0) {
				if (zoom >= ((ScreenHeight() / MAPSIZE)) && zoom >= 2) {
					zoom--;
				}
			}
			else if (GetMouseWheel() > 0) {
				if (zoom <= 63) {
					zoom++;
				}
			}
			handleAudio();
		}
		
		//ui stuff
		
		//Below are the Keyboard Press detections and what not
		//Map Maker Controls---------------------------------------------------------------------------------------------------------------------------------Map Maker Controls Start---
		if (GetKey(olc::Key::CTRL).bHeld && GetKey(olc::Key::SHIFT).bHeld && GetKey(olc::Key::M).bReleased) {
			//MM_active = !MM_active;
		}
		if (GetKey(olc::Key::M).bPressed) {
			playMusic = !playMusic;
			ma_sound_set_volume(&sound, (float)playMusic);
		}
		if (MM_active) {
			if (GetMouse(1).bPressed) {
				MM_selected = (MM_selected + 1) % MM_Tile.size();
			}
			if (GetKey(olc::Key::ENTER).bReleased) { //add new tile thing
				if (MM_Tile.size() < 6) {
					MM_Tile.push_back(Tile());
				}
			}
			if (GetKey(olc::Key::DEL).bReleased) {
				if (MM_Tile.size() > 1) {
					MM_Tile.pop_back();
				}
				if (MM_selected >= MM_Tile.size()) {
					MM_selected = MM_Tile.size() - 1;
				}
			}
			if (GetKey(olc::Key::SHIFT).bHeld) {
				if (MM_active && GetKey(olc::Key::DOWN).bReleased) { //Increment Forest
					if (MM_Tile[MM_selected].forest < GLADE) {
						MM_Tile[MM_selected].forest = (FOREST)((int)MM_Tile[MM_selected].forest + 1);
					}
				}
				if (GetKey(olc::Key::LEFT).bReleased && MM_Tile[MM_selected].type >= GRASS) { //Decrement Elevation
					if (MM_Tile[MM_selected].elev > FLAT) {
						MM_Tile[MM_selected].elev = (ELEVATION)((int)MM_Tile[MM_selected].elev - 1);
					}
				}
				if (GetKey(olc::Key::RIGHT).bReleased && MM_Tile[MM_selected].type >= GRASS) { //Increment Elevation
					if (MM_Tile[MM_selected].elev < MOUNTAIN) {
						MM_Tile[MM_selected].elev = (ELEVATION)((int)MM_Tile[MM_selected].elev + 1);
					}
				}
				if (GetKey(olc::Key::UP).bReleased) { //Decrement Forest
					if (MM_Tile[MM_selected].forest > NONE) {
						MM_Tile[MM_selected].forest = (FOREST)((int)MM_Tile[MM_selected].forest - 1);
					}
				}
				if (GetMouseWheel() > 0) {
					if (MM_Tile[MM_selected].type > SHALLOW) {
						MM_Tile[MM_selected].type = (TERRAIN)((int)MM_Tile[MM_selected].type - 1);
						if (MM_Tile[MM_selected].type < GRASS) {
							MM_Tile[MM_selected].elev = WATER;
						}
					}
				}
				else if (GetMouseWheel() < 0) {
					if (MM_Tile[MM_selected].type < COLD_DESERT) {
						MM_Tile[MM_selected].type = (TERRAIN)((int)MM_Tile[MM_selected].type + 1);
					}
					if (MM_Tile[MM_selected].type < GRASS) {
						MM_Tile[MM_selected].elev = WATER;
					}
				}
				if (GetKey(olc::Key::CTRL).bHeld) {
					if (GetKey(olc::Key::C).bReleased) {
						map = clearMap(MM_Tile[MM_selected]);
						for (int i = 0; i < map.size(); i++) {
							for (int j = 0; j < map[i].size(); j++) {
								sendData(sendTile(j, i));
							}
						}
					}
					if (GetKey(olc::Key::L).bReleased) {
						loadMap(map, "save.txt");
					}
				}
			}
		}
		if (GetKey(olc::Key::CTRL).bHeld && GetKey(olc::Key::S).bReleased) {
			saveMap(map);
		}
		//Map Maker Controls End-----------------------------------------------------------------------------------------------------------------------------Map Maker Controls End-----
		if (!MM_active && drawMap && makeCiv == -1) {
			if (GetKey(olc::Key::B).bPressed) {
				building_UI = !building_UI;
			}
			if (GetKey(olc::Key::U).bPressed) {
				unit_UI = !unit_UI;
			}
			if (GetKey(olc::Key::E).bPressed) {
				if (GetKey(olc::Key::SHIFT).bHeld) {
					sendData("E00");
				}
				else {
					sendData("G" + to_str(p.gold));
					endTurn();
				}
			}
			if (GetKey(olc::Key::N).bPressed) {
				//sendData("n00");
				//map = {};
				//map.resize(MAPSIZE);
				//drawMap = false;
			}
			if (GetKey(olc::Key::O).bPressed) {
				//run_turn();
				//endTurn();
			}
		}
		//Movement and zooming
		if (zoom < 16) {
			dXView = 0;
			dYView = 0;
		}
		//subsprite zoom stuff
		if (dYView >= zoom || zoom < 16)
		{
			dYView = safeC(dYView, zoom);
			yView = safeC(yView + 1);
		}
		if (dYView < 0 || zoom < 16)
		{
			dYView = zoom + dYView;
			yView = safeC(yView - 1);
		}
		if (dXView >= zoom || zoom < 16)
		{
			dXView = safeC(dXView, zoom);
			xView = safeC(xView + 1);
		}
		if (dXView < 0 || zoom < 16)
		{
			dXView = zoom + dXView;
			xView = safeC(xView - 1);
		}

		//Check if a UI Element was clicked on
		int x = GetMouseX();
		int y = GetMouseY();
		bool UI_Click = (makeCiv > -1);

		if (drawMap && makeCiv == -1) {
			if (inBox(x, y, building_sel_bounds)) {
				UI_Click = true;
				if (GetMouse(0).bPressed) {
					building_UI = !building_UI;
				}
			}
			if (inBox(x, y, unit_sel_bounds)) {
				UI_Click = true;
				if (GetMouse(0).bPressed) {
					unit_UI = !unit_UI;
				}
			}
			if (unit_UI > 0) {
				if (inBox(x, y, unit_bounds)) {
					UI_Click = true;
					if (GetMouse(0).bPressed) {
						unit_selected = min(UI_Units.size() - 1, (y - unit_bounds.y) / (unit_bounds.h / ((UI_Units.size()))));
						unit_selected = UI_Units[unit_selected];
						buying_unit = true;
					}
				}
			}
			if (building_UI) {
				if (inBox(x, y, building_bounds)) {
					UI_Click = true;
					if (GetMouse(0).bPressed) {
						building_selected = min(buildings.size() - 1, (x - building_bounds.x) / (building_bounds.w / buildings.size()));
						buying_building = true;
					}
				}
			}
			if (inBox(x, y, commander_bounds)) {
				UI_Click = true;
				if (GetMouse(0).bPressed) {
					if (inBox(x, y, Box(crown.x, crown.y, 11 * 3, 10 * 3))) {
						dragging_crown = true;
					}
				}
			}
			if (inBox(x, y, end_turn)) {
				UI_Click = true;
				if (GetMouse(0).bPressed) {
					sendData("G" + to_str(p.gold));
					endTurn();
				}
			}
			if (inBox(x, y, Box(heal.x, heal.y, 16 * 3, 16 * 3)) && can_heal) {
				if (GetMouse(0).bPressed) {
					UI_Click = true;
					dragging_heal = true;
				}
			}

			UI_Click = UI_Click || inBox(x, y, gold_bounds);
			UI_Click = UI_Click || inBox(x, y, pop_bounds);

			if (!UI_Click) {
				tH.x = safeC(xView + floor((GetMouseX() + dXView - ScreenWidth() / 2) / (float)zoom));
				tH.y = safeC(yView + floor((GetMouseY() + dYView - ScreenHeight() / 2) / (float)zoom));

				if (dragging_unit) { //Move a Unit
					if (buying_unit || buying_building) {
						dragging_unit = false;
					}
					else if (GetMouse(0).bReleased) {
						if (turn == p.turn && map[drag.y][drag.x].unit.owner == p.turn) {
							//Check if unit can be moved there
							float cost = moveCost(Spot(drag.x, drag.y), Spot(tH.x, tH.y), getMaxMP(map[drag.y][drag.x].unit.type, map[drag.y][drag.x].unit.owner), map[drag.y][drag.x].unit);
							if (map[tH.y][tH.x].unit.type == NO_UNIT && cost <= map[drag.y][drag.x].unit.MP) {
								//Move Unit to new Tile
								map[tH.y][tH.x].unit = map[drag.y][drag.x].unit;
								map[tH.y][tH.x].unit.MP -= cost;
								map[drag.y][drag.x].unit = Unit();
								sendData("u" + sendUnit(drag.x, drag.y) + sendUnit(tH.x, tH.y));
								if (units[map[tH.y][tH.x].unit.type-1].naval) {
									playSound(SHIP_MOVE);
								}
								else {
									playSound(MARCH);
								}
								for (int i = 0; i < p.units.size(); i++) {
									if (p.units[i].y == drag.y) {
										if (p.units[i].x == drag.x) {
											p.units[i] = tH;
										}
									}
								}
								tS = tH;
							}
						}
						dragging_unit = false;
					}
				}
				if (buying_building) { //Buy a Building
					if (buying_unit || dragging_unit) {
						buying_building = false;
					}
					else if (!GetMouse(0).bHeld) { 
						//This code handles building placement
						if (turn == p.turn && canBuyBuilding(tH.x, tH.y, building_selected)) {
							//The canBuyBuilding function confirms we're able to place the building here.
							map[tH.y][tH.x].building = buildings[building_selected].type;
							if (map[tH.y][tH.x].building == 8) { //Canals arent actually buildings
								map[tH.y][tH.x].building = 0;
								map[tH.y][tH.x].type = RIVER;
								map[tH.y][tH.x].elev = WATER;
								sendData(sendTile(tH.x, tH.y));
							}
							else if (map[tH.y][tH.x].building == 12) { //Neither are "bridges"
								if (map[tH.y][tH.x].elev < FLAT) {
									map[tH.y][tH.x].elev = FLAT;
								}
								else {
									map[tH.y][tH.x].elev = HILL;
								}
								map[tH.y][tH.x].building = 0;
								map[tH.y][tH.x].type = GRASS;
								sendData(sendTile(tH.x, tH.y));
							}
							else { //Store the building in the player's list of buildings
								p.buildings.push_back(tH);
								map[tH.y][tH.x].owner = p.turn;
								map[tH.y][tH.x].HP = buildings[building_selected].HP; //Give the building its HP
							}
							p.gold -= buildings[building_selected].cost; //Pay the building price
							map[tH.y][tH.x].forest = NONE;
							if (map[tH.y][tH.x].forest > FISH) { //build over a forest
								if (!p.policies[LOGGING]) {
									p.gold -= 2;
								}
								playSound(FOREST_BUILD);
							}
							else {
								playSound(BUILD);
							}
							sendData("b" + sendBuilding(tH.x, tH.y));
						}
						buying_building = false;
					}
				}
				if (buying_unit) { //Buy a Unit
					if (buying_building || dragging_unit) {
						buying_unit = false;
					}
					else if (!GetMouse(0).bHeld) {
						if (turn == p.turn && canBuy(tH.x, tH.y, unit_selected)) {
							int placeholder = unit_selected;
							if (p.started) {
								p.gold -= units[unit_selected].cost;
							}
							else {
								unit_selected = MAYOR-1;
								p.started = true;
								if (map[tH.y][tH.x].elev < FLAT) {
									map[tH.y][tH.x].elev = FLAT;
									map[tH.y][tH.x].type = GRASS;
									sendData(sendTile(tH.x, tH.y));
								}
							}
							p.units.push_back(tH);
							map[tH.y][tH.x].owner = p.turn;
							map[tH.y][tH.x].unit.type = unit_selected + 1;
							map[tH.y][tH.x].unit.owner = p.turn;
							map[tH.y][tH.x].unit.HP = getMaxHP(unit_selected+1, p.turn);
							map[tH.y][tH.x].unit.MP = 0;
							unit_selected = placeholder;
							sendData("u" + sendUnit(tH.x, tH.y));
							if (map[tH.y][tH.x].building > 0) {
								sendData("b" + sendBuilding(tH.x, tH.y));
							}
							playSound(UNIT_PLACE);
						}
						buying_unit = false;
					}
				}
				if (attacking_unit) {
					if (!GetMouse(1).bHeld) {
						int x_dist = coord_dist(tH.x, tS.x);
						int y_dist = coord_dist(tH.y, tS.y);
						SFX sound_effect = ATTACK;
						//Preliminary checks to being able to perform a right click function with a unit - 
						//It must be your turn, and you must have selected a unit which still has movement left
						//And your target must be in that unit's range
						if (turn == p.turn && map[tS.y][tS.x].unit.type > 0 && map[tS.y][tS.x].unit.MP > 0 && map[tS.y][tS.x].unit.owner == p.turn) {
							if (x_dist <= units[map[tS.y][tS.x].unit.type - 1].range && y_dist <= units[map[tS.y][tS.x].unit.type - 1].range) {
								//Attack an enemy unit
								//Mangonels can't attack units
								if (map[tS.y][tS.x].unit.type != MANGONEL && map[tH.y][tH.x].unit.type > 0 && map[tH.y][tH.x].unit.owner != p.turn) {
									map[tH.y][tH.x].unit.HP -= getATK(map[tS.y][tS.x].unit.type, map[tS.y][tS.x].unit.owner);
									//If the enemy is in range as well, then it will defend itself
									if (x_dist <= units[map[tH.y][tH.x].unit.type - 1].range && y_dist <= units[map[tH.y][tH.x].unit.type - 1].range) {
										if (map[tS.y][tS.x].elev < map[tH.y][tH.x].elev) { //elevation advantage
											if (map[tS.y][tS.x].elev >= FLAT) {
												map[tS.y][tS.x].unit.HP -= 1;
												if (players[map[tH.y][tH.x].owner].policies[HILLFORTS]) {
													map[tS.y][tS.x].unit.HP -= 1;
												}
											}
										}
										//Deal with fort defensive advantage for both our unit and the enemy
										if (bIndex(map[tH.y][tH.x].building) == FORT) {
											map[tS.y][tS.x].unit.HP -= 2;
											map[tH.y][tH.x].unit.HP += 1;
										}
										if (bIndex(map[tH.y][tH.x].building) == FORT) {
											map[tS.y][tS.x].unit.HP += 1;
										}
										map[tS.y][tS.x].unit.HP -= units[map[tH.y][tH.x].unit.type - 1].DEF;
									}
									//Attacking a unit takes its remaining movement  points
									map[tS.y][tS.x].unit.MP = 0;
									//Check if our unit dies in combat
									if (map[tS.y][tS.x].unit.HP <= 0) {
										map[tS.y][tS.x].unit = Unit();
										std::vector<C> units = {};
										for (int i = 0; i < p.units.size(); i++) { //Delete the unit from your unit list
											bool found = false;
											if (p.units[i].y == tS.y) {
												if (p.units[i].x == tS.x) {
													found = true;
												}
											}
											if (!found) {
												units.push_back(p.units[i]);
											}
										}
										p.units = units;
									}
									//Choose what sound effect to play, based on unit range
									if (units[map[tS.y][tS.x].unit.type - 1].range > 1) {
										if (map[tS.y][tS.x].unit.type == MAGE) {
											sound_effect = FIREBALL;
										}
										else {
											sound_effect = BOW_FIRE;
										}
									}
									//Check if enemy unit dies in combat
									if (map[tH.y][tH.x].unit.HP <= 0) {
										//If the slain enemy is a peasant or fishing ship, earn a gold (except if they have forced levy)
										if (map[tH.y][tH.x].unit.type == PEASANT || map[tH.y][tH.x].unit.type == FISHING_RAFT) {
											if (!players[map[tH.y][tH.x].owner].policies[LEVY]) {
												p.gold++;
											}
										}
										map[tH.y][tH.x].unit = Unit();
										if (sound_effect == ATTACK) {
											sound_effect = KILL;
										}
									}
									//Play the sound effect
									playSound(sound_effect);
									//Update both units to the server
									sendData("u" + sendUnit(tH.x, tH.y) + sendUnit(tS.x, tS.y));
								}
								//Fish with the peasant, mayor, or fishing raft
								else if (map[tS.y][tS.x].unit.type == PEASANT || map[tS.y][tS.x].unit.type == MAYOR || map[tS.y][tS.x].unit.type == FISHING_RAFT) {
									if (map[tH.y][tH.x].forest == FISH && map[tH.y][tH.x].harvest < harvest) {
										//check if you're in range

										if (map[tS.y][tS.x].unit.MP > 0 && x_dist <= units[map[tS.y][tS.x].unit.type - 1].range && y_dist <= units[map[tS.y][tS.x].unit.type - 1].range) {
											if (map[tS.y][tS.x].unit.owner == p.turn) {
												p.gold++;
												map[tH.y][tH.x].harvest = harvest + 1;
												map[tS.y][tS.x].unit.MP = 0;
												sendData("u" + sendUnit(tH.x, tH.y) + sendUnit(tS.x, tS.y));
												playSound(FISH_SPLASH);
											}
										}
									}
								}
								//Damage a building with your unit, ie attack a building
								else if (map[tH.y][tH.x].building > 0 && map[tH.y][tH.x].owner != p.turn) { //raid building
									int x_dist = coord_dist(tH.x, tS.x);
									int y_dist = coord_dist(tH.y, tS.y);

									if (map[tS.y][tS.x].unit.MP > 0 && x_dist <= units[map[tS.y][tS.x].unit.type - 1].range && y_dist <= units[map[tS.y][tS.x].unit.type - 1].range) {
										bool proceed = true;
										for (int i = -1; i < 2; i++) {
											for (int j = -1; j < 2; j++) {
												if (i != 0 || j != 0) { //check for adjacent enemy forts
													if (map[safeC(tH.y + i)][safeC(tH.x + j)].owner != p.turn && bIndex(map[safeC(tH.y + i)][safeC(tH.x + j)].building) == FORT) {
														proceed = false;
														i = 2; j = 2;
													}
												}
											}
										}
										if (bIndex(map[tH.y][tH.x].building) == FORT || proceed) {
											//Damage the building
											int dmg = getATK(map[tS.y][tS.x].unit.type, p.turn);
											//ranged units do less dmg to buildings
											if (units[map[tS.y][tS.x].unit.type - 1].range > 1) {
												dmg = 2;
											}
											map[tH.y][tH.x].HP -= dmg;
											map[tS.y][tS.x].unit.MP = 0;
											sound_effect = BUILDING_HIT;
											//The building is destroyed, the unit should earn gold
											if (map[tH.y][tH.x].HP <= 0) {
												sound_effect = BUILDING_DESTROY;
												map[tH.y][tH.x].building = 0;
												//Units only get gold if they're next to the building
												if (x_dist <= 1 && y_dist <= 1) {
													map[tS.y][tS.x].unit.gold += buildings[bIndex(map[tH.y][tH.x].building)].loot;
													//Units can only carry 9 gold max
													if (map[tS.y][tS.x].unit.gold > 9) {
														map[tS.y][tS.x].unit.gold = 9;
													}
												}
											}
											//Send building to the server
											playSound(sound_effect);
											//Update unit and building to the server
											sendData("u" + sendUnit(tS.x, tS.y));
											sendData("b" + sendBuilding(tH.x, tH.y));
										}
									}
								}
							}
						}
						attacking_unit = false;
					}
				}
				if (dragging_crown) {
					crown.x = x - 5 * 3.0;
					crown.y = y;
					if (!GetMouse(0).bHeld) { //Promote Unit to Commander
						if (turn == p.turn && map[tH.y][tH.x].unit.type > 0 && map[tH.y][tH.x].unit.type != COMMANDER && map[tH.y][tH.x].unit.owner == p.turn) {
							if (!units[map[tH.y][tH.x].unit.type - 1].naval) {
								if (p.gold >= (16 - units[map[tH.y][tH.x].unit.type - 1].cost)) {
									p.gold -= (16 - units[map[tH.y][tH.x].unit.type - 1].cost);
									map[tH.y][tH.x].unit.type = COMMANDER;
									map[tH.y][tH.x].unit.HP = min(map[tH.y][tH.x].unit.HP + 10, getMaxHP(COMMANDER, p.turn));
									map[tH.y][tH.x].unit.MP = 0;
									sendData("u" + sendUnit(tH.x, tH.y));
								}
							}
						}
						dragging_crown = false;
					}
				}
				if (dragging_heal) {
					heal.x = x - 5 * 2.0;
					heal.y = y;
					if (!GetMouse(0).bHeld) { //Heal unit
						if (turn == p.turn && map[tH.y][tH.x].unit.type > 0 && map[tH.y][tH.x].unit.owner == p.turn) {
							playSound(CLINK);
							map[tH.y][tH.x].unit.HP = min(map[tH.y][tH.x].unit.HP + 5, getMaxHP(map[tH.y][tH.x].unit.type, p.turn));
							sendData("u" + sendUnit(tH.x, tH.y));
							can_heal = false;
						}
						dragging_heal = false;
					}
				}
				if (GetMouse(0).bHeld) {
					if (GetKey(olc::Key::SHIFT).bHeld) {
						if (MM_active) {
							map[tH.y][tH.x] = MM_Tile[MM_selected];
							sendData(sendTile(tH.x, tH.y));
						}
					}
				}
				if (GetMouse(0).bPressed) {
					attacking_unit = false;
					if (MM_active) {
						map[tH.y][tH.x] = MM_Tile[MM_selected];
						sendData(sendTile(tH.x, tH.y));
					}
					else {
						if (!UI_Click) { //Didn't click on an active UI element
							tS = tH;
							if (GetMouse(0).bPressed) {
								dragging_unit = true;
								drag = tH;
							}
						}
					}
				}
				if (GetMouse(1).bPressed) {
					if (!UI_Click) {
						tS = tH;
					}
					if (turn == p.turn && map[tS.y][tS.x].unit.type > 0 && map[tS.y][tS.x].unit.MP > 0) {
						attacking_unit = true;
						if (units[map[tS.y][tS.x].unit.type - 1].range > 1 && map[tS.y][tS.x].unit.type != MAGE) {
							playSound(BOW_DRAW);
						}
					}
				}
			}
			if (!GetMouse(0).bHeld) {
				buying_unit = false;
				dragging_unit = false;
				buying_building = false;
				dragging_crown = false;
			}
			if (!GetMouse(1).bHeld) {
				attacking_unit = false;
			}
		}

		//Print(to_str(timer), ScreenWidth() - 80, 75, 2);
		if (drawMap && map.size() == MAPSIZE) {
			DrawMap();
			DrawDrag();
			if (makeCiv > -1) {
				Draw(icons, ScreenWidth() - (2 + 16 * 2), ScreenHeight() - (2 + 16 * 2), 0, 80, 16, 16, 2, colors[turn]);
				if (turn == p.turn) {
					olc::Pixel drawColor = olc::WHITE;
					Draw(icons, ScreenWidth() - (2 + 16 * 2), ScreenHeight() - (2 + 16 * 2), 16, 80, 16, 16, 2, drawColor);
				}

				static int hovered = 0;
				int scale = 2.0;
				int w = 163 * scale;
				int h = 209 * scale;
				int dX = (ScreenWidth() - w) / 2;
				int dY = (ScreenHeight() - h) / 2;
				olc::Pixel col = UI_Grey;
				FillRectDecal(dX, dY, w, h, olc::BLACK);
				FillRectDecal(dX + scale, dY + scale, w - 2 * scale, h - 2 * scale, UI_Blue1);
				//Title Bar
				FillRectDecal(dX + 2 * scale, dY + 2 * scale, w - 4 * scale, 9 * scale, UI_Blue2);
				//Body
				FillRectDecal(dX + 2 * scale, dY + 11 * scale, w - 4 * scale, h - (41 * scale), UI_Blue3);
				//Footer
				FillRectDecal(dX + 2 * scale, dY + h - 40 * scale, w - 4 * scale, 38 * scale, UI_Blue4);

				//Draw Done button
				Draw(icons, dX + w - (29 * scale), dY + h - scale, 64, 112, 29, 11, scale);

				int points = 7;
				for (int i = 0; i < policies.size(); i++) {
					if (policies[i].active) {
						points -= policies[i].cost;
					}
				}
				int num = 6;
				for (int i = 1; i < units.size(); i++) {
					if (units[i].active) {
						num--;
					}
				}

				//Check if user is clicking to change tabs or clicking done
				if (GetMouse(0).bPressed) {
					if (range(GetMouseX(), GetMouseY(), dX, dY - 12 * scale, 15 * scale, 13 * scale)) {
						makeCiv = 0;
					}
					else if (range(GetMouseX(), GetMouseY(), dX + 17 * scale, dY - 12 * scale, 15 * scale, 13 * scale)) {
						makeCiv = 1;
					}
					else if (points <= 1 && num == 0 && range(GetMouseX(), GetMouseY(), dX + w - (29 * scale), dY + h - scale, 29 * scale, 11 * scale)) { //done button clicked
						makeCiv = -1;
						int nextAdd = -1;
						bool add = false;
						do { //add a sorted list of units
							nextAdd = -1;
							for (int i = 0; i < units.size(); i++) {
								if (units[i].active && (nextAdd == -1 || units[i].cost < units[nextAdd].cost)) {
									add = true;
									for (int j = 0; j < UI_Units.size(); j++) {
										if (UI_Units[j] == i) {
											add = false;
											j = UI_Units.size();
										}
									}
									if (add) {
										nextAdd = i;
									}
								}
							}
							if (nextAdd != -1) {
								UI_Units.push_back(nextAdd);
							}
						} while (nextAdd != -1);

						for (int i = 0; i < policies.size(); i++) {
							if (i < NUM_POLICIES) {
								p.policies[i] = policies[i].active;
							}
						}
						players[p.turn] = p;
						sendData(sendPolicies()); //Send Policies to the server
					}
				}

				if (makeCiv == 1) { //Units
					//Units
					Draw(icons, dX + 17 * scale, dY - 12 * scale, 96, 0, 15, 13, scale);
					Draw(icons, dX + 19 * scale, dY - 10 * scale, 96, 16, 11, 11, scale);
					//Policies
					Draw(icons, dX, dY - 12 * scale, 80, 0, 15, 13, scale);
					Draw(icons, dX+2*scale, dY - 10 * scale, 112, 16, 11, 11, scale);

					int num_drawn = 0;
					Print(units[hovered].name, ScreenWidth() / 2, dY + scale + 6 * scale, true, 2);
					if (num <= 0) {
						Print("*RED*" + to_str(num) + "*RED*", ScreenWidth() / 2 + (163 * scale)/2 - 10 * scale, dY + scale + 6 * scale, true, 2);
					}
					else {
						Print("*GREEN*" + to_str(num) + "*GREEN*", ScreenWidth() / 2 + (163 * scale) / 2 - 10 * scale, dY + scale + 6 * scale, true, 2);
					}
					dX += 5 * scale;
					dY += 13 * scale;

					for (int i = 1; i < units.size(); i++) { //Draw each unit in the body
						if (units[i].can_buy) {
							if (units[i].active) {
								col = Gold;
							}
							else { col = UI_Blue5; }
							if (range(GetMouseX(), GetMouseY(), dX + (num_drawn % 4) * 39 * scale, dY + (num_drawn / 4) * 39 * scale, 36 * scale, 36 * scale)) {
								hovered = i;
								if (GetMouse(0).bPressed) { //unit is clicked on
									if (units[i].active) { //if enough unit slots are available, then flip
										units[i].active = false;
									}
									else if (num > 0) {
										units[i].active = true;
									}
								}
								if (!units[i].active) {
									col = UI_Blue1;
								}
							}
							FillRectDecal(dX + (num_drawn % 4) * 39 * scale, dY + (num_drawn / 4) * 39 * scale, 36 * scale, 36 * scale, col);
							FillRectDecal(dX + 2 * scale + (num_drawn % 4) * 39 * scale, dY + 2 * scale + (num_drawn / 4) * 39 * scale, 32 * scale, 32 * scale, UI_Blue2);
							Draw(armySprites, dX + 2 * scale + (num_drawn % 4) * 39 * scale, dY + 2 * scale + (num_drawn / 4) * 39 * scale, units[i].sX, units[i].sY, 16, 16, 2 * scale);
							num_drawn++;
						}
					}
					//Draw the stats of the hovered unit
					dX = (ScreenWidth() - 58 * scale * 2) / 2;
					dY = (ScreenHeight() - h) / 2  + h - (4 * scale + 17*scale*2);
					Draw(icons, dX, dY, 19, 50, 58, 17, scale * 2);
					UI_Unit u = units[hovered];
					Print(to_str(u.cost), dX + 17 * 2*scale - 1, dY + 5 * 2*scale, true, 2*scale); //Cost					
					Print(to_str(getMaxHP(hovered+1, p.turn)), dX + 17 * 2*scale - 1, dY + 13 * 2*scale, true, 2*scale); //HP		
					Print(to_str(getATK(hovered+1, p.turn)), dX + 34 * 2*scale, dY + 1 * 2*scale, false, 2 * scale); //ATK
					Print(to_str(u.DEF), dX + 34 * 2*scale, dY + 9 * 2*scale, false, 2*scale); //DEF
					Print(to_str(u.range), dX + 51 * 2*scale, dY + 1 * 2*scale, false, 2*scale); //RANGE
					Print(to_str(getMaxMP(hovered+1, p.turn)), dX + 51 * 2*scale, dY + 9 * 2*scale, false, 2*scale); //MP
				}
				else if (makeCiv == 0) { //policies
					//Policies
					Draw(icons, dX + 17 * scale, dY - 12 * scale, 80, 0, 15, 13, scale);
					Draw(icons, dX + 19 * scale, dY - 10 * scale, 96, 16, 11, 11, scale);

					//Units
					Draw(icons, dX, dY - 12 * scale, 96, 0, 15, 13, scale);
					Draw(icons, dX + 2 * scale, dY - 10 * scale, 112, 16, 11, 11, scale);

					
					if (points <= 0) {
						Print("*RED*" + to_str(points) + "*RED*", ScreenWidth() / 2 + (163 * scale) / 2 - 10 * scale, dY + scale + 6 * scale, true, 2);
					}
					else {
						Print("*GREEN*"+ to_str(points) + "*GREEN*", ScreenWidth() / 2 + (163 * scale) / 2 - 10 * scale, dY + scale + 6 * scale, true, 2);
					}
					//Footer Title
					FillRectDecal(dX + 2 * scale, dY + h - (40 * scale), w - (4 * scale), 9 * scale, UI_Blue2);
					Print("Policies", ScreenWidth() / 2, dY + 7 * scale, true, 2);
					

					dX += 5 * scale;
					dY += 14 * scale;
					if (hovered >= policies.size()) {
						hovered = policies.size() - 1;
					}
					for (int i = 0; i < policies.size(); i++) {
						if (policies[i].active) {
							col = Gold;
						}
						else { col = UI_Blue5; }
						if (range(GetMouseX(), GetMouseY(), dX + (i % 4) * 39 * scale, dY + (i / 4) * 39 * scale, 36 * scale, 36 * scale)) {
							hovered = i;
							if (GetMouse(0).bPressed) { //click on a policy to enable or disable it
								if (policies[i].active || points >= policies[i].cost) {
									policies[i].active = !policies[i].active;
								}
							}
							if (!policies[i].active) {
								col = UI_Blue1;
							}
						}
						FillRectDecal(dX+(i%4)*39*scale, dY+(i/4)*39*scale, 36 * scale, 36 * scale, col);
						FillRectDecal(dX + 2 * scale + (i % 4) * 39 * scale, dY + 2 * scale + (i / 4) * 39 * scale, 32 * scale, 32 * scale, UI_Blue2);
						Draw(icons, dX + 2*scale + (i % 4) * 39 * scale, dY + 2*scale + (i / 4) * 39 * scale, policies[i].sX, policies[i].sY, 16, 16, 2*scale);
						//Check mouse stuff in this loop!!
					}
					dX -= 5 * scale;
					dY -= 14 * scale;
					Print(policies[hovered].name + " - *GOLD*" + to_str(policies[hovered].cost) + "*GOLD*", ScreenWidth() / 2, dY + h - (35 * scale), true, 2);
					Print(policies[hovered].description, ScreenWidth() / 2, dY + h - (18 * scale), true, 2, 150 * scale);
				}
			}
			else {
				DrawUI();
			}
			if (p.units.size() == 0 && p.started) {
				Print("You have been defeated!", ScreenWidth() / 2, ScreenHeight() / 2, true, 3);
				if (turn == p.turn) {
					endTurn();
				}
			}
		}
		else if (verify_map && timer % 50 == 0) {
			if (turn == 0) {
				endTurn();
			}
			bool clean = true;
			int num = 0;
			for (int i = 0; i < map.size(); i++) {
				if (map[i].size() < MAPSIZE) {
					if (clean) {
						sendData(requestRow(i));
						clean = false;
						i = map.size();
					}
				}
			}
			if (clean) {
				drawMap = true;
				verify_map = false;
				bool old_stuff = false;
				for (int i = 0; i < map.size(); i++) {
					for (int j = 0; j < map.size(); j++) {
						if (map[i][j].unit.type > 0 && map[i][j].unit.owner == p.turn) {
							p.units.push_back(C(j, i));
							old_stuff = true;
						}
						if (map[i][j].building > 0 && map[i][j].owner == p.turn) {
							p.buildings.push_back(C(j, i));
							old_stuff = true;
						}
					}
				}
				if (old_stuff) {
					p.started = true;
					sendData("g");
				}
			}
		}
		if (!drawMap) {
			int dX = ScreenWidth() / 2;
			int dY = ScreenHeight() / 2;
			Print("Loading Map", dX, dY, true, 2.0);
			
			DrawBuilding(6, dX-80, dY+32, 2, 99, C(10, 73), p.turn);
			DrawBuilding(9, dX-16, dY+32, 2, 99, C(31, 5), p.turn);
			DrawBuilding(6, dX+48, dY+32, 2, 99, C(105, 73), p.turn);
		}
		SetDrawTarget(nullptr);
		return true;
	}

	//Building info
	int bIndex(char t) {
		int type = (int)t;
		int ret = 0;

		for (int i = 0; i < buildings.size(); i++) {
			if (type >= buildings[i].type && type <= buildings[i].max_type) {
				return i;
			}
		}

		return 0;
	}

	//Stats
	int getATK(int type, Player owner) {
		int ape = units[type - 1].ATK;
		if (owner.policies[HARDY]) {
			if (type == PEASANT || type == FISHING_RAFT) {
				return 2;
			}
		}
		return ape;
	}
	int getMaxMP(int type, Player owner) {
		int ape = units[type - 1].MP;
		if (owner.policies[HUSBANDRY]) {
			if (type >= LIGHT_CAVALRY && type <= COMMANDER) {
				return ape + 1;
			}
		}
		if (owner.policies[DONKEYS]) {
			if (type == MERCHANT) {
				return ape + 1;
			}
		}
		return ape;
	}
	int getMaxHP(int type, Player owner) {
		int ape = units[type - 1].HP;
		if (owner.policies[HARDY]) {
			if (type == PEASANT || type == FISHING_RAFT) {
				return 8;
			}
		}
		if (owner.policies[DONKEYS]) {
			return ape + 2;
		}
		return ape;
	}
	int getBuildingHP(char t, Player owner) {
		int type = bIndex(t);
		int ape = buildings[type].HP;
		if (owner.policies[FORTIFICATIONS]) {
			return ape + 3;
		}
		return ape;
	}


	//Whenever the server says that this player's turn is starting, run this function
	void startTurn() {
		int bType;
		bool delta = false;
		harvest++; // This enables fish to be harvested again
		if (p.units.size() > 0) {
			p.gold++;
		}
		else if (p.started) {
			endTurn();
			return;
		}
		if (p.policies[MEDICINE]) {
			can_heal = true;
		}
		std::vector<C> unit_list = {};
		std::vector<C> building_list = {};
		std::string s = "u";
		p.upkeep = 0;
		for (int i = p.units.size()-1; i >= 0; i--) {
			C c = p.units[i];
			bType = -1;
			if (map[c.y][c.x].building > 0) {
				bType = bIndex(map[c.y][c.x].building);
			}
			if (map[c.y][c.x].unit.owner == p.turn && map[c.y][c.x].unit.type > 0) {
				unit_list.push_back(c);
				if (map[c.y][c.x].unit.type != MAYOR) { //mayors dont pay upkeep costs
					if (!p.policies[LEVY] || (map[c.y][c.x].unit.type != PEASANT && map[c.y][c.x].unit.type != SPEARMAN && map[c.y][c.x].unit.type != FISHING_RAFT && map[c.y][c.x].unit.type != PIKEMAN)) { //levy means no upkeep on fish, peasants, and spears
						if (!p.policies[BARRACKS] || (bType == FORT || bType == PIER)) { //barracks means forts and piers provide upkeep for unit on that tile
							p.upkeep += units[map[c.y][c.x].unit.type - 1].cost * .05;
						}
					}
				}
				map[c.y][c.x].unit.MP = getMaxMP(map[c.y][c.x].unit.type, map[c.y][c.x].unit.owner); //Give unit its MP
				if (bType == HOUSE || bType == PIER) { //Heal on House or dock
					map[c.y][c.x].unit.HP += 2;
					if (bType == PIER) {
						map[c.y][c.x].unit.HP++;
						if (map[c.y][c.x].unit.gold > 0) { //ships drop off gold at pier
							p.gold += map[c.y][c.x].unit.gold;
							map[c.y][c.x].unit.gold = 0;
						}
					}
				}
				else if (bType == FORT) { //Heal on Fort, also drop off gold
					map[c.y][c.x].unit.HP += 5;
					if (map[c.y][c.x].unit.gold > 0) {
						p.gold += map[c.y][c.x].unit.gold;
						map[c.y][c.x].unit.gold = 0;
					}
				}
				for (int a = -1; a < 2; a++) {
					for (int b = -1; b < 2; b++) {
						if (map[safeC(c.y + a)][safeC(c.x + b)].unit.type == MAGE) { //adjacent mage provides healing
							map[c.y][c.x].unit.HP += 2;
							a = 2;
							b = 2;
						}
					}
				}
				map[c.y][c.x].unit.HP = min(getMaxHP(map[c.y][c.x].unit.type, map[c.y][c.x].unit.owner), map[c.y][c.x].unit.HP); //make sure not to go beyond max HP 
				s += sendUnit(c.x, c.y);
			}
		}
		p.units = unit_list;

		if (s != "u") {
			sendData(s);
		}

		p.max_pop = 6;
		p.max_buildings = 12;
		s = "b";
		for (int i = p.buildings.size() - 1; i >= 0; i--) {
			delta = false;
			C c = p.buildings[i];
			if (map[c.y][c.x].owner == p.turn && map[c.y][c.x].building > 0) {
				bType = bIndex(map[c.y][c.x].building);
				if (map[c.y][c.x].HP >= getBuildingHP(map[c.y][c.x].building, map[c.y][c.x].owner) / 2.0) {
					//Buildings that provide max population space
					if (bType == HOUSE || bType == FORT || bType == PIER) {
						int max_space = 1;
						if (bType == HOUSE || bType == FORT) {
							max_space = 3;
						}
						if (p.policies[HOUSING]) {
							max_space *= 2;
						}
						p.max_pop += max_space;
					}
					else if (bType == OFFICE) { //office
						p.max_buildings += 9;
					}
					else if (bType == WORKSHOP) { //workshop
						p.gold++;
						p.max_pop -= 2;
					}
					else if (map[c.y][c.x].building >= 1 && map[c.y][c.x].building < 4) { //Farm
						map[c.y][c.x].building++;
						delta = true;
					}
					else if (map[c.y][c.x].building == 4 && map[c.y][c.x].unit.type > 0 && map[c.y][c.x].unit.owner == p.turn) { //Collect from farm
						int farm_yield = 1;
						map[c.y][c.x].building = 1;
						for (int a = -1; a < 2; a++) {
							for (int b = -1; b < 2; b++) {
								if (bIndex(map[safeC(c.y + a)][safeC(c.x + b)].building) == WINDMILL) { //Windmill
									farm_yield = 2;
								}
								if (map[safeC(c.y + a)][safeC(c.x + b)].type == RIVER || map[safeC(c.y + a)][safeC(c.x + b)].type == LAKE) {
									map[c.y][c.x].building = 2;
								}
							}
						}
						delta = true;
						p.gold += farm_yield;
					}
				}
				else { //building is on fire and may lose HP
					if (rand() % 2 == 0) {
						map[c.y][c.x].HP -= 2;
					}
				}
				if (map[c.y][c.x].HP < getBuildingHP(map[c.y][c.x].building, map[c.y][c.x].owner)) {
					delta = true;
					map[c.y][c.x].HP++;
					if (map[c.y][c.x].unit.type == PEASANT) {
						map[c.y][c.x].HP += 3;
					}
				}
				if (bType != PIER) { //check adjacent buildings for fire that might spread
					for (int a = -1; a < 2; a++) {
						for (int b = -1; b < 2; b++) {
							if (a != 0 || b != 0) {
								if (map[safeC(c.y + a)][safeC(c.x + b)].building > 0 && map[safeC(c.y + a)][safeC(c.x + b)].HP < getBuildingHP(map[safeC(c.y + a)][safeC(c.x + b)].building, map[safeC(c.y + a)][safeC(c.x + b)].owner) / 2) {
									if (rand() % 2 == 0) {
										map[c.y][c.x].HP -= 2;
										a = 2;
										delta = true;
									}
								}
							}
						}
					}
				}
				if (map[c.y][c.x].HP <= 0) {
					map[c.y][c.x].building = 0;
					delta = true;
				}
				else {
					building_list.push_back(c);
				}
				if (delta) {
					s += sendBuilding(c.x, c.y);
				}
			}
		}
		if (s != "b") {
			sendData(s);
		}
		p.buildings = building_list;
		if (p.buildings.size() > p.max_buildings) { //over building cap
			p.upkeep += 1 + (p.buildings.size() - p.max_buildings) * .5;
		}
		if (p.units.size() > p.max_pop) { //Over Population Cap
			p.upkeep *= 1.5;
			p.upkeep += 1;
		}
		//Handle Player Debt and Upkeep
		p.debt += p.upkeep;
		p.gold -= (int)p.debt;
		p.debt -= (int)p.debt;
	}

	//Runs when the player ends their turn
	void endTurn() {
		// Claim Buildings, this is done at the end of the turn to prevent u from stepping on each building to claim it
		std::string s = "b"; 
		for (C c : p.units) {
			//Merchants can't claim buildings
			if (map[c.y][c.x].unit.type != MERCHANT) {
				//Some buildings can't be claimed
				if (map[c.y][c.x].building > 0 && map[c.y][c.x].building != STAKES && map[c.y][c.x].building != DEPOT) {
					if (map[c.y][c.x].owner != p.turn) {
						map[c.y][c.x].owner = p.turn;
						p.buildings.push_back(c);
						s += sendBuilding(c.x, c.y);
					}
				}
			}
		}
		if (s != "b") {
			sendData(s);
		}
		
		sendData("e");
		sendData(sendPolicies());
	}

	//Map Maker Functions - Used when player is map making mode rather than playing
	void fillType(std::vector<std::vector<Tile>>& map, int x, int y, Tile fill, int cut = 500) {
		Tile t = map[y][x];
		std::deque<Spot> open = {Spot(x, y)};
		std::vector<Spot> closed = {};
		Spot cur;
		Spot con;
		bool found = false;
		while (open.size() > 0) {
			cur = open.back();
			open.pop_back();
			closed.push_back(cur);

			for (int a = -1; a < 2; a++) {
				for (int b = -1; b < 2; b++) {
					if (a != 0 || b != 0) {
						if (map[safeC(cur.y + a)][safeC(cur.x + b)].type == t.type && map[safeC(cur.y + a)][safeC(cur.x + b)].elev == t.elev) {
							for (Spot s : closed) {
								found = false;
								if (s.y == safeC(cur.y + a)) {
									if (s.x == safeC(cur.x + b)) {
										found = true;
										break;
									}
								}
							}
							if (!found) {
								open.push_back(Spot(safeC(cur.x + b), safeC(cur.y + a)));
							}
						}
					}
				}
			}

			if (closed.size() > cut) {
				return;
			}
		}
		for (int i = 0; i < closed.size(); i++) {
			map[closed[i].y][closed[i].x] = fill;
			sendData(sendTile(closed[i].x, closed[i].y));
		}
	}
	void saveMap(std::vector<std::vector<Tile>>& map) {
		std::ofstream myfile;
		myfile.open("saves/save.txt");
		std::string s = "";
		for (int i = 0; i < map.size(); i++) {
			for (Tile t : map[i]) {
				s += '0' + t.type;
				s += '0' + t.elev;
				s += '0' + t.forest;
				s += '0' + t.owner;
				s += '0' + t.building;
				s += '0' + t.unit.owner;
				s += '0' + t.unit.type;
				s += '0' + t.unit.HP;
				s += '0' + t.unit.MP;
			}
			s += "\n";
		}
		myfile << s;
		myfile.close();
	}
	void loadMap(std::vector<std::vector<Tile>>& map, std::string path) {
		std::ifstream myfile;
		std::string line;
		std::string num = "";
		int i = 0;
		int selector = 0;
		Tile t;
		myfile.open("saves/" + path);
		if (myfile.is_open()) {
			map = {};
			std::vector<Tile> current = {};
			while (std::getline(myfile, line)) {
				std::vector<Tile> row = readRow(line);
				map.push_back(row);
			}
		}
		myfile.close();
	}
	//End Map maker Functions

	//Functions for unit movement and unit/building purchasing
	bool canBuy(int x, int y, int index) {
		int u_type = index + 1;
		bool s = (!p.started);
		if (bIndex(map[y][x].building) == FORT && map[y][x].owner != p.turn) { //can't build units on enemy forts
			return false;
		}
		if (!s) {
			if (p.policies[VOLUNTEERS] && map[y][x].owner == p.turn && (bIndex(map[y][x].building) == FORT || bIndex(map[y][x].building) == PIER)) {
				s = true;
			}
			else {
				for (int i = -1; i < 2; i++) {
					for (int j = -1; j < 2; j++) {
						if (map[safeC(y + i)][safeC(x + j)].unit.owner == p.turn) {
							if ((map[safeC(y + i)][safeC(x + j)].unit.type == COMMANDER || map[safeC(y + i)][safeC(x + j)].unit.type == MAYOR)) { //spawn next to mayor or commander
								s = true;
								i = 2; j = 2;
							}
							else if (u_type == AXEMAN && units[map[safeC(y + i)][safeC(x + j)].unit.type - 1].naval) { //raiders can be spawned next to ships
								if (map[safeC(y + i)][safeC(x + j)].unit.type != FISHING_RAFT) { //except fishing rafts lol
									s = true;
									i = 2; j = 2;
								}
							}
						}
					}
				}
			}
		}
		else { //Mayor, make sure no enemy units nearby
			if (units[index].naval) {
				return false;
			}
			for (int i = -3; i < 7; i++) {
				for (int j = -3; j < 7; j++) {
					if (map[safeC(y + i)][safeC(x + j)].unit.type > 0) {
						s = false;
						i = 7; j = 7;
					}
				}
			}
			return s && map[y][x].forest == NONE && map[y][x].elev != MOUNTAIN && map[y][x].unit.type == NO_UNIT;
		}
		if (units[index].naval) { //ships
			return s && map[y][x].elev < FLAT && map[y][x].unit.type == NO_UNIT && p.gold >= units[index].cost;
		}
		return s && map[y][x].elev >= FLAT && map[y][x].forest == NONE && map[y][x].elev != MOUNTAIN && map[y][x].unit.type == NO_UNIT && p.gold >= units[index].cost;
	}

	/*int checkBridge(Spot c) {
		bool bdt = map[safeC(c.y - 1)][c.x].building == 12 || map[safeC(c.y + 1)][c.x].building == 12;
		bool blr = map[c.y][safeC(c.x - 1)].building == 13 || map[c.y][safeC(c.x + 1)].building == 13;
		bool gdt = map[safeC(c.y - 1)][c.x].elev >= FLAT || map[safeC(c.y + 1)][c.x].elev >= FLAT;
		bool glr = map[c.y][safeC(c.x - 1)].elev >= FLAT || map[c.y][safeC(c.x + 1)].elev >= FLAT;
		bool td = gdt || bdt;
		bool lr = glr || blr;
		if (blr) { //bridge left or right
			td = false;
		}
		if (bdt) { //bridge down or top
			lr = false;
		}
		if (td) {
			return 12;
		}
		if (lr) {
			return 13;
		}
		return -1;
	}*/

	bool canBuyBuilding(int x, int y, int index) {
		bool s = false;
		int cost = buildings[index].cost;
		for (int i = -1; i < 2; i++) {
			for (int j = -1; j < 2; j++) {
				if ((map[safeC(y + i)][safeC(x + j)].unit.type == PEASANT || map[safeC(y + i)][safeC(x + j)].unit.type == MAYOR) && map[safeC(y + i)][safeC(x + j)].unit.owner == p.turn) {
					s = true;
					i = 2; j = 2;
				}
			}
		}
		if (!s) { //No adjacent peasant/mayor, no need to go further
			return false;
		}
		if (index == BRIDGE) { //bridge
			if (map[y][x].elev >= HILL || map[y][x].unit.type > 0) {
				return false;
			}
			/*int bridgeDir = checkBridge(Spot(x, y));
			bool td = bridgeDir == 12;
			bool lr = bridgeDir == 13;
			if (td || lr) {
				for (int i = -1; i < 2; i++) {
					for (int j = -1; j < 2; j++) {
						if (map[safeC(y + i)][safeC(x + j)].building == 12) {
							if ((td && i == 0) || lr) {
								return false;
							}
						}
						if (map[safeC(y + i)][safeC(x + j)].building == 13) {
							if ((lr && j == 0) || td) {
								return false;
							}
						}
					}
				}
			}
			s = s && (td || lr);*/
		}
		else if (index == CANAL) {
			if (map[y][x].elev > FLAT || map[y][x].elev < FLAT || map[y][x].unit.type > 0 || map[y][x].forest > NONE) {
				return false;
			}
			bool canPlace = false;
			for (int i = -1; i < 2; i++) {
				for (int j = -1; j < 2; j++) {
					if (map[safeC(y + i)][safeC(x + j)].elev < FLAT) {
						canPlace = true;
						i = 2;
						j = 2;
					}
				}
			}
			s = canPlace && s;
		}
		else if (index == PIER) {
			if (map[y][x].elev >= FLAT) {
				return false;
			}
		}
		s = s && (map[y][x].building == 0 || (GetKey(olc::Key::SHIFT).bHeld && map[y][x].owner == p.turn));
		s = s && (map[y][x].elev >= FLAT || ((index == BRIDGE || index == PIER) && map[y][x].elev < FLAT));
		if (map[y][x].forest > FISH) {
			if (!p.policies[LOGGING]) {
				cost += 2;
			}
			s = s && GetKey(olc::Key::SHIFT).bHeld;
		}
		return s && map[y][x].elev != MOUNTAIN && p.gold >= cost;
	}

	//Terrain movement cost
	float tCost(int x, int y, int turn, int type) {
		Tile t = map[y][x];
		float price = 1.0;
		if (units[type-1].naval) { //boats
			if (t.elev == WATER) {
				return 1;
			}
			return 999.0;
		}
		if ((t.elev == WATER && !(t.building == 12 || t.building == 13)) || (t.unit.owner != turn && t.unit.type > 0)) {
			return 999.0;
		}
		if (!players[turn].policies[RANGERS]) {
			if (t.elev == HILL) {
				price = 1.5;
			}
			if (t.forest != NONE) {
				price += .5;
			}
		}
		if (t.elev == MOUNTAIN) {
			price = 99.0;
		}
		if (bIndex(t.building) == STAKES) {
			return getMaxMP(type, turn);
		}

		return price;
	}

	//A star pathfinding, but only returns the cost to move to a spot
	float moveCost(Spot start, Spot end, int MP, Unit u) {
		int type = u.type;
		int turn = u.owner;
		if (coord_dist(start.x, end.x) > MP || coord_dist(start.y, end.y) > MP || tCost(end.x, end.y, turn, type) > 50) {
			return 99;
		}
		if (units[u.type-1].naval) {
			if (map[end.y][end.x].building == 12 || map[end.y][end.x].building == 13) {
				return 99;
			}
		}

		std::deque<Spot> open;
		std::deque<Spot> closed;
		int total_cost = 0;
		Spot current = start;
		Spot consider;

		current.g = 0;
		current.h = dist(start, end);
		open = { current };
		bool valid = true;
		int min = 99;
		int len;
		while (open.size() > 0) {
			min = 0;
			len = open.size();
			// Look for the most optimal tile in the open list
			for (int i = 1; i < len; i++) {
				if (open[i].g + open[i].h < open[min].g + open[min].h) {
					min = i;
				}
			}
			// Remove the most optimal tile in the open list, add it to the closed list, and consider it
			current = open[min];
			consider = open.back();
			open.back() = current;
			open[min] = consider;
			open.pop_back();
			closed.push_back(current);

			if (current == end) { //At location. Return Path Cost
				return current.g;
			}

			//Look at the current tile's neighbors
			for (int a = -1; a < 2; a++) {
				for (int b = -1; b < 2; b++) {
					if ((a != 0 || b != 0)) { 
						consider = Spot(safeC(current.x + a), safeC(current.y + b));
						consider.g = current.g + tCost(consider.x, consider.y, turn, type); //Distance to start
						if (consider.g <= MP) {
							consider.h = current.g + coord_dist(consider.x, end.x) + coord_dist(consider.y, end.y);

							// Loop through open and closed to see if the tile has already been checked.
							valid = true;
							int len = open.size();
							for (int i = 0; i < len; i++) {
								if (open[i] == consider) {
									if (open[i].g + open[i].h > consider.g + consider.h) {
										open[i] = consider;
									}
									valid = false;
									i = len;
								}
							}
							if (valid) {
								len = closed.size();
								for (int i = 0; i < len; i++) {
									if (closed[i] == consider) {
										valid = (closed[i].g + closed[i].h > consider.g + consider.h);
									}
								}
							}
							if (valid) {
								open.push_back(consider);
							}
						}
					}
				}
			}
		}
		return 99;
	}
	//---------------------------------------------------------------

	//UI and Drawing-------------------------------------------------------------------------------
	void DrawUI() {
		int dX, dY;
		int scale = 2.0;
		int sX, sY, sW, sH;
		if (MM_active) {
			scale = 3.0;
			for (int i = 0; i < MM_Tile.size(); i++) {
				dX = 2;
				dY = 2 + 24 * scale * i;
				sX = 50;
				sY = 0;
				if (i == MM_selected) {
					sY = 23;
				}
				//  x  y sX  sY  w   h  scale
				Draw(icons, dX, dY, sX, sY, 22, 22, scale);
				//Draw Terrain
				sX = sourceX(MM_Tile[i].type);
				sY = 17;
				dX += 2 * scale;
				dY += 2 * scale;
				Draw(flat, dX, dY, sX, sY, 16, 16, scale);
				//Draw Elev pip
				sX = 73;
				sY = 0;
				dY += 17 * scale;
				if (MM_Tile[i].elev > FLAT) {
					dX += 2 * scale * (MM_Tile[i].elev - FLAT);
				}
				Draw(icons, dX, dY, sX, sY, 1, 1, scale);
				//Draw Forest pip
				sY = 2;
				dX = 2 + 19 * scale;
				dY = (2 + 24 * scale * i) + 2 * scale + (2 * MM_Tile[i].forest * scale);
				Draw(icons, dX, dY, sX, sY, 1, 1, scale);
			}
		}
		else {
			scale = 3.0;
			//Gold
			Draw(icons, 2, 2, 0, 0, 43, 15, scale);
			Draw(icons, 2 + 2 * scale, 2 + 2 * scale, 80, 32, 11, 11, scale);
			Print(to_str(p.gold), 2 + 27*scale, 2 + 8*scale, true, scale);
			
			std::string col = "*WHITE*";

			//Pop vs Max Pop
			Draw(icons, 4 + 43*scale, 2, 0, 16, 43, 15, scale); //bg thing

			Draw(icons, 4 + 45*scale, 2+2*scale, 96, 16, 11, 11, scale); //unit
			Draw(icons, 4 + 49*scale, 2+6*scale, 112, 0, 11, 11, scale, colors[p.turn]); //unit color

			if (p.units.size() > p.max_pop) {
				col = "*RED*";
			}
			else if (p.units.size() == p.max_pop) {
				col = "*ORANGE*";
			}
			Print(col + to_str(p.units.size()) + col, 2 + 64*scale, 2 + 8*scale, true, scale); //Pop
			Print(to_str(p.max_pop), 4 + 78 * scale, 2 + 8 * scale, true, scale); //Max Pop

			//Buildings vs Max Buildings
			Draw(icons, 4 + 86 * scale, 2, 0, 16, 43, 15, scale); //bg thing
			Draw(icons, 4 + 88 * scale, 2 + 2 * scale, 80, 16, 11, 11, scale); //unit
			col = "*WHITE*";
			if (p.buildings.size() > p.max_buildings) {
				col = "*RED*";
			}
			else if (p.buildings.size() == p.max_buildings) {
				col = "*ORANGE*";
			}
			Print(col + to_str(p.buildings.size()) + col, 2 + 107 * scale, 2 + 8 * scale, true, scale); //Pop
			Print(to_str(p.max_buildings), 4 + 121 * scale, 2 + 8 * scale, true, scale); //Max Pop

			//Unit Upkeep
			Draw(icons, 8 + 129*scale, 2, 0, 0, 43, 15, scale);
			Draw(icons, 8 + 131 * scale, 2 + 2 * scale, 112, 32, 11, 11, scale);
			int upkeep_display = p.upkeep * 100;
			Print(to_str(upkeep_display/100.0), 8 + 157 * scale, 2 + 8 * scale, true, scale);

			//Buying Building
			Draw(icons, ScreenWidth() - (2 + 15*scale), 2, 80+building_UI*16, 0, 15, 15, scale);
			Draw(icons, ScreenWidth() - (2 + 15*scale) + 2 * scale, 2 + 2 * scale, 80, 16, 11, 11, scale);
			//Buying Unit
			Draw(icons, ScreenWidth() - (2 + 31 * scale), 2, 80 + unit_UI * 16, 0, 15, 15, scale);
			Draw(icons, ScreenWidth() - (2 + 31 * scale) + 2 * scale, 2 + 2 * scale, 96, 16, 11, 11, scale);
			Draw(icons, ScreenWidth() - (2 + 31 * scale) + 2 * scale, 2 + 2 * scale, 2, 34, 11, 11, scale, colors[p.turn]); //Player colors 2 34
			
			//Player Turn and Current Turn
			Draw(icons, ScreenWidth()-(2+16*2), ScreenHeight()-(2+16*2), 0, 80, 16, 16, 2, colors[turn]);
			if (turn == p.turn) {
				olc::Pixel drawColor = olc::WHITE;
				for (int i = 0; i < p.units.size(); i++) {
					C c = p.units[i];
					if (map[c.y][c.x].unit.MP > 0) {
						drawColor = olc::Pixel(120, 120, 120);
						i = p.units.size();
					}
				}
				Draw(icons, ScreenWidth() - (2 + 16 * 2), ScreenHeight() - (2 + 16 * 2), 16, 80, 16, 16, 2, drawColor);
			}

			//Commander Upgrade Icon and Crown
			Draw(icons, ScreenWidth() - (2 + 17 * scale), (2 + 17 * scale), 96, 66, 17, 24, scale);
			if (!dragging_crown) {
				crown.x = ScreenWidth() - (2 + 17 * scale) + 3 * scale;
				crown.y = (2 + 17 * scale) + 3 * scale;
			}
			Draw(icons, crown.x, crown.y, 80, 66, 11, 10, scale);
			int cost = 16;
			if (map[tH.y][tH.x].unit.owner == p.turn && map[tH.y][tH.x].unit.type > 0) {
				cost = 16 - units[map[tH.y][tH.x].unit.type - 1].cost;
			}
			dX = 9 * scale + ScreenWidth() - (2 + 17 * scale);
			dY = 18 * scale + (2 + 17 * scale);
			Print(to_str(cost), dX, dY, true, scale);

			//Unit Heal
			if (can_heal) {
				if (dragging_heal) {
					Draw(icons, heal.x, heal.y, 48, 80, 16, 16, 2);
				}
				else {
					Draw(icons, heal.x, heal.y, 48, 80, 16, 16, 3);
					heal.x = ScreenWidth() - (2 + 17 * scale);
					heal.y = (4 + (24 + 17) * scale);
				}
			}
			if (unit_UI) { //Unit Purchasing
				dX = 2;
				dY = 4 + 15 * scale;

				//Draw Top
				sX = 0; sY = 48; sW = 79; sH = 1;
				Draw(icons, dX, dY, sX, sY, sW, sH, scale);
				dY += scale;
				//Draw Middle
				sX = 0; sY = 49; sW = 79; sH = 19;
				int selected_dY = -1;
				for (int i : UI_Units) {
					Draw(icons, dX, dY, sX, sY, sW, sH, scale);
					UI_Unit u = units[i];
					//Draw a unit on the buying unit UI list
					Draw(armySprites, dX + 2 * scale, dY + scale, u.sX, u.sY, 16, 17, scale); //Draw Unit
					Draw(armySprites, dX + 2 * scale, dY + 2*scale, u.sX, 17 + u.sY, 16, 14, scale, colors[p.turn]); //Draw Unit's colors
					//Print Cost
					Print(to_str(u.cost), dX + 35 * scale, dY + 6 * scale, true, scale);
					//HP
					Print(to_str(getMaxHP(i+1, p.turn)), dX + 35 * scale, dY + 14 * scale, true, scale);
					//ATK
					Print(to_str(getATK(i+1, p.turn)), dX + 53 * scale, dY + 2 * scale, false, scale);
					//DEF
					Print(to_str(u.DEF), dX + 53 * scale, dY + 10 * scale, false, scale);
					//RANGE
					Print(to_str(u.range), dX + 70 * scale, dY + 2 * scale, false, scale);
					//MP
					Print(to_str(getMaxMP(i+1, p.turn)), dX + 70 * scale, dY + 10 * scale, false, scale);
					if (i == unit_selected) {
						selected_dY = dY;
					}
					dY += 18 * scale;
				}
				if (selected_dY != -1) {
					DrawBox(dX + scale, selected_dY, 76 * scale, 18 * scale, Gold, scale);
				}
				//Draw Bottom
				dY += scale;
				sX = 0; sY = 48; sW = 79; sH = 1;
				Draw(icons, dX, dY, sX, sY, sW, sH, scale);
			}
			if (building_UI) { //building purchasing
				dX = 2;
				dY = 6 + 144 * scale;
				//Draw Left
				sX = 0; sY = 222; sW = 1; sH = 34;
				Draw(building_sprites, dX, dY, sX, sY, sW, sH, scale);
				dX += scale;
				//Draw Middle
				sX = 1; sY = 222; sW = 18; sH = 34;
				int selected_dX = -1;
				if (building_selected > buildings.size() - 1) {
					building_selected = buildings.size() - 1;
				}
				for (int i = 0; i < buildings.size(); i++) {
					Draw(building_sprites, dX, dY, sX, sY, sW, sH, scale);
					if (i == 0) {
						DrawBuilding(4, dX+scale, dY - 8 * scale, scale, 99, C(0, 0), p.turn);
					}
					else {
						DrawBuilding(buildings[i].type, dX+scale, dY - 8 * scale, scale, 99, C(0, 0), p.turn);
					}

					Print(to_str(buildings[i].cost), dX+11*scale, dY+24*scale, false, scale);
					if (building_selected == i) {
						selected_dX = dX;
					}
					dX += 17 * scale;
				}
				//Draw Right
				sX = 0; sY = 222; sW = 1; sH = 34;
				dX += scale;
				Draw(building_sprites, dX, dY, sX, sY, sW, sH, scale);

				if (selected_dX > -1) {
					DrawBox(selected_dX, dY + scale, 17 * scale, 31 * scale, Gold, scale);
				}
			}

			//Unit hovered stats
			if (map[tH.y][tH.x].unit.type != NO_UNIT) {
				//also, draw the polices that unit's owner has
				int scl = 2;
				dX = 2 + ScreenWidth() - (4 + 129 * scl);
				dY = ScreenHeight() - (2 + 13 * scl);
				if (map[tH.y][tH.x].unit.owner < players.size()) {
					for (int i = 0; i < NUM_POLICIES; i++) {
						if (players[map[tH.y][tH.x].unit.owner].policies[i]) {
							FillRectDecal(dX, dY-18, 16, 16, UI_Blue2);
							Draw(icons, dX, dY-18, policies[i].sX, policies[i].sY, 16, 16);
							dX += 17;
						}
					}
				}
				dX = ScreenWidth() - (4 + 129 * scl);
				Draw(icons, dX, dY, 0, 179, 113, 13, scl);
				dY += 7 * scl;
				dX += 1;
				Print(to_str((int)map[tH.y][tH.x].unit.HP), dX+18*scl, dY, true, scl); //HP
				Print(to_str((int)map[tH.y][tH.x].unit.MP), dX + 38 * scl, dY, true, scl); //MP
				Print(to_str(getATK(map[tH.y][tH.x].unit.type, map[tH.y][tH.x].unit.owner)), dX+55*scl, dY, true, scl); //ATK
				Print(to_str(units[map[tH.y][tH.x].unit.type-1].DEF), dX+72*scl, dY, true, scl); //DEF
				Print(to_str(units[map[tH.y][tH.x].unit.type-1].range), dX+89*scl, dY, true, scl); //RANGE
				Print(to_str((int)map[tH.y][tH.x].unit.gold), dX + 106 * scl, dY, true, scl); //MP
				
			}
			//Building hovered stats
			if (map[tH.y][tH.x].building > 0) {
				int scl = 2;
				dX = ScreenWidth() - (2 + 28*scl);
				dY = ScreenHeight() - (16 * 2 + 22 + 43*scl);
				Draw(building_sprites, dX, dY, 20, 213, 28, 43, scl);
				DrawBuilding(map[tH.y][tH.x].building, dX + 3*scl, dY - 7 * scl, scl, map[tH.y][tH.x].HP, C(tH.x, tH.y), map[tH.y][tH.x].owner);
				Print(to_str(buildings[bIndex(map[tH.y][tH.x].building)].loot), dX + 16*scl - 1, dY + 29*scl, true, scl);
				Print(to_str((int)map[tH.y][tH.x].HP), dX + 19 * scl - 1, dY + 37 * scl, true, scl);
				int hp = 29*2 * (float)map[tH.y][tH.x].HP / getBuildingHP(map[tH.y][tH.x].building, map[tH.y][tH.x].owner);
				FillRectDecal(dX + 20*scl, dY + 3*scl + (29*2-hp), 5*scl, hp, colors[map[tH.y][tH.x].owner]);
			}
		}
		//Off 16 zoom warning
		if (zoom > 16 && zoom % 16 != 0) {
			Draw(icons, ScreenWidth() - 18, ScreenHeight() - (16*2+20), 176, 176, 16, 16);
		}
		//FPS warning
		if (GetFPS() < 58) {
			Draw(icons, ScreenWidth() - 36, ScreenHeight() - (16 * 2 + 20), 160, 176, 16, 16);
		}
	}

	void Draw(olc::Decal* d, int x, int y, int sX, int sY, int sizeX, int sizeY, float scale = 1, olc::Pixel ape = olc::WHITE) {
		DrawPartialDecal(olc::vf2d(x, y), d, olc::vf2d(sX, sY), olc::vf2d(sizeX, sizeY),  olc::vf2d(scale, scale), ape);
	}
	//void PixelGameEngine::DrawPartialRotatedDecal(const olc::vf2d& pos, olc::Decal* decal, const float fAngle, const olc::vf2d& center, const olc::vf2d& source_pos, const olc::vf2d& source_size, const olc::vf2d& scale, const olc::Pixel& tint)
	void rDraw(olc::Decal* d, float angle, int x, int y, int sX, int sY, int sizeX, int sizeY, float scale = 1, olc::Pixel ape = olc::WHITE) {
		DrawPartialRotatedDecal(olc::vf2d(x, y), d, angle, olc::vf2d(sizeX / 2, sizeY / 2), olc::vf2d(sX, sY), olc::vf2d(sizeX, sizeY), olc::vf2d(scale, scale), ape);
	}

	float latTemp(int x) {
		//return 100 - square((float)(x - MAPSIZE / 2.0) / (MAPSIZE / 20.0));
		//return (100.0 / square(1.0 + square((x - MAPSIZE/2) / (MAPSIZE/3.0))));
		float temp = 100 * (1 - (abs(MAPSIZE / 2 - x) / (float)(MAPSIZE / 4)));
		if (temp > 0) {
			return temp;
		}
		return 0;
	}

	void DrawMap() {
		int width = ceil(float(ScreenWidth()) / float(zoom));
		int height = ceil(float(ScreenHeight()) / float(zoom));
		if (dXView > 0) { width += 2; }
		if (dYView > 0) { height += 2; }
		for (int i = -1 - ceil(height / 2); i <= 1 + ceil(height / 2); i++) {
			for (int j = -1 - ceil(width / 2); j <= ceil(width / 2); j++) {
				int y = safeC(i + yView);
				int x = safeC(j + xView);
				DrawTile(map, x, y, (ScreenWidth() / 2 + ((j * zoom) - dXView)), (ScreenHeight() / 2 + ((i * zoom) - dYView)), zoom);
				//int temp = latTemp(y-5);
				//Print(to_str(temp), (ScreenWidth() / 2 + ((j * zoom) - dXView)) + zoom/2, (ScreenHeight() / 2 + ((i * zoom) - dYView)) + zoom/2, true, zoom / 16.0);
			}
		}
	}

	//building type, draw area, map area (for animation seed), 
	void DrawBuilding(int type, int dX, int dY, float scale, int HP = 99, C c = C(15, 15), int owner = 0, olc::Pixel pix = olc::WHITE) {
		float z = scale;
		int index = bIndex(type);

		int sX = buildings[index].sX;
		int sY = buildings[index].sY;


		if (index == 0) { //Farm
			sY += 32 * (type - 1);
		}
		/*else if (index == 2) { //bridge
			if (type == 12) {
				sY += 32;
			}
		}*/
		if (buildings[index].anim_num > 1) {
			sY += 32 * (((int)(randC(c.x, c.y) * 5) + timer / buildings[index].anim_speed) % buildings[index].anim_num);
		}
		DrawPartialDecal(olc::vf2d(dX, dY), building_sprites, olc::vf2d(sX, sY), olc::vf2d(16, 32), olc::vf2d(z, z), pix);
		if (buildings[index].top_color) {
			DrawPartialDecal(olc::vf2d(dX, dY), building_sprites, olc::vf2d(sX, 0), olc::vf2d(16, 16), olc::vf2d(z, z), colors[owner]);
		}
		if (buildings[index].bot_color) {
			DrawPartialDecal(olc::vf2d(dX, dY+16*scale), building_sprites, olc::vf2d(sX, 0), olc::vf2d(16, 16), olc::vf2d(z, z), colors[owner]);
		}
		if (HP < getBuildingHP(type, owner)/2.0) { //draw fire sprite
			sX = 48;
			sY = 48;
			sY += 32 * (((int)(randC(c.x, c.y) * 5) + timer / 4) % 4);
			DrawPartialDecal(olc::vf2d(dX, dY), building_sprites, olc::vf2d(sX, sY), olc::vf2d(16, 32), olc::vf2d(z, z));
		}
	}

	void DrawDrag() { //Draw unit that is being dragged
		float z = (float)zoom / 16.0;
		int x = drag.x;
		int y = drag.y;
		int dX = GetMouseX() - 7 * z;
		int dY = GetMouseY();
		olc::Pixel ap = colors[map[y][x].unit.owner];
		olc::Pixel pix = olc::RED;
		pix.a = 125;
		int sX, sY;
		if (dragging_unit) {
			if (map[y][x].unit.type == NO_UNIT) {
				dragging_unit = false;
				return;
			}
			dY -= 7 * z;
			sX = units[map[y][x].unit.type - 1].sX;
			sY = units[map[y][x].unit.type - 1].sY;
			//Draw a unit being dragged
			DrawPartialDecal(olc::vf2d(dX, dY - z), armySprites, olc::vf2d(sX, sY), olc::vf2d(16, 17), olc::vf2d(z, z));
			//  x  y sX  sY  w   h  scale
			DrawPartialDecal(olc::vf2d(dX, dY), armySprites, olc::vf2d(sX, sY + 17), olc::vf2d(16, 14), olc::vf2d(z, z), ap); //Draw Color

			int hp = 12 * ((float)map[y][x].unit.HP / getMaxHP(map[y][x].unit.type, map[y][x].unit.owner));
			int mov = 12 * ((float)map[y][x].unit.MP / getMaxMP(map[y][x].unit.type, map[y][x].unit.owner));
			//			     x  y sX  sY  w   h  scale
			Draw(armySprites, dX + z, (dY - zoom) + 9 * z, 1, 9, 14, 5, z);
			DrawPartialDecal(olc::vf2d(dX + 2 * z, (dY - zoom) + 10 * z), flat, olc::vf2d(418, 0), olc::vf2d(1, 1), olc::vf2d(hp * z, z));
			DrawPartialDecal(olc::vf2d(dX + 2 * z, (dY - zoom) + 12 * z), flat, olc::vf2d(420, 0), olc::vf2d(1, 1), olc::vf2d(mov * z, z));
		}
		else if (buying_unit) {
			dY += 4 * z;
			ap.a = 125;
			if (canBuy(tH.x, tH.y, unit_selected)) {
				pix = olc::WHITE;
				pix.a = 200;
				ap.a = 200;
			}
			else {
				bool ape = canBuy(tH.x, tH.y, unit_selected);
			}
			//Draw a unit being purchased
			Draw(armySprites, dX, dY - zoom, units[unit_selected].sX, units[unit_selected].sY, 16, 17, z, pix);
			Draw(armySprites, dX, dY - (zoom-z), units[unit_selected].sX, 17 + units[unit_selected].sY, 16, 14, z, colors[p.turn]);
		}
		else if (buying_building) {
			sY = 16;
			ap.a = 125;
			x = tH.x;
			y = tH.y;
			if (canBuyBuilding(tH.x, tH.y, building_selected)) {
				pix = olc::WHITE;
				pix.a = 200;
				ap.a = 200;
			}
			DrawBuilding(buildings[building_selected].type, dX, dY-zoom, z, 99, C(15, 15), p.turn, pix);
		}
		if (attacking_unit) {
			if (map[tS.y][tS.x].unit.type > 0 && map[tS.y][tS.x].unit.owner == p.turn && map[tS.y][tS.x].unit.MP > 0) {
				int w = ceil(float(ScreenWidth()) / float(zoom));
				int h = ceil(float(ScreenHeight()) / float(zoom));
				int hX = GetMouseX();
				int hY = GetMouseY();
				bool sFound = false;
				x = safeC(xView + (-1 - ceil(w / 2)));
				y = safeC(yView + (-1 - ceil(h / 2)));
				if (dXView > 0) { w += 2; }
				if (dYView > 0) { h += 2; }
				for (int i = -1 - ceil(h / 2); i <= 1 + ceil(h / 2); i++) {
					for (int j = -1 - ceil(w / 2); j <= ceil(w / 2); j++) {
						x = safeC(j + xView);
						y = safeC(i + yView);
						if (y == tS.y && x == tS.x) {
							sX = (ScreenWidth() / 2 + ((j * zoom) - dXView));
							sY = (ScreenHeight() / 2 + ((i * zoom) - dYView));
							sFound = true;
						}
						if (sFound) {
							i = h;
							j = w;
						}
					}
				}
				if (sFound) {
					float angle = -atan2((sX - (hX-8*z)), (sY - (hY-8*z)));

					float dist_x = (sX - (hX - 8 * z)) * (sX - (hX - 8 * z));
					float dist_y = (sY - (hY - 8 * z)) * (sY - (hY - 8 * z));
					float dist = sqrt(dist_x + dist_y);
					if (dist < zoom) {
						dist = zoom;
					}
					if (dist > units[map[tS.y][tS.x].unit.type - 1].range * zoom) {
						dist = units[map[tS.y][tS.x].unit.type - 1].range* zoom;
					}

					DrawPartialRotatedDecal(olc::vf2d(sX + 8 * z, sY + 8 * z), icons, angle, olc::vf2d(6, 9), olc::vf2d(80, 80), olc::vf2d(12, 10), olc::vf2d(z, (dist/9)));
				}
			}
		}
	}

	void DrawElevation(int x, int y, int dX, int dY, float z, int e = -1) {
		int elev = e;
		if (elev == -1) {
			elev = map[y][x].elev;
		}
		int sX = 0;
		int sY = 32;

		if (elev == MOUNTAIN) {
			//sY += 16;
			DrawElevation(x, y, dX, dY, z, elev - 1);
			sX = 0;
			sY = 80;
			if (map[safeC(y + 1)][x].elev == MOUNTAIN) {
				sX += 16;
				if (map[safeC(y - 1)][x].elev == MOUNTAIN) {
					sX += 32;
				}
			}
			Draw(terrain, dX, (dY - 16 * z), sX, sY, 16, 32, z);
			return;
		}


		bool u = map[safeC(y - 1)][x].elev >= elev; // up * 8
		bool d = map[safeC(y + 1)][x].elev >= elev; // down * 4
		bool l = map[y][safeC(x - 1)].elev >= elev; // left * 2
		bool r = map[y][safeC(x + 1)].elev >= elev; // right * 1
		if (!u && !d && !l && !r) {
			//DrawPartialDecal(olc::vf2d(dX, dY - zoom), terrain, olc::vf2d(sX, sY), olc::vf2d(16, 32), olc::vf2d(z, z));
			//return;
		}
		sX = 16 * ((u * 8) + (d * 4) + (l * 2) + r);

		DrawPartialDecal(olc::vf2d(dX, dY), terrain, olc::vf2d(sX, sY), olc::vf2d(16, 16), olc::vf2d(z, z));
		bool dr = d && r && map[safeC(y + 1)][safeC(x + 1)].elev < elev;
		bool dl = d && l && map[safeC(y + 1)][safeC(x - 1)].elev < elev;

		if (dr) {
			DrawPartialDecal(olc::vf2d(dX, dY), terrain, olc::vf2d(272, sY), olc::vf2d(16, 16), olc::vf2d(z, z));
		}
		if (dl) {
			DrawPartialDecal(olc::vf2d(dX, dY), terrain, olc::vf2d(256, sY), olc::vf2d(16, 16), olc::vf2d(z, z));
		}
	}

	void DrawTile(std::vector<std::vector<Tile>>& map, int x, int y, int dX, int dY, int zom = 16, bool borders = false, bool extra = true) {
		if (dY + zom < 0 || dX + zom < 0) {
			return;
		}
		bool drawDot = false;
		olc::Pixel dotColor = GREY;
		if (!buying_unit && !buying_building && map[tS.y][tS.x].unit.type > 0) {
			int MP = map[tS.y][tS.x].unit.MP;
			dotColor = colors[map[tS.y][tS.x].unit.owner];
			if (map[tS.y][tS.x].unit.owner != turn) {
				MP = getMaxMP(map[tS.y][tS.x].unit.type, map[tS.y][tS.x].unit.owner);
			}
			float mCost = moveCost(Spot(tS.x, tS.y), Spot(x, y), MP, map[tS.y][tS.x].unit);
			drawDot = mCost <= MP;
			
			if (mCost > MP - 1) {
				dotColor = GREY;
			}
			dotColor.a = 125;
		}
		
		float z = (float)zom / 16.0;
		if (zom >= 16) {
			int sX = sourceX(map[y][x].type);
			int sY = 0;
			int useRand = randC(x, y) * 4;
			if (map[y][x].elev == HILL || map[y][x].elev == MOUNTAIN || map[y][x].forest != NONE || map[y][x].building) {
				useRand = 0;
			}
			if (map[y][x].elev == WATER) { //Water Tiles
				useRand = timer / 50;
				if (map[y][x].type == 'a') {
					useRand++;
				}
				else if (map[y][x].type == 'r') {
					useRand += 2;
				}
				useRand %= 4;
			}
			int rX = 0;
			int rY = 0;
			switch (useRand) {
			case 1: rX = 1; rY = 0; break;
			case 2: rX = 0; rY = 1; break;
			case 3: rX = 1; rY = 1; break;
			default:rX = 0; rY = 0; break;
			}
			DrawPartialDecal(olc::vf2d(dX, dY), flat, olc::vf2d(sX + rX * 16, sY + 17 + rY * 16), olc::vf2d(16, 16), olc::vf2d(z, z));
			//Draw Tile Overlays/Borders
				//Adjacent Tile Terrain overlay
			TERRAIN adjType;
			for (int a = -1; a < 2; a++) {
				for (int b = -1; b < 2; b++) {
					if ((a == 0) != (b == 0)) {
						int side = abs(b);
						adjType = map[safeC(y + a)][safeC(x + b)].type;
						if (typePrecedent(adjType) > typePrecedent(map[y][x].type)) {
							sX = sourceX(adjType);
							sY = 0;
							if (map[y][x].elev == WATER) {
								sY += 50;
							}
							float offset = z + (float)zom * (13.0 / 16.0);
							if ((offset - (int)offset) == .25) {
								offset += 1.0 / 8.0;
							}
							if (side > 0) {
								if ((offset - (int)offset) == .75) {
									offset += 2.0 / 8.0;
								}
								if (b == -1) {
									DrawPartialDecal(olc::vf2d(dX, dY), flat, olc::vf2d(sX + 14, sY), olc::vf2d(2, 16), olc::vf2d(z, z));
								}
								else {
									DrawPartialDecal(olc::vf2d(dX + offset, dY), flat, olc::vf2d(sX, sY), olc::vf2d(2, 16), olc::vf2d(z, z));
								}
							}
							else {
								if ((offset - (int)offset) == .75) {
									offset += a * 2 / 8.0;
								}
								if (a == -1) {
									DrawPartialDecal(olc::vf2d(dX, dY), flat, olc::vf2d(sX + 16, sY + 14), olc::vf2d(16, 2), olc::vf2d(z, z));
								}
								else {
									DrawPartialDecal(olc::vf2d(dX, dY + offset), flat, olc::vf2d(sX + 16, sY), olc::vf2d(16, 2), olc::vf2d(z, z));
								}
							}
						}
					}
				}
			}
			if (drawDot) { //Possible Unit Movement
				DrawPartialDecal(olc::vf2d(dX, dY), flat, olc::vf2d(448, 16), olc::vf2d(16, 16), olc::vf2d(z, z), dotColor);
			}
			/*if (borders && map[y][x].owner != 0) { //DRAW COUNTRY BORDERS
				olc::Pixel tint = colors[map[y][x].owner];
				//colors[std::std::min(7, map[y][x].owner)];
				bool lT, rT, uT, dT, bL, bR, tL, tR;
				lT = map[y][safeC(x - 1)].owner != map[y][x].owner;
				rT = map[y][safeC(x + 1)].owner != map[y][x].owner;
				uT = map[safeC(y - 1)][x].owner != map[y][x].owner;
				dT = map[safeC(y + 1)][x].owner != map[y][x].owner;
				bL = map[safeC(y + 1)][safeC(x - 1)].owner != map[y][x].owner;
				bR = map[safeC(y + 1)][safeC(x + 1)].owner != map[y][x].owner;
				tL = map[safeC(y - 1)][safeC(x - 1)].owner != map[y][x].owner;
				tR = map[safeC(y - 1)][safeC(x + 1)].owner != map[y][x].owner;
				DrawPartialDecal(olc::vf2d(dX, dY), terrain, olc::vf2d((rT + 2 * lT + 4 * dT + 8 * uT) * 16, 64), olc::vf2d(16, 16), olc::vf2d(z, z), tint);
				if (bL && !lT && !dT) {
					DrawPartialDecal(olc::vf2d(dX, dY + (11 * (z))), terrain, olc::vf2d(256, 64 + 11),
						olc::vf2d(3, 5), olc::vf2d(z, z), tint);
				}
				if (bR && !rT && !dT) {
					DrawPartialDecal(olc::vf2d(dX + (13 * (z)), dY + (11 * (z))), terrain, olc::vf2d(272 + 13, 64 + 11),
						olc::vf2d(3, 5), olc::vf2d(z, z), tint);
				}
				if (tL && !lT && !uT) {
					DrawPartialDecal(olc::vf2d(dX, dY), terrain, olc::vf2d(288, 64),
						olc::vf2d(3, 5), olc::vf2d(z, z), tint);
				}
				if (tR && !rT && !uT) {
					DrawPartialDecal(olc::vf2d(dX + (13 * (z)), dY), terrain, olc::vf2d(304 + 13, 64),
						olc::vf2d(3, 5), olc::vf2d(z, z), tint);
				}
			}*/
			if (map[y][x].elev >= HILL) {
				DrawElevation(x, y, dX, dY, z);
			}
			//Forest, Hills, Mountains
			if (map[y][x].elev >= HILL && map[safeC(y+1)][x].elev < HILL) { //Hill
				dY -= 3 * z;
			}
			if (map[y][x].building > 0) {
				DrawBuilding(map[y][x].building, dX, dY-zom, z, map[y][x].HP, C(x, y), map[y][x].owner);
			}
			if (map[y][x].forest > FISH) {
				sY = 0;
				sX = forestX(map[y][x].forest);
				if ((x % 100) == timer/4 || (tH.y == y && tH.x == x)) {
					sX += 64;
				}
				DrawPartialDecal(olc::vf2d(dX, dY - zom), terrain, olc::vf2d(sX, sY), olc::vf2d(16, 32), olc::vf2d(z, z));
			}
			else if (map[y][x].forest == FISH && map[y][x].harvest < harvest) {
				int step = ((int)(randC(x, y) * 8) + (timer / 8)) % 8;
				sX = 176 + 16*(step%4);
				sY = 16 * (int)(step/4);
				if (sY > 16) {
					std::cout << sX << std::endl;
					std::cout << sY << std::endl;
				}
				DrawPartialDecal(olc::vf2d(dX, dY), terrain, olc::vf2d(sX, sY), olc::vf2d(16, 16), olc::vf2d(z, z));
			}
			if (map[y][x].unit.type > 0 && !(dragging_unit && drag.x == x && drag.y == y)) { //Unit on Tile
				olc::Pixel ap = colors[map[y][x].unit.owner];
				sX = units[map[y][x].unit.type - 1].sX;
				sY = units[map[y][x].unit.type - 1].sY;
				float off = 1;
				if (map[y][x].building == 7 || map[y][x].building == 12) { //Fort or Bridge
					off = 6;
				}
				if (map[y][x].building == 11) { //dock
					off = 5;
				}
				//Draw a unit
				DrawPartialDecal(olc::vf2d(dX, dY - ((off)*z)), armySprites, olc::vf2d(sX, sY), olc::vf2d(16, 17), olc::vf2d(z, z));
				//  x  y sX  sY  w   h  scale
				DrawPartialDecal(olc::vf2d(dX, dY - ((off-1)*z)), armySprites, olc::vf2d(sX, sY+17), olc::vf2d(16, 14), olc::vf2d(z, z), ap);

				int hp = 12 * ((float)map[y][x].unit.HP / getMaxHP(map[y][x].unit.type, map[y][x].unit.owner));
				int mov = 12 * ((float)map[y][x].unit.MP / getMaxMP(map[y][x].unit.type, map[y][x].unit.owner));
				//			     x  y sX  sY  w   h  scale
				Draw(armySprites, dX + z, (dY - zom) + (9-off)*z, 1, 9, 14, 5, z);
				DrawPartialDecal(olc::vf2d(dX + 2 * z, (dY - zom) + (10-off) * z), flat, olc::vf2d(418, 0), olc::vf2d(1, 1), olc::vf2d(hp*z, z));
				DrawPartialDecal(olc::vf2d(dX + 2 * z, (dY - zom) + (12-off) * z), flat, olc::vf2d(420, 0), olc::vf2d(1, 1), olc::vf2d(mov*z, z));
			}
			//UI tile selection/tile hovered overlay
			if (tS.y == y && tS.x == x) {
				DrawPartialDecal(olc::vf2d(dX, dY), flat, olc::vf2d(416, 16), olc::vf2d(16, 16), olc::vf2d(z, z), UI_Blue1);
			}
			if (tH.y == y && tH.x == x) {
				olc::Pixel pix = Gold;
				if (attacking_unit && map[tS.y][tS.x].unit.MP > 0) {
					if (coord_dist(tH.x, tS.x) <= units[map[tS.y][tS.x].unit.type - 1].range && coord_dist(tH.y, tS.y) <= units[map[tS.y][tS.x].unit.type - 1].range) {
						pix = olc::RED;
					}
				}
				DrawPartialDecal(olc::vf2d(dX, dY), flat, olc::vf2d(416, 16), olc::vf2d(16, 16), olc::vf2d(z, z), pix);
			}
		}
		else {
			olc::Pixel p(255, 255, 255);
			switch (map[y][x].type) {
			case OCEAN: p = Water; break;
			case RIVER: p = River; break;
			case LAKE: p = Lake; break;
			case GRASS: p = Grass; break;
			case STEPPE: p = Steppe; break;
			case SAVANNA: p = Savanna; break;
			case COLD_DESERT: p = Cold_Desert; break;
			case DESERT: p = Desert; break;
			case TUNDRA: p = Tundra; break;
			case ICE: p = Ice; break;
			case BOG: p = Bog; break;
			case MEADOW: p = Meadow; break;
			case SHALLOW: p = Ocean; break;
			}
			if (map[y][x].forest != NONE) {
				p = Forest;
			}
			else if (map[y][x].elev == HILL) {
				p = Hill;
			}
			else if (map[y][x].elev == MOUNTAIN) {
				p = Mountain;
			}
			/*if (drawCurrentDirection) {
				if (map[y][x].elev == FLAT) {
					int intens = 255;// std::min(255, map[y][x].num / 35);
					switch (map[y][x].prevailing) {
					case -1: break;
					case 0: p = olc::Pixel(255, 165, 0, intens);  break; //d -> orange
					case 1: p = olc::Pixel(255, 0, 0, intens); break; //r -> red
					case 2: p = olc::Pixel(120, 255, 200, intens);  break; //u -> teal
					case 3: p = olc::Pixel(0, 255, 0, intens);  break; //l -> green
					}
					intens = map[y][x].forest_C;
					if (intens > 255) {
						intens = 255;
					}
					p = olc::Pixel(0, 255, 0, intens);
				}
			}
			//p = olc::Pixel(0, 0, 255, map[y][x].temp * 2.55);*/
			FillRect(olc::vf2d(dX, dY), olc::vf2d(zom, zom), p);
			if (map[y][x].owner != 0) {
				olc::Pixel p = colors[map[y][x].owner];
				p.a = 75;
				FillRect(olc::vf2d(dX, dY), olc::vf2d(zom, zom), p);
			}
		}
	}

	void DrawBox(int x, int y, int w, int h, olc::Pixel color=olc::WHITE, int thickness = 1) {
		FillRectDecal(olc::vf2d(x, y), olc::vf2d(thickness, h), color);
		FillRectDecal(olc::vf2d(x + w, y), olc::vf2d(thickness, h + thickness), color);
		FillRectDecal(olc::vf2d(x, y), olc::vf2d(w, thickness), color);
		FillRectDecal(olc::vf2d(x, y + h), olc::vf2d(w, thickness), color);
	}
	//End UI and Drawing-----------------------------------------------------------------------------------------


	//Printing --------------------------------------------------------------------------------
	int charWidth(char c) {
		switch (c) {
		case '"':case ',': case '.': case '\'': case '!': case ':': return 1;
		case ';': return 2;
		case ' ': case '[': case ']': case '|': return 3;
		case '\n': return 0;
		default: return TEXT_W;
		}
	}

	int Print(std::string text, int x = -1, int y = -1, bool center = false, float scale = 1.0f, int width = 1920/2) {
		int cap = width;
		olc::Pixel p = WHITE;
		if (y == -1) {
			cap = x;
		}
		std::vector<int> splitIndexes;
		std::string line = "";
		bool skipping = false;
		std::string color = "";
		std::vector<int> lineWidth = { 0 };
		int spaces = 0; int splits = 0;
		int drawX = x;
		int drawY = y;
		int offX = 0; int offY = 0;
		int sH = TEXT_H; int sW = TEXT_W;
		int sY = 1;
		int sX = 65;
		bool prnt = true;

		int wordlen = 0;
		for (char& c : text) {
			if (c == '*') {
				skipping = !skipping;
			}
			else if (!skipping) {
				wordlen += scale * (charWidth(c) + 1);
				lineWidth[lineWidth.size() - 1] += scale * (charWidth(c) + 1);
				line += c;
				if (c == ' ' || c == '\n') {
					if (c == '\n' || lineWidth[lineWidth.size() - 1] > cap) {
						line = "";
						if (lineWidth[lineWidth.size() - 1] > cap && spaces > 0) {
							splitIndexes.push_back(spaces - 1);
							lineWidth[lineWidth.size() - 1] -= wordlen;
							lineWidth.push_back(wordlen);
						}
						else {
							splitIndexes.push_back(spaces);
							lineWidth.push_back(0);
						}
					}
					wordlen = 0;
					spaces++;
				}
			}
		}
		if (lineWidth[lineWidth.size() - 1] > cap && spaces > 0) {
			splitIndexes.push_back(spaces - 1);
			lineWidth[lineWidth.size() - 1] -= wordlen;
			lineWidth.push_back(wordlen);
		}
		spaces = 0;
		if (y == -1) {
			return splitIndexes.size();
		}
		if (center) {
			drawX = 1 + x - (lineWidth[splits] / 2); //if ur having problems try scale + x
			drawY = y - scale * lineWidth.size() * (1 + (sH/2));
		}
		skipping = false;
		for (char& c : text) {
			sH = TEXT_H; sW = TEXT_W;
			if (c == '*') {
				skipping = !skipping;
				if (!skipping) {
					if (GetColor(color, timer) != p) {
						p = GetColor(color, timer);
					}
					else {
						p = WHITE;
					}
					color = "";
				}
			}
			else if (!skipping) {
				offX = 0; offY = 0;
				sH = sH; sW = charWidth(c);
				sY = 0; sX = 0;
				prnt = true;
				if (c >= 48 && c <= 57) { //Numbers
					sY = sH + 1;
					sX = ((int)c - 48) * (sW+1);
				}
				else if (c >= 65 && c <= 90) { //Letters, uppercase(?)
					sY = 0;
					sX = ((int)c - 65) * (sW+1);
				}
				else if (c >= 97 && c <= 122) { //Letters again, lowercase(?)
					sY = 0;
					sX = ((int)c - 97) * (sW+1);
				}
				else {
					sY = 17;
					switch (c) {
					case '"': case '\'': sX = 1; sY = 21; sW = 1; sH = 3; break;
					case '.': sX = 118; sW = 1; break;
					case ',': sX = 1; sW = 1; offY = 1; break;
					case '>': sX = 6; break;
					case '?': sX = 18; break;
					case '!': sX = 14; sW = 1; break;
					case '+': sX = 24; break;
					case '-': sX = 30; break;
					case '$': sX = 36; break;
					case ':': sX = 44; sW = 1; break;
					case '<': sX = 84; break;
					case '[': sX = 48; sW = 3; break;
					case ']': sX = 56; sW = 3; break;
					case '#': sX = 60; break;
					case '(': sX = 66; break;
					case ')': sX = 72; break;
					case '=': sX = 78; break;
					case '/': sX = 92; break;
					case '%': sX = 98; break;
					case '|': sX = 60; sW = 3; break;
					case ';': sX = 89; sW = 2; break;
					case ' ': sW = 2; prnt = false;
					case '\n': prnt = false;
					default: prnt = false;
					}
				}
				if (prnt) {
					DrawPartialDecal(olc::vf2d(drawX + offX * scale, drawY + offY * scale), olc::vf2d(sW * scale, sH * scale), font, olc::vf2d(sX, sY), olc::vf2d(sW, sH), p);
				}
				if (c != '\n') {
					drawX += scale * (sW + offX + 1);
				}
				if (c == ' ' || c == '\n') {
					if (splitIndexes.size() > splits && splitIndexes[splits] == spaces++) {
						drawX = x;
						splits++;
						if (center) {
							drawX -= (lineWidth[splits] / 2);
						}
						drawY += scale * (sH + 3);
					}
				}
			}
			else {
				color += c;
			}
		}
		return splits;
	}
	//Server Code---------------------------------------------------------------------------------
	void sendData(std::string s) {
		buffer.push_back(s);
	}

	std::vector<Tile> readRow(std::string in) {
		int which = 0;
		std::vector<Tile> row = {};
		Tile t;
		for (char c : in) {
			switch (which) {
			case 0: t.type = (TERRAIN)(c - '0'); break; //Type
			case 1: t.elev = (ELEVATION)(c - '0');  break; //Elevation
			case 2: t.forest = (FOREST)(c - '0'); break; //Forest

			case 3: t.owner = (c - '0'); break; //Owner
			case 4: t.building = (c - '0'); break; //Building
			case 5: t.HP = (c - '0'); break; //Building HP

			case 6: t.unit.owner = (c - '0'); break; //U Owner
			case 7: t.unit.type = (c - '0'); break; //U Type
			case 8: t.unit.HP = (c - '0'); break; //U HP
			case 9: t.unit.MP = (c - '0'); break; //U MP
			case 10: t.unit.gold = (c - '0'); //U Gold
				row.push_back(t); t = Tile(); if (t.elev < FLAT) {
					if (t.type >= GRASS) { t.elev = FLAT; }
				}
				which = -1; break;
			}
			which++;
		}
		return row;
	}

	std::string sendPolicies() {
		std::string s = "p";
		for (int i = 0; i < NUM_POLICIES; i++) {
			s += '0' + p.policies[i];
		}
		return s;
	}

	std::string sendTile(int x, int y) {
		std::string s = "t";
		//First the Coordinates
		s += to_str(x) + ".";
		s += to_str(y) + ".";
		//Now the Building Data
		s += '0' + map[y][x].type;
		s += '0' + map[y][x].elev;
		s += '0' + map[y][x].forest;

		return s;
	}

	std::string sendUnit(int x, int y) {
		std::string s = "";
		//First the Coordinates
		s += to_str(x) + ".";
		s += to_str(y) + ".";
		//Now the Unit Data
		s += '0' + map[y][x].unit.owner;
		s += '0' + map[y][x].unit.type;
		s += '0' + map[y][x].unit.HP;
		s += '0' + map[y][x].unit.MP;
		s += '0' + map[y][x].unit.gold;

		return s;
	}
	std::string sendBuilding(int x, int y) {
		std::string s = "";
		//First the Coordinates
		s += to_str(x) + ".";
		s += to_str(y) + ".";
		//Now the Building Data
		s += '0' + map[y][x].owner;
		s += '0' + map[y][x].building;
		s += '0' + map[y][x].HP;

		return s;
	}
	std::string requestRow(int index) {
		std::string s = "r";
		s += to_str(index) + ".";
		std::cout << "Requesting: " << s << std::endl;
		return s;
	}


	void unitChange(std::string s) {
		if (s == "") {
			return;
		}
		int x = readInt(s);
		int y = readInt(s);

		map[y][x].unit.owner = s[0] - '0';
		map[y][x].unit.type = s[1] - '0';
		map[y][x].unit.HP = s[2] - '0';
		map[y][x].unit.MP = s[3] - '0';
		map[y][x].unit.gold = s[4] - '0';

		unitChange(s.substr(5));
	}
	void buildingChange(std::string s) {
		if (s == "") {
			return;
		}
		int x = readInt(s);
		int y = readInt(s);

		map[y][x].owner = s[0] - '0';
		map[y][x].building = s[1] - '0';
		map[y][x].HP = s[2] - '0';

		if (map[y][x].forest != NONE && map[y][x].building > 0) {
			map[y][x].forest = NONE;
		}

		buildingChange(s.substr(3));
	}

	void tileChange(std::string s) {
		int x = readInt(s);
		int y = readInt(s);

		map[y][x].type = (TERRAIN)(s[0] - '0');
		map[y][x].elev = (ELEVATION)(s[1] - '0');
		map[y][x].forest = (FOREST)(s[2] - '0');
	}

	void serverInit() {
		//Set Up Server--------------------------------------
		//std::cout << "IP: ";
		//std::cin >> IP;
		//std::cout << "Port: ";
		//std::cin >> port;
		client_join();
		std::thread cListA(&Terrain::client_listen, this);
		std::thread cListB(&Terrain::client_send, this);
		cListA.detach();
		cListB.detach();
		//-----------------------------------------------------
	}
	void client_join() {
		WSAData data;
		WORD ver = MAKEWORD(2, 2);
		WSAStartup(ver, &data);

		sock = socket(AF_INET, SOCK_STREAM, 0);

		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(port);
		inet_pton(AF_INET, IP.c_str(), &hint.sin_addr);

		int connection = connect(sock, (sockaddr*)&hint, sizeof(hint));
		if (connection == SOCKET_ERROR)	{
			std::cout << "Can't connect to server, Err #" << WSAGetLastError() << std::endl;
			closesocket(sock);
			WSACleanup();
		}
	}
	void client_send() {
		int target = 0;
		while (connected) {
			if (buffer.size() > 0) {
				if (timer == target) {
					std::string s = buffer.front();
					send(sock, s.c_str(), s.size() + 1, 0);
					target = (timer + 5) % 500;
					buffer.pop_front();
				}
			}
			else {
				target = (timer + 5) % 500;
			}
		}
		closesocket(sock);
		WSACleanup();
	}
	void client_listen() {
		char buf[600];
		while (connected) {
			int bytesReceived = recv(sock, buf, 600, 0);
			if (bytesReceived > 1) {
				//std::cout << "Server: " << buf << std::endl;
				std::string msg(buf);
				char c = msg[0];
				char context = msg[1] - '0';
				char sender = msg[2] - '0';
				static int num = 0;
				bool getMessage = (sender == 0) || (context == 0 && sender == p.turn) || (context == 1 && sender != p.turn);

				if (p.turn == 0) {
					if (c == 't') { //Assign Player Turn
						std::cout << "My turn is now " << '0' + sender << std::endl;
						p.turn = sender;
					}
					num++;
				}
				else if (getMessage) {
					std::cout << "Server: " << num++ << " " << c << std::endl;
					msg = msg.substr(3);
					
					if (c == 'r') { //Get Row
						if (map.size() > 0) {
							int index = readInt(msg);
							std::cout << "Received Row: " << index << std::endl;
							std::vector<Tile> row = readRow(msg);
							map[index] = row;
						}
					}
					else if (c == 'm') { //Map Size
						std::cout << "Server: " << num << std::endl;
						MAPSIZE = std::stoi(msg);
						if (!drawMap) {
							xView = MAPSIZE / 2;
							yView = MAPSIZE / 2;
						}
						if (map.size() == 0) {
							map.resize(MAPSIZE);
							MODI = std::max(.4, (MAPSIZE) / (1000.0));
						}
						else {
							verify_map = true;
						}
					}
					else if (c == 'u') { //Unit Change
						unitChange(msg);
					}
					else if (c == 'b') { //Building/Owner Change
						buildingChange(msg);
					}
					else if (c == 'g') { //Tile Type Changed - prob not used
						tileChange(msg);
					}
					else if (c == 'e') { //Set Turn
						turn = std::stoi(msg);
						if (turn == p.turn && drawMap) {
							startTurn();
							playSound(CHIMES);
						}
					}
					else if (c == 'p') { //policies
						int p_turn = sender;
						while (players.size() < p_turn) {
							players.push_back(Player());
						}
						std::cout << "Received msg: " << msg << std::endl;
						for (int i = 0; i < min(NUM_POLICIES, msg.length()); i++) {
							players[p_turn].policies[i] = (msg[i] - '0');
						}
					}
					else if (c == 'G') { //Set Gold
						p.gold = std::stoi(msg);
					}
				}
				ZeroMemory(buf, 600);
			}
			else {
				closesocket(sock);
				WSACleanup();
				exit(1);
			}
		}
	}
	//--------------------------------------------------------------------------------------------
	
};


int main() {
	//ShowWindow(GetConsoleWindow(), SW_HIDE);
	std::cout << "Enter an IP: " << std::endl;
	std::cin >> IP;
	std::cout << "Enter a Port : " << std::endl;
	std::cin >> port;
	Terrain generator;
	if (generator.Construct(1920 / 2, 1080 / 2, 1, 1, true, true)) {
		generator.Start();
	}
	//audio.deinit();
	ma_engine_uninit(&audio);
	//System("pause");
	return 0;
};



