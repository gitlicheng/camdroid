#ifndef AXP152_REGU_HH
#define AXP152_REGU_HH

#include <linux/mfd/axp-mfd.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

/* AXP15 Regulator Registers */
#define AXP152_LDO0		    POWER15_LDO0OUT_VOL
#define AXP152_RTC		       POWER15_STATUS
#define AXP152_ANALOG1		   POWER15_LDO34OUT_VOL
#define AXP152_ANALOG2      POWER15_LDO34OUT_VOL
#define AXP152_DIGITAL1     POWER15_LDO5OUT_VOL
#define AXP152_DIGITAL2     POWER15_LDO6OUT_VOL
#define AXP152_LDOIO0       POWER15_GPIO0_VOL

#define AXP152_DCDC1        POWER15_DC1OUT_VOL
#define AXP152_DCDC2        POWER15_DC2OUT_VOL
#define AXP152_DCDC3        POWER15_DC3OUT_VOL
#define AXP152_DCDC4        POWER15_DC4OUT_VOL

#define AXP152_LDO0EN		   POWER15_LDO0_CTL                    //REG[15H]
#define AXP152_RTCLDOEN		 POWER15_STATUS                      //REG[00H]
#define AXP152_ANALOG1EN		 POWER15_LDO3456_DC1234_CTL          //REG[12H]
#define AXP152_ANALOG2EN    POWER15_LDO3456_DC1234_CTL
#define AXP152_DIGITAL1EN   POWER15_LDO3456_DC1234_CTL
#define AXP152_DIGITAL2EN   POWER15_LDO3456_DC1234_CTL
#define AXP152_LDOI0EN      POWER15_GPIO2_CTL                   //REG[92H]

#define AXP152_DCDC1EN      POWER15_LDO3456_DC1234_CTL
#define AXP152_DCDC2EN      POWER15_LDO3456_DC1234_CTL
#define AXP152_DCDC3EN      POWER15_LDO3456_DC1234_CTL
#define AXP152_DCDC4EN      POWER15_LDO3456_DC1234_CTL

#define AXP152_BUCKMODE     POWER15_DCDC_MODESET                 //REG[80H]
#define AXP152_BUCKFREQ     POWER15_DCDC_FREQSET                 //REG[37H]

#define AXP_LDO(_pmic, _id, min, max, step, vreg, shift, nbits, ereg, ebit)	\
{									\
	.desc	= {							\
		.name	= #_pmic"_LDO" #_id,					\
		.type	= REGULATOR_VOLTAGE,				\
		.id	= _pmic##_ID_LDO##_id,				\
		.n_voltages = (step) ? ((max - min) / step + 1) : 1,	\
		.owner	= THIS_MODULE,					\
	},								\
	.min_uV		= (min) * 1000,					\
	.max_uV		= (max) * 1000,					\
	.step_uV	= (step) * 1000,				\
	.vol_reg	= _pmic##_##vreg,				\
	.vol_shift	= (shift),					\
	.vol_nbits	= (nbits),					\
	.enable_reg	= _pmic##_##ereg,				\
	.enable_bit	= (ebit),					\
}

#define AXP_BUCK(_pmic, _id, min, max, step, vreg, shift, nbits, ereg, ebit)	\
{									\
	.desc	= {							\
		.name	= #_pmic"_BUCK" #_id,					\
		.type	= REGULATOR_VOLTAGE,				\
		.id	= _pmic##_ID_BUCK##_id,				\
		.n_voltages = (step) ? ((max - min) / step + 1) : 1,	\
		.owner	= THIS_MODULE,					\
	},								\
	.min_uV		= (min) * 1000,					\
	.max_uV		= (max) * 1000,					\
	.step_uV	= (step) * 1000,				\
	.vol_reg	= _pmic##_##vreg,				\
	.vol_shift	= (shift),					\
	.vol_nbits	= (nbits),					\
	.enable_reg	= _pmic##_##ereg,				\
	.enable_bit	= (ebit),					\
}

#define AXP_DCDC(_pmic, _id, min, max, step, vreg, shift, nbits, ereg, ebit)	\
{									\
	.desc	= {							\
		.name	= #_pmic"_DCDC" #_id,					\
		.type	= REGULATOR_VOLTAGE,				\
		.id	= _pmic##_ID_DCDC##_id,				\
		.n_voltages = (step) ? ((max - min) / step + 1) : 1,	\
		.owner	= THIS_MODULE,					\
	},								\
	.min_uV		= (min) * 1000,					\
	.max_uV		= (max) * 1000,					\
	.step_uV	= (step) * 1000,				\
	.vol_reg	= _pmic##_##vreg,				\
	.vol_shift	= (shift),					\
	.vol_nbits	= (nbits),					\
	.enable_reg	= _pmic##_##ereg,				\
	.enable_bit	= (ebit),					\
}

#define AXP_SW(_pmic, _id, min, max, step, vreg, shift, nbits, ereg, ebit)	\
{									\
	.desc	= {							\
		.name	= #_pmic"_SW" #_id,					\
		.type	= REGULATOR_VOLTAGE,				\
		.id	= _pmic##_ID_SW##_id,				\
		.n_voltages = (step) ? ((max - min) / step + 1) : 1,	\
		.owner	= THIS_MODULE,					\
	},								\
	.min_uV		= (min) * 1000,					\
	.max_uV		= (max) * 1000,					\
	.step_uV	= (step) * 1000,				\
	.vol_reg	= _pmic##_##vreg,				\
	.vol_shift	= (shift),					\
	.vol_nbits	= (nbits),					\
	.enable_reg	= _pmic##_##ereg,				\
	.enable_bit	= (ebit),					\
}

#define AXP_REGU_ATTR(_name)					\
{									\
	.attr = { .name = #_name,.mode = 0644 },					\
	.show =  _name##_show,				\
	.store = _name##_store, \
}

struct axp_regulator_info {
	struct regulator_desc desc;

	int	min_uV;
	int	max_uV;
	int	step_uV;
	int	vol_reg;
	int	vol_shift;
	int	vol_nbits;
	int	enable_reg;
	int	enable_bit;
};

#endif /* AXP152_REGU_HH */
