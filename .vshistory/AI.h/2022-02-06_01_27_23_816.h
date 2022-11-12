//Functions related to an AI player

void run_turn(std::vector<std::vector<Tile>>& map, Player& p) {
	if (!p.started) { //Look for commander location

	}
	else { //I've started
		int unit_values[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		int my_val = 0;
		int build_farm = 25;
		int build_house = 0;
		int build_windmill = 0;
		int build_fort = 0;
		int unit_priority[7] = { 0, 0, 0, 0, 0, 0, 0 };
		bool stop = false;
		for (int i = 0; i < unit_priorty.size(); i++) {
			if (p.gold > unit_priority[i]) {
				unit_priorty[i] += 15 + i * 5;
			}
		}
		//Evaluate enemy threat level
		for (int i = 0; i < MAPSIZE; i++) {
			for (int j = 0; j < MAPSIZE; j++) {
				if (map[i][j].unit.owner > 0) {
					unit_values[map[i][j].unit.owner-1] += units[map[i][j]].unit.type - 1].cost
					if (map[i][j].unit.owner != p.turn) {
						switch (map[i][j].unit.type) {
						case 1: unit_priority[0] += 5; build_farm += 5; build_windmill += 5; break; //Peasant
						case 2: unit_priorty[2] += 5; unit_priority[3] += 5; unit_priority[4] += 5; break; //Spearmen
						case 3: unit_priorty[2] += 5; unit_priority[5] += 5; break; //Archers
						case 4: unit_priority[2] += 5; unit_priority[3] += 5; break; //Heavy Inf
						case 5: unit_priority[5] += .1; unit_priority[2] -= 5; break; //CA
						case 6: unit_priority[1] += 5; unit_priority[4] -= 5; break; //Light Cav
						case 7: unit_priority[3] += 5; unit_priority[6] += 5; break; //Heavy Cav
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
			build_house = 100;
		}
		if (max < 2 * my_val && p.gold >= 8) {
			build_windmill = 70;
		}
		int runs = 0;
		while (p.gold > 0 && !stop) {
			runs++;
			if (p.gold <= 4 && runs > 3 && rand() % 10 == 0) {
				stop = true;
			}
			else {

			}
		}
	}
}