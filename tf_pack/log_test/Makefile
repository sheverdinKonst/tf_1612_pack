#
# This software is licensed under the Public Domain.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=log_test
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_MAINTAINER:=Belyaev Alex <abelyaev@fort-telecom.ru>
PKG_LICENSE:=CC0-1.0

include $(INCLUDE_DIR)/package.mk


define Package/log_test
	SECTION:=tfortis
	# Select package by default
	#DEFAULT:=y
	CATEGORY:=TFortis
	TITLE:=Log Test application.
	URL:=https://www.tfortis.com
endef

define Package/log_test/description
	This is LOG Test application.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(call Build/Compile/Default)
endef

define Package/log_test/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/log_test $(1)/usr/bin/
endef

$(eval $(call BuildPackage,log_test))
