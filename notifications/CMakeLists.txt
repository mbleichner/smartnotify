project(plasma-applet-smartnotifications)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(SOURCEFILES notificationserver.cpp)

qt4_add_resources(SOURCEFILES notifications.qrc)
qt4_add_dbus_adaptor(SOURCEFILES org.freedesktop.Notifications.xml notificationserver.h NotificationServer notificationadaptor)
qt4_add_dbus_interface(SOURCEFILES org.freedesktop.Notifications.xml notificationproxy)
kde4_add_kcfg_files(SOURCEFILES notificationconfig.kcfgc)
kde4_add_ui_files(SOURCEFILES notificationconfig_settings.ui notificationconfig_help.ui)
kde4_add_plugin(plasma_applet_smartnotifications ${SOURCEFILES})

target_link_libraries(plasma_applet_smartnotifications ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS})

install(TARGETS plasma_applet_smartnotifications DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-applet-smartnotifications.desktop  DESTINATION ${SERVICES_INSTALL_DIR})