#pragma once
#include <queue>
#include "goal.h"
#include "priorityVector.h"
#include "character.h"
#include "fileManager.h"
#include "dictionary.h"
#include "../libs/GPGOAP/goap.h"
#include "../libs/GPGOAP/astar.h"  // ASTER GASTER WOWOWOWOW

using namespace std;

class NPC : public Character {
protected:
	string* description;

	set<string*> addedConditions;
	set<string*> addedActions;
	set<string> trackablePeople;
	set<string*> trackableRooms;
	set<string*> trackableConvos;
	vector<string*> conditionNames;

	queue<Room*> path;
	string conversaAlvo;

	actionplanner_t ap;
	worldstate_t stateZero;
	worldstate_t states[16];
	worldstate_t world;
	Goal currentGoal;
	PriorityVector<Goal> goalList;
	const char* plan[16]; // The planner will return the action plan in this array.
	int plansz = 16; // Size of our return buffers
	int planCost = 0;
	int currentStep = 0;
	Dictionary<vector<string>> dict;

	Dictionary<string> lastSeen;

	queue<Room*> findPath(Room* salaInicial, Room* salaAlvo);
	queue<Room*> findPath(Room* salaAlvo);
	queue<Room*> search();
	queue<Room*> search(Room* salaPista);

	int pathSize(Room* inicio, Room* alvo) {
		return mapp->optimalPath(inicio, alvo).size();
	};

	void addTrackableRoom(string room);
	void addTrackablePeople(string person);
	void addTrackableConvo(string convo);

	void advancePath();
	void move(string room) override;

	virtual int decideActionParticular(string acao) { return descansar; }
	virtual void setupWorldParticular() {}
	virtual void setupObjectivesParticular() {}
	virtual void setupActionsParticular() {}
	void updateWorld();
	void updateWorldVariables();
	void updateLastSeen(string pursueTarget, string room);
	virtual void updateWorldExtra() {}
	void advancePlans();
	void updateProcess(string currentProcess);
	virtual void updateProcessExtra(string currentProcess) {}
	void changePlans() { changePlans(false); };
	void changePlans(bool justUpdated);

	bool isCurrentStateFulfilled();
	void setBusy(bool novo) { busy = novo; }

public:
	explicit NPC(Map* m, string name, string description, int gender, int strength, int dexterity);
	~NPC();

	string* getDescription() { return description; }

	void setCondition(string condition, bool update);
	bool hasCondition(string info) override;
	void takeAction() override { Character::takeAction(); updateWorld(); }
	void receiveEvent(vector<string> args) override;

	int decideAction();
	void executeReaction(string topic, string phrase, string sender, bool shouldRespond) override;
	void checkRoom(vector<Character*> peopleInRoom) override;
	void seeCharMoving(Character* character, Room* otherRoom, bool entering) override;
	void setSalaAlvo(Room* nova) { findPath(nova); }

	void addGoal(vector<string*> conditions, vector<bool> conditionStates, int priority);
	void addGoal(string* condition, bool conditionState, int priority);
	void addGoal(bfield_t values, bfield_t dontcare, bool conditionState, int priority);

	void setupWorld();
	void clear();

	vector<string> getActionList();
	vector<string> getAtomList();
	vector<Goal> getGoalList() { return goalList.getVector(); }
};