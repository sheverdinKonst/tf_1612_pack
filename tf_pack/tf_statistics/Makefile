
#
# Copyright (C) 2024 TFortis
# date 05.12.2024
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tf_tf_statistics
PKG_VERSION:=0.1
PKG_RELEASE:=1
PKG_MAINTAINER:=Sheverdin Konstantin <sheverdin@fort-telecom.ru>

include $(INCLUDE_DIR)/package.mk

define Package/tf_statistics
	SECTION:=tfortis
	DEFAULT:=y
	CATEGORY:=TFortis
	TITLE:=tfortis statistics for snmp
	URL:=https://www.tfortis.com
endef

define Package/tf_statistics/description
	tfortis statistics for snmp
endef

define Build/Compile
endef

define Build/Configure
endef

define Build/Prepare
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

define Package/tf_statistics/install
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,tf_statistics))

