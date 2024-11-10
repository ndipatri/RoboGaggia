#ifndef SETTINGS_H
#define SETTINGS_H

#include "Common.h"

void settingsInit();

struct SettingsStorage {
  int version;
  int referenceCupWeight;
  int weightToBeanRatio;
};

SettingsStorage loadSettings();

void saveSettings(SettingsStorage _settingsStorage);

#endif