add_executable( TSLDownload TSLDownload.cpp )
target_link_libraries(TSLDownload Qt${QT_VERSION_MAJOR}::Network)
get_target_property(qtCore_install_prefix Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
get_filename_component(qtCore_install_prefix ${qtCore_install_prefix} DIRECTORY)
add_custom_command(
	OUTPUT TSL.qrc
	DEPENDS TSLDownload
	COMMAND $<TARGET_FILE:TSLDownload> "${CMAKE_CURRENT_BINARY_DIR}" ${TSL_URL} ${TSL_INCLUDE}
	WORKING_DIRECTORY ${qtCore_install_prefix}
)

configure_file( translations/tr.qrc tr.qrc COPYONLY )
qt_add_translation(SOURCES translations/en.ts translations/et.ts translations/ru.ts)

file(GLOB WIDGETS CONFIGURE_DEPENDS
	"dialogs/*.cpp" "dialogs/*.h" "dialogs/*.ui"
	"effects/*.cpp" "effects/*.h" "effects/*.ui"
	"widgets/*.cpp" "widgets/*.h" "widgets/*.ui"
)
set(SCHEMAS
	../schema/recipients.fbs
	../schema/header.fbs
)
add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
	${PROJECT_NAME}.rc
	${SOURCES}
	${WIDGETS}
	images/images.qrc
	fonts/fonts.qrc
	${CMAKE_CURRENT_BINARY_DIR}/tr.qrc
	${CMAKE_CURRENT_BINARY_DIR}/TSL.qrc
	main.cpp
	Application.cpp
	Application.h
	CheckConnection.cpp
	CheckConnection.h
	CDoc1.cpp
	CDoc1.h
	CDoc2.cpp
	CDoc2.h
	Crypto.cpp
	Crypto.h
	CryptoDoc.cpp
	CryptoDoc.h
	DateTime.cpp
	DateTime.h
	DigiDoc.cpp
	DigiDoc.h
	Diagnostics.cpp
	Diagnostics.h
	DiagnosticsTask.cpp
	DiagnosticsTask.h
	DocumentModel.cpp
	DocumentModel.h
	IKValidator.cpp
	IKValidator.h
	MainWindow.cpp
	MainWindow_MyEID.cpp
	MainWindow.h
	MainWindow.ui
	LdapSearch.cpp
	LdapSearch.h
	QCardLock.cpp
	QCardLock.h
	QCryptoBackend.cpp
	QCryptoBackend.h
	QPKCS11.cpp
	QPKCS11.h
	QSigner.cpp
	QSigner.h
	QSmartCard.cpp
	QSmartCard.h
	Styles.cpp
	Styles.h
	PrintSheet.cpp
	PrintSheet.h
	Settings.cpp
	Settings.h
	SslCertificate.cpp
	SslCertificate.h
	TokenData.cpp
	TokenData.h
)

target_link_libraries(${PROJECT_NAME}
	qdigidoccommon
	Qt${QT_VERSION_MAJOR}::PrintSupport
	Qt${QT_VERSION_MAJOR}::Svg
	${LIBDIGIDOCPP_LIBRARY}
	${LDAP_LIBRARIES}
	$<TARGET_NAME_IF_EXISTS:flatbuffers::flatbuffers>
	$<TARGET_NAME_IF_EXISTS:flatbuffers::flatbuffers_shared>
	ZLIB::ZLIB
)

if(NOT BUILD_DATE)
	string(TIMESTAMP BUILD_DATE "%d.%m.%Y")
endif()

if(${QT_VERSION_MAJOR} STREQUAL "6")
	find_package(Qt6 COMPONENTS Core5Compat SvgWidgets REQUIRED)
	target_link_libraries(${PROJECT_NAME} Qt6::Core5Compat Qt6::SvgWidgets)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
	AUTOUIC ON
	AUTORCC ON
	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist.cmake
	MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
	MACOSX_BUNDLE_GUI_IDENTIFIER "ee.ria.${PROJECT_NAME}"
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR} ${LIBDIGIDOCPP_INCLUDE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE
	CDOC2_GET_URL="${CDOC2_GET_URL}"
	CDOC2_POST_URL="${CDOC2_POST_URL}"
	MOBILEID_URL="${MOBILEID_URL}"
	SMARTID_URL="${SMARTID_URL}"
	BUILD_DATE="${BUILD_DATE}"
)

foreach(SCHEMA ${SCHEMAS})
	get_filename_component(stem ${SCHEMA} NAME_WE)
	get_filename_component(name ${SCHEMA} NAME)
	set(GENERATED_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/${stem}_generated.h)
	add_custom_command(
		OUTPUT ${GENERATED_INCLUDE}
		COMMENT "Compiling flatbuffer for ${name}"
		COMMAND flatbuffers::flatc
			--cpp
			--scoped-enums
			-o ${CMAKE_CURRENT_BINARY_DIR}
			-I ${CMAKE_CURRENT_SOURCE_DIR}
			${SCHEMA}
		DEPENDS flatbuffers::flatc ${SCHEMA}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
	target_sources(${PROJECT_NAME} PRIVATE ${SCHEMA} ${GENERATED_INCLUDE})
endforeach()

if( APPLE )
	file(GLOB_RECURSE RESOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/mac/Resources/*.icns
		${CMAKE_CURRENT_SOURCE_DIR}/mac/Resources/*.strings)
	foreach(_file ${RESOURCE_FILES})
		get_filename_component(_file_dir ${_file} PATH)
		file(RELATIVE_PATH _file_dir ${CMAKE_CURRENT_SOURCE_DIR}/mac ${_file_dir})
		set_source_files_properties( ${_file} PROPERTIES MACOSX_PACKAGE_LOCATION ${_file_dir})
	endforeach(_file)

	target_sources(${PROJECT_NAME} PRIVATE ${RESOURCE_FILES} Application_mac.mm MacMenuBar.cpp MacMenuBar.h dialogs/CertificateDetails_mac.mm Diagnostics_unix.cpp)
	set_source_files_properties( Application_mac.mm dialogs/CertificateDetails_mac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc" )
	set_source_files_properties( LdapSearch.cpp PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations" )
	target_link_libraries(${PROJECT_NAME} "-framework Quartz" "-fobjc-arc")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND cp -a ${PKCS11_MODULE} $<TARGET_FILE_DIR:${PROJECT_NAME}>
		COMMAND mkdir -p $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Library/QuickLook
		COMMAND cp -a $<TARGET_BUNDLE_DIR:DigiDocQL> $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Library/QuickLook
	)
	add_custom_target(macdeployqt DEPENDS ${PROJECT_NAME}
		COMMAND ${qtCore_install_prefix}/macdeployqt $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>
		COMMAND rm -rf $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/PlugIns/bearer
		COMMAND cp -a /Library/Frameworks/digidocpp.framework $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks
		COMMAND rm $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks/digidocpp.framework/Resources/digidoc-tool
		COMMAND find $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks -name Headers | xargs rm -rf
		COMMAND install_name_tool -delete_rpath /Library/Frameworks $<TARGET_FILE:${PROJECT_NAME}>
		COMMAND ln -sf digidocpp.framework/Libraries/libcrypto.dylib
			$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks/libcrypto.3.dylib
		COMMAND ln -sf digidocpp.framework/Libraries/libssl.dylib
			$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks/libssl.3.dylib
	)

	add_custom_target(codesign DEPENDS ${PROJECT_NAME}
		COMMAND codesign -f -s \"$$SIGNCERT\"
			$<TARGET_FILE_DIR:${PROJECT_NAME}>/*.*
			$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks/digidocpp.framework/Libraries/*
			$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks/*.framework
			$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/PlugIns/*/*
			$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Library/QuickLook/DigiDocQL.qlgenerator
		COMMAND if echo \"$$SIGNCERT\" | grep -q "Developer ID" \; then
				codesign -f -s \"$$SIGNCERT\" $<TARGET_BUNDLE_DIR:${PROJECT_NAME}> --entitlements ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.eToken.entitlements\;
			else
				codesign -f -s \"$$SIGNCERT\" $<TARGET_BUNDLE_DIR:${PROJECT_NAME}> --entitlements ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.entitlements\;
			fi
	)

	add_custom_target(productbuild DEPENDS ${PROJECT_NAME}
		COMMAND productbuild --component $<TARGET_BUNDLE_DIR:${PROJECT_NAME}> /Applications
			--sign \"$$INSTCERT\" ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-store_${VERSION}$ENV{VER_SUFFIX}.pkg
	)
	add_custom_target(zip DEPENDS ${PROJECT_NAME}
		COMMAND zip --symlinks -r -D ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_${VERSION}$ENV{VER_SUFFIX}.zip ${PROJECT_NAME}.app
	)
	add_custom_target(zipdebug DEPENDS ${PROJECT_NAME}
		COMMAND dsymutil -o ${PROJECT_NAME}.dSYM $<TARGET_FILE:${PROJECT_NAME}>
		COMMAND zip -r ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-dbg_${VERSION}$ENV{VER_SUFFIX}.zip ${PROJECT_NAME}.dSYM
	)
elseif(WIN32)
	target_sources(${PROJECT_NAME} PRIVATE QCNG.cpp QCNG.h CertStore.cpp CertStore.h Diagnostics_win.cpp)
	target_compile_options(${PROJECT_NAME} PRIVATE "/guard:cf")
	target_link_options(${PROJECT_NAME} PRIVATE "/guard:cf" $<$<BOOL:${CROSSSIGNCERT}>:/INTEGRITYCHECK>)
	target_link_libraries(${PROJECT_NAME} NCrypt Crypt32 Cryptui)
	cmake_parse_arguments(GETLIB "" "optimized;debug" "" ${LIBDIGIDOCPP_LIBRARY})
	if(GETLIB_debug AND CMAKE_BUILD_TYPE STREQUAL "Debug")
		get_filename_component(LIBS_PATH ${GETLIB_debug} DIRECTORY)
	elseif(GETLIB_optimized)
		get_filename_component(LIBS_PATH ${GETLIB_optimized} DIRECTORY)
	else()
		get_filename_component(LIBS_PATH ${GETLIB_UNPARSED_ARGUMENTS} DIRECTORY)
	endif()
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(PLATFORM x64)
		set(OPENSSL_SUFFIX "-x64")
	else()
		set(PLATFORM x86)
		set(OPENSSL_SUFFIX "")
	endif()
	set(MSI_FILE "Digidoc4_Client-${VERSION}$ENV{VER_SUFFIX}.${PLATFORM}")
	list(APPEND CANDLE_CMD "$ENV{WIX}bin\\candle.exe" -nologo
		-arch ${PLATFORM}
		-dMSI_VERSION=${VERSION}
		-dlibs_path=${LIBS_PATH}
		-dclient_path=$<TARGET_FILE:${PROJECT_NAME}>
		-dico_path=${CMAKE_CURRENT_SOURCE_DIR}/images/digidoc.ico
		-dqt_suffix=$<$<CONFIG:Debug>:d>
		-dschemasLocation=${LIBS_PATH}/schema
		SchemasFragment.wxs
		${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.wxs
		${CMAKE_SOURCE_DIR}/cmake/modules/WelcomeDlg2.wxs
		${CMAKE_SOURCE_DIR}/cmake/modules/WixUI_Minimal2.wxs
	)
	list(APPEND LIGHT_CMD "$ENV{WIX}bin\\light.exe" -nologo -ext WixUIExtension
		${PROJECT_NAME}.wixobj SchemasFragment.wixobj WelcomeDlg2.wixobj WixUI_Minimal2.wixobj
		-dWixUIDialogBmp=${CMAKE_SOURCE_DIR}/cmake/modules/dlgbmp.bmp
		-dWixUIBannerBmp=${CMAKE_SOURCE_DIR}/cmake/modules/banner.bmp)
	add_custom_target(msi DEPENDS ${PROJECT_NAME}
		COMMAND "$ENV{WIX}bin\\heat.exe" dir ${LIBS_PATH}/schema -nologo -cg Schemas -gg -scom -sreg -sfrag -srd -dr SchemaFolder -var var.schemasLocation -out SchemasFragment.wxs
		COMMAND ${CANDLE_CMD}
		COMMAND ${LIGHT_CMD} -loc ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.en-US.wxl -cultures:en-US -o "${MSI_FILE}.en-US.msi"
		COMMAND ${LIGHT_CMD} -loc ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.et-EE.wxl -cultures:et-EE -o "${MSI_FILE}.et-EE.msi"
		#Build MSI with QT
		COMMAND ${CANDLE_CMD} -dqt_path=${qtCore_install_prefix}
		COMMAND ${LIGHT_CMD} -loc ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.en-US.wxl -cultures:en-US -o "${MSI_FILE}.en-US.qt.msi"
		COMMAND ${LIGHT_CMD} -loc ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.et-EE.wxl -cultures:et-EE -o "${MSI_FILE}.et-EE.qt.msi"
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	)
	if(SIGNCERT)
		list(APPEND SIGNCMD signtool.exe sign /a /v /s MY /n "${SIGNCERT}" /fd SHA256
			/du http://installer.id.ee /tr http://sha256timestamp.ws.symantec.com/sha256/timestamp /td SHA256)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND ${SIGNCMD} "$<$<BOOL:${CROSSSIGNCERT}>:/ph;/ac;${CROSSSIGNCERT}>" $<TARGET_FILE:${PROJECT_NAME}>
			COMMAND_EXPAND_LISTS
		)
		add_custom_command(TARGET msi POST_BUILD
			COMMAND ${SIGNCMD} "${MSI_FILE}.en-US.msi" "${MSI_FILE}.et-EE.msi" "${MSI_FILE}.en-US.qt.msi" "${MSI_FILE}.et-EE.qt.msi"
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		)
	endif()
	configure_file(${CMAKE_SOURCE_DIR}/AppxManifest.xml.cmake ${CMAKE_BINARY_DIR}/AppxManifest.xml)
	add_custom_target(appx DEPENDS ${PROJECT_NAME}
		COMMAND ${CMAKE_COMMAND} -E remove ${MSI_FILE}.appx
		COMMAND ${CMAKE_COMMAND} -E remove_directory appx
		COMMAND ${CMAKE_COMMAND} -E make_directory appx
		COMMAND ${CMAKE_COMMAND} -E copy AppxManifest.xml appx
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> appx
		COMMAND ${CMAKE_COMMAND} -E copy ${LIBS_PATH}/libcrypto-3${OPENSSL_SUFFIX}.dll appx
		COMMAND ${CMAKE_COMMAND} -E copy ${LIBS_PATH}/libssl-3${OPENSSL_SUFFIX}.dll appx
		COMMAND ${CMAKE_COMMAND} -E copy ${LIBS_PATH}/zlib1.dll appx
		COMMAND ${CMAKE_COMMAND} -E copy ${LIBS_PATH}/xerces-c_3_2.dll appx
		COMMAND ${CMAKE_COMMAND} -E copy ${LIBS_PATH}/XalanMessages_1_12.dll appx
		COMMAND ${CMAKE_COMMAND} -E copy ${LIBS_PATH}/Xalan-C_1_12.dll appx
		COMMAND ${CMAKE_COMMAND} -E copy ${LIBS_PATH}/xsec_2_0.dll appx
		COMMAND ${CMAKE_COMMAND} -E copy ${LIBS_PATH}/digidocpp.dll appx
		COMMAND ${CMAKE_COMMAND} -E copy ${LIBS_PATH}/digidocpp.conf appx
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Assets appx/Assets
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBS_PATH}/schema appx/schema
		COMMAND ${qtCore_install_prefix}/windeployqt --no-translations --no-compiler-runtime --no-patchqt appx
		COMMAND ${CMAKE_COMMAND} -E remove_directory appx/bearer
		COMMAND makeappx.exe pack -d appx -p ${MSI_FILE}.appx
		# https://msdn.microsoft.com/en-us/library/windows/desktop/jj835832(v=vs.85).aspx
		#Popups GUI
		#COMMAND MakeCert.exe /n "CN=8BBBE4D8-620A-4884-A12A-72F1A2030D8B" /r /h 0 /eku "1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13" /m 120 /a sha256 /sv qdigidoc4.pvk qdigidoc4.cer
		#COMMAND Pvk2Pfx.exe /pvk qdigidoc4.pvk /spc qdigidoc4.cer /pfx qdigidoc4.pfx /f
		COMMAND signtool.exe sign -f ${CMAKE_SOURCE_DIR}/qdigidoc4.pfx -fd SHA256 -v ${MSI_FILE}.appx
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	)
else()
	find_package( Threads REQUIRED )
	target_sources(${PROJECT_NAME} PRIVATE Diagnostics_unix.cpp)
	target_link_libraries(${PROJECT_NAME} Threads::Threads )
	configure_file( qdigidoc4.1.cmake qdigidoc4.1 )
	install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
	install( FILES ${CMAKE_CURRENT_BINARY_DIR}/qdigidoc4.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 )
	install( FILES qdigidoc4.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications )
	install( FILES qdigidoc4.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages )
	foreach(RES 16 22 32 48 128 256 512)
		install(
			FILES images/digidoc_${RES}.png
			DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${RES}x${RES}/apps/
			RENAME qdigidoc4.png
		)
		install(
			FILES images/qdigidoc_client_document_${RES}.png
			DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${RES}x${RES}/mimetypes/
			RENAME application-vnd.etsi.asic-e+zip.png
		)
		install(
			FILES images/qdigidoc_crypto_document_${RES}.png
			DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${RES}x${RES}/mimetypes/
			RENAME application-x-cdoc.png
		)
		foreach(TGT
				application-vnd.etsi.asic-s+zip.png
				application-x-ddoc.png
				application-vnd.lt.archyvai.adoc-2008.png)
			install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink application-vnd.etsi.asic-e+zip.png \
				\$ENV{DESTDIR}/${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/${RES}x${RES}/mimetypes/${TGT})")
		endforeach()
		foreach(TGT
				application-x-cdoc2.png)
			install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink application-x-cdoc.png \
				\$ENV{DESTDIR}/${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/${RES}x${RES}/mimetypes/${TGT})")
		endforeach()
		# Workaround Ubuntu 21.10 Yaru icon theme issues
		install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory \
			\$ENV{DESTDIR}/${CMAKE_INSTALL_FULL_DATADIR}/icons/Yaru/${RES}x${RES}/mimetypes/)")
		foreach(TGT
				application-vnd.etsi.asic-e+zip.png
				application-vnd.etsi.asic-s+zip.png
				application-x-ddoc.png
				application-vnd.lt.archyvai.adoc-2008.png
				application-x-cdoc.png
				application-x-cdoc2.png)
			install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \
				${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/${RES}x${RES}/mimetypes/${TGT} \
				\$ENV{DESTDIR}/${CMAKE_INSTALL_FULL_DATADIR}/icons/Yaru/${RES}x${RES}/mimetypes/${TGT})")
		endforeach()
	endforeach()
endif()
