/**
 * info.h - Receive information requests and send them back to the client
 */
#ifndef __INFO_H__
#define __INFO_H__
 
#include "crtp.h"

/**
 * Initialize the information task
 *
 */
void infoInit();

/**
 * Battery minimum voltage before sending an automatic warning message
 */
#define INFO_BAT_WARNING 3.3

#endif //__INFO_H__

