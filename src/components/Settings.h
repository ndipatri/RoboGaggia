#ifndef SETTINGS_H
#define SETTINGS_H

#include "Common.h"

struct SettingsStorage {
  int version;
  int referenceCupWeight;
};

SettingsStorage loadSettings();

void saveSettings(SettingsStorage _settingsStorage);

#endif