#ifndef STATE_H
#define STATE_H

#include "Common.h"
#include "Heater.h"
#include "WaterPump.h"
#include "Network.h"
#include "Scale.h"
#include "Telemetry.h"
#include "WaterReservoir.h"
#include "UserInput.h"
#include "Display.h"

extern GaggiaState manualNextGaggiaState;

extern GaggiaState  helloState,
                    featuresState,
                    wandFeaturesState,
                    tareCup1State,
                    measureBeansState,
                    tareCup2State,
                    heatingToBrewState,
                    preinfusionState,
                    brewingState,
                    heatingToDispenseState,
                    dispenseHotWaterState,
                    doneBrewingState,
                    purgeBeforeSteam1, 
                    purgeBeforeSteam2, 
                    purgeBeforeSteam3, 
                    heatingToSteamState, 
                    steamingState,
                    coolStartState,
                    coolingState,
                    coolDoneState,
                    groupClean1State,
                    groupClean2State,
                    groupClean3State,
                    backflushInstructions1State,
                    backflushInstructions2State,
                    backflushInstructions3State,
                    backflushCycle1State,
                    backflushCycle2State,
                    backflushCycleDoneState,
                    joiningNetwork,
                    ignoringNetwork,
                    naState,

                    currentGaggiaState;

// Using all current state, we derive the next state of the system
GaggiaState getNextGaggiaState();

// Once we know the state, we affect appropriate change in our
// system attributes (e.g. temp, display)
// Things we do while we are in a state 
void processCurrentGaggiaState();

// Things we do when we leave a state
void processOutgoingGaggiaState();

// Things we do when we enter a state
void processIncomingGaggiaState(GaggiaState *nextGaggiaState);

void stateInit();

char* getStateName(int stateEnum);

#endif