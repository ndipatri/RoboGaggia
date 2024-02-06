
#include "WaterPump.h"

// These were emperically derived.  They are highly dependent on the actual system , but should now work
// for any RoboGaggia.
// see https://en.wikipedia.org/wiki/PID_controller#Loop_tuning
double pressure_PID_kP = 0.03;
double pressure_PID_kI = 2.0;
double pressure_PID_kD = 1.0;

//double pressure_PID_kP = 0.5;  main gain is too high.. way too much overshooting
//double pressure_PID_kI = 8.0;
//double pressure_PID_kD = 0.0;

double TARGET_FLOW_RATE = 4.0;

// We do 'Pressure Profiling', meaning we modulate the water pump power based
// on the measure pressure, only during PREINFUSION and CLEANING
double PRE_INFUSION_TARGET_BAR = 3.0;

double MIN_PUMP_DUTY_CYCLE = 35.0;
double MAX_PUMP_DUTY_CYCLE = 100.0;

WaterPumpState waterPumpState;

Adafruit_ADS1115 ads1115;  // the adc for the pressure sensor

// This is a reasonable mount of time to update flow rate given the scale
// takes almost 2 seconds to settle
int FLOW_RATE_SAMPLE_PERIOD_MILLIS = 2000; 

// The TRIAC on/off signal for the AC Potentiometer
// https://rocketcontroller.com/product/1-channel-high-load-ac-dimmer-for-use-witch-micro-controller-3-3v-5v-logic-ac-50-60hz/
// NOTE: Sendin this high triggers the TRIAC (turns it on and allows current flow), but at each zero crossing, the TRIAC
// resets itself and stop current flow.
#define DISPENSE_POT D7 

// This provides a signal to help us time how to 'shape' the AC wave powering the water pump.
// The zero crossings tell us when we can disable part of the AC wave to modulate the power.
// 'Pulse Shape Modulation' (PSM)
#define ZERO_CROSS_DISPENSE_POT A2  


// This sends water to the group head
#define SOLENOID_VALVE_SSR  TX


// For all unspecified states while dispensing, such as 
// push water out of steam wand.
double DEFAULT_DISPENSE_TARGET_BAR = 3.0;


double BACKFLUSH_TARGET_BAR = 4.0;

// Once we hit this, we clamp the pump duty cycle so we don't exceed.. THis is a 
// software Overflow Prevention feature. 
double MAX_BAR = 14.0;


// see https://docs.google.com/spreadsheets/d/1_15rEy-WI82vABUwQZRAxucncsh84hbYKb2WIA9cnOU/edit?usp=sharing
// as shown in shart above, the following values were derived by hooking up a bicycle pump w/ guage to the
// pressure sensor and measuring a series of values vs bar pressure. 
double PRESSURE_SENSOR_SCALE_FACTOR = 1658.0;
int PRESSURE_SENSOR_OFFSET = 2470; 



void readPumpState() {
  // reading from first channel of the 1015

  // no pressure ~1100
  int rawPressure = ads1115.readADC_SingleEnded(0);
  int normalizedPressureInBars = (rawPressure-PRESSURE_SENSOR_OFFSET)/PRESSURE_SENSOR_SCALE_FACTOR;

  publishParticleLog("pump", "dutyCycle: " + String(waterPumpState.pumpDutyCycle) + "', normalizedPressure: " + String(normalizedPressureInBars));

  waterPumpState.measuredPressureInBars = normalizedPressureInBars;
}

void configureWaterPump(int gaggiaState) {

  waterPumpState.targetPressureInBars = DEFAULT_DISPENSE_TARGET_BAR;

  if (gaggiaState == BREWING) {
    
      // For brewing, we use flow profiling...
    
      // The reason we do this here is because PID can't change its target value,
      // so we must create a new one when our target pressure changes..
      PID *thisWaterPumpPID = new PID(&waterPumpState.flowRateGPS,  // input
                                      &waterPumpState.pumpDutyCycle,  // output
                                      &waterPumpState.targetFlowRateGPS,  // target
                                      pressure_PID_kP, pressure_PID_kI, pressure_PID_kD, PID::DIRECT);
    
      thisWaterPumpPID->SetOutputLimits(MIN_PUMP_DUTY_CYCLE, MAX_PUMP_DUTY_CYCLE);
      
      // we only want the PID to calculcate when we've manually updated the flow rate
      // and call Compute().. so we should make this number very small so it always computes
      // when we tell it to.
      thisWaterPumpPID->SetSampleTime(10);
      thisWaterPumpPID->SetMode(PID::AUTOMATIC);

      delete waterPumpState.waterPumpPID;
      waterPumpState.waterPumpPID = thisWaterPumpPID;

  } else {

      if (gaggiaState == PREINFUSION) {
        waterPumpState.targetPressureInBars = PRE_INFUSION_TARGET_BAR;
      }

      if (gaggiaState == BACKFLUSH_CYCLE_1 ||
          gaggiaState == BACKFLUSH_CYCLE_2) {
        waterPumpState.targetPressureInBars = BACKFLUSH_TARGET_BAR;
      }
    
      // for pre-infusion, cleaning, and hot water dispense, we use pressure profiling
    
      // The reason we do this here is because PID can't change its target value,
      // so we must create a new one when our target pressure changes..
      PID *thisWaterPumpPID = new PID(&waterPumpState.measuredPressureInBars,  // input
                                      &waterPumpState.pumpDutyCycle,  // output
                                      &waterPumpState.targetPressureInBars,  // target
                                      pressure_PID_kP, pressure_PID_kI, pressure_PID_kD, PID::DIRECT);
    
      // The Gaggia water pump doesn't energize at all below 30 duty cycle.
      // This number range is the 'dutyCycle' of the power we are sending to the water
      // pump.

      double maxOutput = MAX_PUMP_DUTY_CYCLE;
      // For preinfusion, we hold dutyCycle at 30%. When the puck is dry, it provides little
      // backpressure and so the PID will immediately ramp up to max duty cycle, which is NOT
      // want we want for preinfusion.... This is a silly way to use the PID, in general, but works
      // only for preinfusion.    
      if (gaggiaState == PREINFUSION || gaggiaState == PURGE_BEFORE_STEAM_2) {
        // This is ridiculous but the PID code will ignore the SetOutputLimits
        // call if min and max are identical.
        maxOutput = MIN_PUMP_DUTY_CYCLE + MIN_PUMP_DUTY_CYCLE*.01;
      }
      
      thisWaterPumpPID->SetOutputLimits(MIN_PUMP_DUTY_CYCLE, maxOutput);
      
      // we only want the PID to calculcate when we've manually updated the flow rate
      // and call Compute().. so we should make this number very small so it always computes
      // when we tell it to.
      thisWaterPumpPID->SetSampleTime(10);
      thisWaterPumpPID->SetMode(PID::AUTOMATIC);

      delete waterPumpState.waterPumpPID;
      waterPumpState.waterPumpPID = thisWaterPumpPID;
  }
}

void stopDispensingWater() {
  publishParticleLog("dispenser", "dispensingOff");

  detachInterrupt(ZERO_CROSS_DISPENSE_POT);

  digitalWrite(SOLENOID_VALVE_SSR, LOW);
  digitalWrite(DISPENSE_POT, LOW);
}

// Pulse Skip Modulation means you choose an arbitrary sequence of AC
// cycles to send to the vibe pump, and the rest you 'skip'.
// We'll call this arbitrary sequence of cycles an 'epoch'..
// https://www.instructables.com/Arduino-controlled-light-dimmer-The-circuit/

// total cycles in epoch
volatile int cyclesInEpoch = 10;

// We track number of cycles within an epoch so we can decide 
// which should be on or off depending on duty cycle.
volatile int cycleCount = 1;

// -1 means new cycle
// 1 means TRIAC was ON for first zero crossing within this cycle
// 0 means TRIAC was OFF for first zero crossing within this cycle
volatile int currentCycleValue = -1;

volatile int dutyCycle = 0;

void handleZeroCrossingInterrupt() {
  // At each zero cross, the TRIAC automatically turns off.. so we need
  // to always turn it on for a cycle that is meant to be 'on' 

  if (cycleCount > cyclesInEpoch) {
    // New Epic!
    cycleCount = 1;
    dutyCycle = waterPumpState.pumpDutyCycle;
  }

  volatile bool shouldTurnOnTRIAC = false;
  if (currentCycleValue < 0) {
    // new cycle! need to calculate new value based on duty cycle and current cycle count
    // within the epoch

    // This should be between 0 and 100
    // 0 would mean NO cycles are on during epoch,
    // 50 would mean half of cycles are on during epoch,
    // 100 would be ALL cycles on during epoch.

    int pivot = cyclesInEpoch * (dutyCycle/100.0);
    if(cycleCount <= pivot) {
      shouldTurnOnTRIAC = true;
      currentCycleValue = 1;
    } else {
      shouldTurnOnTRIAC = false;
      currentCycleValue = 0;
    }
    
  } else {
    // just copy value from first zero crossing in this cycle..
    shouldTurnOnTRIAC = (currentCycleValue == 1);
  
    // Now that we've completed a cycle, reset...
    currentCycleValue = -1;
    cycleCount += 1;
  }
  
  if (shouldTurnOnTRIAC) {
    digitalWrite(DISPENSE_POT, HIGH);
  } else {
    digitalWrite(DISPENSE_POT, LOW);
  }

  delayMicroseconds(10);
}
// The solenoid valve allows water to through to grouphead.
void startDispensingWater(boolean turnOnSolenoidValve) {
  publishParticleLog("dispenser", "dispensingOn");

  if (turnOnSolenoidValve) {
    digitalWrite(SOLENOID_VALVE_SSR, HIGH);
  } else {
    digitalWrite(SOLENOID_VALVE_SSR, LOW);
  }

  publishParticleLog("dispenser", "pumpDutyCycle: " + String(waterPumpState.pumpDutyCycle));

  // The zero crossings from the incoming AC sinewave will trigger
  // this interrupt handler, which will modulate the power duty cycle to
  // the water pump..
  //
  // This will not trigger unless water dispenser is running.
  //
  attachInterrupt(ZERO_CROSS_DISPENSE_POT, handleZeroCrossingInterrupt, RISING, 0);
}

int setTargetFlowRate(String _flowRate) {
  
  TARGET_FLOW_RATE = _flowRate.toFloat();

  return 1;
}

int setPID_kP(String _PID_kP) {
  
  pressure_PID_kP = _PID_kP.toFloat();

  return 1;
}

int setPID_kI(String _PID_kI) {
  
  pressure_PID_kI = _PID_kI.toFloat();

  return 1;
}

int setPID_kD(String _PID_kD) {
  
  pressure_PID_kD = _PID_kD.toFloat();

  return 1;
}

String getPumpState() {
  
  readPumpState();

  return String(waterPumpState.measuredPressureInBars);
}

// This will calculate based on the last time this function was called
void updateFlowRateMetricIfNecessary() {
  if (millis() > waterPumpState.nextSampleMillis) {

    double measuredWeightNow = scaleState.measuredWeight;

    double newFlowRateGPS = 0.0;
    if (waterPumpState.nextSampleMillis > 0) {

      // This is the difference between when we thought we were ending this sampling
      // interval and when we did + the length of the sampling interval
      int flowRateInterval = millis() - waterPumpState.nextSampleMillis + FLOW_RATE_SAMPLE_PERIOD_MILLIS;
      Log.error("FlowRate Interval: " + String(flowRateInterval));
      Log.error("FlowRate weight: " + String(measuredWeightNow));
      Log.error("FlowRate previousWeight: " + String(waterPumpState.previousMeasuredWeight));

      newFlowRateGPS = ( // current extracted weight
                              measuredWeightNow -
                              // previous extracted weight
                              waterPumpState.previousMeasuredWeight) /
                              (flowRateInterval/1000.0); 
    }
    // This is observed by the PID
    waterPumpState.flowRateGPS = newFlowRateGPS;


    Log.error("FlowRate Reading: " + String(waterPumpState.flowRateGPS));

    // Now that we have new flow rate, recalculate PID..

    // This triggers the PID to use previous and current
    // measured values to calculate the current required
    // dutyCycle of the water pump
  
    // NJD for testing
    //waterPumpState.pumpDutyCycle = 80;

    waterPumpState.waterPumpPID->Compute();

    waterPumpState.nextSampleMillis = millis() + FLOW_RATE_SAMPLE_PERIOD_MILLIS;
    waterPumpState.previousMeasuredWeight = measuredWeightNow;
  }
}

void waterPumpInit() {
  ads1115.begin();// Initialize ads1015 at the default address 0x48
  // use hte following if x48 is already taken
  //ads1115.begin(0x49);  // Initialize ads1115 at address 0x49
  
  // Water Pump Potentiometer
  pinMode(DISPENSE_POT, OUTPUT);
  pinMode(ZERO_CROSS_DISPENSE_POT, INPUT_PULLDOWN);

  // water dispenser
  pinMode(SOLENOID_VALVE_SSR, OUTPUT);

  waterPumpState.targetFlowRateGPS = TARGET_FLOW_RATE;

  Particle.variable("PID_kP", pressure_PID_kP);
  Particle.variable("PID_kI", pressure_PID_kI);
  Particle.variable("PID_kD", pressure_PID_kD);
  Particle.variable("targetFlowRate", TARGET_FLOW_RATE);
  Particle.variable("currentPressureBars", getPumpState);

  Particle.function("setTargetFlowRate", setTargetFlowRate);
  Particle.function("setPID_kP", setPID_kP);
  Particle.function("setPID_kI", setPID_kI);
  Particle.function("setPID_kD", setPID_kD);
}