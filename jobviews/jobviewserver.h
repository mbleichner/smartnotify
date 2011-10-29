#ifndef JOBVIEWSERVER_HEADER
#define JOBVIEWSERVER_HEADER

#include "jobview.h"
#include "ui_jobviewconfig_settings.h"

#include <QtCore>
#include <QtGui>
#include <QtDBus>
#include <QtDebug>

#include <KConfigDialog>

#include <Plasma/Dialog>
#include <Plasma/Applet>
#include <Plasma/IconWidget>
#include <Plasma/Meter>

class JobViewServer : public Plasma::Applet {
  Q_OBJECT

  protected:
    bool connected;
    int lastID;
    QTimer iconTimer;
    QMap<QString, JobView*> activeJobViews;
    QMap<QString, Plasma::Meter*> activeMeters;
    Plasma::Dialog* popup;
    Plasma::IconWidget* iconWidget;
    Plasma::Label* meterContainer;
    QGraphicsLinearLayout* meterLayout;
    QGraphicsWidget* jobViewContainer;
    QGraphicsLinearLayout* jobViewLayout;
    Plasma::Label* popupHeadline;

  public:
    JobViewServer(QObject *parent, const QVariantList &args);
    void init();

  public Q_SLOTS:
    QDBusObjectPath requestView(const QString &appName, const QString &appIconName, int capabilities);
    void showConfigurationInterface();

  protected:
    void setupUi();
    void updateStatusIndicators();

  protected Q_SLOTS:
    void readConfiguration();
    void showPopup();
    void startServer();
    void updateMeter(uint percent);
    void jobViewTerminated();
};

class SettingsPage : public QWidget,  public Ui_Settings {
  Q_OBJECT
  public:
    SettingsPage(QWidget* parent = 0);
};

#endif