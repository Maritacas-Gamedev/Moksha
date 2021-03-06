#include "map.h"

// GENERATION AND INITIALIZATION ----------------------------------------------------------

Map::Map(vector<Room*> receivedRooms, IObserver *observer) {
	this->observer = observer;

	// Gerar salas
	for (int i = 0; i < receivedRooms.size(); i++) {
		Node novo = Node(receivedRooms[i]);
		rooms.push_back(novo);
	}

	// Popular n?s com anexasIndex
	for (int i = 0; i < rooms.size(); i++) {
		// Colocar o index de cada sala anexa no Node
		for (int j = 0; j < rooms[i].getElement()->getAdjacentRoomCount(); j++)
			for (int k = 0; k < rooms.size(); k++)
				if (rooms[i].getElement()->getAdjacentRoomCodename(j) == rooms[k].getElement()->getCodename()) {
					rooms[i].addAdjacent(k);
					break;
				}

		loadRoom(receivedRooms[i]);
	}
};


void Map::loadRoom(Room *room) {
	vector<string> objectList = FileManager::getFileList("files/objects");
	vector<string*> objectNames = room->getObjectNames();

	// Procurar objetos na lista
	for (int i = 0; i < objectList.size(); i++) {
		FileDict fileObject = FileManager::readFromFile(objectList[i]);

		for (int j = 0; j < objectNames.size(); j++) {
			// Encontrado - Gerar objeto
			if ((fileObject.hasKey("codename") && (*objectNames[j]).compare(fileObject.getValue("codename")) == 0) ||
				                                  (*objectNames[j]).compare(fileObject.getValue("name")) == 0) {
				Object* newObject = new Object(fileObject);								// Generate object
				newObject->add(observer, j + room->getIndex()*MAX_OBJECT_COUNT);		// Generate callcard id
				room->addObject(newObject);												// Add object to room
				break;
			}
		}
	}
}


Object* Map::getObject(int id) {
	return rooms[id / MAX_OBJECT_COUNT].getElement()->getObject(id % MAX_OBJECT_COUNT);
}

// PATHFINDING -----------------------------------------------------------------------

queue<Room*> Map::optimalPath(Room *origin, Room *destination) {
	queue<Room*> path;
	clearVisited();

	// Edge case
	if (origin == destination)
		return path;

	// Find origin node
	queue<Node*> searching;
	stack<Node*> reverser;
	for (int i = 0; i < rooms.size(); i++) {
		if (rooms[i].getElement()->getCodename() == origin->getCodename()) {
			rooms[i].setVisited(1);
			searching.push(&rooms[i]);
			break;
		}
	}

	// Iterate through the adjacent nodes to the examined node
	while (!searching.empty()) {
		Node* checkedRoom = searching.front();
		for (int i = 0; i < checkedRoom->getAdjacents().size(); i++) {
			// Not examined yet - add to the search list and set the current node as its parent
			if (rooms[checkedRoom->getAdjacents()[i]].getVisited() == 0) {
				searching.push(&rooms[checkedRoom->getAdjacents()[i]]);			// Pushes it to the list
				rooms[checkedRoom->getAdjacents()[i]].setParent(checkedRoom);		// Sets the current node as the parent
				rooms[checkedRoom->getAdjacents()[i]].setVisited(1);			// Sets visited to 1

				// Target room found!
				if (rooms[checkedRoom->getAdjacents()[i]].getElement() == destination) {
					reverser.push(&rooms[checkedRoom->getAdjacents()[i]]);

					// Build reverse path
					while (reverser.top()->hasParent())
						reverser.push(reverser.top()->getParent());
					while (!reverser.empty()) {
						path.push(reverser.top()->getElement());
						reverser.pop();
					}

					path.pop();	// Removes the first node from the list (the origin itself)
					return path;
				}
			}
		}
		searching.pop();
	}

	return path;
};


queue<Room*> Map::breadthSearch(Room *origin) {
	queue<Room*> retorno;
	stack<int> st;
	clearVisited(); // Resets visited

	// Searches for the origin node and begins the search from it
	for (int i = 0; i < rooms.size(); i++) {
		if (rooms[i].getElement()->getCodename() == origin->getCodename()) {
			breadthSearchHelper(&retorno, st, i);
			break;
		}
	}

	retorno.pop(); // Removes the origin from the list
	return retorno;
}


void Map::breadthSearchHelper(queue<Room*> *retorno, stack<int> &st, int checkedRoom) {
	if (!st.empty())
		st.pop(); // Removes from the processing list

	if (rooms[checkedRoom].getVisited() == 0) {
		retorno->push(rooms[checkedRoom].getElement()); // Adds to return list

		// Set to visited
		rooms[checkedRoom].setVisited(1);

		// Puts all adjacent nodes in the processing list
		for (int i = 0; i < rooms[checkedRoom].getAdjacents().size(); i++) {
			st.push(rooms[checkedRoom].getAdjacents()[i]);
		}
	}

	// Continues processing
	if (!st.empty())
		breadthSearchHelper(retorno, st, st.top());
}


// GETTERS E HELPERS -----------------------------------------------------

bool Map::hasRoom(string codename) {
	for (int i = 0; i < rooms.size(); i++)
		if (rooms[i].getElement()->getCodename() == codename)
			return true;
	return false;
};

bool Map::hasRoomByName(string name) {
	for (int i = 0; i < rooms.size(); i++)
		if (rooms[i].getElement()->getName() == name)
			return true;
	return false;
};

Room* Map::getRoom(int index) {
	return rooms[index].getElement();
};

Room* Map::getRoom(string codename) {
	for (int i = 0; i < rooms.size(); i++)
		if (rooms[i].getElement()->getCodename() == codename)
			return getRoom(i);

	throw invalid_argument("There's no room with that name :(");
};

Room* Map::getRoomByName(string name) {
	for (int i = 0; i < rooms.size(); i++)
		if (rooms[i].getElement()->getName() == name)
			return getRoom(i);

	throw invalid_argument("There's no room with that name :(");
};

void Map::clearVisited() {
	for (int i = 0; i < rooms.size(); i++) {
		rooms[i].setVisited(0);
		rooms[i].setParent(&rooms[i]);
	}
}