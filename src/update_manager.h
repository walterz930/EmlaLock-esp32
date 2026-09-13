#pragma once
#include <Arduino.h>
#ifndef EMLALOCK_VERSION
#define EMLALOCK_VERSION "1.0.6"
#endif
void initUpdateManager();
void handleUpdateCheck();
void handleUpdateStart();
const char* firmwareVersion();
