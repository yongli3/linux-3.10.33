/*
 * Driver header for OV2655 camera sensor chip.
 *
 * Copyright (c) 2010 Samsung Electronics, Co. Ltd
 * Contact: Cao Xin <caoxin1988@yeah.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef OV2655_H
#define OV2655_H

/**
 * @clk_rate: the clock frequency in Hz
 * @gpio_nreset: GPIO driving nRESET pin
 * @gpio_nstby: GPIO driving nSTBY pin
 */

struct ov2655_platform_data {
	unsigned long clk_rate;
	int gpio_nreset;
	int gpio_nstby;
};

#endif /* OV2655_H */
