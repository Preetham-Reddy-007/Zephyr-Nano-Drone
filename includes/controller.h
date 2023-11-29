/**
 * controller.h - Controller interface
 */
#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include "stabilizer_types.h"

typedef enum {
  ControllerTypeAutoSelect,
  ControllerTypePID,
  ControllerTypeMellinger,
  ControllerTypeINDI,
  ControllerTypeBrescianini,
#ifdef CONFIG_CONTROLLER_OOT
  ControllerTypeOot,
#endif
  ControllerType_COUNT,
} ControllerType;

void controllerInit(ControllerType controller);
bool controllerTest(void);
void controller(control_t *control, const setpoint_t *setpoint,
                                         const sensorData_t *sensors,
                                         const state_t *state,
                                         const uint32_t tick);
ControllerType controllerGetType(void);
const char* controllerGetName();


#ifdef CONFIG_CONTROLLER_OOT
void controllerOutOfTreeInit(void);
bool controllerOutOfTreeTest(void);
void controllerOutOfTree(control_t *control, const setpoint_t *setpoint, const sensorData_t *sensors, const state_t *state, const uint32_t tick);
#endif

#endif //__CONTROLLER_H__
