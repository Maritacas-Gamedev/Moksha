#include "npc.h"

NPC::NPC(Mapa* m, string name, int gender, int forca, int destreza) : Personagem(gender, forca, destreza) {
	this->name = name;
	this->mapa = m;

	FileDict fileObjeto = FileManager::readFromFile("files/characters/" + getName() + ".txt");
	this->dict = fileObjeto;
}


// PATHFINDING ---------------

queue<Sala*> NPC::findPath(Sala* salaAlvo) {
	return findPath(currentRoom, salaAlvo);
};

queue<Sala*> NPC::findPath(Sala* salaInicial, Sala* salaAlvo) {
	return mapa->optimalPath(salaInicial, salaAlvo);
};

queue<Sala*> NPC::search() {
	return search(currentRoom);
};

queue<Sala*> NPC::search(Sala* salaPista) {
	// Path to the clue
	queue<Sala*> retorno;
	retorno.push(currentRoom);
	if (salaPista != currentRoom)
		retorno = findPath(salaPista);

	// Start search
	queue<Sala*> search = mapa->breadthSearch(salaPista);
	while (!search.empty()) {
		queue<Sala*> caminho = findPath(retorno.back(), search.front());
		while (!caminho.empty()) {
			retorno.push(caminho.front());
			caminho.pop();
		}
		search.pop();
	}

	if (retorno.front() == currentRoom)
		retorno.pop();
	return retorno;
};


string NPC::nextRoomInPath() {
	string retorno = "";
	if (path.empty())
		advancePlansExtra(plan[currentStep]); // Refresh plans

	if (!path.empty()) {
		retorno = path.front()->getNome();
		path.pop();
	}

	return retorno;
}

// REACTIONS ----------------------------------

void NPC::executeReaction(string topic, string phrase, string sender, bool shouldRespond) {
	if (isUnconscious())
		return;

	if (topic != "")
		setCondition(topic, true);
	if (dict.hasKey(topic) && shouldRespond)
		say(topic, dict.getValue(topic), set<string>({ sender }));
}


void NPC::setCondition(string condition, bool update) {
	addedConditions.push_back(new string(condition));
	goap_worldstate_set(&ap, &world, addedConditions.back()->c_str(), update);
}


void NPC::checkRoom(vector<Personagem*> pessoasNaSala) {
	for (int i = 0; i < alvos.size(); i++) {
		goap_worldstate_set(&ap, &world, ("with_" + alvos[i]).c_str(), false);
		for (int j = 0; j < pessoasNaSala.size(); j++) {
			string theirName = pessoasNaSala[j]->getName();
			if (theirName != name) {
				updateLastSeen(theirName, currentRoom->getNome());
				if (alvos[i] == theirName) {
					goap_worldstate_set(&ap, &world, ("with_" + alvos[i]).c_str(), true);
					break;
				}
			}
		}
	}

	updateWorld();
};


void NPC::seeCharMoving(Personagem* person, string otherRoom, bool entering) {
	int idx = alvoIndex(person->getName());

	if (idx != -1) {
		if (!entering)
			updateLastSeen(person->getName(), otherRoom);
		else
			updateLastSeen(person->getName(), currentRoom->getNome());

		goap_worldstate_set(&ap, &world, ("with_" + alvos[idx]).c_str(), entering);
		updateWorld();
	}
}


// PLANS -------------------------------------------

void NPC::setupPlans() {
	goap_actionplanner_clear(&ap); // initializes action planner

	// describe repertoire of actions
	static vector<string> search_atoms = editVector("search_", names, "");
	static vector<string> with_atoms = editVector("with_", names, "");
	static vector<string> alive_atoms = editVector("", names, "_alive");
	for (int i = 0; i < search_atoms.size(); i++)
		if (names[i] != name) {
			goap_set_pre(&ap, search_atoms[i].c_str(), alive_atoms[i].c_str(), true);
			goap_set_pst(&ap, search_atoms[i].c_str(), with_atoms[i].c_str(), true);
		}
	setupAcoesAdicional();

	// describe current world state.
	goap_worldstate_clear(&world);
	for (int i = 0; i < alive_atoms.size(); i++) {
		goap_worldstate_set(&ap, &world, alive_atoms[i].c_str(), true);
		if (names[i] != name)
			goap_worldstate_set(&ap, &world, ("with_" + names[i]).c_str(), false);
	}
	goap_worldstate_set(&ap, &world, "armed", inventory.hasItem("Knife"));
	setupMundoAdicional();

	// describe goal
	goap_worldstate_clear(&currentGoal.goal);
	setupObjetivosAdicional();
	goalList = PriorityVector<Goal>(vector<Goal>(), goalCompare);
	goalList.push(currentGoal);

	// calculate initial plan
	planCost = astar_plan(&ap, world, currentGoal.goal, plan, states, &plansz);
	currentStep = -1;
	updateWorld();
	advancePlans();
}


void NPC::updateWorld() {
	updateWorldExtra();

	// Detect that current action is impossible
	if (plansz > 0) {
		bfield_t prereqs;
		goap_get_pre(&ap, plan[currentStep], &prereqs);
		if (prereqs & world.values != prereqs)
			changePlans(true);
	}
}


void NPC::updateLastSeen(string pursueTarget, string room) {
	string alvoVec[1] = { pursueTarget };
	lastSeen.addPair(set<string>(alvoVec, alvoVec + 1), room);

	string currentProcess = plan[currentStep];
	if (currentProcess.compare("search_" + pursueTarget) == 0) {
		path = search(mapa->getSala( lastSeen.getValues(pursueTarget) ));
	}
}


void NPC::advancePlans() {
	currentStep++;

	// The plan has come to its end
	if (currentStep >= plansz && plansz > 0)
		changePlans();

	// Advance plans
	if (plansz > 0) {
		string currentProcess = plan[currentStep];

		if (currentProcess.substr(0, 7).compare("search_") == 0) {
			if (lastSeen.hasKey( currentProcess.substr(7, 10000) ))
				path = search(mapa->getSala(lastSeen.getValues( currentProcess.substr(7, 10000) )));
			else
				path = search();

		}

		advancePlansExtra(currentProcess);
	}
}


void NPC::changePlans(bool justUpdated) {
	if (!justUpdated)
		updateWorldExtra();

	// Calculate a new plan
	planCost = astar_plan(&ap, world, currentGoal.goal, plan, states, &plansz);
	currentStep = -1;
	advancePlans();
}


// A��ES ------------------------------------------------

int NPC::decideAction() {
	// Check if current action was completed
	while (isCurrentStateFulfilled() && plansz > 0)
		advancePlans();

	// Current objective is top priority
	if (currentGoal.goal.values == (goalList.highest()->goal.values)) {
		// Current objective was completed - find next non-completed objective
		vector<Goal>::iterator it = goalList.highest();
		if ( ((world.values & ~currentGoal.goal.dontcare) == currentGoal.goal.values || currentStep >= plansz) && !goalList.empty() ) {
			do {
				if (it->onetime)
					goalList.getVector().erase(it);

				goalList.descend(it);
				currentGoal = *it;
				currentStep = 0;
			} while ( (world.values & ~currentGoal.goal.dontcare) == currentGoal.goal.values && it != goalList.lowest() );
			changePlans();
		}
	
	// Current objective is not top priority - find next non-completed objective
	} else {
		vector<Goal>::iterator it = goalList.highest();
		while ((world.values & ~currentGoal.goal.dontcare) == currentGoal.goal.values && it != goalList.lowest() ) {
			goalList.descend(it);
			currentGoal = *it;
			currentStep = 0;
		}
		changePlans();
	}

	// Decide action
	if (plansz > 0) {
		actionArgs.clear();
		currentAction = decidirAcaoAdicional(plan[currentStep]);
	} else
		currentAction = descansar;

	return currentAction;
}


// HELPER ----------------------------------------

int NPC::alvoIndex(string nome) {
	for (int i = 0; i < alvos.size(); i++) {
		if (nome == alvos[i])
			return i;
	}

	return -1;
}


bool NPC::isCurrentStateFulfilled() {
	bfield_t worldState = (world.values & ~states[currentStep].dontcare);
	bfield_t objState = states[currentStep].values;
	if (currentStep > 0)
		objState = states[currentStep].values & ~states[currentStep-1].values;
	bool fulfilled = ((worldState & objState) == objState);
	if (fulfilled && currentStep > 0) {
		objState = states[currentStep].values | ~states[currentStep - 1].values;
		fulfilled = fulfilled && ((worldState | objState) == objState);
	}

	return fulfilled;
}


bool NPC::hasCondition(string info) {
	bool retorno;
	if (!goap_worldstate_get(&ap, &world, info.c_str(), &retorno))
		return false;
	return retorno;
}


// GETTER
vector<string> NPC::getActionList() {
	vector<string> retorno;
	for (int i = 0; i < ap.numactions; i++)
		retorno.push_back(ap.act_names[i]);
	return retorno;
}


vector<string> NPC::getAtomList() {
	vector<string> retorno;
	for (int i = 0; i < ap.numatoms; i++)
		retorno.push_back(ap.atm_names[i]);
	return retorno;
}