#!  /bin/bash
while [ 1 ]; 
  do sync;
  echo 3 >/proc/sys/vm/drop_caches; 
  sleep 3;
done
