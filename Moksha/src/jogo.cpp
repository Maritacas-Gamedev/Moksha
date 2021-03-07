#include "jogo.h"
#include <iostream>

Jogo::Jogo() {
	player = Jogador();
	player.add(this, OBSERVER_OFFSET);
	characters.push_back(&player);

	Baxter* baxter = new Baxter(&mapa);
	baxter->add(this, OBSERVER_OFFSET + 1);
	characters.push_back(baxter);
	npcs.push_back(baxter);

	Hilda* hilda = new Hilda(&mapa);
	hilda->add(this, OBSERVER_OFFSET + 2);
	characters.push_back(hilda);
	npcs.push_back(hilda);

	Santos* santos = new Santos(&mapa);
	santos->add(this, OBSERVER_OFFSET + 3);
	characters.push_back(santos);
	npcs.push_back(santos);

	Jenna* jenna = new Jenna(&mapa);
	jenna->add(this, OBSERVER_OFFSET + 4);
	characters.push_back(jenna);
	npcs.push_back(jenna);
}


void Jogo::setup() {
	initializeGame();
	if (!loadGame()) 
		for (int i = 0; i < npcs.size(); i++)
			npcs[i]->setupPlans();
}


// SAVE/LOAD --------------------------------------
void Jogo::initializeGame() {
	// Gerar jogo
	time = 1;
	loop = 1;

	xml_document doc;
	if (!doc.load_file("files/base.xml"))
		throw invalid_argument("ERROR INITIALIZING GAME - BASE.XML COULD NOT BE FOUND");

	// Gerar mapa
	vector<string> salaLista = FileManager::getFileList("files/rooms");
	vector<Sala*> salas;

	for (int i = 0; i < salaLista.size(); i++) {
		FileDict fileSala = FileManager::readFromFile(salaLista[i]);

		Sala* room = new Sala(i, fileSala.getValue("name"), fileSala.getValue("text"),
			fileSala.getValues("adjacent"), fileSala.getValues("objects"));
		salas.push_back(room);
	}

	mapa = Mapa(salas, this);

	// Generate characters
	xml_node load_package = doc.child("GameData").child("Game").child("Characters");
	int i = 0;
	for (xml_node_iterator it = load_package.begin(); it != load_package.end(); ++it, i++) {
		string s = it->attribute("Room").value();
		Personagem* personagem = findCharacter(it->name());
		personagem->setCurrentRoom(mapa.getSala(s));
	}
}


bool Jogo::loadGame() {
	xml_document doc;
	if (!doc.load_file("files/save.xml"))
		return false;

	// Load game
	xml_node load_package = doc.child("GameData").child("Game");
	time = load_package.attribute("Time").as_int();
	loop = load_package.attribute("Loop").as_int();

	// Load map
	load_package = doc.child("GameData").child("Game").child("Map");
	for (xml_node_iterator it = load_package.begin(); it != load_package.end(); ++it) {
		Sala* estaSala = mapa.getSala(it->attribute("Name").value());
		estaSala->limparObjects();

		// Load objects
		vector<string> objectNames;
		xml_node objects = it->child("Objects");

		for (xml_node_iterator ait = objects.begin(); ait != objects.end(); ++ait)
			objectNames.push_back(ait->name());

		estaSala->setObjectNames(objectNames);
		mapa.carregarSala(estaSala);
	}

	// Load characters
	load_package = doc.child("GameData").child("Game").child("Characters");
	for (xml_node_iterator it = load_package.begin(); it != load_package.end(); ++it) {
		string s = it->attribute("Room").value();
		Personagem* thisCharacter = findCharacter(it->name());
		thisCharacter->setCurrentRoom(mapa.getSala(s));

		// Load inventory
		xml_node inventario = it->child("Inventory");
		for (xml_node_iterator ait = inventario.begin(); ait != inventario.end(); ++ait) {
			obtainObject(ait->name(), thisCharacter);
		}

		// Load atoms
		if (thisCharacter != &player) {
			((NPC*)thisCharacter)->setupPlans();
			xml_node atoms = it->child("Atoms");
			for (xml_node_iterator ait = atoms.begin(); ait != atoms.end(); ++ait) {
				string s = ait->attribute("value").value();
				((NPC*)thisCharacter)->setCondition(ait->name(), (s == "true"));
			}
		}
	}

	// Load conversations
	load_package = doc.child("GameData").child("Game").child("Conversations");
	for (xml_node_iterator it = load_package.begin(); it != load_package.end(); ++it) {
		conversations.push_back(Conversation( it->name(), it->attribute("room").value(), stoi(it->attribute("stage").value()) ));
	}

	return true;
}


void Jogo::saveGame() {
	xml_document doc;
	if (!doc.load_file("files/save.xml"))
		doc.load_file("files/base.xml");
	xml_node node = doc.child("GameData").child("Game");

	// Save game
	node.attribute("Time").set_value(to_string(time).c_str());
	node.attribute("Loop").set_value(to_string(loop).c_str());

	// Save map
	node = doc.child("GameData").child("Game").child("Map");
	for (xml_node_iterator it = node.begin(); it != node.end(); ++it) {
		Sala* thisSala = mapa.getSala(it->attribute("Name").value());

		// Save objects
		xml_node objects = it->child("Objects");
		objects.remove_children();
		for (int i = 0; i < thisSala->getObjectNames().size(); i++) {
			objects.append_child(thisSala->getObjectNames()[i].c_str());
		}
	}

	// Save characters
	node = doc.child("GameData").child("Game").child("Characters");
	for (xml_node_iterator it = node.begin(); it != node.end(); ++it) {
		Personagem* thisCharacter = findCharacter(it->name());
		it->attribute("Room").set_value(thisCharacter->getCurrentRoom()->getNome().c_str() );

		// Save inventory
		xml_node inventario = it->child("Inventory");
		inventario.remove_children();
		vector<Item> itemList = thisCharacter->getInventory();
		for (int i = 0; i < itemList.size(); i++)
			inventario.append_child(itemList[i].getNome().c_str());

		// Save atoms
		if (thisCharacter != &player) {
			xml_node atoms = it->child("Atoms");
			vector<string> atomList = ((NPC*)thisCharacter)->getAtomList();

			// Create and/or update each atom
			for (int i = 0; i < atomList.size(); i++) {
				if (!atoms.child(atomList[i].c_str())) {
					atoms.append_child(atomList[i].c_str());
					atoms.child(atomList[i].c_str()).append_attribute("value");
				}
				atoms.child(atomList[i].c_str()).attribute("value").set_value( thisCharacter->hasCondition(atomList[i]) );
			}
		}
	}

	// Save conversations
	node = doc.child("GameData").child("Game").child("Conversations");
	for (int i = 0; i < conversations.size(); i++) {
		string convoName = conversations[i].getName();

		if (!node.child(convoName.c_str())) {
			node.append_child(convoName.c_str());
			node.child(convoName.c_str()).append_attribute("stage");
			node.child(convoName.c_str()).append_attribute("room");
		}

		node.child(convoName.c_str()).attribute("stage").set_value(conversations[i].getStage());
		node.child(convoName.c_str()).attribute("room").set_value(conversations[i].getRoom().c_str());
	}

	doc.save_file("files/save.xml");
}


// UPDATE --------------------------------------------------------
void Jogo::update(int id) {
	// Objeto
	if (id < OBSERVER_OFFSET) {
		objectAction(mapa.getObject(id));
	}

	// Personagem
	else {
		characterAction(characters[id - OBSERVER_OFFSET]);
	}
}


void Jogo::objectAction(Object* object) {
	// Obtain
	switch (object->getNotifyID()) {
	case object->obter:
		Personagem *character = findCharacter(object->getUser());
		obtainObject(object->getName(), character);						// Give object to character
		character->setStatus("obtaining " + object->getName() + ".");	// Update char's status
		character->getCurrentRoom()->removeObject(*object);				// Remove object from the room

		if (player.getName() == object->getUser())						// Player char - edge case
			advanceTime();
		break;
	}
}


void Jogo::characterAction(Personagem* character) {
	int id = character->getNotifyID();
	string antigaSala;

	switch (id) {
	case avancar:
		advanceTime();
		break;

	case imprimir:
		printText(character->getNotifyText());
		break;

	case mover:
		// Characters see char leaving
		for (int i = 0; i < characters.size(); i++)
			if (characters[i] != character && characters[i]->getCurrentRoom() == character->getCurrentRoom())
				characters[i]->seeCharMoving(character, character->getNotifyText(), false);

		// Char enters the room
		antigaSala = character->getCurrentRoom()->getNome();
		character->setCurrentRoom(moveRoom(character->getCurrentRoom(), character->getNotifyText()));
		character->checkRoom( getPeopleInRoom(character->getCurrentRoom()) );

		// Characters see char entering
		for (int i = 0; i < characters.size(); i++)
			if (characters[i] != character && characters[i]->getCurrentRoom() == character->getCurrentRoom())
				characters[i]->seeCharMoving(character, antigaSala, true);
		break;

	case mencionar:
		for (int i = 0; i < characters.size(); i++) {
			if (character->getNotifyTargets().count(characters[i]->getName()) && characters[i]->getCurrentRoom() == character->getCurrentRoom()) {
				characters[i]->executeReaction(character->getNotifyText(), "", character->getName(), true);
				break;
			}
		}
		break;

	case falar:
		for (int i = 0; i < characters.size(); i++) {
			if (character->getNotifyTargets().count(characters[i]->getName()) && characters[i]->getCurrentRoom() == character->getCurrentRoom()) {
				vector<string> args = splitString(character->getNotifyText(), '|');
				characters[i]->executeReaction(args[0], args[1], character->getName(), false);
			}
		}
		break;

	case conversar:
		conversations.push_back(Conversation(character->getNotifyText(), character->getCurrentRoom()->getNome()));
		break;

	case ouvir:
		for (int i = 0; i < conversations.size(); i++)
			if (conversations[i].participates(character->getNotifyText())) {
				conversations[i].addListener(character->getName());
				break;
			}
		break;

	case descansar:
		break;

	case atacar:
		Personagem* vitima = findCharacter(character->getNotifyText());
		if (vitima->getCurrentRoom() == character->getCurrentRoom())
			vitima->beAttacked(character);
		break;
	}

}


void Jogo::advanceTime() {
	// Decide action
	for (int i = 0; i < npcs.size(); i++) {
		int action = npcs[i]->decideAction();
		if (action == conversar)
			npcs[i]->takeAction();
	}
	if (player.getAction() == ouvir)
		player.takeAction();

	// Conversations
	advanceConversations();

	// Order by priority
	PriorityVector<Personagem*> orderAction = PriorityVector<Personagem*>(vector<Personagem*>(), actionCompare);
	for (int i = 0; i < characters.size(); i++) {
		if (characters[i]->getAction() != conversar && characters[i]->getAction() != ouvir)
			orderAction.push(characters[i]);
	}

	// Take action
	while (!orderAction.empty()) {
		(*orderAction.highest())->takeAction();
		orderAction.pop();
	}
	player.setInConversation(false);

	// Update the player's room status
	player.updateRoom( getPeopleInRoom(player.getCurrentRoom()) );
	for (int i = 0; i < npcs.size(); i++)
		npcs[i]->checkRoom( getPeopleInRoom(npcs[i]->getCurrentRoom()) );


	// Jogo
	time++;
	saveGame();
}


void Jogo::advanceConversations() {
	// Iterate through each current convo
	for (vector<Conversation>::iterator it = conversations.begin(); it != conversations.end(); it++) {
		// Try until a message shoots through
		while (1) {
			// If the convo is over
			if (it->ended()) {
				conversations.erase(it);
				break;
			}

			// Advance
			xml_node conversation = it->nextLine();

			string n = conversation.name();
			if (n != "Narrator") {
				Personagem* speaker = findCharacter(conversation.name());

				// Test if valid
				// Is the speaker in the room?
				if (speaker->getCurrentRoom()->getNome() != it->getRoom())
					continue;

				// Does the speaker fulfill the necessary conditions?
				string infoAtom = ""; bool valid = true;
				for (xml_node_iterator cit = conversation.begin(); cit != conversation.end(); ++cit) {
					string info = cit->attribute("info").value();
					bool t1 = speaker->hasCondition(cit->name());
					string nao = cit->attribute("n").value();
					bool t2 = (nao == "n");
					if (info == "info")
						infoAtom = cit->name();
					else if (t1 == t2) {
						valid = false;
						break;
					}
				}
				if (!valid) continue;

				// Send message
				speaker->sayLine( infoAtom, conversation.attribute("line").value(), it->getParticipants(conversation.name()) );
			}
			else if (it->getParticipants()->count(player.getName()) && player.getCurrentRoom()->getNome() == it->getRoom())
				printText(conversation.attribute("line").value());
			else
				continue;

			// Lock every participant
			for (set<string>::iterator ait = it->getParticipants()->begin(); ait != it->getParticipants()->end(); ait++)
				findCharacter(*ait)->setInConversation(true);
			it->clearListeners();
			break;
		}

		break;
	}
}


void Jogo::receiveArgs(vector<string> args) {
	player.receberArgs(args);
}


// LIDAR COM MAPA ------------------------------------------

vector<Personagem*> Jogo::getPeopleInRoom(Sala* room) {
	vector<Personagem*> retorno;
	for (int i = 0; i < characters.size(); i++) {
		if (characters[i]->getCurrentRoom() == room)
			retorno.push_back(characters[i]);
	}

	return retorno;
}


// ACTIONS ----------------------------------
void Jogo::obtainObject(string name, Personagem* receiver) {
	vector<string> actionVec = FileManager::readFromFile("files/items/" + name + ".txt").getValues("actions");
	set<string> actionSet = set<string>();
	for (int i = 0; i < actionVec.size(); i++) {
		actionSet.insert(actionVec[i]);
	}

	receiver->addItem(name, actionSet);
	notify(_obter);
}


Sala* Jogo::moveRoom(Sala* origin, string destination) {
	// Movement
	origin = mapa.getSala(destination);
	return origin;
}


void Jogo::printText(string texto) {
	this->texto = texto;
	notify(_imprimir);
}


vector<Item> Jogo::getInventory() {
	return player.getInventory();
}


// HELPER FUNCTIONS ---------------------------------------------
#include <stdexcept>
Personagem* Jogo::findCharacter(string nome) {
	for (int i = 0; i < characters.size(); i++) {
		if (characters[i]->getName() == nome)
			return characters[i];
	}

	throw invalid_argument("There's no character with that name :(");
}