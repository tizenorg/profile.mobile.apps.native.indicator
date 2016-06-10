CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(indicator-win C)

SET(INSTALL_EXEC_PREFIX "${INSTALL_PREFIX}/bin")
SET(INSTALL_RESDIR "${INSTALL_PREFIX}/res")
SET(INSTALL_IMAGEDIR "${INSTALL_RESDIR}/images")
SET(INSTALL_EDJEDIR "${INSTALL_RESDIR}/resource")
SET(INSTALL_LOCALEDIR "${INSTALL_RESDIR}/locale")
SET(CMAKE_VERBOSE_MAKEFILE 0)

SET(PROJECT_ROOT_DIR "${CMAKE_SOURCE_DIR}/..")
SET(PROJECT_RESOURCES_DIR "${PROJECT_ROOT_DIR}/res")
SET(PROJECT_EDC_DIR "${PROJECT_RESOURCES_DIR}/resource")
SET(PROJECT_IMAGES_DIR "${PROJECT_RESOURCES_DIR}/images")
SET(PROJECT_PO_DIR "${PROJECT_RESOURCES_DIR}/po")

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	capi-appfw-application
	capi-appfw-app-manager
	capi-system-runtime-info
	capi-network-bluetooth
	capi-appfw-preference
	capi-system-system-settings
	capi-media-player
	capi-media-sound-manager
	capi-media-metadata-extractor
	capi-network-wifi
	capi-ui-efl-util
	appcore-common
	dlog
	elementary
	notification
	vconf
	minicontrol-monitor
	feedback
	pkgmgr-info
	efl-assist
	tzsh-indicator-service
	capi-system-device
	capi-telephony
	capi-network-wifi-direct
	capi-network-nfc
	capi-network-tethering
	storage
	capi-base-utils-i18n
	capi-message-port
	callmgr_client
)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -std=gnu99 -fvisibility=hidden -Wall -Winline -g -fno-builtin-malloc -fPIC")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

INCLUDE_DIRECTORIES(${PROJECT_ROOT_DIR}/inc)

AUX_SOURCE_DIRECTORY(${PROJECT_ROOT_DIR}/src DAEMON_SRCS)
FILE(GLOB_RECURSE MODULE_SRCS ${PROJECT_ROOT_DIR}/src/modules/*.c)

SET(SRCS ${DAEMON_SRCS} ${MODULE_SRCS})

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_EXEC_PREFIX})
INSTALL(DIRECTORY DESTINATION ${PREFIX}/data})
INSTALL(FILES ${PROJECT_ROOT_DIR}/tizen-manifest.xml DESTINATION ${SYS_PACKAGES_DIR} RENAME org.tizen.indicator.xml)

ADD_SUBDIRECTORY(res)
