$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

# The gps config appropriate for this device
$(call inherit-product, device/common/gps/gps_us_supl.mk)

$(call inherit-product-if-exists, vendor/ZTE/N909/N909-vendor.mk)

DEVICE_PACKAGE_OVERLAYS += device/ZTE/N909/overlay

LOCAL_PATH := device/ZTE/N909
ifeq ($(TARGET_PREBUILT_KERNEL),)
	LOCAL_KERNEL := $(LOCAL_PATH)/kernel
else
	LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_COPY_FILES += \
    $(LOCAL_KERNEL):kernel \
    $(LOCAL_PATH)/init.usb.rc:root/init.usb.rc \
    $(LOCAL_PATH)/init.trace.rc:root/init.trace.rc  # $(LOCAL_PATH)/initlogo.rle:root/initlogo.rle \

        



$(call inherit-product, build/target/product/full.mk)

PRODUCT_BUILD_PROP_OVERRIDES += \
				BUILD_UTC_DATE=0 
				 
                               
PRODUCT_NAME := full_N909
PRODUCT_DEVICE := N909
