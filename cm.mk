## Specify phone tech before including full_phone
$(call inherit-product, vendor/cm/config/gsm.mk)

# Release name
PRODUCT_RELEASE_NAME := N909

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Inherit device configuration
$(call inherit-product, device/ZTE/N909/device_N909.mk)

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := N909
PRODUCT_NAME := cm_N909
PRODUCT_BRAND := ZTE
PRODUCT_MODEL := N909
PRODUCT_MANUFACTURER := ZTE
