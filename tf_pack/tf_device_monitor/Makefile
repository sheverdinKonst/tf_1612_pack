#
# This software is licensed under the Public Domain.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tf_device_monitor
PKG_VERSION:=0.5
PKG_RELEASE:=6

PKG_MAINTAINER:=Sheverdin Konstantin <sheverdin@fort-telecom.ru>
PKG_LICENSE:=CC0-1.0

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/tf_device_monitor
	SECTION:=tfortis
	DEFAULT:=n
	CATEGORY:=TFortis
	DEPENDS:=+jansson
	TITLE:=TFortis Device Monitor
	URL:=https://www.tfortis.com
endef

define Package/tf_device_monitor/description
	Device Manager application for collect and send data about the device..
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/tf_device_monitor/install
	$(CP) ./files/* $(1)/
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tf_device_monitor  $(1)/usr/bin/
endef

$(eval $(call BuildPackage,tf_device_monitor))
