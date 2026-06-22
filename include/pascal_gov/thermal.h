// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#ifndef PASCAL_GOV_THERMAL_H
#define PASCAL_GOV_THERMAL_H

#include "pascal_gov/compiler.h"
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#define PASCAL_GOV_SMITH_BUFFER_SIZE 512

typedef struct {
	float hard_limit_cpu;
	float hard_limit_bat;
	float sched_temp_cool;
	float sched_temp_hot;
	float kp_base;
	float ki_base;
	float kd_base;
	float kp_fast;
	float ki_fast;
	float kd_fast;
	float anti_windup_k;
	float deriv_filter_n;
	float ff_gain;
	float ff_lead_time;
	float ff_lag_time;
	float smith_gain;
	float smith_tau;
	float smith_delay_sec;
} pascal_gov_thermal_config;

typedef struct __attribute__((aligned(32))) {
	float value;
	struct timespec timestamp;
} pascal_gov_history_point;

typedef struct {
	pascal_gov_history_point delay_buffer[PASCAL_GOV_SMITH_BUFFER_SIZE];
	size_t head;
	size_t tail;
	size_t count;
	float model_output_no_delay;
} pascal_gov_smith_predictor;

typedef struct {
	float prev_y;
	float prev_u;
	bool first_run;
} pascal_gov_lead_lag_state;

typedef struct {
	struct timespec last_tick;
	float integral_accum;
	float prev_adjusted_pv;
	float prev_deriv_output;
	float prev_output_sat;
	pascal_gov_lead_lag_state feedforward;
	pascal_gov_smith_predictor smith_predictor;
} pascal_gov_thermal_state;

void pascal_gov_thermal_init(
	pascal_gov_thermal_state *PASCAL_GOV_RESTRICT state);

float pascal_gov_thermal_update(
	pascal_gov_thermal_state *PASCAL_GOV_RESTRICT state, float cpu_temp,
	float bat_temp, float psi_load,
	const pascal_gov_thermal_config *PASCAL_GOV_RESTRICT tunables,
	const struct timespec *PASCAL_GOV_RESTRICT now);

#endif // PASCAL_GOV_THERMAL_H
