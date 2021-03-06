#pragma once
#include <vector>
#include <string>

#ifndef ENUM_H
#define ENUM_H

enum { M, F, imprimir, mover, descansar, ouvir, mencionar, falar, conversar, interagir, atacar };
static vector<string> stringEnum = vector<string>({ "M", "F", "print", 
										  "move", "rest", "listen", "mention", "say", "talk", "interact", "attack" } );

#endif