// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#include "pascal_gov/governor.h"
#include "daemon/config.h"
#include "daemon/epoll.h"
#include "daemon/paths.h"
#include "pascal_gov/compiler.h"
#include "pascal_gov/cpu.h"
#include "pascal_gov/math.h"
#include "pascal_gov/poller.h"
#include "pascal_gov/psi.h"
#include "pascal_gov/sensor.h"
#include "pascal_gov/sysfs.h"
#include "pascal_gov/thermal.h"
#include "pascal_gov/time.h"
#include <unistd.h>

static const pascal_gov_sysfs_check_strategy STRATEGY_LATENCY = {
	PASCAL_GOV_CHECK_RELATIVE, PASCAL_GOV_STRATEGY_LATENCY};

static const pascal_gov_sysfs_check_strategy STRATEGY_GRANUL = {
	PASCAL_GOV_CHECK_RELATIVE, PASCAL_GOV_STRATEGY_GRANULARITY};

static const pascal_gov_sysfs_check_strategy STRATEGY_WAKEUP = {
	PASCAL_GOV_CHECK_RELATIVE, PASCAL_GOV_STRATEGY_WAKEUP};

static const pascal_gov_sysfs_check_strategy STRATEGY_MIGRATION = {
	PASCAL_GOV_CHECK_ABSOLUTE, PASCAL_GOV_STRATEGY_MIGRATION};

static const pascal_gov_sysfs_check_strategy STRATEGY_WALT = {
	PASCAL_GOV_CHECK_ABSOLUTE, PASCAL_GOV_STRATEGY_WALT};

static const pascal_gov_sysfs_check_strategy STRATEGY_UCLAMP = {
	PASCAL_GOV_CHECK_ABSOLUTE, PASCAL_GOV_STRATEGY_UCLAMP};

int pascal_gov_governor_init(pascal_gov_context *ctx)
{
	clock_gettime(CLOCK_MONOTONIC, &ctx->last_cpu_tick);
	return 0;
}

static inline void
execute_cpu_logic(pascal_gov_context *PASCAL_GOV_RESTRICT ctx,
		  const struct timespec *PASCAL_GOV_RESTRICT now)
{
	pascal_gov_psi_data psi_data;

	int psi_status =
		pascal_gov_psi_read_state(&ctx->psi_monitor, &psi_data, now);

	if (PASCAL_GOV_UNLIKELY(psi_status != 0)) {
		pascal_gov_epoll_remove_trigger(ctx);

		if (ctx->trigger_fd >= 0) {
			pascal_gov_psi_unregister_trigger(ctx->trigger_fd);
			ctx->trigger_fd = -1;
		}

		if (pascal_gov_psi_recover(&ctx->psi_monitor,
					   PASCAL_GOV_PATH_PSI_CPU) == 0) {

			ctx->trigger_fd = pascal_gov_psi_register_trigger(
				PASCAL_GOV_PATH_PSI_CPU,
				PASCAL_GOV_CONFIG_CONTROLLER.psi_threshold_us,
				PASCAL_GOV_CONFIG_CONTROLLER.psi_window_us);

			if (ctx->trigger_fd >= 0) {
				pascal_gov_epoll_add_trigger(ctx);
			}
		}

		return;
	}

	if (pascal_gov_dt_sec(&ctx->last_battery_check, now) >=
	    (float)PASCAL_GOV_CONFIG_CONTROLLER.battery_check_interval_sec) {

		pascal_gov_sensor_read_battery(&ctx->bat_capacity_sensor,
					       &ctx->cached_battery_level);

		pascal_gov_sensor_read_bat_temp(&ctx->bat_temp_sensor,
						&ctx->cached_battery_temp);

		ctx->last_battery_check = *now;
	}

	float cpu_temp;
	pascal_gov_sensor_read_cpu_temp(&ctx->cpu_temp_sensor, &cpu_temp);

	float thermal_scale = pascal_gov_thermal_update(
		&ctx->thermal_state, cpu_temp, ctx->cached_battery_temp,
		psi_data.some.current, &PASCAL_GOV_CONFIG_THERMAL, now);

	float trend_factor =
		pascal_gov_cpu_calculate_trend_gain(psi_data.some.velocity);

	float dt_real = pascal_gov_dt_sec(&ctx->last_cpu_tick, now);
	float dt_safe = pascal_gov_clampf(dt_real, 0.000001F, 0.1F);
	ctx->last_cpu_tick = *now;

	float integral_total;
	float integral_dot;
	pascal_gov_cpu_update_integral_params(
		&ctx->cpu_load_state, ctx->cached_battery_level, dt_safe,
		&PASCAL_GOV_CONFIG_CPU_MATH, &integral_total, &integral_dot);

	bool is_break =
		psi_data.some.nis > PASCAL_GOV_CONFIG_CPU_MATH.nis_threshold;

	pascal_gov_demand_input demand_in = {
		.target_psi = psi_data.some.current,
		.psi_velocity = psi_data.some.velocity,
		.dt_real = dt_real,
		.dt_safe = dt_safe,
		.thermal_scale = thermal_scale,
		.trend_factor = trend_factor,
		.integral_total = integral_total,
		.integral_dot = integral_dot,
		.is_structural_break = is_break};

	float load_demand = pascal_gov_cpu_calculate_load_demand(
		&ctx->cpu_load_state, &demand_in, &PASCAL_GOV_CONFIG_CPU_MATH);

	float p_eff = pascal_gov_cpu_calculate_effective_pressure(
		load_demand, trend_factor, &PASCAL_GOV_CONFIG_CPU_MATH);

	int32_t next_poll = (int32_t)pascal_gov_poller_calculate_next_interval(
		&ctx->poller_state, p_eff, psi_data.some.avg300,
		psi_data.some.velocity);

	if (pascal_gov_cpu_is_transient(&ctx->cpu_load_state,
					psi_data.some.current,
					&PASCAL_GOV_CONFIG_CPU_MATH)) {

		int32_t transient_poll = (int32_t)PASCAL_GOV_CONFIG_CPU_MATH
						 .transient_poll_interval;

		if (next_poll > transient_poll)
			next_poll = transient_poll;
	}

	ctx->next_wake_ms = next_poll;

	float thermal_min_lat = pascal_gov_cpu_calculate_thermal_latency_limit(
		thermal_scale, &PASCAL_GOV_CONFIG_CPU_LIMITS);

	float current_latency;
	float current_min_granularity;
	pascal_gov_cpu_calculate_latency_and_granularity(
		p_eff, load_demand, thermal_min_lat,
		&PASCAL_GOV_CONFIG_CPU_MATH, &PASCAL_GOV_CONFIG_CPU_LIMITS,
		&current_latency, &current_min_granularity);

	float current_wakeup_granularity =
		pascal_gov_cpu_calculate_wakeup_granularity(
			p_eff, &PASCAL_GOV_CONFIG_CPU_MATH,
			&PASCAL_GOV_CONFIG_CPU_LIMITS);

	float current_migration_cost = pascal_gov_cpu_calculate_migration_cost(
		psi_data.some.velocity, p_eff, &PASCAL_GOV_CONFIG_CPU_LIMITS);

	float current_walt_init = pascal_gov_cpu_calculate_walt_init(
		p_eff, &PASCAL_GOV_CONFIG_CPU_LIMITS);

	float current_uclamp_min = pascal_gov_cpu_calculate_uclamp_min(
		p_eff, thermal_scale, &PASCAL_GOV_CONFIG_CPU_MATH,
		&PASCAL_GOV_CONFIG_CPU_LIMITS);

	uint64_t lat_u64 = pascal_gov_sanitize_to_clean_u64(
		current_latency,
		(uint64_t)PASCAL_GOV_CONFIG_CPU_LIMITS.max_latency_ns,
		PASCAL_GOV_SYSFS_QUANTIZATION_NS);

	uint64_t gran_u64 = pascal_gov_sanitize_to_clean_u64(
		current_min_granularity,
		(uint64_t)PASCAL_GOV_CONFIG_CPU_LIMITS.max_granularity_ns,
		PASCAL_GOV_SYSFS_QUANTIZATION_NS);

	uint64_t wake_u64 = pascal_gov_sanitize_to_clean_u64(
		current_wakeup_granularity,
		(uint64_t)PASCAL_GOV_CONFIG_CPU_LIMITS.max_wakeup_ns,
		PASCAL_GOV_SYSFS_QUANTIZATION_NS);

	uint64_t mig_u64 = pascal_gov_sanitize_to_clean_u64(
		current_migration_cost,
		(uint64_t)PASCAL_GOV_CONFIG_CPU_LIMITS.min_migration_cost,
		PASCAL_GOV_SYSFS_QUANTIZATION_NS);

	uint64_t walt_u64 = pascal_gov_sanitize_to_u64(
		current_walt_init,
		(uint64_t)PASCAL_GOV_CONFIG_CPU_LIMITS.min_walt_init_pct);

	uint64_t uclamp_u64 = pascal_gov_sanitize_to_u64(
		current_uclamp_min,
		(uint64_t)PASCAL_GOV_CONFIG_CPU_LIMITS.min_uclamp_min);

	pascal_gov_sysfs_update(&ctx->sched_latency, lat_u64, false,
				&STRATEGY_LATENCY);

	pascal_gov_sysfs_update(&ctx->sched_min_granularity, gran_u64, false,
				&STRATEGY_GRANUL);

	pascal_gov_sysfs_update(&ctx->sched_wakeup_granularity, wake_u64, false,
				&STRATEGY_WAKEUP);

	pascal_gov_sysfs_update(&ctx->sched_migration_cost, mig_u64, false,
				&STRATEGY_MIGRATION);

	pascal_gov_sysfs_update(&ctx->sched_walt_init, walt_u64, false,
				&STRATEGY_WALT);

	pascal_gov_sysfs_update(&ctx->sched_uclamp_min, uclamp_u64, false,
				&STRATEGY_UCLAMP);
}

void pascal_gov_governor_process_cpu(pascal_gov_context *ctx)
{
	struct timespec now;
	struct timespec end_calc;

	clock_gettime(CLOCK_MONOTONIC, &now);
	execute_cpu_logic(ctx, &now);
	clock_gettime(CLOCK_MONOTONIC, &end_calc);

	float calc_time_ms = pascal_gov_dt_sec(&now, &end_calc) * 1000.0F;
	int adjusted_wake = ctx->next_wake_ms - (int)calc_time_ms;

	ctx->next_wake_ms = (adjusted_wake > 0) ? adjusted_wake : 1;
}
