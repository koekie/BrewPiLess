#ifndef HumidityControl_H
#define HumidityControl_H
#include "Config.h"
#include "Brewpi.h"
#include "Actuator.h"
#include "DHTSensor.h"
#include "BPLSettings.h"
#include "EnvironmentSensor.h"
#if SMART_DISPLAY
#include "SharedLcd.h"
#endif

#if EnableHumidityControlSupport

#define MINIMUM_HUMIDITY_SENSOR_READ_PERIOD 5000
#define IsValidHumidityValue(a) ((a) <=100)

extern ValueActuator defaultActuator;
#define HC_ModeOff 0
#define HC_ModeControl 1

// hard coding. It's so tedious to implement the interface.
#define MinimumHumidifyingRunningTime 60
#define MinimumHumidifyingIdleTime 60
#define MinimumDehumidifyingRunningTime 300
#define MinimumDehumidifyingIdleTime 300
#define MinimumDeadTime 600

#define DefaultIdleHigh 3
#define DefaultIdleLow 3
#define HumidifyingTargetHigh 5
#define DehumidifyingTargetLow 5

typedef enum _HumidityControlState {
    HC_Idle,
    HC_Dehumidifying,
    HC_Humidifying
} HumidityControlState;

#define ToSystemTick(a) (a)*1000

class HumidityControl{
public:
    static EnvironmentSensor *chamberSensor;
    static EnvironmentSensor *roomSensor;

	static Actuator* humidifier;
	static Actuator* dehumidifier;


    HumidityControl():_mode(HC_ModeOff),_humidity(INVALID_HUMIDITY_VALUE),_state(HC_Idle),_prevState(HC_Idle){}    
    bool isChamberSensorInstalled(){ return chamberSensor != &nullEnvironmentSensor; }
    bool isRoomSensorInstalled(){ return roomSensor != &nullEnvironmentSensor; }

    uint8_t roomHumidity(){
        return roomSensor->humidity();
    }
    void updateState(){
        #if SerialDebug
        Serial.printf("***\n");
        #endif
        switch(_state){
            case HC_Idle:
                if(_humidity > _cfg->target +_cfg->idleHigh){
                    // start dehumidifying, if possible
                    #if SerialDebug
                    Serial.println("[Humidity Control]: Start dehumidifying if possible ...");
                    #endif
                    if( dehumidifier != &defaultActuator){
                        #if SerialDebug
                        Serial.println("* Dehumidifier is currently not active");
                        #endif
                        if( (_prevState == HC_Dehumidifying && (_lastreadtime - _lastStateChangeTime > ToSystemTick(_cfg->minDehumidifyingIdleTime)))
                         || ( (_prevState == HC_Humidifying || _prevState == HC_Idle )&& (_lastreadtime - _lastStateChangeTime > ToSystemTick(_cfg->minDeadTime) )) ){
                            #if SerialDebug
                            Serial.printf("[Humidity Control]: humidity: %d\n", _humidity);
                            Serial.printf("[Humidity Control]: target: %d\n", _cfg->target);
                            Serial.printf("[Humidity Control]: last read time: %d\n", _lastreadtime);
                            Serial.printf("[Humidity Control]: last state change: %d\n", _lastStateChangeTime);
                            Serial.printf("[Humidity Control]: previous state: %d\n", _prevState);
                            #endif
                            _startDehumidifying();
                        }
                    }
                }else  if(_humidity < _cfg->target - _cfg->idleLow){
                    // start humidifying
                    #if SerialDebug
                    Serial.println("[Humidity Control]: Start humidifying if possible ...");
                    #endif
                    if( humidifier != &defaultActuator){
                        #if SerialDebug
                        Serial.println("* Humidifier is currently not active");
                        #endif
                        if( (_prevState == HC_Humidifying && (_lastreadtime - _lastStateChangeTime > ToSystemTick(_cfg->minHumidifyingIdleTime)))
                         || ( (_prevState == HC_Dehumidifying || _prevState == HC_Idle )&& (_lastreadtime - _lastStateChangeTime > ToSystemTick(_cfg->minDeadTime) )) ){
                            #if SerialDebug
                            Serial.printf("[Humidity Control]: humidity: %d\n", _humidity);
                            Serial.printf("[Humidity Control]: target: %d\n", _cfg->target);
                            Serial.printf("[Humidity Control]: last read time: %d\n", _lastreadtime);
                            Serial.printf("[Humidity Control]: last state change: %d\n", _lastStateChangeTime);
                            Serial.printf("[Humidity Control]: previous state: %d\n", _prevState);
                            #endif
                             _startHumidifying();
                        }
                    }
                }
                break;

            case HC_Dehumidifying:
                #if SerialDebug
                Serial.println("[Humidity Control]: Busy DEhumidifying ...");
                #endif
                if( (_humidity < _cfg->target - _cfg->dehumidifyingTargetLow  || _cfg->mode == HC_ModeOff) && 
                    _lastreadtime - _lastStateChangeTime > ToSystemTick(_cfg->minDehumidifyingRunningTime)){
                        #if SerialDebug
                        Serial.printf("[Humidity Control]: humidity: %d\n", _humidity);
                        Serial.printf("[Humidity Control]: target: %d\n", _cfg->target);
                        Serial.printf("[Humidity Control]: last read time: %d\n", _lastreadtime);
                        Serial.printf("[Humidity Control]: last state change: %d\n", _lastStateChangeTime);
                        Serial.printf("[Humidity Control]: previous state: %d\n", _prevState);
                        #endif
                        _stopDehumidifying();
                        if(_cfg->mode == HC_ModeOff) _mode = HC_ModeOff;
                }

                break;

            case HC_Humidifying:
                #if SerialDebug
                Serial.println("[Humidity Control]: Busy humidifying ...");
                #endif
                if( (_humidity > _cfg->target + _cfg->humidifyingTargetHigh || _cfg->mode == HC_ModeOff)
                    && _lastreadtime - _lastStateChangeTime > ToSystemTick(_cfg->minHumidifyingRunningTime)){
                        #if SerialDebug
                        Serial.printf("[Humidity Control]: humidity: %d\n", _humidity);
                        Serial.printf("[Humidity Control]: target: %d\n", _cfg->target);
                        Serial.printf("[Humidity Control]: last read time: %d\n", _lastreadtime);
                        Serial.printf("[Humidity Control]: last state change: %d\n", _lastStateChangeTime);
                        Serial.printf("[Humidity Control]: previous state: %d\n", _prevState);
                        #endif
                        _stopHumidifying();                    
                        if(_cfg->mode == HC_ModeOff) _mode = HC_ModeOff;
                }
                break;
        }
    }

    void loop(){
            if(!isChamberSensorInstalled() && !isRoomSensorInstalled()) return;
            uint32_t currenttime = millis();
            if ((currenttime - _lastreadtime) > MINIMUM_HUMIDITY_SENSOR_READ_PERIOD){                
                _lastreadtime = currenttime;
                _humidity= chamberSensor->humidity();

                #if SMART_DISPLAY
                smartDisplay.humidityData(isChamberSensorInstalled(),_humidity,isRoomSensorInstalled(),roomSensor->humidity());
                #endif

                //DBG_PRINTF("Humidity:%d\n",_humidity);
                
                #if SerialDebug
                Serial.printf("[Humidity Control]: Going through the loop with -> State: %d, Mode: %d\n", _state, _mode);
                #endif
                if( _mode != HC_ModeOff && IsValidHumidityValue(_humidity)){
                    updateState();
                }
            }
    }

    uint8_t humidity(){
        return _humidity;
    }
    bool isHumidityValid(){
        return  _humidity <=100; 
    }

    uint8_t state() {
        return _state;
    }
    
    uint8_t mode() {
        return _cfg->mode;
    }

    void setMode(uint8_t mode){
        _cfg->mode = mode;
        modeChanged();
    }    
    
    uint8_t targetRH(){
        if(_cfg->mode == HC_ModeOff)  return INVALID_HUMIDITY_VALUE;
        return _cfg->target;
    }

    void setTarget(uint8_t target){
        _cfg->target = target;
    }

    void begin(){
        _cfg =  theSettings.humidityControlSettings();
        _mode = _cfg->mode;
        _state = HC_Idle;
        // someday, the interface will be implmented. BUt now, let's use default
        _cfg->idleLow = DefaultIdleLow;
        _cfg->idleHigh = DefaultIdleHigh;
        _cfg->humidifyingTargetHigh = HumidifyingTargetHigh;
        _cfg->dehumidifyingTargetLow = DehumidifyingTargetLow;
        _cfg->minHumidifyingIdleTime  = MinimumHumidifyingIdleTime;
        _cfg->minHumidifyingRunningTime = MinimumHumidifyingRunningTime;
        _cfg->minDehumidifyingIdleTime = MinimumDehumidifyingIdleTime;
        _cfg->minDehumidifyingRunningTime = MinimumDehumidifyingRunningTime;
    }

    void modeChanged(){
        if(_mode == _cfg->mode) return;

        if(_cfg->mode == HC_ModeOff){
            // mode changed to OFF, but the minimum running time might not meet.
            if(_state == HC_Idle){
                #if SerialDebug
                Serial.print("[Humidity Control]: Stopping DEhumidifying because the Mode has been switched to off.");
                #endif
                 _mode = HC_ModeOff;
            } else if (_state == HC_Humidifying){
                if(_lastreadtime - _lastStateChangeTime > ToSystemTick(_cfg->minHumidifyingRunningTime)){
                    #if SerialDebug
                    Serial.print("[Humidity Control]: Stopping humidifying because the Mode has been switched to off.");
                    #endif
                    _stopHumidifying();
                    _mode=HC_ModeOff;
                }else{
                    // waiting for minMumidifyingRunningTime
                    #if SerialDebug
                    Serial.println("Mode switched to off, but the minimum running time has not been met yet, so continue until then ...");
                    #endif
                }
            }else if (_state == HC_Dehumidifying){
                if(_lastreadtime - _lastStateChangeTime > ToSystemTick(_cfg->minDehumidifyingRunningTime)){
                    #if SerialDebug
                    Serial.print("[Humidity Control]: Stopping DEhumidifying because the Mode has been switched to off.");
                    #endif
                    _stopDehumidifying();
                    _mode=HC_ModeOff;
                }else{
                    // waiting for minDehumidifyingRunningTime
                    #if SerialDebug
                    Serial.println("Mode switched to off, but the minimum running time has not been met yet, so continue until then ...");
                    #endif
                }
            }
        }else {
            _mode = _cfg->mode;
        }
    }
private:
    void _startHumidifying(){
        _prevState = _state;
        _state = HC_Humidifying;
        _lastStateChangeTime=_lastreadtime;
        humidifier->setActive(true);
        DBG_PRINTF("start humidifier\n");
        #if SerialDebug
        Serial.printf("[Humidity Control]: *** START humidifier ***\n");
        #endif
    }

    void _stopHumidifying(){
        _prevState = _state;
        _state = HC_Idle;
        _lastStateChangeTime=_lastreadtime;
        humidifier->setActive(false);
        DBG_PRINTF("stop humidifier\n");
        #if SerialDebug
        Serial.printf("[Humidity Control]: *** STOP Humidifier ***\n");
        #endif
    }

    void _startDehumidifying(){
        _prevState = _state;
        _state = HC_Dehumidifying;
        _lastStateChangeTime=_lastreadtime;
        dehumidifier->setActive(true);
        DBG_PRINTF("start dehumidifier\n");        
        #if SerialDebug
        Serial.printf("[Humidity Control]: *** START DEhumidifier ***\n");
        #endif
    }

    void _stopDehumidifying(){
        _prevState = _state;
        _state = HC_Idle;
        _lastStateChangeTime=_lastreadtime;
        dehumidifier->setActive(false);
        DBG_PRINTF("stop dehumidifier\n");
        #if SerialDebug
        Serial.printf("[Humidity Control]: *** STOP DEhumidifier ***\n");
        #endif
    }

    uint8_t _mode;
    uint8_t _humidity;

    HumidityControlState _state;
    HumidityControlState _prevState;    

    uint32_t _lastreadtime;
    uint32_t _lastStateChangeTime;

    HumidityControlSettings *_cfg;
};

extern HumidityControl humidityControl;

#endif
#endif