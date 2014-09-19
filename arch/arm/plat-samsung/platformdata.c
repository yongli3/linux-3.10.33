/* linux/arch/arm/plat-samsung/platformdata.c
 *
 * Copyright 2010 Ben Dooks <ben-linux <at> fluff.org>
 *
 * Helper for platform data setting
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/platform_device.h>

#include <plat/devs.h>
#include <plat/sdhci.h>
#include <plat/fb.h>
#include <plat/media.h>
#include <mach/media.h>

void __init *s3c_set_platdata(void *pd, size_t pdsize,
			      struct platform_device *pdev)
{
	void *npd;

	if (!pd) {
		/* too early to use dev_name(), may not be registered */
		printk(KERN_ERR "%s: no platform data supplied\n", pdev->name);
		return NULL;
	}

	npd = kmemdup(pd, pdsize, GFP_KERNEL);
	if (!npd) {
		printk(KERN_ERR "%s: cannot clone platform data\n", pdev->name);
		return NULL;
	}

	pdev->dev.platform_data = npd;
	return npd;
}

void __init *s3c_fb0_set_platdata(void *pd, size_t pdsize,
			      struct platform_device *pdev)
{
	struct s3c_fb_platdata *npd;
	int i, default_win, num_overlay_win;
	phys_addr_t pmem_start;
	int frame_size;

	if (!pd) {
		/* too early to use dev_name(), may not be registered */
		printk(KERN_ERR "%s: no platform data supplied\n", pdev->name);
		return NULL;
	}

	npd = kmemdup(pd, pdsize, GFP_KERNEL);
	if (!npd) {
		printk(KERN_ERR "%s: cannot clone platform data\n", pdev->name);
		return NULL;
	}
	else {
		for(i=0; i<npd->nr_wins; i++)
			npd->nr_buffers[i] = 1;

		default_win = npd->default_win;
		num_overlay_win = CONFIG_FB_S3C_NUM_OVLY_WIN;

		if(num_overlay_win > default_win)
		{
			printk(KERN_WARNING "%s: NUM_OVLY_WIN should be less than default win", __func__);
			num_overlay_win = 0;
		}
		
		for (i=0; i<num_overlay_win; i++)
			npd->nr_buffers[i] = CONFIG_FB_S3C_NUM_BUF_OVLY_WIN;
		npd->nr_buffers[default_win] = CONFIG_FB_S3C_NR_BUFFERS;

		frame_size = (npd->win[0]->xres * npd->win[0]->yres * 4);

		pmem_start = s5p_get_media_memory_bank(S5P_MDEV_FIMD, 1);
		printk("---pmem_start is 0x%x\n", pmem_start);

		for(i=0; i<num_overlay_win; i++)
		{
			npd->pmem_start[i] = pmem_start;
			npd->pmem_size[i] = frame_size * npd->nr_buffers[i];
			pmem_start += npd->pmem_size[i];
		}

		npd->pmem_start[default_win] = pmem_start;
		npd->pmem_size[default_win] = frame_size * npd->nr_buffers[default_win];
	}

	pdev->dev.platform_data = npd;
	return npd;
}
void s3c_sdhci_set_platdata(struct s3c_sdhci_platdata *pd,
			     struct s3c_sdhci_platdata *set)
{
	set->cd_type = pd->cd_type;
	set->ext_cd_init = pd->ext_cd_init;
	set->ext_cd_cleanup = pd->ext_cd_cleanup;
	set->ext_cd_gpio = pd->ext_cd_gpio;
	set->ext_cd_gpio_invert = pd->ext_cd_gpio_invert;

	if (pd->max_width)
		set->max_width = pd->max_width;
	if (pd->cfg_gpio)
		set->cfg_gpio = pd->cfg_gpio;
	if (pd->host_caps)
		set->host_caps |= pd->host_caps;
	if (pd->host_caps2)
		set->host_caps2 |= pd->host_caps2;
	if (pd->pm_caps)
		set->pm_caps |= pd->pm_caps;
}
