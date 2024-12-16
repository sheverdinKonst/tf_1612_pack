
#
# Copyright (C) 2024 TFortis
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tf_clish
PKG_VERSION:=0.1
PKG_RELEASE:=6
# date 10.12.2024
PKG_MAINTAINER:=Sheverdin Konstantin <sheverdin@fort-telecom.ru>

include $(INCLUDE_DIR)/package.mk

define Package/tf_clish
	SECTION:=tfortis
	DEFAULT:=y
	CATEGORY:=TFortis
	TITLE:=scripts for tf_clish
	URL:=https://www.tfortis.com
endef

define Package/tf_clish/description
	sripts for tf_clish
endef

define Build/Compile
endef

define Build/Configure
endef

define Build/Prepare
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

define Package/tf_clish/install
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,tf_clish))

