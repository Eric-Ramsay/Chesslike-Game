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
	if (connection == SOCKET_ERROR) {
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
//----------------------------------