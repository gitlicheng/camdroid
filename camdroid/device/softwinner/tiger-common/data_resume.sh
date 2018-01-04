#!/system/bin/busybox sh


BUSYBOX="/system/bin/busybox"
FILE_TAR="/databk/data_backup.tar"

$BUSYBOX echo "do data resume job..."
$BUSYBOX mkdir /databk
$BUSYBOX mount -t ext4 /dev/block/nandi /databk
if [ -f $FILE_TAR ]; then
    $BUSYBOX echo "$FILE_TAR is exist,bengin to resume data"
    $BUSYBOX tar -xf $FILE_TAR -C ./
else
   $BUSYBOX echo "$FILE_TAR  is not exist,do nothing and return"
   
fi
$BUSYBOX umount /databk
$BUSYBOX rmdir /databk
$BUSYBOX echo "resume data finish"

