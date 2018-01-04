#
# Makefile for MMC/SD card drivers
#

obj-$(CONFIG_MMC_BLOCK)		+= mmc_block.o
ifdef  CONFIG_FATFS_FS_SUPPORT
mmc_block-objs			:= block.o queue.o mmc_fatfs.o
else
mmc_block-objs			:= block.o queue.o
endif
obj-$(CONFIG_MMC_TEST)		+= mmc_test.o

obj-$(CONFIG_SDIO_UART)		+= sdio_uart.o

