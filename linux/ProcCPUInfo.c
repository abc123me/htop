#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#include "sys/sysinfo.h"

#include "ProcCPUInfo.h"

#define NULL_STR "(null)"

/*{
#include "stdi.h"
#include "stdint.h"
}*/

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
inline void CoreInfo_destroy(struct CoreInfo* info) {
	free(info);
}

/*{
struct CPUInfo {
	uint16_t logical_cores;
	uint16_t physical_cores;
	uint16_t coreInfo_len;
	uint32_t cache_kb;
	struct CoreInfo* coreInfo;
	char* vendor;
	char* model;
};
}*/
void CPUInfo_print(struct CPUInfo* cpu) {
	if(cpu){
		puts("--========== CPU Info ==========--");
		printf("Logical cores: %i\n", cpu->logical_cores);
		printf("Physical cores: %i\n", cpu->physical_cores);
		char* m = cpu->model; if(!m) m = NULL_STR;
		char* v = cpu->vendor; if(!v) v = NULL_STR;
		printf("Model: %s (%s)\n", m, v);
		printf("Cache: %i KB\n", cpu->cache_kb);
		if(cpu->coreInfo){
			for(uint16_t i = 0; i < cpu->coreInfo_len; i++){
				struct CoreInfo ci = cpu->coreInfo[i];
				CoreInfo_print(&ci);
			}
		}
	} else puts("CPUInfo is null!");
}
struct CPUInfo* CPUInfo_create() {
	struct CPUInfo* info = (struct CPUInfo*) malloc(sizeof(struct CPUInfo));
	uint16_t cores = get_nprocs_conf();
	info->logical_cores = cores;
	info->physical_cores = 0;
	uint16_t cilen = sizeof(struct CoreInfo);
	info->coreInfo = (struct CoreInfo*) malloc(cilen * cores);
	for(uint16_t core = 0; core < cores; core++){
		struct CoreInfo* c_info = &info->coreInfo[core];
		CoreInfo_init(c_info);
		c_info->logical_id = core;
	}
	info->coreInfo_len = cores;
	info->cache_kb = 0;
	info->model = NULL;
	info->vendor = NULL;
}
void CPUInfo_destroy(struct CPUInfo* info) {
	CoreInfo_destroy(info->coreInfo);
	if(info->vendor) free(info->vendor);
	if(info->model) free(info->model);
	free(info);
}
void CPUInfo_setModel(struct CPUInfo* info, char* model){
	if(info->model) free(info->model);
	info->model = duplicate_str(model);
}
void CPUInfo_setVendor(struct CPUInfo* info, char* vendor){
	if(info->vendor) free(info->vendor);
	info->vendor = duplicate_str(vendor);
}
void CPUInfo_readProcFile(FILE* cpuinfo, struct CPUInfo* info) {
	// Buffer is 8K so that if the CPU has tons of flags
	// then the buffer will be plenty big enough to hold the line
	uint32_t buflen = 8192, bufsize = buflen * sizeof(char);
	char* buf = (char*) malloc(bufsize);
	char* cur_line = (char*) malloc(bufsize);
	char* key = NULL; 
	char* value = NULL;
	struct CoreInfo* cur_core = NULL;
	uint32_t line_pos = 0;
	while(1) {
		size_t read = fread(buf, sizeof(char), buflen, cpuinfo);
		if(read <= 0) break;
		for(size_t i = 0; i < read; i++){
			if(buf[i] == '\n') {
				cur_line[line_pos] = NULL;
				seperate_kv_pair(&key, &value, cur_line, line_pos);
				if(key){
					if(strcmp("processor", key) == 0){
						uint16_t core = atoi(value);
						cur_core = &info->coreInfo[core];
						cur_core->logical_id = core;
					} else if(handle_cpu_pair(info, key, value))
						handle_core_pair(cur_core, key, value);
				}
				line_pos = 0;
			} else if(line_pos >= buflen) line_pos = 0;
			else cur_line[line_pos++] = buf[i];
		}
	}
	free(cur_line);
	free(buf);
}

uint8_t whitespace(char c) {
	return c < '!' || c > '~';
}
char* strtrim(char* str) {
	while(whitespace(*str)) str++;
	size_t len = strlen(str);
	if(len <= 1) return str;
	char* end = str + len;
	while(whitespace(*(--end)));
	*(++end) = NULL;
	return str;
}
void seperate_kv_pair(char** key_ptr, char** val_ptr, char* line, uint32_t len) {
	*key_ptr = NULL;
	*val_ptr = NULL;
	for(uint32_t i = 0; i < len; i++){
		if(line[i] == ':'){
			line[i] = NULL;
			*key_ptr = strtrim(line);
			uint32_t offset = i + 1;
			if(offset < len) 
				*val_ptr = strtrim(line + offset);
			break;
		}
	}
}
void handle_core_pair(struct CoreInfo* info, char* key, char* value) {
	if(strcmp("core id", key) == 0)
		info->physical_id = atoi(value);
	else if(strcmp("cpu MHz", key) == 0)
		info->freq_mhz = atof(value);
}
char* duplicate_str(char* str) {
	char* tmp = malloc(strlen(str));
	strcpy(tmp, str);
	return tmp;
}
uint8_t handle_cpu_pair(struct CPUInfo* info, char* key, char* value) {
	if(strcmp("model name", key) == 0)
		CPUInfo_setModel(info, value);
	else if(strcmp("vendor_id", key) == 0)
		CPUInfo_setVendor(info, value);
	else if(strcmp("cpu cores") == 0)
		info->physical_cores = atoi(value);
	else if(strcmp("cache size", key) == 0) 
		info->cache_kb = atoi(value);
	else return 1;
	return 0;
}
