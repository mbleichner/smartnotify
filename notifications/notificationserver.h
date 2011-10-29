#ifndef NOTIFICATIONS_HEADER
#define NOTIFICATIONS_HEADER

#include "ui_notificationconfig_settings.h"
#include "ui_notificationconfig_help.h"

#include <QtCore>
#include <QtGui>
#include <QtDBus>
#include <QtDebug>

#include <KConfigDialog>

#include <Plasma/Dialog>
#include <Plasma/Applet>
#include <Plasma/IconWidget>

struct Notification {
  QString applicationName;
  QIcon icon;
  QString title;
  QString html;
  QString plain;
  QStringList actions;
  QVariantMap hints;
  QTime time;
};

struct AnimationParameters {
  double scrollStep;
  double fadeStep;
  int pauseTime;
};

enum AnimationPhase { Inactive, FadeIn, PauseBeforeScroll, Scroll, PauseAfterScroll, FadeOut, PauseAtEnd };

class NotificationServer : public Plasma::Applet {
  Q_OBJECT

  protected:
    QList<Notification*> queue;
    QList<Notification*> history;
    Plasma::Dialog* popup;
    QGraphicsTextItem* movingLabel;
    QGraphicsGridLayout* historyLayout;
    Plasma::IconWidget* iconWidget;
    QTimer* animationTimer;
    AnimationPhase phase;
    QColor color;
    QFont font;
    AnimationParameters ap;

  public:
    NotificationServer(QObject *parent, const QVariantList &args);
    void init();

  public Q_SLOTS:
    uint Notify(const QString &app_name, uint id, const QString &icon, const QString &summary, const QString &body, const QStringList &actions, const QVariantMap &hints, int timeout);
    void showConfigurationInterface();

  protected:
    void setupUi();
    QIcon decodeImageData(const QDBusArgument imageData);

  protected Q_SLOTS:
    void readConfiguration();
    void animationStep();
    void showPopup();
    void startServer();
};

class SettingsPage : public QWidget,  public Ui_Settings {
  Q_OBJECT
  public:
    SettingsPage(QWidget* parent = 0);
  public Q_SLOTS:
    void sendNotification();
};

class HelpPage : public QWidget,  public Ui_Help {
  Q_OBJECT
  public: HelpPage(QWidget* parent = 0) : QWidget(parent) { setupUi(this); };
};

#endif