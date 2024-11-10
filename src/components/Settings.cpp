#include "Settings.h"

// Refresh our local working copy of Settings 
SettingsStorage loadSettings() {
    SettingsStorage settingsStorage;    

    EEPROM.get(SETTINGS_EEPROM_ADDRESS, settingsStorage);
    
    // the very first value will be garbage and we have to initialize it..
    if (settingsStorage.version != 1) {
        SettingsStorage defaultStorage = { 1, 104, 3};
        settingsStorage = defaultStorage;
    }
    
    return settingsStorage;
}

void saveSettings(SettingsStorage settingsStorage) {
    EEPROM.put(SETTINGS_EEPROM_ADDRESS, settingsStorage);
}

int setReferenceCupWeight(String _referenceCupWeight) {
  Log.error("setting new weight:" + String(_referenceCupWeight));
  
  SettingsStorage settingsStorage = loadSettings();

  settingsStorage.referenceCupWeight = _referenceCupWeight.toInt();
  Log.error("new weight:" + String(settingsStorage.referenceCupWeight));
  
  saveSettings(settingsStorage);

  Log.error("stored weight:" + String(loadSettings().referenceCupWeight));

  return 1;
}

int getReferenceCupWeight() {
  return loadSettings().referenceCupWeight;
}

int setWeightToBeanRatio(String _weightToBeanRatio) {
  SettingsStorage settingsStorage = loadSettings();

  settingsStorage.weightToBeanRatio = _weightToBeanRatio.toInt();

  saveSettings(settingsStorage);

  return 1;
}

int getWeightToBeanRatio() {
  return loadSettings().weightToBeanRatio;
}

// This assumes nothing is currently on the scale
void settingsInit() {
  Particle.variable("referenceCupWeight", getReferenceCupWeight); 
  Particle.function("setReferenceCupWeight", setReferenceCupWeight);

  Particle.variable("weightToBeanRatio", getWeightToBeanRatio); 
  Particle.function("setWeightToBeanRatio", setWeightToBeanRatio);
}