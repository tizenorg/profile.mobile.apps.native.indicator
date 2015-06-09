MESSAGE("EDJE_CC CUSTOM FLAG : ${EDJEFLAGS}")

ADD_CUSTOM_TARGET(indicator_port.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/icons_edc "${EDJEFLAGS}"
		${CMAKE_CURRENT_SOURCE_DIR}/indicator_port.edc
		${CMAKE_CURRENT_BINARY_DIR}/indicator_port.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/indicator_port.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_port.edj)

ADD_CUSTOM_TARGET(ticker.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/icons_edc "${EDJEFLAGS}"
		${CMAKE_CURRENT_SOURCE_DIR}/ticker.edc
		${CMAKE_CURRENT_BINARY_DIR}/ticker.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ticker.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} ticker.edj)

ADD_CUSTOM_TARGET(ticker_default.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/icons_edc "${EDJEFLAGS}"
		${CMAKE_CURRENT_SOURCE_DIR}/ticker_default.edc
		${CMAKE_CURRENT_BINARY_DIR}/ticker_default.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ticker_default.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} ticker_default.edj)

ADD_CUSTOM_TARGET(ticker_animated_icon.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/animated_icon "${EDJEFLAGS}"
		${CMAKE_CURRENT_SOURCE_DIR}/ticker_animated_icon.edc
		${CMAKE_CURRENT_BINARY_DIR}/ticker_animated_icon.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ticker_animated_icon.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} ticker_animated_icon.edj)

ADD_CUSTOM_TARGET(indicator_icon_theme.edj
		COMMAND edje_cc "${EDJEFLAGS}"
		${CMAKE_CURRENT_SOURCE_DIR}/indicator_icon_theme.edc
		${CMAKE_CURRENT_BINARY_DIR}/indicator_icon_theme.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/indicator_icon_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_icon_theme.edj)

ADD_CUSTOM_TARGET(indicator_icon_nonfixed_theme.edj
		COMMAND edje_cc "${EDJEFLAGS}"
		${CMAKE_CURRENT_SOURCE_DIR}/indicator_icon_nonfixed_theme.edc
		${CMAKE_CURRENT_BINARY_DIR}/indicator_icon_nonfixed_theme.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/indicator_icon_nonfixed_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_icon_nonfixed_theme.edj)

ADD_CUSTOM_TARGET(indicator_icon_animation.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/icons_edc "${EDJEFLAGS}"
		${CMAKE_CURRENT_SOURCE_DIR}/indicator_icon_animation.edc
		${CMAKE_CURRENT_BINARY_DIR}/indicator_icon_animation.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/indicator_icon_animation.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_icon_animation.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/indicator_port.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/ticker.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/ticker_default.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/ticker_animated_icon.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/indicator_icon_theme.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/indicator_icon_nonfixed_theme.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/indicator_icon_animation.edj DESTINATION ${EDJDIR})

INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.png")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.PNG")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.gif")

