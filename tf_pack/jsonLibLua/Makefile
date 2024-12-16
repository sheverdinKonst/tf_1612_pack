#
# Copyright (C) 2024 TFortis
#

include $(TOPDIR)/rules.mk

PKG_NAME:=jsonLibLua
PKG_VERSION:=0.0
PKG_RELEASE:=5
PKG_MAINTAINER:=David Kolf http://dkolf.de/src/dkjson-lua.fsl

include $(INCLUDE_DIR)/package.mk

define Package/jsonLibLua
  SECTION:=tfortis
  SUBMENU:=Libs
  DEFAULT:=y
  CATEGORY:=TFortis
  TITLE:=JSON module for Lua 5.1 - 5.4
  URL:=https://www.tfortis.com
endef

define Package/jsonLibLua/description
  David Kolf's JSON module for Lua 5.1 - 5.4
endef


define Build/Compile
endef

define Build/Configure
endef

define Build/Prepare
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

define Package/jsonLibLua/install
	 $(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,jsonLibLua))

