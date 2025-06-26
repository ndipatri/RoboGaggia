# RoboGaggia State Machine

This Mermaid diagram shows all possible states along with whether each is transient or steady, and the user or data events that cause transitions.

```mermaid
stateDiagram-v2
    [*] --> JOINING_NETWORK
    note right of JOINING_NETWORK
      transient
    end note
    JOINING_NETWORK --> PREHEAT : networkState.connected
    JOINING_NETWORK --> IGNORING_NETWORK : SHORT_PRESS/LONG_PRESS

    IGNORING_NETWORK --> PREHEAT : WiFi.off
    note right of IGNORING_NETWORK : transient

    PREHEAT --> MEASURE_BEANS : SHORT_PRESS & temp < 1.5×TARGET_BREW_TEMP
    PREHEAT --> COOL_START : SHORT_PRESS & temp ≥ 1.5×TARGET_BREW_TEMP
    PREHEAT --> CLEAN_OPTIONS : LONG_PRESS
    note right of PREHEAT : steady

    MEASURE_BEANS --> TARE_CUP_AFTER_MEASURE : SHORT_PRESS
    MEASURE_BEANS --> PREHEAT : LONG_PRESS
    note right of MEASURE_BEANS : steady

    TARE_CUP_AFTER_MEASURE --> HEATING_TO_BREW : SHORT_PRESS
    TARE_CUP_AFTER_MEASURE --> PREHEAT : LONG_PRESS
    note right of TARE_CUP_AFTER_MEASURE : steady

    HEATING_TO_BREW --> PREINFUSION : heater ≥ TARGET_BREW_TEMP
    HEATING_TO_BREW --> PREHEAT : LONG_PRESS
    note right of HEATING_TO_BREW : transient

    PREINFUSION --> BREWING : weight > threshold
    PREINFUSION --> PREHEAT : LONG_PRESS
    note right of PREINFUSION : transient

    BREWING --> DONE_BREWING : weight ≥ target
    BREWING --> PREHEAT : LONG_PRESS
    note right of BREWING : transient

    DONE_BREWING --> HEATING_TO_STEAM : SHORT_PRESS
    DONE_BREWING --> PREHEAT : LONG_PRESS
    note right of DONE_BREWING : steady

    HEATING_TO_STEAM --> STEAMING : heater ≥ TARGET_STEAM_TEMP
    HEATING_TO_STEAM --> PREHEAT : LONG_PRESS
    note right of HEATING_TO_STEAM : transient

    STEAMING --> GROUP_CLEAN_2 : SHORT_PRESS/LONG_PRESS
    note right of STEAMING : steady

    GROUP_CLEAN_2 --> GROUP_CLEAN_3 : SHORT_PRESS
    GROUP_CLEAN_2 --> PREHEAT : LONG_PRESS
    note right of GROUP_CLEAN_2 : steady

    GROUP_CLEAN_3 --> PREHEAT : timeout or LONG_PRESS
    note right of GROUP_CLEAN_3 : transient

    CLEAN_OPTIONS --> DESCALE : SHORT_PRESS
    CLEAN_OPTIONS --> BACKFLUSH_INSTRUCTION_1 : LONG_PRESS
    note right of CLEAN_OPTIONS : steady

    DESCALE --> HEATING_TO_DISPENSE : SHORT_PRESS
    DESCALE --> PREHEAT : LONG_PRESS
    note right of DESCALE : steady

    HEATING_TO_DISPENSE --> DISPENSE_HOT_WATER : heater ≥ TARGET_HOT_WATER_DISPENSE_TEMP
    HEATING_TO_DISPENSE --> PREHEAT : LONG_PRESS
    note right of HEATING_TO_DISPENSE : transient

    DISPENSE_HOT_WATER --> PREHEAT : SHORT_PRESS/LONG_PRESS
    note right of DISPENSE_HOT_WATER : steady

    COOL_START --> COOLING : SHORT_PRESS
    COOL_START --> PREHEAT : LONG_PRESS
    note right of COOL_START : steady

    COOLING --> COOL_DONE : SHORT_PRESS or temp < TARGET_BREW_TEMP
    COOLING --> PREHEAT : LONG_PRESS
    note right of COOLING : transient

    COOL_DONE --> COOL_START : SHORT_PRESS & temp ≥ TARGET_BREW_TEMP
    COOL_DONE --> PREHEAT : SHORT_PRESS (temp < target) or LONG_PRESS
    note right of COOL_DONE : steady

    BACKFLUSH_INSTRUCTION_1 --> BACKFLUSH_INSTRUCTION_2 : SHORT_PRESS
    BACKFLUSH_INSTRUCTION_1 --> PREHEAT : LONG_PRESS
    note right of BACKFLUSH_INSTRUCTION_1 : steady

    BACKFLUSH_INSTRUCTION_2 --> BACKFLUSH_CYCLE_1 : SHORT_PRESS
    BACKFLUSH_INSTRUCTION_2 --> PREHEAT : LONG_PRESS
    note right of BACKFLUSH_INSTRUCTION_2 : steady

    BACKFLUSH_CYCLE_1 --> BACKFLUSH_INSTRUCTION_3 : counter reached
    BACKFLUSH_CYCLE_1 --> PREHEAT : LONG_PRESS
    note right of BACKFLUSH_CYCLE_1 : transient

    BACKFLUSH_INSTRUCTION_3 --> BACKFLUSH_CYCLE_2 : SHORT_PRESS
    BACKFLUSH_INSTRUCTION_3 --> PREHEAT : LONG_PRESS
    note right of BACKFLUSH_INSTRUCTION_3 : steady

    BACKFLUSH_CYCLE_2 --> BACKFLUSH_CYCLE_DONE : counter reached
    BACKFLUSH_CYCLE_2 --> PREHEAT : LONG_PRESS
    note right of BACKFLUSH_CYCLE_2 : transient

    BACKFLUSH_CYCLE_DONE --> PREHEAT : SHORT_PRESS/LONG_PRESS
    note right of BACKFLUSH_CYCLE_DONE : steady

    PURGE_BEFORE_STEAM_1 --> PURGE_BEFORE_STEAM_2 : SHORT_PRESS
    PURGE_BEFORE_STEAM_1 --> PREHEAT : LONG_PRESS
    note right of PURGE_BEFORE_STEAM_1 : steady

    PURGE_BEFORE_STEAM_2 --> PURGE_BEFORE_STEAM_3 : timer expires
    PURGE_BEFORE_STEAM_2 --> PREHEAT : LONG_PRESS
    note right of PURGE_BEFORE_STEAM_2 : transient

    PURGE_BEFORE_STEAM_3 --> HEATING_TO_STEAM : SHORT_PRESS
    PURGE_BEFORE_STEAM_3 --> PREHEAT : LONG_PRESS
    note right of PURGE_BEFORE_STEAM_3 : steady

    GROUP_CLEAN_1 --> GROUP_CLEAN_2 : heater ≥ TARGET_HOT_WATER_DISPENSE_TEMP
    GROUP_CLEAN_1 --> PREHEAT : LONG_PRESS
    note right of GROUP_CLEAN_1 : transient

    SLEEP --> PREHEAT : SHORT_PRESS/LONG_PRESS
    note right of SLEEP : steady

    IGNORING_NETWORK --> PREHEAT : WiFi.off
    note right of IGNORING_NETWORK : transient

    note over SLEEP
      Entered from any state after inactivity timeout
    end note
```
