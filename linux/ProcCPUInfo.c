#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/sysinfo.h"

#include "XAlloc.h"

#include "ProcCPUInfo.h"

#define PROC_FS_CPUINFO "/proc/cpuinfo"
#define PROC_FS_BUFLEN 512
#define PROC_FS_LINE_BUFLEN 4096
#define CPU_INFO_MODEL_MAX_LEN 128
#define CPU_INFO_VENDOR_MAX_LEN 32
#define CPU_INFO_NO_MODEL_MSG "No model"
#define CPU_INFO_NO_VENDOR_MSG "No vendor"

static char* NULL_STR = "(null)";
static char* EMPTY_STR = "";

/*{
#include "stdio.h"
#include "stdint.h"
}*/

static inline uint8_t whitespace(char c) {
	return c < '!' || c > '~';
}

static char* strtrim(char* str) {
	if(!str) return NULL;
	char* end = str + strlen(str);
	while(str < end && whitespace(*str))
		str++; //Strip off leading whitespace until were out of memory
	while(end > str && whitespace(*(--end)));
	// Strip off trailing whitespace until we pass the beginning
	*(++end) = '\x00'; // Add the new null-terminator
	return str;
}

static void seperate_kv_pair(char** key_ptr, char** val_ptr, char* line, uint32_t len) {
	*key_ptr = NULL; //Initialize with NULL
	*val_ptr = NULL; //Initialize with NULL
	for(uint32_t i = 0; i < len; i++){
		if(line[i] == ':'){
			line[i] = '\x00';
			*key_ptr = strtrim(line);
			uint32_t offset = i + 1;
			if(offset < len) 
				*val_ptr = strtrim(line + offset);
			break;
		}
	}
}

static uint8_t handle_core_pair(struct CoreInfo* info, char* key, char* value) {
	if(strcmp("core id", key) == 0)
		info->physical_id = atoi(value);
	else if(strcmp("cpu MHz", key) == 0)
		info->freq_mhz = atof(value);
	else return 0;
	return 1;
}

static uint8_t handle_cpu_pair(struct CPUInfo* info, char* key, char* value) {
	if(strcmp("model name", key) == 0)
		CPUInfo_setModel(info, value);
	else if(strcmp("vendor_id", key) == 0)
		CPUInfo_setVendor(info, value);
	else if(strcmp("cpu cores", key) == 0)
		info->physical_cores = atoi(value);
	else if(strcmp("cache size", key) == 0) 
		info->cache_kb = atoi(value);
	else return 0;
	return 1;
}

/*{
struct CoreInfo {
	float freq_mhz;
	uint16_t physical_id;
	uint16_t logical_id;
};
}*/
void CoreInfo_print(struct CoreInfo* core) {
	if(core){
		printf("--======== Core %i ========--\n", core->logical_id);
		printf("Speed: %.1f MHz\n", core->freq_mhz);
		printf("Physical core: %i\n", core->physical_id);
	} else puts("CoreInfo is null!");
}
void CoreInfo_init(struct CoreInfo* info) {
	info->freq_mhz = -1;
	info->physical_id = 0;
	info->logical_id = 0;
}

/*{
struct CPUInfo {
	uint16_t logical_cores;
	uint16_t physical_cores;
	uint32_t cache_kb;
	struct CoreInfo* coreInfo; //Use CPUInfo_getCore(struct CPUInfo* info, uint16_t core) to get a core
	char vendor[CPU_INFO_VENDOR_MAX_LEN]; 
	char model[CPU_INFO_MODEL_MAX_LEN];
};
}*/
void CPUInfo_print(struct CPUInfo* cpu) {
	if(cpu){
		puts("--========== CPU Info ==========--");
		printf("Logical cores: %i\n", cpu->logical_cores);
		printf("Physical cores: %i\n", cpu->physical_cores);
		char* m = cpu->model; if(!m) m = "null";
		char* v = cpu->vendor; if(!v) v = "null";
		printf("Model: %s (%s)\n", m, v);
		printf("Cache: %i KB\n", cpu->cache_kb);
		printf("Average frequency: %.1f MHz\n", CPUInfo_getAverageFrequencyMHz(cpu));
		for(uint16_t i = 0; i < cpu->logical_cores; i++){
			struct CoreInfo* ci = CPUInfo_getCore(cpu, i);
			if(ci) CoreInfo_print(ci);
			else printf("CoreInfo struct [%i] is null!", i);
		}
	} else puts("CPUInfo is null!");
}
struct CPUInfo* CPUInfo_create() {
	uint16_t cores = get_nprocs_conf();
	struct CPUInfo* info = (struct CPUInfo*) xMalloc(sizeof(struct CPUInfo));
	info->coreInfo = (struct CoreInfo*) xMalloc(sizeof(struct CoreInfo) * cores);
	info->logical_cores = cores;
	info->physical_cores = 0;
	for(uint16_t core = 0; core < cores; core++){
		struct CoreInfo* c_info = CPUInfo_getCore(info, core);
		if(!c_info) { puts("Dafuq?!"); exit(-1); } 
		CoreInfo_init(c_info);
		c_info->logical_id = core;
	}
	info->cache_kb = 0;
	CPUInfo_setModel(info, NULL);
	CPUInfo_setVendor(info, NULL);
	return info;
}
void CPUInfo_destroy(struct CPUInfo* info) {
	if(!info){
		puts("CPUInfo is null, attempt to free() null");
		exit(-1);
	}
	if(info->coreInfo)
		free(info->coreInfo);
	free(info);
}
void CPUInfo_setModel(struct CPUInfo* info, char* str) {
	if(!str) str = CPU_INFO_NO_MODEL_MSG;
	strncpy(info->model, str, CPU_INFO_MODEL_MAX_LEN);
}
void CPUInfo_setVendor(struct CPUInfo* info, char* str) {
	if(!str) str = CPU_INFO_NO_VENDOR_MSG;
	strncpy(info->vendor, str, CPU_INFO_VENDOR_MAX_LEN);
}
float CPUInfo_getAverageFrequencyMHz(struct CPUInfo* info) {
	float freq = 0;
	uint16_t j = 0;
	for(uint16_t i = 0; i < info->logical_cores; i++){
		struct CoreInfo* core_info = CPUInfo_getCore(info, i);
		if(!core_info) continue;
		freq += core_info->freq_mhz;
		j++;
	}
	freq /= j;
	return freq;
}
// CPUInfo_readProcfs
// 0 - Success
// 1 - Failed to open /proc/cpuinfo
// 2 - CPUInfo struct* is null
// 3 - CPUInfo_getCore returned null?!
static char procfs_buf[PROC_FS_BUFLEN];
static char procfs_line_buf[PROC_FS_LINE_BUFLEN];
uint8_t CPUInfo_readProcfs(struct CPUInfo* cpu_info) {
	FILE* proc_cpuinfo_fp = fopen(PROC_FS_CPUINFO, "r");
	if(!proc_cpuinfo_fp) return 1;
	if(!cpu_info) return 2;
	char* key = NULL; 
	char* value = NULL;
	struct CoreInfo* core_info = NULL;
	uint32_t line_pos = 0;
	while(1) {
		size_t bytes_read = fread(procfs_buf, 1, PROC_FS_BUFLEN, proc_cpuinfo_fp);
		if(bytes_read <= 0) break;
		for(size_t i = 0; i < bytes_read; i++){
			if(procfs_buf[i] == '\n') {
				procfs_line_buf[line_pos] = '\x00'; //Add a null terminator to EOL
				seperate_kv_pair(&key, &value, procfs_line_buf, line_pos);
				
				if(key){ //If the key is null, ignore it
					if(strcmp("processor", key) == 0){ //It is the logical core ID?
						uint16_t logical_id = atoi(value);
						core_info = CPUInfo_getCore(cpu_info, logical_id); //Get the pre-allocated CoreInfo struct
						if(core_info) core_info->logical_id = logical_id;  //Set the CoreInfo's logical ID
						else { return 3; }
					} else if(!handle_cpu_pair(cpu_info, key, value)) //Could it be CPU specific?
						if(core_info) handle_core_pair(core_info, key, value); //Could it be core specific?
				}
				
				line_pos = 0; //Reset line buffer
			} else if(line_pos >= PROC_FS_LINE_BUFLEN) line_pos = 0;
			else procfs_line_buf[line_pos++] = procfs_buf[i];
		}
	}
	fclose(proc_cpuinfo_fp);
	return 0;
}
struct CoreInfo* CPUInfo_getCore(struct CPUInfo* info, uint16_t core) {
	if(info && info->coreInfo && core < info->logical_cores){
		return &(info->coreInfo[core]);
	} else return NULL;
}
