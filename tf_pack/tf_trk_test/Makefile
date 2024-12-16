##
## This software is licensed under the Public Domain.21
##
#include $(TOPDIR)/rules.mk
#
#PKG_NAME:=tf_rtk_test
#
##PKG_VERSION:= $(shell sed -n 's/^#define PKG_VERSION "\(.*\)"/\1/p' src/hwsys_ctrl/hwsys_ctrl.h)
#PKG_VERSION:= 1.0
##PKG_RELEASE:= $(shellsed -n 's/^#define PKG_RELEASE "\(.*\)"/\1/p' src/hwsys_ctrl/hwsys_ctrl.h)
#PKG_RELEASE:= 4
#
#PKG_MAINTAINER:=Konstantin Shevedrin <sheverdin@fort-telecom.ru>
#PKG_LICENSE:=CC0-1.0
#
#include $(INCLUDE_DIR)/package.mk
#include $(INCLUDE_DIR)/cmake.mk
#
#define Package/tf_rtk_test
#	SECTION:=tfortis
#	# Select package by default
#	DEFAULT:=y
#	CATEGORY:=TFortis
#	TITLE:=	app from realtek
#	URL:=https://www.tfortis.com
#endef
#
#define Package/tf_rtk_test/description
#	App for control and autorestrt PoE
#endef
#
#define Build/Prepare
#	mkdir -p $(PKG_BUILD_DIR)
#	mkdir -p $(PKG_BUILD_DIR)/sdk
#	#mkdir -p $(PKG_BUILD_DIR)/include
#	#mkdir -p $(PKG_BUILD_DIR)/system
#	$(CP) ./sdk/* $(PKG_BUILD_DIR)/sdk
#	$(CP) ./sdk/CMakeLists.txt $(PKG_BUILD_DIR)
#	#$(CP) ./sdk/include/* $(PKG_BUILD_DIR)/include
#	#$(CP) ./sdk/system/*  $(PKG_BUILD_DIR)/system
#endef
#
#define Package/tf_rtk_test/install
#	$(INSTALL_DIR) $(1)/usr/bin
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tf_rtk_test $(1)/usr/bin/
#endef
#
#$(eval $(call BuildPackage,tf_rtk_test))
#