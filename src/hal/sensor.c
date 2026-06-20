// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#include "pascal_gov/sensor.h"
#include "pascal_gov/parser.h"
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#define TEMP_SCALE_MILLI_THRESHOLD 10000.0F
#define TEMP_SCALE_DECI_THRESHOLD 100.0F

void pascal_gov_sensor_init_thermal(
	pascal_gov_thermal_sensor *PASCAL_GOV_RESTRICT sensor, int fd,
	float default_val)
{
	sensor->fd = fd;
	sensor->default_val = default_val;
}

void pascal_gov_sensor_init_battery(
	pascal_gov_battery_sensor *PASCAL_GOV_RESTRICT sensor, int fd)
{
	sensor->fd = fd;
}

int pascal_gov_sensor_read_temp(
	pascal_gov_thermal_sensor *PASCAL_GOV_RESTRICT sensor,
	float *PASCAL_GOV_RESTRICT out_temp)
{
	if (sensor->fd < 0) {
		*out_temp = sensor->default_val;
		return -EBADF;
	}

	ssize_t bytes_read =
		pread(sensor->fd, sensor->buffer, sizeof(sensor->buffer), 0);

	if (bytes_read <= 0) {
		*out_temp = sensor->default_val;
		return -EIO;
	}

	bool has_digits;

	int32_t parsed_val = pascal_gov_parse_i32(
		sensor->buffer, (size_t)bytes_read, &has_digits);

	if (!has_digits) {
		*out_temp = sensor->default_val;
		return -EINVAL;
	}

	float final_temp = (float)parsed_val;
	float abs_temp = (final_temp < 0.0F) ? -final_temp : final_temp;

	if (PASCAL_GOV_LIKELY(abs_temp >= TEMP_SCALE_MILLI_THRESHOLD))
		*out_temp = final_temp * 0.001F;
	else if (abs_temp >= TEMP_SCALE_DECI_THRESHOLD)
		*out_temp = final_temp * 0.1F;
	else
		*out_temp = final_temp;

	return 0;
}

int pascal_gov_sensor_read_battery(
	pascal_gov_battery_sensor *PASCAL_GOV_RESTRICT sensor,
	float *PASCAL_GOV_RESTRICT out_capacity)
{
	if (sensor->fd < 0) {
		*out_capacity = 100.0F;
		return -EBADF;
	}

	ssize_t bytes_read =
		pread(sensor->fd, sensor->buffer, sizeof(sensor->buffer), 0);

	if (bytes_read <= 0) {
		*out_capacity = 100.0F;
		return -EIO;
	}

	bool has_digits;

	int32_t parsed_val = pascal_gov_parse_i32(
		sensor->buffer, (size_t)bytes_read, &has_digits);

	*out_capacity = has_digits ? (float)parsed_val : 100.0F;

	return 0;
}
