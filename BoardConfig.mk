USE_CAMERA_STUB := true

# inherit from the proprietary version
-include vendor/ZTE/N909/BoardConfigVendor.mk

TARGET_ARCH := arm
TARGET_NO_BOOTLOADER := true
TARGET_BOARD_PLATFORM := unknown
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER := true

TARGET_BUILD_FOR_N909 := true 

TARGET_BOOTLOADER_BOARD_NAME := N909
#TARGET_USE_RMT_STORAGE_RECOVERY := false 
BOARD_KERNEL_CMDLINE := console=ttyHSL0,115200,n8 androidboot.hardware=qcom vmalloc=200M
BOARD_KERNEL_BASE := 0x00200000
BOARD_KERNEL_PAGESIZE := 4096

# fix this up by examining /proc/mtd on a running device
BOARD_BOOTIMAGE_PARTITION_SIZE := 0x105c0000
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 0x105c0000
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 0x105c0000
BOARD_USERDATAIMAGE_PARTITION_SIZE := 0x105c0000
BOARD_FLASH_BLOCK_SIZE := 131072

# user/userdebug #
TARGET_BUILD_VARIANT := userdebug

TARGET_PREBUILT_KERNEL := device/ZTE/N909/kernel

BOARD_HAS_NO_SELECT_BUTTON := true
#for recovery 
TARGET_RECOVERY_PIXEL_FORMAT :="RGBX_8888"
TARGET_RECOVERY_INITRC := device/ZTE/N909/init.rc
                          

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_USES_MMCUTILS := true
BOARD_HAS_LARGE_FILESYSTEM := true


## MIUI RECOVERY
MIUI_DEVICE_CONF := ../../../device/ZTE/N909/device.conf
MIUI_INIT_CONF := ../../../device/ZTE/N909/init.conf
## TWRP 
# Resolution 
DEVICE_RESOLUTION :=480x854
# storage for device 
TW_INTERNAL_STORAGE_PATH := "/emcc"
TW_INTERNAL_STORAGE_MOUNT_POINT := "emcc"
TW_EXTERNAL_STORAGE_PATH := "/sdcard"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "sdcard"

TW_CUSTOM_POWER_BUTTON := 116

TW_BRIGHTNESS_PATH="/sys/class/leds/lcd-backlight/brightness"

# Defaults to external storage instead of internal on dual storage devices 
TW_DEFAULT_EXTERNAL_STORAGE := true

# enable touch event logging to help debug touchscreen issues (before release )
TW_EVENT_LOGGING := true
#TW_BOARD_CUSTOM_GRAPHICS := ../../../device/ZTE/N909/recovery/twrp-graphics.c     #这是编译TWRP的选项，编译TWRP的时候，去掉最前面的 '#' ，就可以编译了。
TW_BOARD_CHN_GRAPHICS := true
TW_RECOVERY_BLACK_SCREEN := true 

#BOARD_CUSTOM_GRAPHICS := ../../../device/ZTE/N909/graphics.c
BOARD_CUSTOM_CN_GRAPHICS := ../../../device/ZTE/N909/recovery/graphics_cn.c    #这是编译中文版CWM的选项，在编译TWRP的时候，要关闭
#BOARD_USE_CUSTOM_RECOVERY_FONT := "font_cn_16x16.h"

# Vold and FS
BOARD_VOLD_MAX_PARTITIONS := 20
BOARD_VOLD_EMMC_SHARES_DEV_MAJOR := true
BOARD_VOLD_DISC_HAS_MULTIPLE_MAJORS := true
#BOARD_UMS_LUNFILE := "/sys/class/android_usb/android0/f_mass_storage/lun/file"
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/devices/platform/msm_hsusb/gadget/lun%d/file"
ADDITIONAL_DEFAULT_PROPERTIES += ro.secure=0
ADDITIONAL_DEFAULT_PROPERTIES += ro.adb.secure=0
ADDITIONAL_DEFAULT_PROPERTIES += ro.debuggable=1
ADDITIONAL_DEFAULT_PROPERTIES += ro.allow.mock.location=1
TARGET_DEVICE_RESOLUTION := 480x800
TARGET_DEVICT_WIDTH := 480
BOARD_REC_LANG_CHINESE := true

