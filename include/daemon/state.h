// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#ifndef PASCAL_GOV_DAEMON_STATE_H
#define PASCAL_GOV_DAEMON_STATE_H

#include "pascal_gov/cpu.h"
#include "pascal_gov/poller.h"
#include "pascal_gov/psi.h"
#include "pascal_gov/sensor.h"
#include "pascal_gov/sysfs.h"
#include "pascal_gov/thermal.h"
#include <stdbool.h>
#include <time.h>

struct pascal_gov_context;
typedef void (*pascal_gov_event_cb)(struct pascal_gov_context *ctx);

typedef struct PASCAL_GOV_ALIGNED(64) pascal_gov_context {
	pascal_gov_psi_monitor psi_monitor;
	pascal_gov_thermal_sensor cpu_temp_sensor;
	pascal_gov_thermal_sensor bat_temp_sensor;
	pascal_gov_battery_sensor bat_capacity_sensor;
	pascal_gov_sysfs_cache sched_latency;
	pascal_gov_sysfs_cache sched_min_granularity;
	pascal_gov_sysfs_cache sched_wakeup_granularity;
	pascal_gov_sysfs_cache sched_migration_cost;
	pascal_gov_sysfs_cache sched_walt_init;
	pascal_gov_sysfs_cache sched_uclamp_min;
	pascal_gov_thermal_state thermal_state;
	pascal_gov_load_state cpu_load_state;
	pascal_gov_poller_state poller_state;

	float cached_battery_level;
	float cached_battery_temp;

	struct timespec last_battery_check;
	struct timespec last_cpu_tick;

	int epoll_fd;
	int signal_fd;
	int trigger_fd;
	int next_wake_ms;

	volatile bool shutdown_requested;

	pascal_gov_event_cb on_trigger;
	pascal_gov_event_cb on_timeout;
} pascal_gov_context;

#endif // PASCAL_GOV_DAEMON_STATE_H
