#include "Settings.h"

// Refresh our local working copy of Settings 
SettingsStorage loadSettings() {
    SettingsStorage settingsStorage;    

    EEPROM.get(SETTINGS_EEPROM_ADDRESS, settingsStorage);
    
    // the very first value will be garbage and we have to initialize it..
    if (settingsStorage.version != 0) {
        SettingsStorage defaultStorage = { 0, 58 };
        settingsStorage = defaultStorage;
    }
    
    return settingsStorage;
}

void saveSettings(SettingsStorage settingsStorage) {
    EEPROM.put(SETTINGS_EEPROM_ADDRESS, settingsStorage);
}

