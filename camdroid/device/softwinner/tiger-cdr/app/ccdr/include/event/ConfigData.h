#ifndef _CONFIG_DATA_H_
#define _CONFIG_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

int cfgDataSetInt(const char *key, const int value);
int cfgDataGetInt(const char *key, int *value, const int defValue);

int cfgDataSetString(const char *key, const char *value);
int cfgDataGetString(const char *key, char *value, const char *defValue);

#ifdef __cplusplus
}
#endif

#endif	//_CONFIG_DATA_H_

