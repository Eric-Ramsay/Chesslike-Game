#define OLC_PGE_APPLICATION
#define NOMINMAX
#define MINIAUDIO_IMPLEMENTATION

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
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
std::string IP = "192.168.1.140"; // "70.136.29.184";
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
	olc::Decal* buildings;
	 
	//------------------------------------------------------

	enum SFX {
		BOW_DRAW, BOW_FIRE, KILL, ATTACK, BUILD, UNIT_PLACE, CHIMES
	};
	std::vector<const char *> music = { "./Sounds/song1.mp3", "./Sounds/song2.mp3" };
	std::vector<std::string> sfx = {
		"./Sounds/bow_draw.mp3",
		"./Sounds/bow_fire.mp3",
		"./Sounds/kill.mp3",
		"./Sounds/attack.mp3",
		"./Sounds/build.mp3",
		"./Sounds/unit_place.mp3",
		"./Sounds/chimes.mp3",
	};
	bool audio_init = false;
	int music_index = 0;

	//Audio Manager
	void handleAudio() {
		static bool started = false;
		if (!audio_init) {
			audio_init = true;
			ma_engine_init(NULL, &audio);
		}
		else if (drawMap) {
			if (audio.inlinedSoundCount == 0) {
				ma_engine_play_sound(&audio, music[music_index], NULL);
				music_index++;
			}
		}
	}
	void playSound(SFX s) {
		if (audio.inlinedSoundCount < 5) {
			ma_engine_play_sound(&audio, sfx[s], NULL);
		}
	}

	//I should probably have a function to play audio, that way i can keep stuff clean
	
	//-------------------
	
	//PLAYER
	Player p;

	//Map Making Variables
	bool MM_active = false;
	int MM_selected = 0;
	std::deque<Tile> MM_Tile = {Tile(), Tile(), Tile()};

	//Game Loop Variables
	float fTargetFrameTime = 1.0f / 60.0; // Virtual FPS of 60fps
	float fAccumulatedTime = 0.0f;
	int timer = 0;
	int orbits = 0;

	//Used for Dragging things when buying units/buildings, or moving units on the map
	C tS = C(0, 0);
	C tH = C(0, 0);
	C drag = C(0, 0);
	C crown = C(0, 0);
	bool dragging_crown = false;
	bool dragging_unit = false;
	bool buying_unit = false;
	bool buying_building = false;

	//List of units
	std::vector<UI_Unit> units;

	//General UI variables
	bool unit_UI = false;
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
	int zoom = 32;

	//UI bounding boxes, so that player doesnt accidentally click behind UI 
	Box unit_bounds = Box(2, 4 + 15 * 3, 76 * 3, 129 * 3);
	Box building_bounds = Box(2, 6 + 144 * 3, 105 * 3, 34 * 3);
	Box unit_sel_bounds = Box(ScreenWidth() - (2 + 31*3), 2, 15*3, 15*3);
	Box building_sel_bounds = Box(ScreenWidth() - (2 + 15 * 3), 2, 15*3, 15*3);
	Box gold_bounds = Box(2, 2, 43 * 3, 15 * 3);
	Box pop_bounds = Box(2 + 44 * 3, 2, 43 * 3, 15 * 3);
	Box commander_bounds = Box(ScreenWidth() - (2 + 17 * 3), (2 + 17 * 3), 17 * 3, 24 * 3);


public:

	bool OnUserCreate() override { //Runs once when game starts	
		srand(time(NULL));

		//Initialize Sprites and Decals
		flat_s = new olc::Sprite("./Sprites/flat.png"); flat = new olc::Decal(flat_s);
		terrain_s = new olc::Sprite("./Sprites/terrain types.png");  terrain = new olc::Decal(terrain_s);
		icons_s = new olc::Sprite("./Sprites/icons.png"); icons = new olc::Decal(icons_s);
		buildings_s = new olc::Sprite("./Sprites/buildings.png"); buildings = new olc::Decal(buildings_s);
		font_s = new olc::Sprite("./Sprites/font.png");	font = new olc::Decal(font_s);
		armySprites_s = new olc::Sprite("./Sprites/units.png"); armySprites = new olc::Decal(armySprites_s);

		//Initialize units
		units = initUnits();

		unit_sel_bounds = Box(ScreenWidth() - (2 + 31 * 3), 2, 15 * 3, 15 * 3);
		building_sel_bounds = Box(ScreenWidth() - (2 + 15 * 3), 2, 15 * 3, 15 * 3);
		commander_bounds = Box(ScreenWidth() - (2 + 17 * 3), (2 + 17 * 3), 17 * 3, 24 * 3);

		Clear(olc::BLANK);
		serverInit();
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
			timer++;
			if (timer == 0) {
				orbits++;
				if (!drawMap && orbits > 2) {
					sendData("m");
					orbits = 0;
				}
			}
			if (timer == 500) {
				timer = 0;
			}
			if (!GetKey(olc::Key::SHIFT).bHeld) {
				if (GetKey(olc::Key::LEFT).bHeld || GetKey(olc::Key::A).bHeld) { //Pan Left
					dXView -= zoom / 4;
					if (zoom < 16) {
						xView = safeC(xView - 1);
					}
				}
				else if (GetKey(olc::Key::RIGHT).bHeld || GetKey(olc::Key::D).bHeld) { //Pan Right
					dXView += zoom / 4;
					if (zoom < 16) {
						xView = safeC(xView + 1);
					}
				}
				if (GetKey(olc::Key::UP).bHeld || GetKey(olc::Key::W).bHeld) { //Pan Up
					dYView -= zoom / 4;
					if (zoom < 16) {
						yView = safeC(yView - 1);
					}
				}
				else if (GetKey(olc::Key::DOWN).bHeld || GetKey(olc::Key::S).bHeld) { //Pan Down
					dYView += zoom / 4;
					if (zoom < 16) {
						yView = safeC(yView + 1);
					}
				}
				else if (GetKey(olc::Key::X).bHeld || GetMouseWheel() < 0) {
					if (zoom >= ((ScreenHeight() / MAPSIZE)) && zoom >= 2) {
						zoom--;
					}
				}
				else if (GetKey(olc::Key::Z).bHeld || GetMouseWheel() > 0) {
					if (zoom <= 63) {
						zoom++;
					}
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
		if (!MM_active) {
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
				sendData("n00");
				map = {};
				map.resize(MAPSIZE);
				drawMap = false;
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
			dYView = safeC(dYView, zoom);
			yView = safeC(yView - 1);
		}
		if (dXView >= zoom || zoom < 16)
		{
			dXView = safeC(dXView, zoom);
			xView = safeC(xView + 1);
		}
		if (dXView < 0 || zoom < 16)
		{
			dXView = safeC(dXView, zoom);
			xView = safeC(xView - 1);
		}

		//Check if a UI Element was clicked on
		int x = GetMouseX();
		int y = GetMouseY();
		bool UI_Click = false;

		if (drawMap) {
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
			if (unit_UI) {
				if (inBox(x, y, unit_bounds)) {
					UI_Click = true;
					if (GetMouse(0).bPressed) {
						unit_selected = min(6, (y - unit_bounds.y) / (unit_bounds.h / 7));
						buying_unit = true;
					}
				}
			}
			if (building_UI) {
				if (inBox(x, y, building_bounds)) {
					UI_Click = true;
					if (GetMouse(0).bPressed) {
						building_selected = min(5, (x - building_bounds.x) / (building_bounds.w / 6));
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
						if (turn == p.turn) {
							//Check if unit can be moved there
							float cost = moveCost(Spot(drag.x, drag.y), Spot(tH.x, tH.y), units[map[drag.y][drag.x].unit.type - 1].MP, map[drag.y][drag.x].unit.owner);
							if (map[tH.y][tH.x].unit.type == 0 && cost <= map[drag.y][drag.x].unit.MP) {
								//Move Unit to new Tile
								map[tH.y][tH.x].unit = map[drag.y][drag.x].unit;
								map[tH.y][tH.x].unit.MP -= cost;
								map[drag.y][drag.x].unit = Unit();
								sendData("u" + sendUnit(drag.x, drag.y) + sendUnit(tH.x, tH.y));
								for (int i = 0; i < p.units.size(); i++) {
									if (p.units[i].y == drag.y) {
										if (p.units[i].x == drag.x) {
											p.units[i] = tH;
										}
									}
								}
								tS = tH;
								if (map[tH.y][tH.x].building > 0) {
									if (map[tH.y][tH.x].owner != p.turn) {
										map[tH.y][tH.x].owner = p.turn;
										p.buildings.push_back(tH);
										sendData("b" + sendBuilding(tH.x, tH.y));
									}
								}
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
						if (turn == p.turn && canBuyBuilding(tH.x, tH.y, building_selected)) {
							p.buildings.push_back(tH);
							map[tH.y][tH.x].owner = p.turn;
							int cost = 0;
							if (building_selected == 0) { //Farm
								cost = 1;
								map[tH.y][tH.x].building = 1;
							}
							else if (building_selected == 1) { //House
								map[tH.y][tH.x].building = 5;
								cost = 3;
							}
							else if (building_selected == 2) { //bridge
								map[tH.y][tH.x].building = 8;
								cost = 6;
							}
							else if (building_selected == 3) { //workshop
								map[tH.y][tH.x].building = 9;
								cost = 6;
							}
							else if (building_selected == 4) { //Windmill
								map[tH.y][tH.x].building = 6;
								cost = 8;
							}
							else if (building_selected == 5) { //Fort
								map[tH.y][tH.x].building = 7;
								cost = 8;
							}
							p.gold -= cost;
							if (map[tH.y][tH.x].forest != NONE) {
								map[tH.y][tH.x].forest = NONE;
								p.gold -= 3;
							}
							playSound(BUILD);
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
								unit_selected = 8;
								p.started = true;
							}
							p.units.push_back(tH);
							map[tH.y][tH.x].owner = p.turn;
							map[tH.y][tH.x].unit.type = unit_selected + 1;
							map[tH.y][tH.x].unit.owner = p.turn;
							map[tH.y][tH.x].unit.HP = units[unit_selected].HP;
							map[tH.y][tH.x].unit.MP = 0;
							unit_selected = placeholder;
							sendData("u" + sendUnit(tH.x, tH.y));
							playSound(UNIT_PLACE);
						}
						buying_unit = false;
					}
				}
				if (attacking_unit) {
					if (!GetMouse(1).bHeld) {
						if (turn == p.turn && map[tH.y][tH.x].unit.type > 0 && map[tS.y][tS.x].unit.type > 0) {
							int x_dist = coord_dist(tH.x, tS.x);
							int y_dist = coord_dist(tH.y, tS.y);

							if (map[tS.y][tS.x].unit.MP > 0 && x_dist <= units[map[tS.y][tS.x].unit.type - 1].range && y_dist <= units[map[tS.y][tS.x].unit.type - 1].range) {
								if (map[tH.y][tH.x].unit.owner != p.turn && map[tS.y][tS.x].unit.owner == p.turn) {
									//Attack Unit!
									map[tH.y][tH.x].unit.HP -= units[map[tS.y][tS.x].unit.type - 1].ATK;
									//Unit Defends itself
									if (x_dist <= units[map[tH.y][tH.x].unit.type - 1].range && y_dist <= units[map[tH.y][tH.x].unit.type - 1].range) {
										if (map[tS.y][tS.x].elev < map[tH.y][tH.x].elev) {
											map[tS.y][tS.x].unit.HP -= 1;
										}
										if (map[tH.y][tH.x].building == 7) {
											map[tS.y][tS.x].unit.HP -= 1;
										}
										map[tS.y][tS.x].unit.HP -= units[map[tH.y][tH.x].unit.type - 1].DEF;
									}
									map[tS.y][tS.x].unit.MP = 0;
									if (map[tS.y][tS.x].unit.HP <= 0) { //Ur unit dies
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

									if (map[tH.y][tH.x].unit.HP <= 0) { //Enemy unit dies
										if (map[tH.y][tH.x].unit.type == 1) { //Earn a gold if the unit is a peasant
											p.gold++;
										}
										map[tH.y][tH.x].unit = Unit();
										if (units[map[tS.y][tS.x].unit.type - 1].range > 1) { //Bow Sound Effect
											playSound(BOW_FIRE);
										}
										else { //Sword Kill Sound effect
											playSound(KILL);
										}
									}
									else {
										if (units[map[tS.y][tS.x].unit.type - 1].range > 1) { //Bow Sound Effect
											playSound(BOW_FIRE);
										}
										else { //Sword Attack Sound effect
											playSound(ATTACK);
										}
									}

									sendData("u" + sendUnit(tH.x, tH.y) + sendUnit(tS.x, tS.y));
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
						if (turn == p.turn && map[tH.y][tH.x].unit.type > 0 && map[tH.y][tH.x].unit.type != 8 && map[tH.y][tH.x].unit.owner == p.turn) {
							if (p.gold >= (16 - units[map[tH.y][tH.x].unit.type - 1].cost)) {
								p.gold -= (16 - units[map[tH.y][tH.x].unit.type - 1].cost);
								map[tH.y][tH.x].unit.type = 8;
								map[tH.y][tH.x].unit.HP = 20;
								map[tH.y][tH.x].unit.MP = 0;
								sendData("u" + sendUnit(tH.x, tH.y));
							}
						}
						dragging_crown = false;
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
						if (units[map[tS.y][tS.x].unit.type - 1].range > 1) {
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
			DrawUI();
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
			
			DrawPartialDecal(olc::vf2d(dX - 80, dY + 32), buildings, olc::vf2d(32, 16 + 32 * ((timer / 5) % 6)), olc::vf2d(16, 32), olc::vf2d(2, 2));
			DrawPartialDecal(olc::vf2d(dX - 16, dY + 32), buildings, olc::vf2d(32, 16 + 32 * ((1 + (timer / 5)) % 6)), olc::vf2d(16, 32), olc::vf2d(2, 2));
			DrawPartialDecal(olc::vf2d(dX + 48, dY + 32), buildings, olc::vf2d(32, 16 + 32 * ((2  + (timer / 5)) % 6)), olc::vf2d(16, 32), olc::vf2d(2, 2));
		}
		
		SetDrawTarget(nullptr);
		return true;
	}

	//Whenever the server says that this player's turn is starting, run this function
	void startTurn() {
		harvest++; // This enables fish to be harvested again
		if (p.units.size() > 0) {
			p.gold++;
		}
		std::vector<C> unit_list = {};
		std::vector<C> building_list = {};
		std::string s = "u";
		for (int i = p.units.size()-1; i >= 0; i--) {
			C c = p.units[i];
			if (map[c.y][c.x].unit.owner == p.turn && map[c.y][c.x].unit.type > 0) {
				unit_list.push_back(c);
				map[c.y][c.x].unit.MP = units[map[c.y][c.x].unit.type - 1].MP; //Give unit its MP
				if (map[c.y][c.x].unit.HP < units[map[c.y][c.x].unit.type - 1].HP) { //Heal Unit
					if (map[c.y][c.x].building == 5) { //Heal on House
						map[c.y][c.x].unit.HP = min(units[map[c.y][c.x].unit.type - 1].HP, map[c.y][c.x].unit.HP + 2);
					}
					else if (map[c.y][c.x].building == 7) {
						map[c.y][c.x].unit.HP = min(units[map[c.y][c.x].unit.type - 1].HP, map[c.y][c.x].unit.HP + 5);
					}
				}
				s += sendUnit(c.x, c.y);
			}
		}
		p.units = unit_list;
		if (s != "u") {
			sendData(s);
		}

		p.max_pop = 6;
		s = "b";
		for (int i = p.buildings.size() - 1; i >= 0; i--) {
			C c = p.buildings[i];
			if (map[c.y][c.x].owner == p.turn) {
				building_list.push_back(c);
				if (map[c.y][c.x].building == 5 || map[c.y][c.x].building == 7) { //House
					p.max_pop += 3;
				}
				else if (map[c.y][c.x].building == 9) { //workshop
					p.gold++;
					p.max_pop -= 2;
				}
				else if (map[c.y][c.x].building >= 1 && map[c.y][c.x].building < 4) { //Farm
					map[c.y][c.x].building++;
					s += sendBuilding(c.x, c.y);
				}
				else if (map[c.y][c.x].building == 4 && map[c.y][c.x].unit.type > 0 && map[c.y][c.x].unit.owner == p.turn) { //Collect from farm
					int farm_yield = 1;
					map[c.y][c.x].building = 1;
					for (int a = -1; a < 2; a++) {
						for (int b = -1; b < 2; b++) {
							if (map[safeC(c.y + a)][safeC(c.x + b)].building == 6) { //Windmill
								farm_yield = 2;
							}
							if (map[safeC(c.y + a)][safeC(c.x + b)].type == RIVER || map[safeC(c.y + a)][safeC(c.x + b)].type == LAKE) {
								map[c.y][c.x].building = 2;
							}
						}
					}
					p.gold += farm_yield;
					s += sendBuilding(c.x, c.y);
				}
			}
		}
		if (s != "b") {
			sendData(s);
		}
		p.buildings = building_list;
	}

	//Runs when the player ends their turn
	void endTurn() {
		sendData("e");
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
		bool s = (!p.started);
		if (!s) {
			for (int i = -1; i < 2; i++) {
				for (int j = -1; j < 2; j++) {
					if (map[safeC(y + i)][safeC(x + j)].unit.type >= 8 && map[safeC(y + i)][safeC(x + j)].unit.owner == p.turn) {
						s = true;
						i = 2; j = 2;
					}
				}
			}
		}
		else { //Mayor, make sure no enemy units nearby
			for (int i = -3; i < 7; i++) {
				for (int j = -3; j < 7; j++) {
					if (map[safeC(y + i)][safeC(x + j)].unit.type > 0) {
						s = false;
						i = 7; j = 7;
					}
				}
			}
		}
		return s && p.units.size() < p.max_pop && map[y][x].elev >= FLAT && map[y][x].forest == NONE && map[y][x].elev != MOUNTAIN && map[y][x].unit.type == 0 && p.gold >= units[index].cost;
	}

	bool canBuyBuilding(int x, int y, int index) {
		bool s = false;
		int cost = 1; //farm
		if (index == 1) { //House
			cost = 3;
		}
		else if (index == 3) { //workshop
			cost = 6;
		}
		else if (index == 4) { //windmill
			cost = 8;
		}
		else if (index == 5) { //Fort
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
		if (!s) { //No adjacent peasant/mayor, no need to go further
			return false;
		}
		if (index == 2) { //bridge
			cost = 6;
			for (int i = -1; i < 2; i++) {
				for (int j = -1; j < 2; j++) {
					if (map[safeC(y + i)][safeC(x + j)].building == 8) {
						return false;
					}
				}
			}
			bool td = map[safeC(tH.y - 1)][tH.x].elev >= FLAT && map[safeC(tH.y + 1)][tH.x].elev >= FLAT && map[tH.y][safeC(tH.x - 1)].elev < FLAT && map[tH.y][safeC(tH.x + 1)].elev < FLAT;
			bool lr = map[tH.y][safeC(tH.x - 1)].elev >= FLAT && map[tH.y][safeC(tH.x + 1)].elev >= FLAT && map[safeC(tH.y - 1)][tH.x].elev < FLAT && map[safeC(tH.y + 1)][tH.x].elev < FLAT;
			s = s && td || lr;
		}
		s = s && (map[y][x].building == 0 || (GetKey(olc::Key::SHIFT).bHeld && map[y][x].owner == p.turn));
		s = s && ((index != 2 && map[y][x].elev >= FLAT) || (index == 2 && map[y][x].elev < FLAT));
		if (map[y][x].forest > FISH) {
			cost += 3;
			s = s && GetKey(olc::Key::SHIFT).bHeld;
		}
		return s && map[y][x].elev != MOUNTAIN && p.gold >= cost;
	}

	//Terrain movement cost
	float tCost(int x, int y, int turn) {
		Tile t = map[y][x];
		float price = 1.0;
		if ((t.elev == WATER && t.building != 8) || (t.unit.owner != turn && t.unit.type > 0)) {
			price = 999.0;
		}
		else if (t.elev == HILL) {
			price = 1.5;
		}
		else if (t.elev == MOUNTAIN) {
			price = 2.0;
		}
		if (t.forest != NONE) {
			price += .5;
		}
		return price;
	}

	//A star pathfinding, but only returns the cost to move to a spot
	float moveCost(Spot start, Spot end, int MP, int turn) {
		static unsigned int ind = 0;
		if (coord_dist(start.x, end.x) > MP || coord_dist(start.y, end.y) > MP || tCost(end.x, end.y, turn) > 50) {
			return 99;
		}
		ind++;

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
						consider.g = current.g + tCost(consider.x, consider.y, turn); //Distance to start
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

	//AI Functions----------------------------------------------------------------------
	void run_turn() {
		//This should really be an enum on the classes page
		int farm = 0; int house = 1; int windmill = 2; int fort = 3;

		int unit_values[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; //Relative strength of other players
		int unit_p[7] = { 0, 0, 0, 0, 0, 0, 0 }; //Priority for purchasing units
		int building_p[4] = { 25, 0, 0, 0 }; //Priority for buying buildings
		int b_costs[4] = { 1, 3, 8, 8 }; //Priority for buying buildings
		int my_val = 0;
		
		bool stop = false;
		bool has_commander = false;
		int runs = 0; // Used for preventing infinite loops in while statements

		int index = 0; // used for placing buildings/units
		C c(-1, -1);

		int bIndex = 0; //Tells what building is being bought
		//
		//I should make a function to initialize these two vectors, so that when say a commander is moved or a peasant is placed, the list of tiles updates
		//
		std::vector<C> can_place = {};
		std::vector<C> can_placeB = {};
		
		//Evaluate enemy threat level
		for (int i = 0; i < MAPSIZE; i++) {
			for (int j = 0; j < MAPSIZE; j++) {
				if (map[i][j].unit.owner > 0) {
					unit_values[map[i][j].unit.owner - 1] += units[map[i][j].unit.type - 1].cost;
					if (map[i][j].unit.owner != p.turn) {
						switch (map[i][j].unit.type) {
						case 1: unit_p[0] += 5; building_p[farm] += 5; building_p[windmill] += 5; break; //Peasant
						case 2: unit_p[2] += 5; unit_p[3] += 5; unit_p[4] += 5; break; //Spearmen
						case 3: unit_p[2] += 5; unit_p[5] += 5; break; //Archers
						case 4: unit_p[2] += 5; unit_p[3] += 5; break; //Heavy Inf
						case 5: unit_p[5] += .1; unit_p[2] -= 5; break; //CA
						case 6: unit_p[1] += 5; unit_p[4] -= 5; break; //Light Cav
						case 7: unit_p[3] += 5; unit_p[6] += 5; break; //Heavy Cav
						}
					}
					else {
						if (map[i][j].unit.type >= 8) { //Flag showing that the AI can place a commander, thus can buy units
							has_commander = true;
						}
						else if (map[i][j].unit.type == 1) {
							unit_p[0] -= 10;
						}
					}
				}
				else if (canBuy(j, i, 0)) { //Check if unit can be placed on this tile
					can_place.push_back(C(j, i));
				}
				if (canBuyBuilding(j, i, 0)) { //Check if building can be placed on this tile
					can_placeB.push_back(C(j, i));
				}
			}
		}
		if (!p.started) { //Place the commander somewhere
			runs = 0;
			while (!p.started && runs < 100) {
				stop = false;
				c = can_place[rand() % can_place.size()];
				for (int a = -1; a < 2; a++) {
					for (int b = -1; b < 2; b++) {
						if (map[safeC(a + c.y)][safeC(b + c.x)].elev != FLAT || map[safeC(a + c.y)][safeC(b + c.x)].forest != NONE) {
							stop = true;
							a = 2; b = 2;
						}
					}
				}
				if (!stop) {
					p.started = true;
					p.units.push_back(c);
					map[c.y][c.x].owner = p.turn;
					map[c.y][c.x].unit.type = 9;
					map[c.y][c.x].unit.owner = p.turn;
					map[c.y][c.x].unit.HP = units[8].HP;
					map[c.y][c.x].unit.MP = 0;
					sendData("u" + sendUnit(tH.x, tH.y));
					run_turn(); //Run this function again
				}
			}
		}
		else { //Do your turn as normal
			for (int i = 0; i < 7; i++) { //Give priority to unit purchasing based on what AI can buy
				if (p.gold >= units[i].cost) {
					unit_p[i] += 15 + i * 5;
				}
				else {
					unit_p[i] = 0;
				}
			}
			my_val = unit_values[p.turn - 1];
			int max = 0;
			for (int i = 0; i < 8; i++) {
				if (i != (p.turn - 1)) {
					if (unit_values[i] > max) {
						max = unit_values[i];
					}
				}
			}

			//Evaluate Buildings
			if (has_commander) {
				if (p.units.size() >= p.max_pop && p.gold >= 3) {
					building_p[house] = 90;
				}
				if (max < 2 * my_val && p.gold >= 8) {
					building_p[windmill] = 70;
				}
				runs = 0;
				while (p.gold > 0 && !stop) {
					runs++;
					//Decide what to do
					if (p.max_pop > p.units.size() && can_place.size() > 0) {//Buy Units
						for (int i = 6; i >= 0; i--) { 
							if (p.gold >= units[i].cost && rand() % 100 < unit_p[i]) {
								int ape = 0;
								do {
									index = rand() % can_place.size();
								} while (can_place[index].x == -1 && ape++ < can_place.size());
								if (ape < can_place.size()) { //Buy a unit
									c = can_place[index];
									p.gold -= units[i].cost;
									p.units.push_back(c);
									map[c.y][c.x].owner = p.turn;
									map[c.y][c.x].unit.type = i + 1;
									map[c.y][c.x].unit.owner = p.turn;
									map[c.y][c.x].unit.HP = units[i].HP;
									map[c.y][c.x].unit.MP = 0;
									sendData("u" + sendUnit(c.x, c.y));
									can_place[index].x = -1;
									unit_p[i] -= 10;
									i = 6;
								}
							}
						}
					}
					//Buy Buildings
					if (can_placeB.size() > 0) {
						for (int i = 0; i < 4; i++) { //Buy buildings
							index = -1;
							bIndex = i;
							if (p.gold >= b_costs[i] && rand() % 100 < building_p[i]) {
								//I want to build this building, find a tile to build it on
								int ape = 0;
								do {
									index = rand() % can_placeB.size();
									c = can_placeB[index];
									if (c.x != -1) {
										if (i != farm && i != fort) { //Windmill or house
											stop = (i != windmill);
											for (int a = -1; a < 2; a++) {
												for (int b = -1; b < 2; b++) {
													if (map[safeC(a + c.y)][safeC(b + c.x)].elev != FLAT || map[safeC(a + c.y)][safeC(b + c.x)].forest != NONE) {
														stop = (i == windmill);
														a = 2; b = 2;
													}
												}
											}
											if (stop) {
												can_placeB[index].x = -1;
												index = -1;
											}
											else {
												bIndex = i;
											}
										}
									}
								} while ((index == -1 || can_placeB[index].x == -1) && ape++ < can_placeB.size());
								if (ape >= can_placeB.size()) {
									index = -1;
								}
							}
							if (index > -1 && bIndex > -1) {
								c = can_placeB[index];
								if (bIndex == 0) { //Farm
									p.gold -= 1;
									map[c.y][c.x].building = 1;
								}
								else if (bIndex == 1) { //House
									map[c.y][c.x].building = 5;
									p.gold -= 3;
								}
								else if (bIndex == 2) { //Windmill
									map[c.y][c.x].building = 6;
									p.gold -= 8;
								}
								else if (bIndex == 3) { //Fort
									map[c.y][c.x].building = 7;
									p.gold -= 8;
								}
								p.buildings.push_back(c);
								map[c.y][c.x].owner = p.turn;
								sendData("b" + sendBuilding(c.x, c.y));
							}
						}
					}
					
					if (runs >= 10 || (p.gold <= 4 && rand() % 10 == 0)) {
						stop = true;
					}
				}
			}
			else {
				for (int i = p.units.size() - 1; i >= 0; i--) {
					C c = p.units[i];
					if (map[c.y][c.x].unit.owner == p.turn) { //Promote Commander
						if (p.gold >= (16 - units[map[c.y][c.x].unit.type - 1].cost)) {
							p.gold -= (16 - units[map[c.y][c.x].unit.type - 1].cost);
							map[c.y][c.x].unit.type = 8;
							map[c.y][c.x].unit.HP = 20;
							map[c.y][c.x].unit.MP = 0;
							sendData("u" + sendUnit(c.x, c.y));
							i = p.units.size();
						}
					}
				}
			}
		}
	}
	//End AI--------------------------------------------------------------------------------


	//UI and Drawing-------------------------------------------------------------------------------
	void DrawUI() {
		int dX, dY;
		int scale = 2.0;
		if (MM_active) {
			scale = 3.0;
			int sX, sY;
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
			Print(to_str(p.gold), 2 + 27*scale, 2 + 8*scale, true, scale);
			
			//Population
			Draw(icons, 2 + 44*scale, 2, 0, 16, 43, 15, scale);
			Print(to_str(p.units.size()), 2 + 64*scale, 2 + 8*scale, true, scale); //Pop
			Print(to_str(p.max_pop), 2 + 79 * scale, 2 + 8 * scale, true, scale); //Max Pop

			//Buying Building
			Draw(icons, ScreenWidth() - (2 + 15*scale), 2, 80+building_UI*16, 0, 15, 15, scale);
			Draw(icons, ScreenWidth() - (2 + 15*scale) + 2 * scale, 2 + 2 * scale, 80, 16, 15, 15, scale);
			//Buying Unit
			Draw(icons, ScreenWidth() - (2 + 31 * scale), 2, 80 + unit_UI * 16, 0, 15, 15, scale);
			Draw(icons, ScreenWidth() - (2 + 31 * scale) + 2*scale, 2+2*scale, 96, 16, 15, 15, scale);
			
			//Player Turn and Current Turn
			Draw(icons, ScreenWidth() - 22, ScreenHeight() - 40, 32, 32, 10, 10, 2, colors[turn]);
			Draw(icons, ScreenWidth() - 22, ScreenHeight() - 62, 32, 32, 10, 10, 2, colors[p.turn]);

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

			if (unit_UI) { //Unit Purchasing
				dX = 2;
				dY = 4 + 15 * scale;
				Draw(icons, dX, dY, 0, 48, 76, 129, scale);
				for (int i = 0; i < units.size() - 2; i++) {
					UI_Unit u = units[i];
					Draw(armySprites, dX + 2 * scale, dY + 2 * scale, u.sX, 15 + u.sY, 16, 17, scale);
					//Print Cost
					Print(to_str(u.cost), dX + 36 * scale, dY + 7 * scale, true, scale);
					//HP
					Print(to_str(u.HP), dX + 36 * scale, dY + 15 * scale, true, scale);
					//ATK
					Print(to_str(u.ATK), dX + 52 * scale, dY + 3 * scale, false, scale);
					//DEF
					Print(to_str(u.DEF), dX + 52 * scale, dY + 11 * scale, false, scale);
					//RANGE
					Print(to_str(u.range), dX + 68 * scale, dY + 3 * scale, false, scale);
					//MP
					Print(to_str(u.MP), dX + 68 * scale, dY + 11 * scale, false, scale);
					if (i == unit_selected) {
						DrawBox(dX+scale, dY+scale, 73*scale, 18*scale, Gold, scale);
					}

					dY += 18*scale;
				}
			}
			if (building_UI) { //building purchasing
				dX = 2;
				dY = 6 + 144 * scale;
				Draw(buildings, dX, dY, 0, 222, 105, 34, scale);
				DrawBox(dX + scale + (17 * building_selected * scale), dY + scale, 17 * scale, 31 * scale, Gold, scale);
			}
		}
		//Off 16 zoom warning
		if (zoom > 16 && zoom % 16 != 0) {
			Draw(icons, ScreenWidth() - 18, ScreenHeight() - 18, 112, 176, 16, 16);
		}
		//FPS warning
		if (GetFPS() < 58) {
			Draw(icons, ScreenWidth() - 36, ScreenHeight() - 18, 96, 176, 16, 16);
		}
	}

	void Draw(olc::Decal* d, int x, int y, int sX, int sY, int sizeX, int sizeY, float scale = 1, olc::Pixel ape = olc::WHITE) {
		DrawPartialDecal(olc::vf2d(x, y), d, olc::vf2d(sX, sY), olc::vf2d(sizeX, sizeY),  olc::vf2d(scale, scale), ape);
	}
	//void PixelGameEngine::DrawPartialRotatedDecal(const olc::vf2d& pos, olc::Decal* decal, const float fAngle, const olc::vf2d& center, const olc::vf2d& source_pos, const olc::vf2d& source_size, const olc::vf2d& scale, const olc::Pixel& tint)
	void rDraw(olc::Decal* d, float angle, int x, int y, int sX, int sY, int sizeX, int sizeY, float scale = 1, olc::Pixel ape = olc::WHITE) {
		DrawPartialRotatedDecal(olc::vf2d(x, y), d, angle, olc::vf2d(sizeX / 2, sizeY / 2), olc::vf2d(sX, sY), olc::vf2d(sizeX, sizeY), olc::vf2d(scale, scale), ape);
	}

	//Look up "hammie one hour" when you feel sad (1)

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
				////Print(to_str(temp), (ScreenWidth() / 2 + ((j * zoom) - dXView)) + zoom/2, (ScreenHeight() / 2 + ((i * zoom) - dYView)) + zoom/2, true, zoom / 16.0);
			}
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
			if (map[y][x].unit.type == 0) {
				dragging_unit = false;
				return;
			}
			dY -= 7 * z;
			sX = units[map[y][x].unit.type - 1].sX;
			sY = units[map[y][x].unit.type - 1].sY;
			//  x  y sX  sY  w   h  scale
			DrawPartialDecal(olc::vf2d(dX, dY - zoom), armySprites, olc::vf2d(sX, sY), olc::vf2d(16, 32), olc::vf2d(z, z));
			DrawPartialDecal(olc::vf2d(dX, dY - zoom), armySprites, olc::vf2d(sX, sY + 32), olc::vf2d(16, 32), olc::vf2d(z, z), ap);

			int hp = 12 * ((float)map[y][x].unit.HP / units[map[y][x].unit.type - 1].HP);
			int mov = 12 * ((float)map[y][x].unit.MP / units[map[y][x].unit.type - 1].MP);

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
			Draw(armySprites, dX, dY - zoom, units[unit_selected].sX, 15 + units[unit_selected].sY, 16, 17, z, pix);
			DrawPartialDecal(olc::vf2d(dX, dY - zoom), armySprites, olc::vf2d(units[unit_selected].sX, units[unit_selected].sY + 32), olc::vf2d(16, 17), olc::vf2d(z, z), ap);
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
			if (building_selected == 0) { //farm
				sX = 0;
				sY = 112;
			}
			else if (building_selected == 1) { //house
				sX = 16;
			}
			else if (building_selected == 2) { //Bridge
				sX = 64;
				if (map[safeC(tH.y - 1)][tH.x].elev >= FLAT && map[safeC(tH.y + 1)][tH.x].elev >= FLAT) {
					sY += 32;
				}
			}
			else if (building_selected == 3) { //workshop
				sX = 80;
			}
			else if (building_selected == 4) { //Windmill
				sX = 32;
				sY += 32 * ((timer / 5) % 6);
			}
			else if (building_selected == 5) { //Fort
				sX = 48;
			}
			DrawPartialDecal(olc::vf2d(dX, dY - zoom), buildings, olc::vf2d(sX, sY), olc::vf2d(16, 32), olc::vf2d(z, z), pix);
			if (building_selected == 1) {
				DrawPartialDecal(olc::vf2d(dX, dY - zoom), buildings, olc::vf2d(sX, sY - 16), olc::vf2d(16, 16), olc::vf2d(z, z), ap);
			}
			else if (building_selected == 3) {
				DrawPartialDecal(olc::vf2d(dX, dY), buildings, olc::vf2d(sX, sY - 16), olc::vf2d(16, 16), olc::vf2d(z, z), ap);
			}
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
					Print(to_str((int)(dist / zoom)), ScreenWidth() / 2, 30, 2);

					DrawPartialRotatedDecal(olc::vf2d(sX + 8 * z, sY + 8 * z), icons, angle, olc::vf2d(6, 9), olc::vf2d(80, 80), olc::vf2d(12, 10), olc::vf2d(z, (dist/9)));
				}
			}
		}
	}

	void DrawElevation(int x, int y, int dX, int dY, float z) {
		int elev = map[y][x].elev;
		int sX = 0;
		int sY = 32;

		if (elev == MOUNTAIN) {
			sY += 16;
		}

		bool u = map[safeC(y - 1)][x].elev >= elev; // up * 8
		bool d = map[safeC(y + 1)][x].elev >= elev; // down * 4
		bool l = map[y][safeC(x - 1)].elev >= elev; // left * 2
		bool r = map[y][safeC(x + 1)].elev >= elev; // right * 1

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
			if (map[tS.y][tS.x].unit.owner == p.turn) {
				dotColor = colors[p.turn];
			}
			if (map[tS.y][tS.x].unit.owner != turn) {
				MP = units[map[tS.y][tS.x].unit.type - 1].MP;
			}
			float mCost = moveCost(Spot(tS.x, tS.y), Spot(x, y), MP, map[tS.y][tS.x].unit.owner);
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
			if (borders && map[y][x].owner != 0) { //DRAW COUNTRY BORDERS
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
			}
			if (map[y][x].elev >= HILL) {
				DrawElevation(x, y, dX, dY, z);
			}
			//Forest, Hills, Mountains
			if (map[y][x].elev >= HILL) { //Hill
				dY -= 3 * z;
			}
			if (map[y][x].building > 0) {
				sY = 16;
				if (map[y][x].building >= 1 && map[y][x].building <= 4) { //Farm
					sX = 0;
					sY += 32 * (map[y][x].building - 1);
				}
				if (map[y][x].building == 5) { //house
					sX = 16;
				}
				if (map[y][x].building == 6) { //Windmill
					sX = 32;
					sY += 32 * (((int)(randC(x, y) * 5) + timer / 8) % 6);
				}
				if (map[y][x].building == 7) { //Fort
					sX = 48;
				}
				if (map[y][x].building == 8) { //Bridge
					sX = 64;
					if (map[safeC(y - 1)][x].elev >= FLAT && map[safeC(y + 1)][x].elev >= FLAT) {
						sY += 32;
					}
				}
				if (map[y][x].building == 9) { //workshop
					sX = 80;
					sY += 32 * (((int)(randC(x, y) * 5) + timer / 8) % 6);
				}
				DrawPartialDecal(olc::vf2d(dX, dY - zom), buildings, olc::vf2d(sX, sY), olc::vf2d(16, 32), olc::vf2d(z, z));
				if (map[y][x].building == 5) { //Draw the flag on the house
					DrawPartialDecal(olc::vf2d(dX, dY - zom), buildings, olc::vf2d(sX, sY - 16), olc::vf2d(16, 16), olc::vf2d(z, z), colors[map[y][x].owner]);
				}
				else if (map[y][x].building == 9) {
					DrawPartialDecal(olc::vf2d(dX, dY), buildings, olc::vf2d(sX, 0), olc::vf2d(16, 16), olc::vf2d(z, z), colors[map[y][x].owner]);
				}
			}
			if (map[y][x].forest > FISH) {
				sY = 0;
				sX = forestX(map[y][x].forest);
				if ((x % 100) == timer/4 || (tH.y == y && tH.x == x)) {
					sX += 64;
				}
				DrawPartialDecal(olc::vf2d(dX, dY - zom), terrain, olc::vf2d(sX, sY), olc::vf2d(16, 32), olc::vf2d(z, z));
			}
			else if (map[y][x].forest == FISH && map[y][x].harvest != harvest) {
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
				if (map[y][x].building == 7 || map[y][x].building == 8) { //Fort or Bridge
					off = 6;
				}

				//  x  y sX  sY  w   h  scale
				DrawPartialDecal(olc::vf2d(dX, dY - (off*z + zom)), armySprites, olc::vf2d(sX, sY), olc::vf2d(16, 32), olc::vf2d(z, z));
				DrawPartialDecal(olc::vf2d(dX, dY - (off*z + zom)), armySprites, olc::vf2d(sX, sY+32), olc::vf2d(16, 32), olc::vf2d(z, z), ap);

				int hp = 12 * ((float)map[y][x].unit.HP / units[map[y][x].unit.type - 1].HP);
				int mov = 12 * ((float)map[y][x].unit.MP / units[map[y][x].unit.type - 1].MP);

				DrawPartialDecal(olc::vf2d(dX + 2 * z, (dY - zom) + (10-off) * z), flat, olc::vf2d(418, 0), olc::vf2d(1, 1), olc::vf2d(hp*z, z));
				DrawPartialDecal(olc::vf2d(dX + 2 * z, (dY - zom) + (12-off) * z), flat, olc::vf2d(420, 0), olc::vf2d(1, 1), olc::vf2d(mov*z, z));
			}
			//UI tile selection/tile hovered overlay
			if (tH.y == y && tH.x == x) {
				olc::Pixel pix = Gold;
				if (attacking_unit && map[tS.y][tS.x].unit.MP > 0) {
					if (coord_dist(tH.x, tS.x) <= units[map[tS.y][tS.x].unit.type - 1].range && coord_dist(tH.y, tS.y) <= units[map[tS.y][tS.x].unit.type - 1].range) {
						pix = olc::RED;
					}
				}
				DrawPartialDecal(olc::vf2d(dX, dY), flat, olc::vf2d(416, 16), olc::vf2d(16, 16), olc::vf2d(z, z), pix);
			}
			if (tS.y == y && tS.x == x) {
				DrawPartialDecal(olc::vf2d(dX, dY), flat, olc::vf2d(416, 16), olc::vf2d(16, 16), olc::vf2d(z, z), UI_LightPurple);
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
					case '"': case '\'': sX = 1; sY = 23; sW = 1; break;
					case '.': sX = 118; sW = 1; break;
					case ',': sX = 2; sW = 1; offY = 1; break;
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

			case 5: t.unit.owner = (c - '0'); break; //U Owner
			case 6: t.unit.type = (c - '0'); break; //U Type
			case 7: t.unit.HP = (c - '0'); break; //U HP
			case 8: t.unit.MP = (c - '0');
				row.push_back(t); t = Tile(); if (t.elev < FLAT) {
					if (t.type >= GRASS) { t.elev = FLAT; }
				}
				which = -1; break; //U MP
			}
			which++;
		}
		return row;
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

		unitChange(s.substr(4));
	}
	void buildingChange(std::string s) {
		if (s == "") {
			return;
		}
		int x = readInt(s);
		int y = readInt(s);

		map[y][x].owner = s[0] - '0';
		map[y][x].building = s[1] - '0';

		if (map[y][x].forest != NONE && map[y][x].building > 0) {
			map[y][x].forest = NONE;
		}

		buildingChange(s.substr(2));
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
		char buf[1200];
		while (connected) {
			int bytesReceived = recv(sock, buf, 1200, 0);
			if (bytesReceived > 1) {
				orbits = 0;
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
						xView = MAPSIZE / 2;
						yView = MAPSIZE / 2;
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
						if (turn == p.turn) {
							startTurn();
							//if (audio.getActiveVoiceCount() < 3) {
							//	audio.play(wav_chimes);
							//}
						}
					}
					else if (c == 'G') { //Set Gold
						p.gold = std::stoi(msg);
					}
				}
				ZeroMemory(buf, 1200);
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
	Terrain generator;
	if (generator.Construct(1920 / 2, 1080 / 2, 1, 1, true, true)) {
		generator.Start();
	}

	//audio.deinit();
	ma_engine_uninit(&audio);
	//System("pause");
	return 0;
};



