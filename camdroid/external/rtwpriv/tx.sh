#!/system/bin/sh

echo "==============================="
echo "==== Continuous Tx testing ===="
echo "==============================="
echo "Example: tx 1 2 3"
echo "parameter 1: 802.11 mode"
echo "            b: 11b"
echo "            g: 11g"
echo "            n: 11n"
echo "parameter 2: channel: 1 - 14"
echo "parameter 3: tx power: Tx power level , the Range is 0~63"
echo "parameter 4: tx rate: i.e., 2 for 1M, 4 for 2M, 11 for 5.5M, бн, 108 for 54M, 128 for MCS0, 129 for MCS1, бн, 142 for MCS15"

rate=108

if [ "$1" = "b" ] ;then
  echo "start 11b tx: channel = $2, power = $3"
  rate=22
fi

if [ "$1" = "g" ] ;then
  echo "start 11g tx: channel = $2, power = $3"
  rate=108
fi

if [ "$1" = "n" ] ;then
  echo "start 11n tx: channel = $2, power = $3"
  rate=135
fi
rtwpriv wlan0 efuse_get realmap							# get the efuse
rtwpriv wlan0 mp_arx stop
rtwpriv wlan0 mp_ctx stop					           # stop continuous Tx
rtwpriv wlan0 mp_stop						             # exit MP mode
ifconfig wlan0 down			             # close WLAN interface

ifconfig wlan0 up							       # Enable Device for MP operation
rtwpriv wlan0 mp_start						             # enter MP mode
rtwpriv wlan0 mp_channel $2				           # set channel to 1 . 2, 3, 4~11 etc.
rtwpriv wlan0 mp_bandwidth 40M=0,shortGI=0    # set 20M mode and long GI
rtwpriv wlan0 mp_ant_tx a					           # select antenna A for operation
rtwpriv wlan0 mp_txpower patha=$3,pathb=$3    # set path A and path B Tx power level
rtwpriv wlan0 mp_rate $4					 # set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11, KK, OFDM54M = 108 N Mode: MCS0 = 128, MCS1 = 129бн..etc.
rtwpriv wlan0 mp_ctx background,pkt				       # start continuous Tx
