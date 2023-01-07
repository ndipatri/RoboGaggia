
#include "WaterPump.h"

// These were emperically derived.  They are highly dependent on the actual system , but should now work
// for any RoboGaggia.
// see https://en.wikipedia.org/wiki/PID_controller#Loop_tuning
double pressure_PID_kP = 0.2;
double pressure_PID_kI = 1.0;
double pressure_PID_kD = 2.0;

//double pressure_PID_kP = 0.5;  main gain is too high.. way too much overshooting
//double pressure_PID_kI = 8.0;
//double pressure_PID_kD = 0.0;

double TARGET_FLOW_RATE = 3.0;

float nextFlowRateSampleMillis = -1;

int FLOW_RATE_SAMPLE_PERIOD_MILLIS = 1200; 

WaterPumpState waterPumpState;

Adafruit_ADS1115 ads1115;  // the adc for the pressure sensor

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

// We do 'Pressure Profiling', meaning we modulate the water pump power based
// on the measure pressure, only during PREINFUSION and CLEANING
double PRE_INFUSION_TARGET_BAR = 1.5;

double BACKFLUSH_TARGET_BAR = 4.0;

// Once we hit this, we clamp the pump duty cycle so we don't exceed.. THis is a 
// software Overflow Prevention feature. 
double MAX_BAR = 14.0;


// see https://docs.google.com/spreadsheets/d/1_15rEy-WI82vABUwQZRAxucncsh84hbYKb2WIA9cnOU/edit?usp=sharing
// as shown in shart above, the following values were derived by hooking up a bicycle pump w/ guage to the
// pressure sensor and measuring a series of values vs bar pressure. 
double PRESSURE_SENSOR_SCALE_FACTOR = 1658.0;
int PRESSURE_SENSOR_OFFSET = 2470; 

int MIN_PUMP_DUTY_CYCLE = 30;
int MAX_PUMP_DUTY_CYCLE = 100;

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
    
      // The Gaggia water pump doesn't energize at all below 30 duty cycle.
      // This number range is the 'dutyCycle' of the power we are sending to the water
      // pump.
      thisWaterPumpPID->SetOutputLimits(MIN_PUMP_DUTY_CYCLE, MAX_PUMP_DUTY_CYCLE);
      thisWaterPumpPID->SetSampleTime(200);
      thisWaterPumpPID->SetMode(PID::AUTOMATIC);

      delete waterPumpState.waterPumpPID;
      waterPumpState.waterPumpPID = thisWaterPumpPID;

  } else {

      if (gaggiaState == PREINFUSION) {
        waterPumpState.targetPressureInBars = PRE_INFUSION_TARGET_BAR;
      }

      if (gaggiaState == CLEAN_SOAP ||
          gaggiaState == CLEAN_RINSE) {
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
      if (gaggiaState == PREINFUSION) {
        maxOutput = MIN_PUMP_DUTY_CYCLE;
      }
      thisWaterPumpPID->SetOutputLimits(MIN_PUMP_DUTY_CYCLE, maxOutput);
      thisWaterPumpPID->SetSampleTime(500);
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

// This is an Interrupt Service Routine (ISR) so it cannot take any arguments
// nor return any values.
//
// We consider our current
void turnOnDispensePotISR() {
  digitalWrite(DISPENSE_POT, HIGH);
}
Timer timer(0, turnOnDispensePotISR, true);
void handleZeroCrossingInterrupt() {
  // IMPORTANT!: the Triac will automatically shut off at every zero
  // crossing.. so it's best to start the cycle with teh triac off and
  // then turn it on for the remainder of the cycle  

  digitalWrite(DISPENSE_POT, LOW);
  delayMicroseconds(10);
  
  // We assume 60Hz cycle (1000ms/60 = 16ms per cycle.)
  // If we wait 8ms before turning on, the next cycle will happen immediately
  // and shut off.. so 8ms is essentially 0% duty cycle
  // If we wait 4ms before turning on, that's half the cycle.. so that's 50%
  // If we wait 0ms before turning on, that's 100% duty cycle.

  // ok for some reason, only values of 40 and 100 work

  // NOTE: dutyCycle below 30 is kinda useless.. doesn't really energize water
  // pump
  
  double regulatedPumpDutyCycle = waterPumpState.pumpDutyCycle;

  if (waterPumpState.measuredPressureInBars >= MAX_BAR) {
    // primative means of clamping pressure
    regulatedPumpDutyCycle /= 3.0;
  }

  int offIntervalMs = 8 - round((8*(regulatedPumpDutyCycle/100.0)));

  // There needs to be a minimal delay between LOW and HIGH for the TRIAC
  offIntervalMs = max(offIntervalMs, 1);

  // 'FromISR' is critical otherwise, the timer hangs the ISR.
  timer.changePeriodFromISR(offIntervalMs);
  timer.startFromISR();  
}

// The solenoid valve allows water to through to grouphead.
void startDispensingWater(boolean turnOnSolenoidValve) {
  publishParticleLog("dispenser", "dispensingOn");

  if (turnOnSolenoidValve) {
    digitalWrite(SOLENOID_VALVE_SSR, HIGH);
  } else {
    digitalWrite(SOLENOID_VALVE_SSR, LOW);
  }

  readPumpState();  

  // This triggers the PID to use previous and current
  // measured values to calculate the current required
  // dutyCycle of the water pump
  // By default, it will update output values every 200ms, regardless
  // of how often we call Compute()
  waterPumpState.waterPumpPID->Compute();

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

boolean updateFlowRateMetricWhenItsTime() {
    if (millis() > nextFlowRateSampleMillis) {

      // Time to calculate new telemetry

      double newFlowRateGPS = ( // current extracted weight
                             scaleState.measuredWeight - scaleState.tareWeight -
                             // previous extracted weight
                             waterPumpState.measuredWeightAtFlowRate) /
                             (FLOW_RATE_SAMPLE_PERIOD_MILLIS/1000.0); 

      // This is observed by the PID
      waterPumpState.flowRateGPS = newFlowRateGPS;
      
      // This is used for the next rate calculation.
      waterPumpState.measuredWeightAtFlowRate = scaleState.measuredWeight - scaleState.tareWeight;
    
      nextFlowRateSampleMillis = millis() + FLOW_RATE_SAMPLE_PERIOD_MILLIS;
    
      return true;
    }

    return false;
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