// SPDX-License-Identifier: GPL-2.0-only
/*
 * Oplus gauge interface adapted from the LineageOS oplus_qpnp_qg driver.
 * Keep measurement, SOC estimation and battery profiles in the shared QG driver.
 */
#include <linux/alarmtimer.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/iio/consumer.h>
#include <uapi/linux/qg.h>
#include <linux/power_supply.h>
#include <linux/slab.h>

#include "qg-sdam.h"
#include "qg-core.h"
#include "../../oplus/oplus_gauge.h"

static struct qpnp_qg *oplus_qg;

static int oplus_qg_read(enum power_supply_property prop, int divisor)
{
	union power_supply_propval val;
	int rc;

	rc = power_supply_get_property(oplus_qg->qg_psy, prop, &val);
	return rc < 0 ? rc : val.intval / divisor;
}

static int oplus_qg_voltage(void)
{
	return oplus_qg_read(POWER_SUPPLY_PROP_VOLTAGE_NOW, 1000);
}

static int oplus_qg_current(void)
{
	return oplus_qg_read(POWER_SUPPLY_PROP_CURRENT_NOW, 1000);
}

static int oplus_qg_temperature(void)
{
	return oplus_qg_read(POWER_SUPPLY_PROP_TEMP, 1);
}

static int oplus_qg_capacity(void)
{
	return oplus_qg_read(POWER_SUPPLY_PROP_CAPACITY, 1);
}

static int oplus_qg_fcc(void)
{
	return oplus_qg_read(POWER_SUPPLY_PROP_CHARGE_FULL, 1000);
}

static int oplus_qg_design_capacity(void)
{
	return oplus_qg_read(POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN, 1000);
}

static int oplus_qg_remaining_capacity(void)
{
	return oplus_qg_read(POWER_SUPPLY_PROP_CHARGE_COUNTER, 1000);
}

static int oplus_qg_soh(void)
{
	return oplus_qg_read(POWER_SUPPLY_PROP_SOH, 1);
}

static int oplus_qg_cycle_count(void)
{
	return oplus_qg_read(POWER_SUPPLY_PROP_CYCLE_COUNT, 1);
}

static bool oplus_qg_authenticate(void)
{
	int rc, voltage;

	/* Stock Ebba validates the battery ID in the 790--1040 mV range. */
	rc = iio_read_channel_processed(oplus_qg->batt_id_chan, &voltage);
	return !rc && voltage >= 790000 && voltage <= 1040000;
}

static void oplus_qg_set_full(bool full)
{
	/* The QG power-supply notifier handles full-charge transitions. */
}

static int oplus_qg_noop(void)
{
	return 0;
}

static struct oplus_gauge_operations oplus_qg_ops = {
	.get_battery_mvolts = oplus_qg_voltage,
	.get_battery_temperature = oplus_qg_temperature,
	.get_batt_design_capacity = oplus_qg_design_capacity,
	.get_batt_remaining_capacity = oplus_qg_remaining_capacity,
	.get_battery_soc = oplus_qg_capacity,
	.get_average_current = oplus_qg_current,
	.get_battery_fcc = oplus_qg_fcc,
	.get_prev_batt_fcc = oplus_qg_fcc,
	.get_battery_cc = oplus_qg_cycle_count,
	.get_battery_soh = oplus_qg_soh,
	.get_battery_authenticate = oplus_qg_authenticate,
	.get_prev_battery_mvolts = oplus_qg_voltage,
	.get_prev_battery_temperature = oplus_qg_temperature,
	.set_battery_full = oplus_qg_set_full,
	.get_prev_battery_soc = oplus_qg_capacity,
	.get_prev_average_current = oplus_qg_current,
	.get_prev_batt_remaining_capacity = oplus_qg_remaining_capacity,
	.get_battery_mvolts_2cell_max = oplus_qg_voltage,
	.get_battery_mvolts_2cell_min = oplus_qg_voltage,
	.get_prev_battery_mvolts_2cell_max = oplus_qg_voltage,
	.get_prev_battery_mvolts_2cell_min = oplus_qg_voltage,
	.update_battery_dod0 = oplus_qg_noop,
	.update_soc_smooth_parameter = oplus_qg_noop,
};

int qg_oplus_init(struct qpnp_qg *chip)
{
	struct oplus_gauge_chip *gauge;

	gauge = devm_kzalloc(chip->dev, sizeof(*gauge), GFP_KERNEL);
	if (!gauge)
		return -ENOMEM;

	oplus_qg = chip;
	gauge->dev = chip->dev;
	gauge->gauge_ops = &oplus_qg_ops;
	oplus_gauge_init(gauge);
	return 0;
}
