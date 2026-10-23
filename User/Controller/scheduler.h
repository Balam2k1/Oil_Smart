#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

typedef void (*TaskFunction_t)(void);

void Scheduler_Init(void);
void Scheduler_Run(void);
void Scheduler_AddTask(TaskFunction_t pFunc, uint32_t PeriodMs);

#endif /* SCHEDULER_H */
