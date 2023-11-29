/**
 * crtpservice.h - Used to send/receive link packats
 */

#ifndef __CRTPSERVICE_H__
#define __CRTPSERVICE_H__

#include <stdbool.h>

/**
 * Initialize the link task
 */
void crtpserviceInit(void);

bool crtpserviceTest(void);

#endif /* __CRTPSERVICE_H__ */

