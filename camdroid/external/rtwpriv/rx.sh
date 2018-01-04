#!/system/bin/sh

echo "==============================="
echo "====     Air Rx testing    ===="
echo "==============================="
echo "Example: rx 1"
echo "parameter 1: channel: 1 - 14"

rtwpriv wlan0 mp_ctx stop
rtwpriv wlan0 mp_stop						                          # exit MP mode
ifconfig wlan0 down						                            # close WLAN interface

ifconfig wlan0 up						                              # Enable Device for MP operation
rtwpriv wlan0 mp_start						                          # Enter MP mode								
rtwpriv wlan0 mp_channel $1					                      # Set channel to 1 . 2, 3, 4~13 etc.
rtwpriv wlan0 mp_bandwidth 40M=0,shortGI=0			            # Set 20M mode and long GI or set to 40M is 40M=1.
rtwpriv wlan0 mp_ant_rx a
rtwpriv wlan0 mp_reset_stats					                        # Select antenna A for operation,if device have 2x2 antennam select antenna "a" or "b" and "ab" for operation.
rtwpriv wlan0 mp_arx start
echo "############Please start tx package !################"
#busybox sleep 5					                        # start air Rx teseting.
#rtwpriv wlan0 mp_query







