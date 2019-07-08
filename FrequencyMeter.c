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

static void CPUFrequencyMeter_init(Meter* this) {
	display_mhz = 0.0f;
	max_mhz = -1.0f;
	return;
}

static void CPUFrequencyMeter_done(Meter* this) {
	if(cpu_info){
		CPUInfo_destroy(cpu_info);
		cpu_info = NULL;
	}
}

static void CPUFrequencyMeter_updateValues(Meter* this, char* buf, int len) {
	if(!cpu_info)
		cpu_info = CPUInfo_create();
	
	uint8_t res = CPUInfo_readProcfs(cpu_info);
	double mhz = -1.0;
	if(res == 0)
		mhz = (double) CPUInfo_getAverageFrequencyMHz(cpu_info);
	if(mhz > this->total)
		this->total = mhz;
	this->values[0] = mhz;
		
	xSnprintf(buf, len - 1, "%.1f", mhz);
}

MeterClass CPUFrequencyMeter_class = {
	.super = {
		.extends = Class(Meter),
		.delete = Meter_delete
	},
	.defaultMode = TEXT_METERMODE,
	.attributes = CPUFrequencyMeter_attributes,
	.updateValues = CPUFrequencyMeter_updateValues,
	.init = CPUFrequencyMeter_init,
	.done = CPUFrequencyMeter_done,
	.maxItems = 1,
	.total = 0.0,
	.name = "CPU_FREQ",
	.uiName = "CPU Frequency",
	.caption = "CPU MHz: "
};
