/*
 * Omnivision OV2655 CMOS Image Sensor driver
 *
 * Copyright (C) 2013, Cao Xin <caoxin1988@yeah.net>
 *
 * Register definitions and initial settings based on a driver written
 * by Sylwester Nawrocki.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/media.h>
#include <linux/module.h>
#include <linux/ratelimit.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/videodev2.h>

#include <media/media-entity.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-image-sizes.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-mediabus.h>
#include <media/ov2655.h>

#define DRIVER_NAME  "OV2655" 

static int ov2655_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;

	dev_dbg(&client->dev, "%s, %d\n", __func__, __LINE__);

	return ret;
}

static int ov2655_remove(struct i2c_client *client)
{
	int ret;

	dev_dbg(&client->dev, "%s, %d\n", __func__, __LINE__);

	return ret;
}

/* id table to match with board config file, such as mach-smdkv210 */
static const struct i2c_device_id ov2655_id[] = {
	{ "OV2655", 0 },
	{ }
};

static struct i2c_driver ov2655_i2c_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner  = THIS_MODULE,
	},
	.probe		= ov2655_probe,
	.remove		= ov2655_remove,
	.id_table	= ov2655_id,
};

static int __init ov2655_driver_init(void)
{
	printk("%s, %d\n", __func__, __LINE__);
	return i2c_add_driver(&ov2655_i2c_driver);
}
module_init(ov2655_driver_init);

static void __exit ov2655_driver_exit(void)
{
	i2c_del_driver(&ov2655_i2c_driver);
}
module_exit(ov2655_driver_exit);

MODULE_AUTHOR("Cao Xin <caoxin1988@yeah.net>");
MODULE_DESCRIPTION("OV2655 CMOS Image Sensor driver");
MODULE_LICENSE("GPL");

