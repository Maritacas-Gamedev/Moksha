#pragma once
#include "../npc.h"

using namespace std;

class Santos : public NPC {
private:

public:
	Santos() : NPC{ NULL, "Santos", "", M, 3, 1 } {}
	explicit Santos(Map* m);
	int decideActionParticular(string action) override;
	void updateProcessExtra(string currentProcess) override;
	void setupWorldParticular() override;
	void setupObjectivesParticular() override;
	void updateWorldExtra() override;
	void setupActionsParticular() override;
};