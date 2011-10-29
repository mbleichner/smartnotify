project(plasma-applet-smartjobviews)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(SOURCEFILES jobviewserver.cpp jobview.cpp)

qt4_add_resources(SOURCEFILES jobviews.qrc)
qt4_add_dbus_adaptor(SOURCEFILES org.kde.JobViewServer.xml jobviewserver.h JobViewServer jobviewserveradaptor)
qt4_add_dbus_interface(SOURCEFILES org.kde.JobViewServer.xml jobviewserverproxy)
qt4_add_dbus_adaptor(SOURCEFILES org.kde.JobView.xml jobview.h JobView jobviewadaptor)
qt4_add_dbus_interface(SOURCEFILES org.kde.JobView.xml jobviewproxy)
kde4_add_kcfg_files(SOURCEFILES jobviewconfig.kcfgc)
kde4_add_ui_files(SOURCEFILES jobviewconfig_settings.ui)

kde4_add_plugin(plasma_applet_smartjobviews ${SOURCEFILES})

target_link_libraries(plasma_applet_smartjobviews ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS})

install(TARGETS plasma_applet_smartjobviews DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-applet-smartjobviews.desktop  DESTINATION ${SERVICES_INSTALL_DIR})