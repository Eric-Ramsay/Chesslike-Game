#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H
#include "olcPixelGameEngine.h"
#include <thread>

int MAPSIZE = 1;
float MODI = std::max(.2, (MAPSIZE) / (2000.0));

//List of units
std::vector<UI_Unit> units;

const olc::Pixel GREY(133, 168, 178);
const olc::Pixel WHITE(229, 223, 210);
const olc::Pixel RED(128, 33, 44);
const olc::Pixel ORANGE(199, 115, 45);
const olc::Pixel YELLOW(246, 219, 83);
const olc::Pixel GREEN(104, 189, 44);
const olc::Pixel TEAL(0, 255, 255);
const olc::Pixel BLUE(25, 167, 255);
const olc::Pixel PURPLE(171, 164, 225);
const olc::Pixel BROWN(139, 76, 45);
const olc::Pixel PINK(237, 190, 190);
const olc::Pixel FISH(83, 167, 142);
const olc::Pixel BLACK(15, 18, 22);
const olc::Pixel Cold_Desert(190, 140, 96);

const int TEXT_W = 5;
const int TEXT_H = 7;

const olc::Pixel DARK_BROWN(50, 19, 0);
const olc::Pixel LIGHT_BROWN(111, 63, 0);
const olc::Pixel Gold(248, 223, 159);

const olc::Pixel UI_DarkGrey(57, 75, 74);
const olc::Pixel UI_LightGrey(86, 114, 113);
const olc::Pixel UI_PaleBlue(190, 214, 253);
const olc::Pixel UI_LightPurple(171, 164, 225);

const olc::Pixel Water(0, 74, 147);
const olc::Pixel River(0, 106, 187);
const olc::Pixel Lake(16, 138, 231);

const olc::Pixel Ice(255, 255, 255);
const olc::Pixel Tundra(139, 76, 31);
const olc::Pixel Steppe(193, 174, 124);
const olc::Pixel Grass(60, 143, 72);
const olc::Pixel Savanna(185, 142, 87);
const olc::Pixel Desert(185, 122, 67);
const olc::Pixel Bog(59, 70, 17);
const olc::Pixel Meadow(76, 182, 92);
const olc::Pixel Ocean(0, 67, 133);
const olc::Pixel Forest(34, 81, 41);

const olc::Pixel Hill(104, 189, 44);
const olc::Pixel Mountain(0, 0, 0);

#endif