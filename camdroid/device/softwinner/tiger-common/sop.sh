source_PATH=/data/data/com.android.audiopatch/files/libswa1.so
dest_PATH=/system/lib/libswa1.so
source_PATH2=/data/data/com.android.audiopatch/files/libswa2.so
dest_PATH2=/system/lib/libswa2.so
source_PATH3=/data/data/com.android.audiopatch/files/librm.so
dest_PATH3=/system/lib/librm.so
source_PATH4=/data/data/com.android.audiopatch/files/libdemux_rmvb.so
dest_PATH4=/system/lib/libdemux_rmvb.so
BUSYBOX=/system/bin/busybox

echo "-----start sop--------"
if [ -f $source_PATH ]; then
	$BUSYBOX mount -o remount,rw /dev/block/nandd /system
	$BUSYBOX cp $source_PATH $dest_PATH	
	$BUSYBOX chmod 0755 $dest_PATH
	$BUSYBOX cp $source_PATH2 $dest_PATH2	
	$BUSYBOX chmod 0755 $dest_PATH2
	$BUSYBOX cp $source_PATH3 $dest_PATH3	
	$BUSYBOX chmod 0755 $dest_PATH3
	$BUSYBOX cp $source_PATH4 $dest_PATH4	
	$BUSYBOX chmod 0755 $dest_PATH4
	$BUSYBOX sync
	$BUSYBOX rm $source_PATH
	$BUSYBOX rm $source_PATH2
	$BUSYBOX rm $source_PATH3
	$BUSYBOX rm $source_PATH4
	$BUSYBOX mount -o remount,ro /dev/block/nandd /system
fi
echo "-----end sop--------"


	
