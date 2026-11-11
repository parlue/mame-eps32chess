#pragma once
#include <string>
// Every backend exchanges complete UCI lines with the same UciSession.
bool virginBegin();
bool virginOpen();
void virginClose();
bool virginSend(const std::string& line);
bool virginRead(std::string& line);
std::string virginError();
