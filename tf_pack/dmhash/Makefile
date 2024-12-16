#
# This software is licensed under the Public Domain.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dmhash
PKG_VERSION:=1.1
PKG_RELEASE:=1

PKG_MAINTAINER:=Belyaev Alex <abelyaev@fort-telecom.ru>
PKG_LICENSE:=CC0-1.0

include $(INCLUDE_DIR)/package.mk


define Package/dmhash
	SECTION:=tfortis
	DEFAULT:=y
	CATEGORY:=TFortis
	TITLE:=MD5 Hash generator for TFortis Device Manager
	URL:=https://www.tfortis.com
	DEPENDS:=+libmbedtls +libuci
endef

define Package/dmhash/description
	MD5 Hash generator for TFortis Device Manager
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/dmhash/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dmhash $(1)/usr/bin/
endef

$(eval $(call BuildPackage,dmhash))
