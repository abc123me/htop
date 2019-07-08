/*
htop - FrequencyMeter.c
(C) 2004-2011 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.

This meter written by Jeremiah B. Lowe
*/

/*{
#include "Meter.h"
}*/

#include "FrequencyMeter.h"
#include "ProcCPUInfo.h"

#include "CRT.h"

#include "stdio.h"

int CPUFrequencyMeter_attributes[] = {
	DEFAULT_COLOR
};

struct CPUInfo* cpu_info = NULL;

static void CPUFrequencyMeter_init(Meter* this) {
	if(cpu_info){
		puts("CPUInfo already declared?!");
		exit(-1);
	}
	if(!cpu_info)
		cpu_info = CPUInfo_create();
}

static void CPUFrequencyMeter_done(Meter* this) {
	if(cpu_info){
		CPUInfo_destroy(cpu_info);
		cpu_info = NULL;
	}
}

static void CPUFrequencyMeter_updateValues(Meter* this, char *buffer, int len) {
	uint8_t res = CPUInfo_readProcfs(cpu_info);
	if(!res){
		float avg = CPUInfo_getAverageFrequencyMHz(cpu_info);
		xSnprintf(buffer, len - 1, "%.1f", avg);
	} else xSnprintf(buffer, len - 1, "Error: %i", res);
}

MeterClass CPUFrequencyMeter_class = {
	.super = {
		.extends = Class(Meter),
		.delete = Meter_delete
	},
	.updateValues = CPUFrequencyMeter_updateValues,
	.defaultMode = TEXT_METERMODE,
	.maxItems = 1,
	.total = 100.0,
	.attributes = CPUFrequencyMeter_attributes,
	.init = CPUFrequencyMeter_init,
	.done = CPUFrequencyMeter_done,
	.name = "CPU Frequency",
	.uiName = "CPU Frequency",
	.caption = "CPU MHz: "
};
