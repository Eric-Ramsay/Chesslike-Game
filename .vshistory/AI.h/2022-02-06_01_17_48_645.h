//Functions related to an AI player

void run_turn(std::vector<std::vector<Tile>>& map, Player& p) {
	if (!p.started) { //Look for commander location

	}
	else { //I've started
		int unit_values[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		int my_val = 0;
		float build_house = 0.0;
		float build_windmill = 0.0;
		float build_fort = 0.0;
		float unit_priority[7] = { 0, 0, 0, 0, 0, 0, 0 };
		for (int i = 0; i < unit_priorty.size(); i++) {
			if (p.gold > unit_priority[i]) {
				unit_priorty[i] += .25;
			}
		}
		//Evaluate enemy threat level
		for (int i = 0; i < MAPSIZE; i++) {
			for (int j = 0; j < MAPSIZE; j++) {
				if (map[i][j].unit.owner > 0) {
					unit_values[map[i][j].unit.owner-1] += units[map[i][j]].unit.type - 1].cost
					if (map[i][j].unit.owner != p.turn) {
						switch (map[i][j].unit.type) {
						case 1: unit_priority[0] += .05; build_farm += .05; break; //Peasant
						case 2:
						case 3:
						case 4:
						case 5:
						case 6:
						case 7:
						}
					}
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