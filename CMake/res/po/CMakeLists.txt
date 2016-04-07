# for i18n

SET(POFILES et.po is.po hy.po hu.po hr.po hi.po nb.po ga.po fr_FR.po fr_CA.po fi.po
eu.po it_IT.po es_MX.po es_ES.po en_US.po en_PH.po en.po el_GR.po de_DE.po da.po
ca.po bg.po az.po ar.po ja_JP.po zh_TW.po zh_SG.po zh_HK.po zh_CN.po uz.po cs.po
uk.po tr_TR.po sv.po sr.po sl.po sk.po ru_RU.po ka.po pt_PT.po pt_BR.po pl.po
nl_NL.po gl.po mk.po lv.po lt.po ko_KR.po kk.po ro.po)

SET(MSGFMT "/usr/bin/msgfmt")

FOREACH(pofile ${POFILES})
	SET(pofile ${PROJECT_PO_DIR}/${pofile})
	MESSAGE("PO: ${pofile}")
	GET_FILENAME_COMPONENT(absPofile ${pofile} ABSOLUTE)
	GET_FILENAME_COMPONENT(lang ${absPofile} NAME_WE)
	SET(moFile ${CMAKE_CURRENT_BINARY_DIR}/${lang}.mo)
	ADD_CUSTOM_COMMAND(
			OUTPUT ${moFile}
			COMMAND ${MSGFMT} -o ${moFile} ${absPofile}
			DEPENDS ${absPofile}
	)
	INSTALL(FILES ${moFile} DESTINATION ${INSTALL_LOCALEDIR}/${lang}/LC_MESSAGES RENAME ${PROJECT_NAME}.mo)
	SET(moFiles ${moFiles} ${moFile})
ENDFOREACH(pofile)

MESSAGE(".mo files: ${moFiles}")
ADD_CUSTOM_TARGET(po ALL DEPENDS ${moFiles})
