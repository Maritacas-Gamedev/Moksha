#pragma once
#include "../npc.h"

using namespace std;

class Liz : public NPC {
private:

public:
	Liz() : NPC{ NULL, "Liz", "", F, 2, 2 } {}
	explicit Liz(Map* m);
	int decideActionParticular(string action) override;
	void setupProcessParticular(string currentProcess) override;
	void setupWorldParticular() override;
	void setupObjectivesParticular() override;
	void updateWorldExtra() override;
	void setupActionsParticular() override;
};