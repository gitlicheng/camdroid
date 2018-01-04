#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <hardware/sensors.h>
#include <utils/Timers.h>

#define TAG "sensor"
#include <dragonboard.h>

#define FIFO_DEV "fifo_acc"

char* GetSensorName(int type)
{
	switch (type) {
		case SENSOR_TYPE_ACCELEROMETER:
			return "Acc";
		case SENSOR_TYPE_MAGNETIC_FIELD:
			return "Mag";
		case SENSOR_TYPE_ORIENTATION:
			return "Ori";
		case SENSOR_TYPE_GYROSCOPE:
			return "Gyr";
		case SENSOR_TYPE_LIGHT:
			return "Lgt";
		case SENSOR_TYPE_PRESSURE:
			return "Pres";
		case SENSOR_TYPE_TEMPERATURE:
			return "Temp";
		case SENSOR_TYPE_PROXIMITY:
			return "Prx";
		case SENSOR_TYPE_GRAVITY:
			return "Grv";
		case SENSOR_TYPE_LINEAR_ACCELERATION:
			return "LAcc";
		case SENSOR_TYPE_ROTATION_VECTOR:
			return "Rot";
		case SENSOR_TYPE_RELATIVE_HUMIDITY:
			return "RelHum";
		case SENSOR_TYPE_AMBIENT_TEMPERATURE:
			return "AmbTemp";
		default:
			return "others";
	}
}

int main(int argc, char **argv)
{
	int i, retVal, cntSensor, fifoFd;
	struct sensors_poll_device_t* device;
	struct sensors_module_t* module;
	struct sensor_t const* list;
	struct fifo_param fifo_data;				// until now, only rtc and senor use this struct type, when U want to tansmit dev's acquisition-data, use it! in other case, directly transmit string

	if((fifoFd = open(FIFO_DEV, O_WRONLY)) < 0) {
		if (mkfifo(FIFO_DEV, 0666) < 0) {
			db_error("mkfifo failed(%s)\n", strerror(errno));
			return -1;
		} else {
			fifoFd = open(FIFO_DEV, O_WRONLY);	// block here until server open the fifo
		}
	}
		
	retVal = hw_get_module(SENSORS_HARDWARE_MODULE_ID, (const hw_module_t **)&module);
	if (0 != retVal) {
		db_warn("get sensors module failed(%s)\n", strerror(errno));
		fifo_data.type = 'A';
		fifo_data.result = 'F';
		write(fifoFd, &fifo_data, sizeof(fifo_param_t));
		close(fifoFd);				// maybe server need chech out wheather fifo's write-endian is closed, or not there will be flood flash(has done, read-endian close fifo when receive "F" signal)
		return -1;
	}

	retVal = sensors_open(&module->common, &device);
	if (0 != retVal) {
		db_error("open sensors failed(%s)\n", strerror(errno));
		fifo_data.type = 'A';
		strcpy(fifo_data.name, ACC);
		fifo_data.result = 'F';
		write(fifoFd, &fifo_data, sizeof(fifo_param_t));
		close(fifoFd);
		return -1;
	} else {
		fifo_data.type = 'A';
		strcpy(fifo_data.name, ACC);
		fifo_data.result = 'W';
		write(fifoFd, &fifo_data, sizeof(fifo_param_t));
	}
	
	cntSensor = module->get_sensors_list(module, &list);
	if (0 == cntSensor) {
		db_warn("no sensors founded!\n");
		fifo_data.type = 'A';
		strcpy(fifo_data.name, ACC);
		fifo_data.result = 'F';
		write(fifoFd, &fifo_data, sizeof(fifo_param_t));
		close(fifoFd);
		return -1;
	}// else {
//		puts("===============sensors information================");
//		printf("%10s%10s%10s%10s\n\n", "vendor", "version", "handle", "type");
//		for (i = 0; i < cntSensor; i++) {
//			printf("[%s]\n", list[i].name);
//			printf("%10s%10d%10d%10d\n\n", list[i].vendor, list[i].version,
//				list[i].handle, list[i].type);
//		}
//	}
	
	// this 2 parts works well, I don't send info to fifo when deactive/active failed
	// but U can add it to check out sensor's driver works well or not
	for (i = 0; i < cntSensor; i++) {
		retVal = device->activate(device, list[i].handle, 0);
		if (0 != retVal)
			db_warn("deactive sensor(%s) failed(%s)\n", list[i].name, strerror(errno));
	}
	for (i = 0; i < cntSensor; i++) {
		retVal = device->activate(device, list[i].handle, 1);
		if (0 != retVal)
			db_warn("active sensor(%s) failed(%s)\n", list[i].name, strerror(errno));
		device->setDelay(device, list[i].handle, ms2ns(10));
	}

	sensors_event_t *sensordata = malloc(cntSensor * sizeof(sensors_event_t));
	if (NULL == sensordata) {
		db_error("malloc failed, memory low!\n");
		fifo_data.type = 'A';
		strcpy(fifo_data.name, ACC);
		fifo_data.result = 'M';			// M-memory low: not check out it in server(means failed) until now
		write(fifoFd, &fifo_data, sizeof(fifo_param_t));
		close(fifoFd);
	}
	db_msg("open sensor OK\n");

	while (1) {
		retVal = device->poll(device, sensordata, cntSensor);
		if (retVal < 0) {
			db_warn("poll failed(%s)\n", strerror(errno));
			fifo_data.type = 'A';
			strcpy(fifo_data.name, ACC);
			fifo_data.result = 'F';
			write(fifoFd, &fifo_data, sizeof(fifo_param_t));
			close(fifoFd);				// one error means fail and finish the tester!
		}
//		printf("%d sensor-event happens\n", retVal);
		for (i = 0; i < cntSensor; i++) {
			if (sensordata[i].version != sizeof(sensors_event_t)) {
				db_debug("incorrect event(version = %d)\n", sensordata[i].version);
			}
//			switch (sensordata[i].type) {
//				case SENSOR_TYPE_ACCELEROMETER:
//				case SENSOR_TYPE_MAGNETIC_FIELD:
//				case SENSOR_TYPE_ORIENTATION:
//				case SENSOR_TYPE_GYROSCOPE:
//				case SENSOR_TYPE_GRAVITY:
//				case SENSOR_TYPE_LINEAR_ACCELERATION:
//				case SENSOR_TYPE_ROTATION_VECTOR:
//					printf("sensor: %s, value: <%5.1f, %5.1f, %5.1f>\n",
//						GetSensorName(sensordata[i].type),
// 						sensordata[i].data[0],
//						sensordata[i].data[1], 
//						sensordata[i].data[2]);
//					break;
//				case SENSOR_TYPE_LIGHT:
//				case SENSOR_TYPE_PRESSURE:
//				case SENSOR_TYPE_TEMPERATURE:
//				case SENSOR_TYPE_PROXIMITY:
//				case SENSOR_TYPE_RELATIVE_HUMIDITY:
//				case SENSOR_TYPE_AMBIENT_TEMPERATURE:
//					printf("sensor: %s, value: <%f>\n",
//						GetSensorName(sensordata[i].type),
//						sensordata[i].data[0]);
//					break;
//				default:
//					printf("sensor: %s, value: <%f, %f, %f...>\n",
//						GetSensorName(sensordata[i].type),
//						sensordata[i].data[0],
//						sensordata[i].data[1],
//						sensordata[i].data[2]);
//					break;
//			}
			fifo_data.type = 'A';
			strcpy(fifo_data.name, ACC);
			fifo_data.result = 'P';
			fifo_data.val.acc[0] = sensordata[i].data[0];
			fifo_data.val.acc[1] = sensordata[i].data[1];
			fifo_data.val.acc[2] = sensordata[i].data[2];
			write(fifoFd, &fifo_data, sizeof(fifo_param_t));
		}
		memset(&fifo_data, 0, sizeof(fifo_param_t));
		sleep(1);
	}
	close(fifoFd);
/*
	for (i = 0; i < cntSensor; i++) {
		retVal = device->activate(device, list[i].handle, 0);
		if (retVal != 0) {
			printf("[%s] deactivate sensor(%s) failed(%s)\n",
				__FILE__, GetSensorName(list[i].type), strerror(errno));
		}
	}

	retVal = sensors_close(device);
	if (0 != retVal) {
		printf("[%s] close sensors failed\n", __FILE__);
	}
	close(fifoFd);
*/

	return 0;
}
