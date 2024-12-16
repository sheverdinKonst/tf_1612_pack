# SPDX-License-Identifier: GPL-2.0-only

include ./common.mk

define Device/tfortis_psw-2g8f
  SOC := rtl8380
  KERNEL_SIZE := 6m
  IMAGE_SIZE := 26m
  DEVICE_VENDOR := TFortis
  DEVICE_MODEL := PSW-2G8F+
  DEVICE_PACKAGES += realtek-poe
endef
TARGET_DEVICES += tfortis_psw-2g8f
