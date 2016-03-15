ADD_CUSTOM_TARGET(indicator_port.edj
		COMMAND edje_cc -id ${PROJECT_EDC_DIR}/icons_edc "${EDJEFLAGS}"
		${PROJECT_EDC_DIR}/indicator_port.edc
		indicator_port.edj
		DEPENDS ${PROJECT_EDC_DIR}/indicator_port.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_port.edj)

ADD_CUSTOM_TARGET(ticker.edj
		COMMAND edje_cc -id ${PROJECT_EDC_DIR}/icons_edc "${EDJEFLAGS}"
		${PROJECT_EDC_DIR}/ticker.edc
		ticker.edj
		DEPENDS ${PROJECT_EDC_DIR}/ticker.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} ticker.edj)

ADD_CUSTOM_TARGET(ticker_default.edj
		COMMAND edje_cc -id ${PROJECT_EDC_DIR}/icons_edc "${EDJEFLAGS}"
		${PROJECT_EDC_DIR}/ticker_default.edc
		ticker_default.edj
		DEPENDS ${PROJECT_EDC_DIR}/ticker_default.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} ticker_default.edj)

ADD_CUSTOM_TARGET(ticker_animated_icon.edj
		COMMAND edje_cc -id ${PROJECT_EDC_DIR}/animated_icon "${EDJEFLAGS}"
		${PROJECT_EDC_DIR}/ticker_animated_icon.edc
		ticker_animated_icon.edj
		DEPENDS ${PROJECT_EDC_DIR}/ticker_animated_icon.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} ticker_animated_icon.edj)

ADD_CUSTOM_TARGET(indicator_icon_theme.edj
		COMMAND edje_cc "${EDJEFLAGS}"
		${PROJECT_EDC_DIR}/indicator_icon_theme.edc
		indicator_icon_theme.edj
		DEPENDS ${PROJECT_EDC_DIR}/indicator_icon_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_icon_theme.edj)

ADD_CUSTOM_TARGET(indicator_icon_nonfixed_theme.edj
		COMMAND edje_cc "${EDJEFLAGS}"
		${PROJECT_EDC_DIR}/indicator_icon_nonfixed_theme.edc
		indicator_icon_nonfixed_theme.edj
		DEPENDS ${PROJECT_EDC_DIR}/indicator_icon_nonfixed_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_icon_nonfixed_theme.edj)

ADD_CUSTOM_TARGET(indicator_icon_animation.edj
		COMMAND edje_cc -id ${PROJECT_EDC_DIR}/icons_edc "${EDJEFLAGS}"
		${PROJECT_EDC_DIR}/indicator_icon_animation.edc
		indicator_icon_animation.edj
		DEPENDS ${PROJECT_EDC_DIR}/indicator_icon_animation.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} indicator_icon_animation.edj)

INSTALL(FILES indicator_port.edj DESTINATION ${INSTALL_EDJEDIR})
INSTALL(FILES ticker.edj DESTINATION ${INSTALL_EDJEDIR})
INSTALL(FILES ticker_default.edj DESTINATION ${INSTALL_EDJEDIR})
INSTALL(FILES ticker_animated_icon.edj DESTINATION ${INSTALL_EDJEDIR})
INSTALL(FILES indicator_icon_theme.edj DESTINATION ${INSTALL_EDJEDIR})
INSTALL(FILES indicator_icon_nonfixed_theme.edj DESTINATION ${INSTALL_EDJEDIR})
INSTALL(FILES indicator_icon_animation.edj DESTINATION ${INSTALL_EDJEDIR})

INSTALL(DIRECTORY ${PROJECT_EDC_DIR}/icons DESTINATION ${INSTALL_EDJEDIR} FILES_MATCHING PATTERN "*.png")
INSTALL(DIRECTORY ${PROJECT_EDC_DIR}/icons DESTINATION ${INSTALL_EDJEDIR} FILES_MATCHING PATTERN "*.PNG")
INSTALL(DIRECTORY ${PROJECT_EDC_DIR}/icons DESTINATION ${INSTALL_EDJEDIR} FILES_MATCHING PATTERN "*.gif")

