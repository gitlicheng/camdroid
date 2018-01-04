#define LOG_NDEBUG 1
#define LOG_TAG "CdrConfig"
#include <cutils/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/properties.h>
#include "ConfigData.h"

int cfgDataSetInt(const char *key, const int value) {
	char str[32];
	memset(str, 0, sizeof(str));
	//itoa(value, str, 10);
	sprintf(str, "%d", value);
	return property_set(key, str);
}

int cfgDataGetInt(const char *key, int *value, const int defValue) {
	char valStr[32];
	char defStr[32];

	memset(defStr, 0, sizeof(defStr));
	memset(valStr, 0, sizeof(valStr));
	
	//itoa(defValue, defStr, 10);
	sprintf(defStr, "%d", defValue);
	if(property_get(key, valStr, defStr) > 0) {
		*value = atoi(valStr);
		return 0;
	}
	
	return -1;
}

int cfgDataSetString(const char *key, const char *value) {
	return property_set(key, value);
}

int cfgDataGetString(const char *key, char *value, const char *defValue) {
	return property_get(key, value, defValue);
}
