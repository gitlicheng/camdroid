#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define LOG_TAG "codec_audio"
#define LOG_NDEBUG 0
#include <utils/Log.h>

#include "hal_codec.h"


#define config_file "/system/etc/phone_volume.conf"

#define MAX_LINE_SZ 100 
#define CONF_SPACE ' '
#define CONF_COMMENT '#'

#define KEY_PHONE_EARPIECE  "PHONE_EARPIECE"
#define KEY_PHONE_HEADSET   "PHONE_HEADSET"
#define KEY_PHONE_SPEAKER   "PHONE_SPEAKER"

#define KEY_BLUETOOTH_UP    "BLUETOOTH_UP"

#define KEY_FM_HEADSET      "FM_HEADSET"
#define KEY_FM_SPEAKER      "FM_SPEAKER"

#define KEY_PHONEPN_GAIN 	"phonepn_gain"
#define KEY_MIXER_GAIN 		"mixer_gain"
#define KEY_HP_GAIN 		"hp_gain"
#define KEY_SPK_GAIN 		"spk_gain"
#define KEY_PCM_VOL 		"pcm_vol"
#define KEY_LINE_GAIN 		"line_gain"
#define KEY_PHONEPN_GAIN 	"phonepn_gain"

static void free_int_list(int *p);
static void print_array(int *array, int array_size);
static int *process_line(FILE *fp, int *ret_len);

static void free_int_list(int *p)
{
	if(p != NULL){
		free(p);
	}
}

void print_array(int *array, int array_size)
{
	int i = 0;
	for(i=0; i< array_size; i++){
		ALOGD("array[%d]=%d", i, array[i]);
	}
}

static int * process_line(FILE *fp, int *ret_len)
{
	char linebuf[MAX_LINE_SZ];
        int *val_array;
	int array_size;
	unsigned int i,j;

	char *key_val;
	char *line;

	if (fgets(linebuf, MAX_LINE_SZ, fp) == NULL){
		return NULL;	
	}

	line = strstr(linebuf,"=") + 1;

	for(i=0,j=0; j< strlen(line); j++){
		if (line[j] == ','){
			i++; 
		}
	}

	if(i==0){
		*ret_len = 0;
	}

	array_size =  i + 1;
	val_array = (int *)malloc( array_size * sizeof(int));
	//ALOGD("linebuf=%s ", linebuf);
	//ALOGD("line =%s ", line);
	//ALOGD("array_size=%d ", array_size);

	i = 0;
	key_val = strtok(line, ",");

	while(key_val != NULL){
		val_array[i++] = atoi(key_val);
	        //ALOGD(" i= %d, key_val=%d  ", i-1, val_array[i-1]);
		key_val = strtok(NULL, ",");
	}

	//ALOGD(" i= %d  ", i);
	*ret_len = i;
	return val_array;
}

int get_volume_config(struct volume_array *vol_array)
{
	FILE *fp;
	char *key_name, *key_value;
	char linebuf[MAX_LINE_SZ];
	char *line;
	int *val_array=NULL;
	int array_size=0; 
	int i=0;

	memset(vol_array, 0, 13 * 6 * sizeof(int) + sizeof(int) );

	fp = fopen (config_file, "r");
	if (fp == NULL)
	{    
		ALOGE("%s: Failed to open audio conf file at %s", __FUNCTION__, config_file);
		return -1;
	} 

	while (fgets(linebuf, MAX_LINE_SZ, fp) != NULL){
		/* trip  leading white spaces */
		while (linebuf[i] == CONF_SPACE)
			i++; 

		/* skip  commented lines */
		if (linebuf[i] == CONF_COMMENT)
			continue;

		line = (char*)&(linebuf[i]);

		if (line == NULL)
			continue;

		key_name = strtok(line, "]");

		if (key_name == NULL)
			continue;

		key_name = key_name + 1; //strip "["

		if(strcmp(key_name, KEY_PHONE_EARPIECE) == 0){
			ALOGD("key_name:%s ", key_name);

			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->earpiece_phonepn_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);


			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->earpiece_mixer_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);


			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->earpiece_hp_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array); 
		 } else if(strcmp(key_name, KEY_PHONE_HEADSET) == 0) {
			ALOGD("key_name:%s ", key_name);

			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->headset_phonepn_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);
			
			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->headset_mixer_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);

			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->headset_hp_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);
		} else if(strcmp(key_name, KEY_PHONE_SPEAKER) == 0) {
			ALOGD("key_name:%s ", key_name);

			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->speaker_phonepn_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);
			
			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->speaker_mixer_gain,val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);

			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->speaker_spk_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);

		} else if(strcmp(key_name, KEY_BLUETOOTH_UP) == 0) {
			ALOGD("key_name:%s ", key_name);

			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
		        memcpy(&(vol_array->up_pcm_gain), val_array, 1 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);
			
		} else if(strcmp(key_name, KEY_FM_HEADSET) == 0) {
			ALOGD("key_name:%s ", key_name);

			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->fm_headset_line_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);
			
			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->fm_headset_hp_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);

		} else if(strcmp(key_name, KEY_FM_SPEAKER) == 0) {
			ALOGD("key_name:%s ", key_name);

			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->fm_speaker_line_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);
			
			val_array = process_line(fp, &array_size);
			if (val_array == NULL){
				break;	
			}
			memcpy(vol_array->fm_speaker_spk_gain, val_array, 6 * sizeof(int));
			//print_array(val_array, array_size);		
			free_int_list(val_array);
	       }
	}

	fclose(fp);
	if(val_array == NULL)
		return -1;
	return 0;
}

