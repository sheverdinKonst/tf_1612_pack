#
# This software is licensed under the Public Domain.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=i2c_test
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_MAINTAINER:=Belyaev Alex <abelyaev@fort-telecom.ru>
PKG_LICENSE:=CC0-1.0

include $(INCLUDE_DIR)/package.mk

define Package/i2c_test
	SECTION:=tfortis
	DEFAULT:=n
	CATEGORY:=TFortis
	TITLE:=i2c_test
	URL:=https://www.tfortis.com
endef

define Package/i2c_test/description
	i2c_test
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/i2c_test/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/i2c_test $(1)/usr/bin/
endef

$(eval $(call BuildPackage,i2c_test))
