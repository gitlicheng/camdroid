#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/power_supply.h>
#include <linux/apm_bios.h>
#include <linux/apm-emulation.h>
#include <mach/sys_config.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/module.h>
#include "axp152_mfd.h"

#define SW_INT_IRQNO_ENMI	0
#define	AXP152_ADDR		0x60 >> 1
#define	AXP152_I2CBUS		0
#define AXP152_IRQNO		32

int pmu_used;
int pmu_twi_id;
int pmu_irq_id;
int pmu_twi_addr;

static struct axp_dev axp152_dev;

/* Reverse engineered partly from Platformx drivers */
enum axp_regls{

	vcc_ldo1,
	vcc_ldo2,
	vcc_ldo3,
	vcc_ldo4,
	vcc_ldo5,
	vcc_ldo6,
	vcc_ldo7,

	vcc_dcdc1,
	vcc_dcdc2,
	vcc_dcdc3,
	vcc_dcdc4,
};

/* The values of the various regulator constraints are obviously dependent
 * on exactly what is wired to each ldo.  Unfortunately this information is
 * not generally available.  More information has been requested from Xbow
 * but as of yet they haven't been forthcoming.
 *
 * Some of these are clearly Stargate 2 related (no way of plugging
 * in an lcd on the IM2 for example!).
 */

static struct regulator_consumer_supply ldo1_data[] = {
	{
		.supply = "axp15_ldo0",
	},
};

static struct regulator_consumer_supply ldo2_data[] = {
	{
		.supply = "axp15_rtcldo",
	},
};

static struct regulator_consumer_supply ldo3_data[] = {
	{
		.supply = "axp15_aldo1",
	},
};

static struct regulator_consumer_supply ldo4_data[] = {
	{
		.supply = "axp15_aldo2",
	},
};

static struct regulator_consumer_supply ldo5_data[] = {
	{
		.supply = "axp15_dldo1",
	},
};

static struct regulator_consumer_supply ldo6_data[] = {
	{
		.supply = "axp15_dldo2",
	},
};

static struct regulator_consumer_supply ldo7_data[] = {
	{
		.supply = "axp15_gpioldo",
	},
};

static struct regulator_consumer_supply buck1_data[] = {
	{
		.supply = "axp15_dcdc1",
	},
};

static struct regulator_consumer_supply buck2_data[] = {
	{
		.supply = "axp15_dcdc2",
	},
};

static struct regulator_consumer_supply buck3_data[] = {
	{
		.supply = "axp15_dcdc3",
	},
};

static struct regulator_consumer_supply buck4_data[] = {
	{
		.supply = "axp15_dcdc4",
	},
};

static struct regulator_init_data axp_regl_init_data[] = {
	[vcc_ldo1] = {
		.constraints = { /* board default 1.25V */
			.name = "axp15_ldo0",
			.min_uV =  2500000,
			.max_uV =  5000000,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo1_data),
		.consumer_supplies = ldo1_data,
	},
	[vcc_ldo2] = {
		.constraints = { /* board default 3.0V */
			.name = "axp15_rtcldo",
			.min_uV = 1300000,
			.max_uV = 1800000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo2_data),
		.consumer_supplies = ldo2_data,

	},
	[vcc_ldo3] = {
		.constraints = {/* default is 1.8V */
			.name = "axp15_aldo1",
			.min_uV = 1200000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo3_data),
		.consumer_supplies = ldo3_data,

	},
	[vcc_ldo4] = {
		.constraints = {
			/* board default is 3.3V */
			.name = "axp15_aldo2",
			.min_uV = 1200000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo4_data),
		.consumer_supplies = ldo4_data,
	},
	[vcc_ldo5] = {
		.constraints = { /* default 3.3V */
			.name = "axp15_dldo1",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo5_data),
		.consumer_supplies = ldo5_data,
	},
	[vcc_ldo6] = {
		.constraints = { /* default 3.3V */
			.name = "axp15_dldo2",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo6_data),
		.consumer_supplies = ldo6_data,
	},
	[vcc_ldo7] = {
		.constraints = { /* default 3.3V */
			.name = "axp15_gpioldo",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo7_data),
		.consumer_supplies = ldo7_data,
	},
	[vcc_dcdc1] = {
		.constraints = { /* default 3.3V */
			.name = "axp15_dcdc1",
			.min_uV = 1700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE ,
		},
		.num_consumer_supplies = ARRAY_SIZE(buck1_data),
		.consumer_supplies = buck1_data,
	},
	[vcc_dcdc2] = {
		.constraints = { /* default 1.24V */
			.name = "axp15_dcdc2",
			//.min_uV = DCDC2MIN * 1000,
			//.max_uV = DCDC2MAX * 1000,
			.min_uV = 700000,
			.max_uV = 2275000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(buck2_data),
		.consumer_supplies = buck2_data,
	},
	[vcc_dcdc3] = {
		.constraints = { /* default 2.5V */
			.name = "axp15_dcdc3",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(buck3_data),
		.consumer_supplies = buck3_data,
	},
	[vcc_dcdc4] = {
		.constraints = { /* default 2.5V */
			.name = "axp15_dcdc4",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(buck4_data),
		.consumer_supplies = buck4_data,
	},
};

static struct axp_funcdev_info axp_regldevs[] = {
	{
		.name = "axp15-regulator",
		.id = AXP152_ID_LDO0,
		.platform_data = &axp_regl_init_data[vcc_ldo1],
	}, {
		.name = "axp15-regulator",
		.id = AXP152_ID_LDO1,
		.platform_data = &axp_regl_init_data[vcc_ldo2],
	}, {
		.name = "axp15-regulator",
		.id = AXP152_ID_LDO2,
		.platform_data = &axp_regl_init_data[vcc_ldo3],
	}, {
		.name = "axp15-regulator",
		.id = AXP152_ID_LDO3,
		.platform_data = &axp_regl_init_data[vcc_ldo4],
	}, {
		.name = "axp15-regulator",
		.id = AXP152_ID_LDO4,
		.platform_data = &axp_regl_init_data[vcc_ldo5],
	}, {
		.name = "axp15-regulator",
		.id = AXP152_ID_LDO5,
		.platform_data = &axp_regl_init_data[vcc_ldo6],
	}, {
		.name = "axp15-regulator",
		.id = AXP152_ID_LDOIO0,
		.platform_data = &axp_regl_init_data[vcc_ldo7],
	}, {
		.name = "axp15-regulator",
		.id = AXP152_ID_DCDC1,
		.platform_data = &axp_regl_init_data[vcc_dcdc1],
	}, {
		.name = "axp15-regulator",
		.id = AXP152_ID_DCDC2,
		.platform_data = &axp_regl_init_data[vcc_dcdc2],
	}, {
		.name = "axp15-regulator",
		.id = AXP152_ID_DCDC3,
		.platform_data = &axp_regl_init_data[vcc_dcdc3],
	}, {
		.name = "axp15-regulator",
		.id = AXP152_ID_DCDC4,
		.platform_data = &axp_regl_init_data[vcc_dcdc4],
	},
};

static struct axp_funcdev_info axp_splydev[]={
	{
		.name = "axp15-supplyer",
		.id = AXP152_ID_SUPPLY,
		.platform_data = NULL,
	},
};

static struct axp_funcdev_info axp_gpiodev[]={
	{
		.name = "axp15-gpio",
		.id = AXP152_ID_GPIO,
	},
};


static struct axp_platform_data axp_pdata = {
	.num_regl_devs = ARRAY_SIZE(axp_regldevs),
	.num_sply_devs = ARRAY_SIZE(axp_splydev),
	.num_gpio_devs = ARRAY_SIZE(axp_gpiodev),
	.regl_devs = axp_regldevs,
	.sply_devs = axp_splydev,
	.gpio_devs = axp_gpiodev,
	.gpio_base = 0,
};

static struct axp_mfd_chip_ops axp152_ops[] = {
	[0] = {
		.init_chip    = axp152_init_chip,
		.enable_irqs  = axp152_enable_irqs,
		.disable_irqs = axp152_disable_irqs,
		.read_irqs    = axp152_read_irqs,
	},
};

static const struct i2c_device_id axp_i2c_id_table[] = {
	{ "axp152_board", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, axp_i2c_id_table);

static struct i2c_board_info __initdata axp_mfd_i2c_board_info[] = {
	{
		.type = "axp152_board",
		.addr = 0x36,
		.addr = AXP152_ADDR,
		.platform_data = &axp_pdata,
		.irq = SW_INT_IRQNO_ENMI,
	},
};

void axp152_power_off(void)
{
	axp_set_bits(axp152_dev.dev, POWER15_OFF_CTL , 0x80);
}

static irqreturn_t axp_mfd_irq_handler(int irq, void *data)
{
	struct axp_dev *chip = data;
	disable_irq_nosync(irq);
	(void)schedule_work(&chip->irq_work);

	return IRQ_HANDLED;
}

static int axp152_script_parser_fetch(char *main, char *sub, u32 *val, u32 size)
{
	script_item_u script_val;
	script_item_value_type_e type;

	type = script_get_item(main, sub, &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		return -1;
	}
	*val = script_val.val;
	return 0;
}

static int axp_i2c_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	int ret = 0;

	axp152_dev.client = client;
	axp152_dev.dev = &client->dev;

	i2c_set_clientdata(client, &axp152_dev);

	axp152_dev.ops = &axp152_ops[0];
	axp152_dev.attrs = axp152_mfd_attrs;
	axp152_dev.attrs_number = ARRAY_SIZE(axp152_mfd_attrs);
	axp152_dev.pdata = &axp_pdata;
	ret = axp_register_mfd(&axp152_dev);
	if (ret < 0) {
		printk("axp mfd register failed\n");
		return ret;
	}

	ret = request_irq(client->irq, axp_mfd_irq_handler,
		IRQF_SHARED|IRQF_DISABLED |IRQF_NO_SUSPEND, "axp15", &axp152_dev);
	if (ret) {
		dev_err(&client->dev, "failed to request irq %d\n",
				client->irq);
		goto out_free_chip;
	}

	return ret;


out_free_chip:
	i2c_set_clientdata(client, NULL);
	return ret;
}

static int axp_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_driver axp_i2c_driver = {
	.driver	= {
		.name	= "axp15_board",
		.owner	= THIS_MODULE,
	},
	.probe		= axp_i2c_probe,
	.remove		= axp_i2c_remove,
	.id_table	= axp_i2c_id_table,
};

static int __init axp152_board_init(void)
{
	int ret = 0;
	printk("axp152 board init \n");

	ret = axp152_script_parser_fetch("pmu1_para", "pmu_used", &pmu_used, sizeof(int));
	if (ret){
		printk("axp driver uning configuration failed(%d)\n", __LINE__);
		return -1;
	}
	if (pmu_used) {
		ret = axp152_script_parser_fetch("pmu1_para", "pmu_twi_id", &pmu_twi_id, sizeof(int));
		printk("axp para:pmu_twi_id=%d\n",pmu_twi_id);
	        if (ret)
	        {
			printk("axp driver uning configuration failed(%d)\n", __LINE__);
			pmu_twi_id = AXP152_I2CBUS;
	        }
	        ret = axp152_script_parser_fetch("pmu1_para", "pmu_irq_id", &pmu_irq_id, sizeof(int));
		printk("axp para:pmu_irq_id=%d\n",pmu_irq_id);
	        if (ret)
	        {
			printk("axp driver uning configuration failed(%d)\n", __LINE__);
			pmu_irq_id = AXP152_IRQNO;
	        }
	        ret = axp152_script_parser_fetch("pmu1_para", "pmu_twi_addr", &pmu_twi_addr, sizeof(int));
		printk("axp para:pmu_twi_addr=%d\n",pmu_twi_addr);
	        if (ret)
	        {
			printk("axp driver uning configuration failed(%d)\n", __LINE__);
			pmu_twi_addr = AXP152_ADDR;
	        }

	        axp_mfd_i2c_board_info[0].addr = pmu_twi_addr;
	        axp_mfd_i2c_board_info[0].irq = pmu_irq_id;

		ret = i2c_add_driver(&axp_i2c_driver);
		if (ret < 0) {
			printk("axp_i2c_driver add failed\n");
			return ret;
		}

		ret = i2c_register_board_info(pmu_twi_id, axp_mfd_i2c_board_info, ARRAY_SIZE(axp_mfd_i2c_board_info));
		if (ret < 0) {
			printk("axp_i2c_board_info add failed\n");
			return ret;
		}
        } else {
		ret = -1;
        }
        return ret;
}

arch_initcall(axp152_board_init);

MODULE_DESCRIPTION("X-POWERS axp board");
MODULE_AUTHOR("Weijin Zhong");
MODULE_LICENSE("GPL");

