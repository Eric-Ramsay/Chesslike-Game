//Functions related to an AI player

void run_turn(std::vector<std::vector<Tile>>& map, Player& p) {
	if (!p.started) { //Look for commander location

	}
	else { //I've started
		int unit_values[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		int my_val = 0;
		float build_house = 0.0;
		units.push_back(UI_Unit(2, 4, 1, 1, 2, 1)); //1) Peasant Levy
		units.push_back(UI_Unit(2, 8, 3, 4, 1, 2)); //2) Spearmen
		units.push_back(UI_Unit(3, 4, 4, 4, 3, 4)); //3) Archers
		units.push_back(UI_Unit(2, 15, 7, 7, 1, 5)); //4) Heavy Infantry
		units.push_back(UI_Unit(5, 6, 4, 3, 3, 5)); //5) Cav Archer
		units.push_back(UI_Unit(6, 12, 5, 4, 1, 5)); //6) Light Cavalry
		units.push_back(UI_Unit(4, 20, 9, 5, 1, 8)); //7) Heavy Cavalry
		float unit_priority[7] = { 0, 0, 0, 0, 0, 0, 0 };
		//Evaluate enemy threat level
		for (int i = 0; i < MAPSIZE; i++) {
			for (int j = 0; j < MAPSIZE; j++) {
				if (map[i][j].unit.owner > 0) {
					unit_values[map[i][j].unit.owner-1] += units[map[i][j]].unit.type - 1].cost
				}
			}
		}
		my_val = unit_values[p.turn - 1];
		for (int i = 0; i < 8; i++) {
			if (i != (p.turn - 1)) {

			}
		}
	}
}