#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <stdint.h>

extern uint16_t Limit_Temp;
extern uint16_t Limit_Pressure;
extern uint16_t Limit_PressureMin;
extern uint16_t Limit_VinMin;  /* Ngưỡng điện áp THẤP (cảnh báo pin yếu) */
extern uint16_t Limit_VinMax;  /* Ngưỡng điện áp CAO (cảnh báo quá áp) */
extern uint16_t Limit_Oil;
extern uint16_t Limit_RPM;
extern uint16_t Limit_MaintenanceTime;

extern uint16_t VOLT_CONS;
extern uint16_t TEMP_CONS;
extern uint16_t PA_CONS;
extern uint16_t OIL_1_CONS;
extern uint16_t OIL_2_CONS;

void AlarmManager_Init(void);
void AlarmManager_Task(void);

#endif /* ALARM_MANAGER_H */
