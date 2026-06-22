// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#ifndef PASCAL_GOV_POLLER_H
#define PASCAL_GOV_POLLER_H

#include "pascal_gov/compiler.h"
#include <stdint.h>
#include <time.h>

typedef struct {
	uint64_t sleep_tolerance_ms;
	uint64_t quantization_step_ms;
	uint64_t hysteresis_threshold_ms;
	uint64_t noise_percent;
	float rise_factor;
	float fall_factor;
} pascal_gov_poller_config;

typedef struct {
	uint64_t current_interval;
	struct timespec last_tick;
	uint64_t target_interval;
	float weight_pressure;
	float weight_derivative;
	uint64_t rng_state;
	pascal_gov_poller_config tunables;
} pascal_gov_poller_state;

void pascal_gov_poller_init(
	pascal_gov_poller_state *PASCAL_GOV_RESTRICT state,
	float weight_pressure, float weight_derivative,
	const pascal_gov_poller_config *PASCAL_GOV_RESTRICT tunables);

uint64_t pascal_gov_poller_calculate_next_interval(
	pascal_gov_poller_state *PASCAL_GOV_RESTRICT state,
	float current_pressure, float avg300, float pressure_velocity);

#endif // PASCAL_GOV_POLLER_H
