#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H
#include "olcPixelGameEngine.h"
#include <thread>

int MAPSIZE = 200;
float MODI = std::max(.2, (MAPSIZE) / (2000.0));
const int NUM_POLICIES = 12;
std::vector<Player> players = {};

//List of units, buildings, and policies
std::vector<UI_Unit> units;
std::vector<UI_Building> buildings;
std::vector<Policy> policies;
std::vector<int> UI_Units = {};

const int TEXT_W = 5;
const int TEXT_H = 7;



#endif