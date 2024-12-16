#
# Copyright (C) 2024 TFortis
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tf_file
PKG_VERSION:=15.7
PKG_RELEASE:=2
PKG_MAINTAINER:=Sheverdin Konstantin <abelyaev@fort-telecom.ru>

include $(INCLUDE_DIR)/package.mk

define Package/tf_file
  SECTION:=tfortis
  DEFAULT:=y
  CATEGORY:=TFortis
  TITLE:=base files in root directory
  URL:=https://www.tfortis.com
endef

define Package/tf_file/description
  base files in root directory
endef

define Build/Compile
endef

define Build/Configure
endef

define Build/Prepare
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

define Package/tf_file/install
	echo "copy /files folder to openwrt/files. Need to all image rebuild. Package instalation not replaced /files"
	cp -r ./files/* $(TOPDIR)/files
endef

$(eval $(call BuildPackage,tf_file))

