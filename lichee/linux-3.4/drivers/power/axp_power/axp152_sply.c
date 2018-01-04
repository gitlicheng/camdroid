#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <linux/mfd/axp-mfd.h>
#include <asm/div64.h>

#include <mach/sys_config.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/mfd/axp-mfd-152.h>
#include "axp-cfg.h"

static const uint64_t AXP15_NOTIFIER_ON = ( AXP152_IRQ_PEKLO |
                                            AXP152_IRQ_PEKSH );

/* irq notifier */
static struct notifier_block irq_notifier;
static struct device *axpdev = NULL;

/* power key input_dev */
static struct input_dev * powerkey_dev = NULL;

static void axp_presslong(struct input_dev *idev)
{
	input_report_key(idev, KEY_POWER, 1);
	input_sync(idev);
	ssleep(2);
	input_report_key(idev, KEY_POWER, 0);
	input_sync(idev);
}

static void axp_pressshort(struct input_dev *idev)
{
	input_report_key(idev, KEY_POWER, 1);
	input_sync(idev);
	msleep(100);
	input_report_key(idev, KEY_POWER, 0);
	input_sync(idev);
}

/*
static void axp_keyup(struct input_dev *idev)
{
	input_report_key(idev, KEY_POWER, 0);
	input_sync(idev);
}

static void axp_keydown(struct input_dev *idev)
{
	input_report_key(idev, KEY_POWER, 1);
	input_sync(idev);
}
*/

static int axp152_irq_event(struct notifier_block *nb,
                unsigned long event, void *data)
{
	uint8_t w[5];

	w[0] = (uint8_t) ((event) & 0xFF);
	w[1] = POWER15_INTSTS2;
	w[2] = (uint8_t) ((event >> 8) & 0xFF);
	w[3] = POWER15_INTSTS3;
	w[4] = (uint8_t) ((event >> 16) & 0xFF);

	if(event & AXP152_IRQ_PEKLO) {
		axp_presslong(powerkey_dev);
	}

	if(event & AXP152_IRQ_PEKSH) {
		axp_pressshort(powerkey_dev);
	}

	printk("event = 0x%x\n",(int) event);
	axp_writes(axpdev,POWER15_INTSTS1,9,w);

	return 0;
}

static int axp15_irq_probe(struct platform_device *pdev)
{
	int ret;

	/* axp15-board dev */
	axpdev = pdev->dev.parent;

	/* register input device */
	powerkey_dev = input_allocate_device();
	if (!powerkey_dev) {
		kfree(powerkey_dev);
		pr_err("alloc powerkey input device error\n");
		return -ENODEV;
	}

	powerkey_dev->name = "axp15_powerkey";
	powerkey_dev->phys = "m1kbd/input2";
	powerkey_dev->id.bustype = BUS_HOST;
	powerkey_dev->id.vendor = 0x0001;
	powerkey_dev->id.product = 0x0001;
	powerkey_dev->id.version = 0x0100;
	powerkey_dev->open = NULL;
	powerkey_dev->close = NULL;
	powerkey_dev->dev.parent = &pdev->dev;
	set_bit(EV_KEY, powerkey_dev->evbit);
	set_bit(EV_REL, powerkey_dev->evbit);
	set_bit(KEY_POWER, powerkey_dev->keybit);
	ret = input_register_device(powerkey_dev);
	if (ret)
		printk("Unable to Register the axp15_powerkey\n");

	/* register notifier */
	irq_notifier.notifier_call = axp152_irq_event;
	ret = axp_register_notifier(axpdev, &irq_notifier, AXP15_NOTIFIER_ON);
	if (ret)
		goto err_notifier;

	/* TODO: get powerkey delay setting from sys_config */

	return 0;

err_notifier:
	input_unregister_device(powerkey_dev);
	kfree(powerkey_dev);
	return ret;
}

static int axp15_irq_remove(struct platform_device *dev)
{
    axp_unregister_notifier(axpdev, &irq_notifier, AXP15_NOTIFIER_ON);
    input_unregister_device(powerkey_dev);
    kfree(powerkey_dev);
    powerkey_dev = NULL;
    return 0;
}

static int axp15_irq_suspend(struct platform_device *dev, pm_message_t state)
{
    return 0;
}

static int axp15_irq_resume(struct platform_device *dev)
{
    return 0;
}

static void axp15_irq_shutdown(struct platform_device *dev)
{
    return;
}

static struct platform_driver axp15_irq_driver = {
	.driver = {
	    .name = "axp15-supplyer",
	    .owner  = THIS_MODULE,
	},
	.probe      = axp15_irq_probe,
	.remove     = axp15_irq_remove,
	.suspend    = axp15_irq_suspend,
	.resume     = axp15_irq_resume,
	.shutdown   = axp15_irq_shutdown,
};

static int axp15_irq_init(void)
{
	int ret =0;
	printk("axp152 sply init \n");
	ret = platform_driver_register(&axp15_irq_driver);
	return ret;
}

static void axp15_irq_exit(void)
{
  platform_driver_unregister(&axp15_irq_driver);
}

subsys_initcall(axp15_irq_init);
module_exit(axp15_irq_exit);

MODULE_DESCRIPTION("AXP15 irq handler driver");
MODULE_LICENSE("GPL");




