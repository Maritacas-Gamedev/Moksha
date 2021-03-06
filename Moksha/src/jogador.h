#pragma once
#include <map>
#include <set>
#include "personagem.h"
#include "stringLib.h"
#include "fileManager.h"

using namespace std;

class Jogador : public Personagem {
private:
	string erroSemObjeto;
	string erroSemAcao;
	string erroSemSala;
	string erroSemItem;
	string erroMente;

	map<string, set<string>> mindTheory;

	void mention(string obj, string receiver) override;
	void move(string location) override;
	void interact(string acao, string objeto) override;

	void addToMind(string topic, string character);

public:
	Jogador();
	void receberArgs(vector<string> args);

	bool hasCondition(string info) override;

	void seeCharMoving(Personagem* pessoa, string outraSala, bool entrando) override;
	void executeReaction(string topico, string frase, string remetente) override;
	void checkRoom(vector<Personagem*> pessoasNaSala) override;
	void updateRoom(vector<Personagem*> pessoasNaSala);

	void serAtacado(Personagem* atacante) override {
		printText(atacante->getName() + " attacked you!");
		Personagem::serAtacado(atacante);
	};
};