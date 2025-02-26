// SPDX-License-Identifier: GPL-2.0

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#if defined(__i386__) || defined(__x86_64__)

#include "helpers/helpers.h"
#include "helpers/sysfs.h"

#include "cpupower_intern.h"

#define MSR_AMD_HWCR	0xc0010015

int cpufreq_has_boost_support(unsigned int cpu, int *support, int *active,
			int *states)
{
	struct cpupower_cpu_info cpu_info;
	int ret;
	unsigned long long val;

	*support = *active = *states = 0;

	ret = get_cpu_info(&cpu_info);
	if (ret)
		return ret;

	if (cpupower_cpu_info.caps & CPUPOWER_CAP_AMD_CBP) {
		*support = 1;

		/* AMD Family 0x17 does not utilize PCI D18F4 like prior
		 * families and has no fixed discrete boost states but
		 * has Hardware determined variable increments instead.
		 */

		if (cpu_info.family == 0x17 || cpu_info.family == 0x18) {
			if (!read_msr(cpu, MSR_AMD_HWCR, &val)) {
				if (!(val & CPUPOWER_AMD_CPBDIS))
					*active = 1;
			}
		} else {
			ret = amd_pci_get_num_boost_states(active, states);
			if (ret)
				return ret;
		}
	} else if (cpupower_cpu_info.caps & CPUPOWER_CAP_INTEL_IDA)
		*support = *active = 1;
	return 0;
}

int cpupower_intel_get_perf_bias(unsigned int cpu)
{
	char linebuf[MAX_LINE_LEN];
	char path[SYSFS_PATH_MAX];
	unsigned long val;
	char *endp;

	if (!(cpupower_cpu_info.caps & CPUPOWER_CAP_PERF_BIAS))
		return -1;

	snprintf(path, sizeof(path), PATH_TO_CPU "cpu%u/power/energy_perf_bias", cpu);

	if (cpupower_read_sysfs(path, linebuf, MAX_LINE_LEN) == 0)
		return -1;

	val = strtol(linebuf, &endp, 0);
	if (endp == linebuf || errno == ERANGE)
		return -1;

	return val;
}

int cpupower_intel_set_perf_bias(unsigned int cpu, unsigned int val)
{
	char path[SYSFS_PATH_MAX];
	char linebuf[3] = {};

	if (!(cpupower_cpu_info.caps & CPUPOWER_CAP_PERF_BIAS))
		return -1;

	snprintf(path, sizeof(path), PATH_TO_CPU "cpu%u/power/energy_perf_bias", cpu);
	snprintf(linebuf, sizeof(linebuf), "%d", val);

	if (cpupower_write_sysfs(path, linebuf, 3) <= 0)
		return -1;

	return 0;
}

#endif /* #if defined(__i386__) || defined(__x86_64__) */
