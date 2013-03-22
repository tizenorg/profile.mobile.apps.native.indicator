ADD_CUSTOM_TARGET(indicator_port.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/icons_edc
		${CMAKE_CURRENT_SOURCE_DIR}/indicator_port.edc
		${CMAKE_CURRENT_BINARY_DIR}/indicator_port.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/indicator_port.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_port.edj)

ADD_CUSTOM_TARGET(indicator_land.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/icons_edc
		${CMAKE_CURRENT_SOURCE_DIR}/indicator_land.edc
		${CMAKE_CURRENT_BINARY_DIR}/indicator_land.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/indicator_land.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_land.edj)

ADD_CUSTOM_TARGET(indicator_icon_theme.edj
		COMMAND edje_cc
		${CMAKE_CURRENT_SOURCE_DIR}/indicator_icon_theme.edc
		${CMAKE_CURRENT_BINARY_DIR}/indicator_icon_theme.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/indicator_icon_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_icon_theme.edj)

ADD_CUSTOM_TARGET(indicator_icon_nonfixed_theme.edj
		COMMAND edje_cc
		${CMAKE_CURRENT_SOURCE_DIR}/indicator_icon_nonfixed_theme.edc
		${CMAKE_CURRENT_BINARY_DIR}/indicator_icon_nonfixed_theme.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/indicator_icon_nonfixed_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_icon_nonfixed_theme.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/indicator_port.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/indicator_land.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/indicator_icon_theme.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/indicator_icon_nonfixed_theme.edj DESTINATION ${EDJDIR})

INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.png")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.PNG")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.gif")

