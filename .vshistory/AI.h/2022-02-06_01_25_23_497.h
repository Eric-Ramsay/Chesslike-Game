//Functions related to an AI player

void run_turn(std::vector<std::vector<Tile>>& map, Player& p) {
	if (!p.started) { //Look for commander location

	}
	else { //I've started
		int unit_values[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		int my_val = 0;
		float build_farm = 0.25;
		float build_house = 0.0;
		float build_windmill = 0.0;
		float build_fort = 0.0;
		float unit_priority[7] = { 0, 0, 0, 0, 0, 0, 0 };
		bool stop = false;
		for (int i = 0; i < unit_priorty.size(); i++) {
			if (p.gold > unit_priority[i]) {
				unit_priorty[i] += .15 + i * .05;
			}
		}
		//Evaluate enemy threat level
		for (int i = 0; i < MAPSIZE; i++) {
			for (int j = 0; j < MAPSIZE; j++) {
				if (map[i][j].unit.owner > 0) {
					unit_values[map[i][j].unit.owner-1] += units[map[i][j]].unit.type - 1].cost
					if (map[i][j].unit.owner != p.turn) {
						switch (map[i][j].unit.type) {
						case 1: unit_priority[0] += .05; build_farm += .05; build_windmill += .05; break; //Peasant
						case 2: unit_priorty[2] += .05; unit_priority[3] += .05; unit_priority[4] += .05; break; //Spearmen
						case 3: unit_priorty[2] += .05; unit_priority[5] += .05; break; //Archers
						case 4: unit_priority[2] += .05; unit_priority[3] += .05; break; //Heavy Inf
						case 5: unit_priority[5] += .1; unit_priority[2] -= .05; break; //CA
						case 6: unit_priority[1] += .05; unit_priority[4] -= .05; break; //Light Cav
						case 7: unit_priority[3] += .05; unit_priority[6] += .05; break; //Heavy Cav
						}
					}
				}
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
		if (p.pop >= p.max_pop && p.gold >= 3) {
			build_house = 1.0;
		}
		if (max < 2 * my_val && p.gold >= 8) {
			build_windmill = .7
		}

		while (p.gold > 0 && !stop) {

		}
	}
}