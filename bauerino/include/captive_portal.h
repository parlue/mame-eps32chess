#pragma once
#include <WebServer.h>
bool portalBegin();
void portalProcess();
void portalRegister(WebServer& server);
void portalRedirect(WebServer& server);
void portalSelfTest();
