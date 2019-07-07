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

#include "CRT.h"

#include "stdio.h"

int CPUFrequencyMeter_attributes[] = {
	DEFAULT_COLOR
};

static void CPUFrequencyMeter_init(Meter* this) {
	
}

static void CPUFrequencyMeter_updateValues(Meter* this, char *buffer, int len) {
	xSnprintf(buffer, len - 1, "%s", "insert text here2");
	
	return;
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
	.name = "CPU Frequency",
	.uiName = "CPU Frequency",
	.caption = "CPU MHz: "
};
