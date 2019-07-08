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
#include "XAlloc.h"

#include "stdio.h"

int CPUFrequencyMeter_attributes[] = {
	CPU_FREQ
};

struct CPUInfo* cpu_info = NULL;

static void CPUFrequencyMeter_done(Meter* this) {
	if(cpu_info){
		CPUInfo_destroy(cpu_info);
		cpu_info = NULL;
	}
}

static void CPUFrequencyMeter_updateValues(Meter* this, char* buf, int len) {
	int mode = this->mode;
	if(!cpu_info)
		cpu_info = CPUInfo_create();
	
	uint8_t res = CPUInfo_readProcfs(cpu_info);
	double mhz = -1.0;
	if(res == 0)
		mhz = (double) CPUInfo_getAverageFrequencyMHz(cpu_info);
	if(mhz > this->total)
		this->total = mhz;
	this->values[0] = mhz;
	
	if(mode == GRAPH_METERMODE)
		return; // Skip it
	else if(mode == LED_METERMODE)
		xSnprintf(buf, len - 1, "%.1f", mhz);
	else 
		xSnprintf(buf, len - 1, "%.1f/%.1f MHz", mhz, this->total);
}

MeterClass CPUFrequencyMeter_class = {
	.super = {
		.extends = Class(Meter),
		.delete = Meter_delete
	},
	.defaultMode = TEXT_METERMODE,
	.attributes = CPUFrequencyMeter_attributes,
	.updateValues = CPUFrequencyMeter_updateValues,
	.done = CPUFrequencyMeter_done,
	.maxItems = 1,
	.total = 0.0,
	.name = "CPU_FREQ",
	.uiName = "CPU Frequency",
	.caption = "CPU MHz: "
};
