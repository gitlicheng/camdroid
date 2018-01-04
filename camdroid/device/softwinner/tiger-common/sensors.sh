#!/system/bin/busybox sh

BUSYBOX="/system/bin/busybox"
 
echo "do sensors job"
chown root:input /sys/devices/virtual/input/input*/enable
chown root:input /sys/devices/virtual/input/input*/delay
