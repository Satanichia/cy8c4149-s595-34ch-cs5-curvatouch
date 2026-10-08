# PSOC4-GENERIC BSP

## Overview

This board support package is intended for creating custom PSOC™ 4 BSPs.



To use code from the BSP, simply include a reference to `cybsp.h`.

## Features

### Kit Features:

* This is a generic template, there is no corresponding physical board and hence no board-specific macros. The user is expected to create a custom BSP with various pin/hardware details - Refer to KBA230822. Code examples using kit/board resources will not be shown for this BSP until the manifest data for the BSP is updated to include additional capabilities. Refer to ModusToolbox user guide for creating custom manifests.
* This is manifest can also be used to allow the board to show up in the ModusToolbox tools

### Kit Contents:

* NA

## BSP Configuration

The BSP has a few hooks that allow its behavior to be configured. Some of these items are enabled by default while others must be explicitly enabled. Items enabled by default are specified in the bsp.mk file. The items that are enabled can be changed by creating a custom BSP or by editing the application makefile.

Components:
* Device specific category reference (e.g.: CAT1) - This component, enabled by default, pulls in any device specific code for this board.

Defines:
* CYBSP_WIFI_CAPABLE - This define, disabled by default, causes the BSP to initialize the interface to an onboard wireless chip if it has one.
* CY_USING_HAL - This define, enabled by default in some BSPs, specifies that the HAL is intended to be used by the application. This will cause the BSP to include the applicable header file and to initialize the system level drivers.  Newer BSPs pull in the v3.x HAL, which enables itself via its own makefile, so CY_USING_HAL is not present.
* CYBSP_CUSTOM_SYSCLK_PM_CALLBACK - This define, disabled by default, causes the BSP to skip registering its default SysClk Power Management callback, if any, and instead to invoke the application-defined function `cybsp_register_custom_sysclk_pm_callback` to register an application-specific callback.

### Clock Configuration

| Clock    | Source    | Output Frequency |
|----------|-----------|------------------|
| CLK_HF   | CLK_IMO   | 48 MHz           |

### Power Configuration

* System Idle Power Mode: Deep Sleep
* VDDA Voltage: 3300 mV
* VDDD Voltage: 3300 mV

See the [BSP Setttings][settings] for additional board specific configuration settings.

## API Reference Manual

The PSOC4-GENERIC Board Support Package provides a set of APIs to configure, initialize and use the board resources.

See the [BSP API Reference Manual][api] for the complete list of the provided interfaces.

## More information
* [PSOC4-GENERIC BSP API Reference Manual][api]
* [PSOC4-GENERIC Documentation](https://github.com/infineon/TARGET_PSOC4-GENERIC)
* [Infineon Technologies AG](http://www.infineon.com)
* [Infineon GitHub](https://github.com/infineon)
* [ModusToolbox&trade;](https://www.infineon.com/design-resources/development-tools/sdk/modustoolbox-software)

[api]: https://infineon.github.io/TARGET_PSOC4-GENERIC/html/modules.html
[settings]: https://infineon.github.io/TARGET_PSOC4-GENERIC/html/md_bsp_settings.html

---
© Infineon Technologies AG or an affiliate of Infineon Technologies AG, 2019-2026.