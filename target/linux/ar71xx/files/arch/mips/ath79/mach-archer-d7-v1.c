/*
 * TP-LINK Archer C5/C7/TL-WDR4900 v2 board support
 *
 * Copyright (c) 2013 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (c) 2014 施康成 <tenninjas@tenninjas.ca>
 * Copyright (c) 2014 Imre Kaloz <kaloz@openwrt.org>
 *
 * Based on the Qualcomm Atheros AP135/AP136 reference board support code
 *   Copyright (c) 2012 Qualcomm Atheros
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <linux/pci.h>
#include <linux/phy.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/ath9k_platform.h>
#include <linux/ar8216_platform.h>

#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "dev-ap9x-pci.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-spi.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"
#include "pci.h"



#define ARCHER_D7_GPIO_LED_WLAN		12
#define ARCHER_D7_GPIO_LED_SYSTEM	14
#define ARCHER_D7_GPIO_LED_WPS		15
#define ARCHER_D7_GPIO_LED_USB		18
#define ARCHER_D7_GPIO_LED_LAN		23





#define ARCHER_C7_GPIO_LED_WLAN2G	12
#define ARCHER_C7_GPIO_LED_SYSTEM	14
#define ARCHER_C7_GPIO_LED_QSS		15
#define ARCHER_C7_GPIO_LED_WLAN5G	17
#define ARCHER_C7_GPIO_LED_USB1		18
#define ARCHER_C7_GPIO_LED_USB2		19

#define ARCHER_C7_GPIO_BTN_RFKILL	13
#define ARCHER_C7_V2_GPIO_BTN_RFKILL	23
#define ARCHER_C7_GPIO_BTN_RESET	16

#define ARCHER_C7_GPIO_USB1_POWER	22
#define ARCHER_C7_GPIO_USB2_POWER	21

#define ARCHER_C7_KEYS_POLL_INTERVAL	20	/* msecs */
#define ARCHER_C7_KEYS_DEBOUNCE_INTERVAL (3 * ARCHER_C7_KEYS_POLL_INTERVAL)

#define ARCHER_C7_WMAC_CALDATA_OFFSET	0x1000
#define ARCHER_C7_PCIE_CALDATA_OFFSET	0x5000
/*
static struct gpio_led archer_c7_leds_gpio[] __initdata = {
	{
		.name		= "tp-link:blue:qss",
		.gpio		= ARCHER_C7_GPIO_LED_QSS,
		.active_low	= 1,
	},
	{
		.name		= "tp-link:blue:system",
		.gpio		= ARCHER_C7_GPIO_LED_SYSTEM,
		.active_low	= 1,
	},
	{
		.name		= "tp-link:blue:wlan2g",
		.gpio		= ARCHER_C7_GPIO_LED_WLAN2G,
		.active_low	= 1,
	},
	{
		.name		= "tp-link:blue:wlan5g",
		.gpio		= ARCHER_C7_GPIO_LED_WLAN5G,
		.active_low	= 1,
	},
	{
		.name		= "tp-link:green:usb1",
		.gpio		= ARCHER_C7_GPIO_LED_USB1,
		.active_low	= 1,
	},
	{
		.name		= "tp-link:green:usb2",
		.gpio		= ARCHER_C7_GPIO_LED_USB2,
		.active_low	= 1,
	},
};*/

static struct gpio_led archer_d7_leds_gpio[] __initdata = {
	{
		.name		= "archer-d7-v1:white:system",
		.gpio		= ARCHER_D7_GPIO_LED_SYSTEM,
		.active_low	= 1,
	},
	{
		.name		= "archer-d7-v1:white:wlan",
		.gpio		= ARCHER_D7_GPIO_LED_WLAN,
		.active_low	= 1,
	},
	{
		.name		= "archer-d7-v1:white:wps",
		.gpio		= ARCHER_D7_GPIO_LED_WPS,
		.active_low	= 1,
	},
	{
		.name		= "archer-d7-v1:white:usb",
		.gpio		= ARCHER_D7_GPIO_LED_USB,
		.active_low	= 1,
	},
	{
		.name		= "archer-d7-v1:white:lan",
		.gpio		= ARCHER_D7_GPIO_LED_LAN,
		.active_low	= 1,
	}
};

static struct gpio_keys_button archer_c7_v2_gpio_keys[] __initdata = {
	{
		.desc		= "Reset button",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = ARCHER_C7_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= ARCHER_C7_GPIO_BTN_RESET,
		.active_low	= 1,
	},
	{
		.desc		= "RFKILL switch",
		.type		= EV_SW,
		.code		= KEY_RFKILL,
		.debounce_interval = ARCHER_C7_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= ARCHER_C7_V2_GPIO_BTN_RFKILL,
	},
};

/* GMAC0 of the AR8327 switch is connected to the QCA9558 SoC via SGMII */
static struct ar8327_pad_cfg archer_c7_ar8327_pad0_cfg = {
	.mode = AR8327_PAD_MAC_SGMII,
	.sgmii_delay_en = true,
};

/* GMAC6 of the AR8327 switch is connected to the QCA9558 SoC via RGMII */
static struct ar8327_pad_cfg archer_c7_ar8327_pad6_cfg = {
	.mode = AR8327_PAD_MAC_RGMII,
	.txclk_delay_en = true,
	.rxclk_delay_en = true,
	.txclk_delay_sel = AR8327_CLK_DELAY_SEL1,
	.rxclk_delay_sel = AR8327_CLK_DELAY_SEL2,
};

static struct ar8327_platform_data archer_c7_ar8327_data = {
	.pad0_cfg = &archer_c7_ar8327_pad0_cfg,
	.pad6_cfg = &archer_c7_ar8327_pad6_cfg,
	.port0_cfg = {
		.force_link = 1,
		.speed = AR8327_PORT_SPEED_1000,
		.duplex = 1,
		.txpause = 1,
		.rxpause = 1,
	},
	.port6_cfg = {
		.force_link = 1,
		.speed = AR8327_PORT_SPEED_1000,
		.duplex = 1,
		.txpause = 1,
		.rxpause = 1,
	},
};

static struct mdio_board_info archer_c7_mdio0_info[] = {
	{
		.bus_id = "ag71xx-mdio.0",
		.mdio_addr = 0,
		.platform_data = &archer_c7_ar8327_data,
	},
};

static void __init archer_d7_v1_setup(void)
{
	u8 *mac = (u8 *) KSEG1ADDR(0x1f01fc00);
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);
	u8 tmpmac[ETH_ALEN];

	ath79_register_gpio_keys_polled(-1, ARCHER_C7_KEYS_POLL_INTERVAL,
									ARRAY_SIZE(archer_c7_v2_gpio_keys),
									archer_c7_v2_gpio_keys);
	
	ath79_register_m25p80(NULL);
	ath79_register_leds_gpio(-1, ARRAY_SIZE(archer_d7_leds_gpio),
				 archer_d7_leds_gpio);

	ath79_init_mac(tmpmac, mac, -1);
	ath79_register_wmac(art + ARCHER_C7_WMAC_CALDATA_OFFSET, tmpmac);


	ath79_register_pci();

	mdiobus_register_board_info(archer_c7_mdio0_info,
				    ARRAY_SIZE(archer_c7_mdio0_info));
	ath79_register_mdio(0, 0x0);

	ath79_setup_qca955x_eth_cfg(QCA955X_ETH_CFG_RGMII_EN);

	/* GMAC0 is connected to the RMGII interface */
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_RGMII;
	ath79_eth0_data.phy_mask = BIT(0);
	ath79_eth0_data.mii_bus_dev = &ath79_mdio0_device.dev;
	ath79_eth0_pll_data.pll_1000 = 0x56000000;

	ath79_init_mac(ath79_eth0_data.mac_addr, mac, 1);
	ath79_register_eth(0);

	/* GMAC1 is connected to the SGMII interface */
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_SGMII;
	ath79_eth1_data.speed = SPEED_1000;
	ath79_eth1_data.duplex = DUPLEX_FULL;
	ath79_eth1_pll_data.pll_1000 = 0x03000101;

	ath79_init_mac(ath79_eth1_data.mac_addr, mac, 0);
	ath79_register_eth(1);

	gpio_request_one(ARCHER_C7_GPIO_USB1_POWER,
			 GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_FIXED,
			 "USB1 power");
	gpio_request_one(ARCHER_C7_GPIO_USB2_POWER,
			 GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_FIXED,
			 "USB2 power");
	ath79_register_usb();
}

MIPS_MACHINE(ATH79_MACH_ARCHER_D7_V1, "ARCHER-D7-V1", "TP-LINK Archer D7 v1",
             archer_d7_v1_setup);

