#ifndef SETTINGS_H
#define SETTINGS_H

#include "include/main.h"

void SaveSettings(AppState *g);
void LoadSettings(AppState *g);

/* Persistent user preferences (theme, etc.) stored in usersettings.json */
void SaveUserSettings(AppState *g);
void LoadUserSettings(AppState *g);

#endif