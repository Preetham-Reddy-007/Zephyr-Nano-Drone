#include "watchdog.h"
#include <zephyr\drivers\watchdog.h>
#include <zephyr\device.h>
#include <zephyr\devicetree.h>
#include <zephyr\logging\log.h>

LOG_MODULE_REGISTER(wdt_nrfx);

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_wdt)
#define WDT_ALLOW_CALLBACK 0
#endif

#define WDT_FEED_TRIES 5

#ifndef WDT_MAX_WINDOW
#define WDT_MAX_WINDOW  353U
#endif

#ifndef WDT_MIN_WINDOW
#define WDT_MIN_WINDOW  0U
#endif

#ifndef WDG_FEED_INTERVAL
#define WDG_FEED_INTERVAL WATCHDOG_RESET_PERIOD_MS
#endif

/*WDT from device tree*/
const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));

int wdt_channel_id; //wdt channel

static int err; // Error flag

#if WDT_ALLOW_CALLBACK
static void wdt_callback(const struct device *wdt_dev, int channel_id)
{
	static bool handled_event;

	if (handled_event) {
		return;
	}

	wdt_feed(wdt_dev, channel_id);

	printk("Handled things..ready to reset\n");
	handled_event = true;
}
#endif /* WDT_ALLOW_CALLBACK */

bool watchdogNormalStartTest(void){
	if (err < 0) {
		printk("Watchdog setup error\n");
		return 0;
	}
	return 1;
}

/*Feed the watchdog*/

void watchdogReset(void){
	wdt_feed(wdt, wdt_channel_id);
}


void watchdogInit(void){

    if (!device_is_ready(wdt)) {
		printk("%s: device not ready.\n", wdt->name);
		return 0;
	}

	struct wdt_timeout_cfg wdt_config = {
		/* Reset SoC when watchdog timer expires. */
		.flags = WDT_FLAG_RESET_SOC,

		/* Expire watchdog after max window */
		.window.min = WDT_MIN_WINDOW,
		.window.max = WDT_MAX_WINDOW,
	};

	#if WDT_ALLOW_CALLBACK
	/* Set up watchdog callback. */
	wdt_config.callback = wdt_callback;
		printk("Attempting to test pre-reset callback\n");
	#else /* WDT_ALLOW_CALLBACK */
		printk("Callback in RESET_SOC disabled for this platform\n");
	#endif /* WDT_ALLOW_CALLBACK */

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id == -ENOTSUP) {
		/* IWDG driver for STM32 doesn't support callback */
		printk("Callback support rejected, continuing anyway\n");
		wdt_config.callback = NULL;
		wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	}
	if (wdt_channel_id < 0) {
		printk("Watchdog install error\n");
		return 0;
	}

	err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (err < 0) {
		printk("Watchdog setup error\n");
		return 0;
	}

	// #if WDT_MIN_WINDOW != 0
	// /* Wait opening window. */
	// k_msleep(WDT_MIN_WINDOW);
	// #endif
	// 	/* Feeding watchdog. */
	// // printk("Feeding watchdog %d times\n", WDT_FEED_TRIES);
	// // for (int i = 0; i < WDT_FEED_TRIES; ++i) {
	// // 	printk("Feeding watchdog...\n");
	// // 	wdt_feed(wdt, wdt_channel_id);
	// // 	k_sleep(K_MSEC(WDG_FEED_INTERVAL));
	// // }
}