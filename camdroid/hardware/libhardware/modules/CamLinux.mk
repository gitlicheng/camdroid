hardware_modules := local_time power
include $(call all-named-subdir-makefiles,$(hardware_modules))
