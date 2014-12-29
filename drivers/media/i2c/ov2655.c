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

#define CAM_PWR_ENABLE(gpio)                gpio_direction_output((gpio), 0);
#define CAM_PWR_DISABLE(gpio)               gpio_direction_output((gpio), 1);
#define CAM_RESET_HIGH(gpio)                gpio_direction_output((gpio), 1);
#define CAM_RESET_LOW(gpio)                 gpio_direction_output((gpio), 0);

enum gpio_id {
	GPIO_PWDN,
	GPIO_RST,
	GPIO_NUM,
};

struct ov2655_info {
	struct v4l2_subdev sd;
	struct media_pad pad;

	/* camera sensor gpio  */
	int gpios[GPIO_NUM];

	struct i2c_client *client;
};

struct ov2655_framesize {
	u16 width;
	u16 height;
};

static const struct ov2655_framesize ov2655_framesizes[] = {
	{
		.width = 800,
		.height = 600,
	},
};

/*
 *  ov2655 i2c read function
 */
static int ov2655_i2c_read(struct v4l2_subdev *sd, unsigned short reg,
			unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char data[2];
	int ret;
	
	data[0] = reg >> 8;
	data[1] = (unsigned char)reg & 0xff;
	dev_dbg(&client->dev, "data[0] = 0x%x\n", data[0]);
	dev_dbg(&client->dev, "data[1] = 0x%x\n", data[1]);

	ret = i2c_master_send(client, data, 2);
	if (ret != 2) {
		dev_err(&client->dev, "write reg error, ret = %d, addr = 0x%x\n", ret, client->addr);
		return -1;
	}

	if (i2c_master_recv(client, value, 1) != 1)
	{
		dev_err(&client->dev, "read error\n");
		return -1;
	}

	return 0;
}

/*
 *  ov2655 i2c write function
 */
static int ov2655_i2c_write(struct v4l2_subdev *sd, unsigned short reg,
			unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char data[3];
	struct i2c_msg msg;
	int ret;
	
	data[0] = reg >> 8;
	data[1] = (unsigned char)reg & 0xff;
	data[2] = value;

	msg.addr = client->addr;
	msg.len = 3;
	msg.buf = data;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret != 1)
		dev_err(&client->dev, "ov2655 i2c write error\n");

	return 0;
}

/*
 * Set power on and reset ov2655
 */
static void ov2655_power_on_and_reset(struct ov2655_info *ov2655)
{
	CAM_PWR_DISABLE(ov2655->gpios[GPIO_PWDN]);          
	msleep(20);
	CAM_PWR_ENABLE(ov2655->gpios[GPIO_PWDN]);   
	msleep(20);
	
	CAM_RESET_LOW(ov2655->gpios[GPIO_RST]); 
	msleep(50);
	CAM_RESET_HIGH(ov2655->gpios[GPIO_RST]);      
	msleep(15);
}

/*
 * Power off camera sensor
 */
static void ov2655_power_off(struct ov2655_info *ov2655)
{
	/* Power off camera */
	CAM_PWR_DISABLE(ov2655->gpios[GPIO_PWDN]);          
	msleep(20);
}

/*
 * Initilize ov2655 camera sensor
 *
 */
static void ov2655_reg_init(struct v4l2_subdev *sd)
{
	int i;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	for (i = 0; i < OV2655_INIT_REG_SIZE; i++)
		ov2655_i2c_write(sd, ov2655_init_reg[i][0], ov2655_init_reg[i][1]);	
	
	dev_dbg(&client->dev, "ov2655_reg_init finished, regcnt = %d\n", OV2655_INIT_REG_SIZE);	
}

/*
 * Read ov2655 ID and detect the camera
 * Return 0 if ov2655 is detected, or -ENODEV otherwise
 */
static int ov2655_read_id(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov2655_info *ov2655 = container_of(sd, struct ov2655_info, sd);
	int ret;
	char pid_h;

	/* power on and reset camera */
	ov2655_power_on_and_reset(ov2655);

	ret = ov2655_i2c_read(sd, 0x300a, &pid_h);
	if (ret < 0) {
		dev_err(&client->dev, "read sensor id error\n");
		return -1;
	}

	dev_dbg(&client->dev, "id = 0x%x\n", pid_h);
	ov2655_reg_init(sd);
	/* power off camera*/
	ov2655_power_off(ov2655);

	return 0;
}

static int ov2655_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
			  struct v4l2_subdev_format *fmt)
{
	return 0;
}

static int ov2655_s_stream(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	dev_dbg(&client->dev, "ov2655_s_stream, on = %d\n", on);

	return 0;
}

static int ov2655_s_power(struct v4l2_subdev *sd, int on)
{	
	struct ov2655_info *ov2655 = container_of(sd, struct ov2655_info, sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (on) {
		ov2655_power_on_and_reset(ov2655);
		ov2655_reg_init(sd);
	}
	else {
		ov2655_power_off(ov2655);
	}

	dev_dbg(&client->dev, "ov2655_s_power, on = %d\n", on);
	return 0;
}

/*
 * ov2655 get default format
 */
static void ov2655_get_default_format(struct v4l2_mbus_framefmt *mf)
{
	mf->width = ov2655_framesizes[0].width;
	mf->height = ov2655_framesizes[0].height;
}

/*
 * v4l2 subdev internal operations: open
 */
static int ov2655_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct v4l2_mbus_framefmt *mf = v4l2_subdev_get_try_format(fh, 0);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	ov2655_get_default_format(mf);
	
	dev_dbg(&client->dev, "ov2655_open\n");

	return 0;
}

static const struct v4l2_subdev_internal_ops ov2655_sd_internal_ops = {
	.open = ov2655_open,
};

static const struct v4l2_subdev_pad_ops ov2655_pad_ops = {
	.set_fmt = ov2655_set_fmt,
};

static const struct v4l2_subdev_video_ops ov2655_video_ops = {
	.s_stream = ov2655_s_stream,
};

static const struct v4l2_subdev_core_ops ov2655_core_ops = {
	.s_power = ov2655_s_power,
};

static const struct v4l2_subdev_ops ov2655_subdev_ops = {
	.core = &ov2655_core_ops,
	.pad = &ov2655_pad_ops,
	.video = &ov2655_video_ops,
};

/*
 * Configure the Reset and Power GPIOs for camera
 */
static int ov2655_configure_gpios(struct ov2655_info *ov2655,
						const struct ov2655_platform_data *pdata)
{
	int ret;

	ov2655->gpios[GPIO_PWDN] = pdata->gpio_pwdn;
	ov2655->gpios[GPIO_RST] = pdata->gpio_reset;

	ret = gpio_request(ov2655->gpios[GPIO_PWDN], "GPF34");
        if (ret)
        {
                printk(KERN_ERR "#### failed to request GPF34 for cam_pwr\n");
        }
	ret = gpio_request(ov2655->gpios[GPIO_RST], "GPE14");
        if (ret)
        {
                printk(KERN_ERR "#### failed to request GPE14 for cam_reset\n");
        }

	return 0;
}

static int ov2655_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct ov2655_info *ov2655;
	struct v4l2_subdev *sd;
	const struct ov2655_platform_data *pdata = client->dev.platform_data;

	dev_dbg(&client->dev, "%s, %d\n", __func__, __LINE__);

	ov2655 = devm_kzalloc(&client->dev, sizeof(struct ov2655_info), GFP_KERNEL);
	if (!ov2655)
		return -ENOMEM;

	ov2655->client = client;

	sd = &ov2655->sd;
	v4l2_i2c_subdev_init(sd, client, &ov2655_subdev_ops);
	strlcpy(sd->name, DRIVER_NAME, sizeof(sd->name));

	sd->internal_ops = &ov2655_sd_internal_ops;

	ret = ov2655_configure_gpios(ov2655, pdata);
	if (ret < 0)
		return ret;

	ov2655->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	ret = media_entity_init(&sd->entity, 1, &ov2655->pad, 0);
	if (ret < 0)
		return ret;

	dev_dbg(&client->dev, "%s, %d, ret = %d\n", __func__, __LINE__, ret);

	/* detect camera sensor is ov2655 */
	ret = ov2655_read_id(sd);
	if (ret < 0)
		dev_err(&client->dev, "Cann't read id of ov2655\n");

	return ret;
}

static int ov2655_remove(struct i2c_client *client)
{
	dev_dbg(&client->dev, "%s, %d\n", __func__, __LINE__);

	return 0;
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

