
#ifndef ___MB_COMMON_H___
#define ___MB_COMMON_H___

#include "iagent_alarm.h"

typedef enum {
    A_Bus_Cant_Connect = 0,
    A_Bus_Disconnected,
    Max_Bus_Alarms
}mb_bus_alarm_id_t;

#define ALARM_ID_BUS(id)  (ALARM_SECTION_MODBUS_BUS + id * 10)

enum {
    A_Slave_Continous_Error,
    Max_Slave_Alarms
};

#define ALARM_ID_SLAVE(id)  (ALARM_SECTION_MODBUS_SLAVE + id * 10)


typedef enum {
    A_Broker_Bootup_Fail = 0,
    A_Broker_Crash,
    A_Broker_Illigal_Cfg,
    A_Broker_Redis_Error,
    A_Broker_Modbus_Not_Configured,
    A_Broker_No_Work,
    Max_Broker_Alarms
}mb_broker_alarm_id_t;

#define ALARM_ID_BROKER(id)  (ALARM_SECTION_MODBUS_BROKER + id * 10)



typedef enum {
    Evt_Broker_Into_Service = 0
} mb_broker_event_id_t;

#define EVENT_ID_BROKER(id)  (EVENT_SECTION_MODBUS_BROKER + id)


#endif