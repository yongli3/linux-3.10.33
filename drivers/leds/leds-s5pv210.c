/* drivers/leds/leds-s5pv210.c
 *
 * (c) 2006 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S5PV210 - LEDs GPIO driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <mach/hardware.h>
#include <mach/regs-gpio.h>
#include <linux/platform_data/leds-s5pv210.h>

/* for DRIVER_ATTR and DEVICE_ATTR test only  */
static int led_aaa_show(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t count)
{
	int var = 110;
	printk("%s, %d\n", __func__, __LINE__);
	return sprintf(buf, "%d\n", var);
}

static DEVICE_ATTR(aaa, 0644, led_aaa_show, NULL);

static struct attribute *led_device_attrs[] = {
	&dev_attr_aaa.attr,
	NULL,
};

static struct attribute_group led_device_attr_group = {
	.attrs = led_device_attrs,
};

static struct attribute_group *led_device_attr_groups[] = {
	&led_device_attr_group,
	NULL,
};

static int led_bbb_show(struct device_driver *driver, char *buf)
{
	printk("%s, %d\n", __func__, __LINE__);
	return 0;
}

static DRIVER_ATTR(bbb, 0644, led_bbb_show, NULL);

static struct attribute *led_drv_attrs[] = {
	&driver_attr_bbb.attr,
	NULL,
};

static struct attribute_group led_drv_attr_group = {
	.name  = "drv_bbb",
	.attrs = led_drv_attrs,
};

static struct attribute_group *led_drv_attr_groups[] = {
	&led_drv_attr_group,
	NULL,
};

/* our context */

struct s5pv210_gpio_led {
	struct led_classdev		 cdev;
	struct s5pv210_led_platdata	*pdata;
};

static inline struct s5pv210_gpio_led *pdev_to_gpio(struct platform_device *dev)
{
	return platform_get_drvdata(dev);
}

static inline struct s5pv210_gpio_led *to_gpio(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct s5pv210_gpio_led, cdev);
}

static void s5pv210_led_set(struct led_classdev *led_cdev,
			    enum led_brightness value)
{
	struct s5pv210_gpio_led *led = to_gpio(led_cdev);
	struct s5pv210_led_platdata *pd = led->pdata;
	int state = (value ? 1 : 0) ^ (pd->flags & S5PV210_LEDF_ACTLOW);

	/* there will be a short delay between setting the output and
	 * going from output to input when using tristate. */

	gpio_set_value(pd->gpio, state);

	if (pd->flags & S5PV210_LEDF_TRISTATE) {
		if (value)
			gpio_direction_output(pd->gpio, state);
		else
			gpio_direction_input(pd->gpio);
	}
}

static int s5pv210_led_remove(struct platform_device *dev)
{
	struct s5pv210_gpio_led *led = pdev_to_gpio(dev);

	led_classdev_unregister(&led->cdev);

	return 0;
}

static int s5pv210_led_probe(struct platform_device *dev)
{
	struct s5pv210_led_platdata *pdata = dev->dev.platform_data;
	struct s5pv210_gpio_led *led;
	int ret;

	led = devm_kzalloc(&dev->dev, sizeof(struct s5pv210_gpio_led),
			   GFP_KERNEL);
	if (led == NULL) {
		dev_err(&dev->dev, "No memory for device\n");
		return -ENOMEM;
	}

	platform_set_drvdata(dev, led);

	led->cdev.brightness_set = s5pv210_led_set;
	led->cdev.default_trigger = pdata->def_trigger;
	led->cdev.name = pdata->name;
	led->cdev.flags |= LED_CORE_SUSPENDRESUME;

	led->pdata = pdata;

	ret = devm_gpio_request(&dev->dev, pdata->gpio, "S5PV210_LED");
	if (ret < 0) {	
		dev_err(&dev->dev, "gpio request failed, caoxin\n");
		return ret;
	}

	/* no point in having a pull-up if we are always driving */

	if (pdata->flags & S5PV210_LEDF_TRISTATE)
		gpio_direction_input(pdata->gpio);
	else
		gpio_direction_output(pdata->gpio,
			pdata->flags & S5PV210_LEDF_ACTLOW ? 1 : 0);

	ret = sysfs_create_group(&dev->dev.kobj, &led_device_attr_group);

	/* register our new led device */

	ret = led_classdev_register(&dev->dev, &led->cdev);
	if (ret < 0)
		dev_err(&dev->dev, "led_classdev_register failed\n");

	printk("caoxin, %s, %d\n", __func__, __LINE__);

	return ret;
}

static struct platform_driver s5pv210_led_driver = {
	.probe		= s5pv210_led_probe,
	.remove		= s5pv210_led_remove,
	.driver		= {
		.name		= "s5pv210_led",
		.owner		= THIS_MODULE,
	},
};

module_platform_driver(s5pv210_led_driver);

MODULE_AUTHOR("Cao Xin <caoxin1988@yeah.net>");
MODULE_DESCRIPTION("S5PV210 LED driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s5pv210_led");
