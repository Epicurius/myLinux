/* SPDX-License-Identifier: GPL-2.0 */
/*
 * xHCI Host Controller USB Port Register Set
 * xHCI Specification Section 5.4, Revision 1.2.
 */

/* Port Status and Control (PORTSC) 5.4.8 */
/* bit 0 - Current Connect Status (CCS) */
#define PORT_CONNECT	(1 << 0)
/* bit 1 - Port Enabled/Disabled (PED) */
#define PORT_PE		(1 << 1)
/* bit 2 - Rsvd */
/* bit 3 - Over-current Active (OCA) */
#define PORT_OC		(1 << 3)
/* bit 4 - Port Reset (PR) */
#define PORT_RESET	(1 << 4)
/*
 * bits 8:5 - Port Link State (PLS), by default '5'.
 * Reading gives the current link PM state of the port.
 * Writing sets the link state, Port Link State Write Strobe (LWS) must be set.
 */
#define PORT_PLS_MASK	(0xf << 5)
#define XDEV_U0		(0x0 << 5)
#define XDEV_U1		(0x1 << 5)
#define XDEV_U2		(0x2 << 5)
#define XDEV_U3		(0x3 << 5)
#define XDEV_DISABLED	(0x4 << 5)
#define XDEV_RXDETECT	(0x5 << 5)
#define XDEV_INACTIVE	(0x6 << 5)
#define XDEV_POLLING	(0x7 << 5)
#define XDEV_RECOVERY	(0x8 << 5)
#define XDEV_HOT_RESET	(0x9 << 5)
#define XDEV_COMP_MODE	(0xa << 5)
#define XDEV_TEST_MODE	(0xb << 5)
/* Values 12-14 are Reserved */
#define XDEV_RESUME	(0xf << 5)
/* bit 9 - Port Power (PP) */
#define PORT_POWER	(1 << 9)
/*
 * bits 13:10 - Port Speed
 * 0 - undefined speed - port hasn't be initialized by a reset yet
 * 1 - full speed
 * 2 - low speed
 * 3 - high speed
 * 4 - super speed
 * 5-15 reserved
 */
#define DEV_SPEED_MASK		(0xf << 10)
#define	XDEV_FS			(0x1 << 10)
#define	XDEV_LS			(0x2 << 10)
#define	XDEV_HS			(0x3 << 10)
#define	XDEV_SS			(0x4 << 10)
#define	XDEV_SSP		(0x5 << 10)
#define DEV_UNDEFSPEED(p)	(((p) & DEV_SPEED_MASK) == (0x0<<10))
#define DEV_FULLSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_FS)
#define DEV_LOWSPEED(p)		(((p) & DEV_SPEED_MASK) == XDEV_LS)
#define DEV_HIGHSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_HS)
#define DEV_SUPERSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_SS)
#define DEV_SUPERSPEEDPLUS(p)	(((p) & DEV_SPEED_MASK) == XDEV_SSP)
#define DEV_SUPERSPEED_ANY(p)	(((p) & DEV_SPEED_MASK) >= XDEV_SS)
#define DEV_PORT_SPEED(p)	(((p) >> 10) & 0x0f)
/* bits 15:14 - Port Indicator Control (PIC) */
#define PORT_LED_OFF	(0 << 14)
#define PORT_LED_AMBER	(1 << 14)
#define PORT_LED_GREEN	(2 << 14)
#define PORT_LED_MASK	(3 << 14)
/* bit 16 - Port Link State Write Strobe (LWS), set this when changing link state */
#define PORT_LINK_STROBE	(1 << 16)
/* bit 17 - Connect Status Change (CSC) */
#define PORT_CSC	(1 << 17)
/* bit 18 - Port Enabled/Disabled Change (PEC) */
#define PORT_PEC	(1 << 18)
/*
 * bit 19 - Warm Port Reset Change (WRC)
 * Warm reset for a USB 3.0 device is done.  A "hot" reset puts the port
 * into an enabled state, and the device into the default state.  A "warm" reset
 * also resets the link, forcing the device through the link training sequence.
 * SW can also look at the Port Reset register to see when warm reset is done.
 */
#define PORT_WRC	(1 << 19)
/* bit 20 - Over-current Change (OCC) */
#define PORT_OCC	(1 << 20)
/* bit 21 - Port Reset Change (PRC) */
#define PORT_RC		(1 << 21)
/*
 * bit 22 - Port Link State Change (PLC), set on some port link state transitions:
 *  Transition				Reason
 *  ------------------------------------------------------------------------------
 *  - U3 to Resume			Wakeup signaling from a device
 *  - Resume to Recovery to U0		USB 3.0 device resume
 *  - Resume to U0			USB 2.0 device resume
 *  - U3 to Recovery to U0		Software resume of USB 3.0 device complete
 *  - U3 to U0				Software resume of USB 2.0 device complete
 *  - U2 to U0				L1 resume of USB 2.1 device complete
 *  - U0 to U0 (???)			L1 entry rejection by USB 2.1 device
 *  - U0 to disabled			L1 entry error with USB 2.1 device
 *  - Any state to inactive		Error on USB 3.0 port
 */
#define PORT_PLC	(1 << 22)
/* bit 23 - Port Config Error Change (CEC), port failed to configure its link partner */
#define PORT_CEC	(1 << 23)
/*
 * bit 24 - Cold Attach Status (CAS)
 * xHC can set this bit to report device attached during Sx state.
 * Warm port reset should be perfomed to clear this bit and move port to connected state.
 */
#define PORT_CAS	(1 << 24)
/* bit 25 - Wake on Connect Enable (WCE) */
#define PORT_WKCONN_E	(1 << 25)
/* bit 26 - Wake on Disconnect Enable (WDE) */
#define PORT_WKDISC_E	(1 << 26)
/* bit 27 - Wake on Over-current Enable (WOE) */
#define PORT_WKOC_E	(1 << 27)
/* bits 29:28 - RsvdZ */
/* bit 30 - Device Removable (DR), for USB 3.0 roothub emulation */
#define PORT_DEV_REMOVE	(1 << 30)
/* bit 31 - Warm Port Reset (WPR), complete when PORT_WRC is '1' */
#define PORT_WR		(1 << 31)

/* bits 17, 18, 19, 20, 21, 22, 23 */
#define PORT_CHANGE_MASK	(PORT_CSC | PORT_PEC | PORT_WRC | PORT_OCC | \
				 PORT_RC | PORT_PLC | PORT_CEC)
/* bits 25, 26, 27 */
#define	PORT_WAKE_BITS	(PORT_WKOC_E | PORT_WKDISC_E | PORT_WKCONN_E)
/*
 * These bits are RO/ROS; can only be read.
 * bits 0, 3, 13:10, 24, 30
 *
 * PORT_CONNECT and DEV_SPEED_MASK are also Sticky - meaning they're in
 * the AUX well and they aren't changed by a hot, warm, or cold reset.
 */
#define	XHCI_PORT_RO	(PORT_CONNECT | PORT_OC | DEV_SPEED_MASK | PORT_CAS | PORT_DEV_REMOVE)
/*
 * These bits are RWS; writing 0 clears the bit, writing 1 sets the bit.
 * bits 8:5, 9, 15:14, 25, 26, 27
 * link state, port power, port indicator state, "wake on" enable state
 */
#define XHCI_PORT_RWS	(PORT_PLS_MASK | PORT_POWER | PORT_LED_MASK | PORT_WKCONN_E | \
			 PORT_WKDISC_E | PORT_WKOC_E)
/*
 * These bits are RW1S; writing 1 sets the bit, writing 0 has no effect.
 * bits 4, 31
 */
#define	XHCI_PORT_RW1S	(PORT_RESET | PORT_WR)
/*
 * These bits are RW1CS; writing 1 clears the bit, writing 0 has no effect.
 * bits 1, 17, 18, 19, 20, 21, 22, 23
 */
#define XHCI_PORT_RW1CS	(PORT_PE | PORT_CSC | PORT_PEC | PORT_WRC | PORT_OCC | PORT_RC | \
			 PORT_PLC | PORT_CEC)
/*
 * These bits are RW; writing 1 set the bit, writing 0 clears the bit
 * bit 16
 */
#define	XHCI_PORT_RW	(PORT_LINK_STROBE)


/* USB3 Port Power Management Status and Control (PORTPMSC) 5.4.9.1 */
/*
 * bits 7:0 - U1 Timeout, inactivity timer value for transitions into U1.
 * Timeout can be 0us to 127us (0x7f), in 1us increments.
 * Value 0xff means an infinite timeout.
 */
#define PORT_U1_TIMEOUT(p)	((p) & 0xff)
#define PORT_U1_TIMEOUT_MASK	0xff
/*
 * bits 15:8 - U2 Timeout, inactivity timer value for transitions into U2.
 * Timeout can be 0us to 65024ms (0xfe), in 256us increments.
 * Value 0xff means an infinite timeout.
 */
#define PORT_U2_TIMEOUT(p)	(((p) & 0xff) << 8)
#define PORT_U2_TIMEOUT_MASK	(0xff << 8)
/* bit 16 - Force Link PM Accept (FLA) */
/* bits 31:17 - RsvdP */

/* USB2 Port Power Management Status and Control (PORTPMSC) 5.4.9.2 */
/* bits 2:0 - L1 Status (L1S) */
#define	PORT_L1S_MASK		7
#define	PORT_L1S_SUCCESS	1
/* bit 3 - Remote Wake Enable (RWE) */
#define	PORT_RWE		(1 << 3)
/* bits 7:4 - Best Effort Service Latency (BESL) */
#define	PORT_HIRD(p)		(((p) & 0xf) << 4)
#define	PORT_HIRD_MASK		(0xf << 4)
/* bits 15:8 - L1 Device Slot */
#define	PORT_L1DS_MASK		(0xff << 8)
#define	PORT_L1DS(p)		(((p) & 0xff) << 8)
/* bit 16 - Hardware LPM Enable (HLE) */
#define	PORT_HLE		(1 << 16)
/* bits 27:17 - RsvdP */
/* bits 31:28 - Port Test Control (Test Mode) */
#define PORT_TEST_MODE_SHIFT	28

/* USB3 Port Link Info Register (PORTLI) 5.4.10.1 */
/* bits 15:0 - Link Error Count */
/* bits 19:16 - Rx Lane Count (RLC) */
#define PORT_RX_LANES(p)	(((p) >> 16) & 0xf)
/* bits 23:20 - Tx Lane Count (TLC) */
#define PORT_TX_LANES(p)	(((p) >> 20) & 0xf)
/* bits 31:24 - RsvdP */

/* USB2 Port Link Info Register (PORTLI) 5.4.10.2 */
/* bits 31:0 - RsvdP */

/* USB3 Port Hardware LPM Control Register (PORTHLPMC) 5.4.11.1 */
/* bits 15:0 - Link Soft Error Count */
/* bits 31:16 - RsvdP */

/* USB2 Port Hardware LPM Control Register (PORTHLPMC) 5.4.11.2 */
/* bits 1:0 - Host Initiated Resume Duration Mode (HIRDM) */
#define PORT_HIRDM(p)((p) & 3)
/*
 * bits 9:2 - L1 Timeout, can be 128us to 65280us (0xff), in 128us increments.
 * The default timeout is 128us.
 */
#define PORT_L1_TIMEOUT(p)(((p) & 0xff) << 2)
#define XHCI_L1_TIMEOUT		512
/* bits 13:10 - Best Effort Service Latency Deep (BESLD) */
#define PORT_BESLD(p)(((p) & 0xf) << 10)
/* bits 31:14 - RsvdP */

/*
 * Set default HIRD/BESL value to 4 (350/400us) for USB2 L1 LPM resume latency.
 * Safe to use with mixed HIRD and BESL systems (host and device) and is used
 * by other operating systems.
 *
 * XHCI 1.0 errata 8/14/12 Table 13 notes:
 * "Software should choose xHC BESL/BESLD field values that do not violate a
 * device's resume latency requirements,
 * e.g. not program values > '4' if BLC = '1' and a HIRD device is attached,
 * or not program values < '4' if BLC = '0' and a BESL device is attached.
 */
#define XHCI_DEFAULT_BESL	4

/*
 * USB3 specification define a 360ms tPollingLFPSTiemout for USB3 ports
 * to complete link training. usually link trainig completes much faster
 * so check status 10 times with 36ms sleep in places we need to wait for
 * polling to complete.
 */
#define XHCI_PORT_POLLING_LFPS_TIME  36
