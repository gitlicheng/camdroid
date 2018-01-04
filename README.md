# Welcome to Mango Pi
#About
Mango Pi is a Open Source Board
    
#Features
```
CPU:    ALLWINNER V3S ARM Cortex-A7, max frequency 1.2G
DRR2:   integrated 512M DDR2 in SOC
ROM:    32MB SPI Nor Flash
LCD:    general 40P RGB LCD FPC socket
        
WIFI:   ESP8909 in Board
Interface:  
        SDIO
        SPI
        I2C
        UART
        100M Ethernet (contain EPHY)
        OTG USB
        MIPI CSI
        SPEAKER + MIC
        
```

###Build
```
git clone https://github.com/mirkerson/Mango-Pi.git

cd mago-pi/camdriod
source build/envsetup.sh
lunch
mklichee
extract-bsp
make -j8
pack

```

The Project use Large files, so You may install git lfs in https://git-lfs.github.com/

###Join the chat at QQ Group: 560888351 
