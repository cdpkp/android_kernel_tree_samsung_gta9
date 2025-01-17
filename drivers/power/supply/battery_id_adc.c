/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/iio/consumer.h>
#include <linux/iio/iio.h>
#include <linux/platform_device.h>    /* platform device */
#include "battery_id_adc.h"
#include <linux/power/gxy_psy_sysfs.h>
static struct platform_device *g_md_adc_pdev;
static int g_adc_num;
static int g_adc_val;

static int bat_id_get_adc_info(struct device *dev)
{
    int ret, val;
    struct iio_channel *md_channel;

    md_channel = iio_channel_get(dev, "gxy-battid");

    ret = IS_ERR(md_channel);
    if (ret) {
        if (PTR_ERR(md_channel) == -EPROBE_DEFER) {
            pr_err("%s EPROBE_DEFER\r\n",
                    __func__);
            return -EPROBE_DEFER;
        }
        pr_err("fail to get iio channel (%d)\n", ret);
        goto Fail;
    }
    g_adc_num = md_channel->channel->channel;
    ret = iio_read_channel_raw(md_channel, &val);
    iio_channel_release(md_channel);
    if (ret < 0) {
        pr_err("iio_read_channel_raw fail\n");
        goto Fail;
    }

    g_adc_val = val;
    pr_info("md_ch = %d, val = %d\n", g_adc_num, g_adc_val);
    return ret;
Fail:
    return -1;
}

int bat_id_get_adc_num(void)
{
    return g_adc_num;
}
EXPORT_SYMBOL(bat_id_get_adc_num);

int bat_id_get_adc_val(void)
{
    return g_adc_val;
}
EXPORT_SYMBOL(bat_id_get_adc_val);

signed int battery_get_bat_id_voltage(void)
{
    struct iio_channel *channel;
    int ret, val, number;

    if (!g_md_adc_pdev) {
        pr_err("fail to get g_md_adc_pdev\n");
        goto BAT_Fail;
    }
    channel = iio_channel_get(&g_md_adc_pdev->dev, "gxy-battid");

    ret = IS_ERR(channel);
    if (ret) {
        pr_err("fail to get iio channel 4 (%d)\n", ret);
        goto BAT_Fail;
    }
    number = channel->channel->channel;
    ret = iio_read_channel_processed(channel, &val);
    iio_channel_release(channel);
    if (ret < 0) {
        pr_err("iio_read_channel_processed fail\n");
        goto BAT_Fail;
    }
    pr_debug("md_battery = %d, val = %d\n", number, val);

    return val;
BAT_Fail:
    pr_err("BAT_Fail, load default profile\n");
    return 0;

}
EXPORT_SYMBOL(battery_get_bat_id_voltage);

int battery_get_profile_id(void)
{
    int id_volt = 0;
    enum gxy_battery_type id;
    id_volt = battery_get_bat_id_voltage();
    /*Tab A9 code for AX6739A-1103 by lina at 20230613 start*/
    if (id_volt >= BATTERY_ATL_ID_VOLT_LOW && id_volt <= BATTERY_ATL_ID_VOLT_HIGH) {
        id = GXY_BATTERY_TYPE_ATL;
    } else if (id_volt >= BATTERY_BYD_ID_VOLT_LOW && id_volt <= BATTERY_BYD_ID_VOLT_HIGH ){
        id = GXY_BATTERY_TYPE_BYD;
    } else {
        pr_err("bat_id get fail, load default id");
        id = GXY_BATTERY_TYPE_ATL;
    }
    /*Tab A9 code for AX6739A-1103 by lina at 20230613 end*/
    pr_err("[%s] bat_id_get_adc_num:%d, id_volt:%d, bat_id:%d\n",
        __func__, bat_id_get_adc_num(), id_volt, id);
    return id;
}
EXPORT_SYMBOL(battery_get_profile_id);
int bat_id_adc_probe(struct platform_device *pdev)
{
    int ret;

    pr_info("battery_id_adc probe start.\n");
    ret = bat_id_get_adc_info(&pdev->dev);
    if (ret < 0) {
        pr_err("bat_id get adc info fail");
        return ret;
    }
    g_md_adc_pdev = pdev;
    pr_info("battery_id_adc probe end.\n");
    return 0;
}


static const struct of_device_id bat_id_auxadc_of_ids[] = {
    {.compatible = "gxy, bat_id_adc"},
    {}
};


static struct platform_driver bat_id_adc_driver = {

    .driver = {
            .name = "bat_id_adc",
            .of_match_table = bat_id_auxadc_of_ids,
    },

    .probe = bat_id_adc_probe,
};

static int __init battery_id_adc_init(void)
{
    int ret;

    ret = platform_driver_register(&bat_id_adc_driver);
    if (ret) {
        pr_err("bat_id adc driver init fail %d", ret);
        return ret;
    }
    return 0;
}

module_init(battery_id_adc_init);

MODULE_AUTHOR("zh");
MODULE_DESCRIPTION("battery adc driver");
MODULE_LICENSE("GPL");