//Functions related to an AI player

void run_turn(std::vector<std::vector<Tile>>& map) {
	int unit_values[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int my_val = 0;
	int build_farm = 25;
	int build_house = 0;
	int build_windmill = 0;
	int build_fort = 0;
	int unit_priority[7] = { 0, 0, 0, 0, 0, 0, 0 };
	bool stop = false;
	bool has_commander = false;
	int runs = 0; // 
	std::vector<C> can_place = {};
	std::vector<C> can_placeB = {};
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
				else if (map[i][j].unit.type >= 8) { //Flag showing that the AI can place a commander, thus can buy units
					has_commander = true;
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

	}
	else { //Do your turn as normal
		for (int i = 0; i < unit_priorty.size(); i++) { //Give priority to unit purchasing based on what AI can buy
			if (p.gold >= units[i].cost) {
				unit_priorty[i] += 15 + i * 5;
			}
			else {
				unit_priorty[i] = 0;
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
			if (p.pop >= p.max_pop && p.gold >= 3) {
				build_house = 100;
			}
			if (max < 2 * my_val && p.gold >= 8) {
				build_windmill = 70;
			}
			runs = 0;
			while (p.gold > 0 && !stop) {
				runs++;
				//Decide what to do
				for (int i = 6; i >= 0; i--) {
					if (p.gold >= units[i].cost && rand() % 100 < unit_priority[i]) {

					}
				}
				if (p.gold <= 4 && runs > 3 && rand() % 10 == 0) {
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
						sendData(sendUnit(c.x, c.y));
						i = p.units.size();
					}
				}
			}
		}
	}
}