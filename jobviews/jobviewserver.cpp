#include "jobviewserver.h"
#include "jobviewserveradaptor.h"
#include "jobviewadaptor.h"
#include "jobviewserverproxy.h"
#include "jobviewconfig.h"

#include <algorithm>

#include <KIcon>

#include <Plasma/Frame>
#include <Plasma/Label>
#include <Plasma/Theme>

JobViewServer::JobViewServer(QObject *parent, const QVariantList &args) : Plasma::Applet(parent, args), lastID(0) {
  new JobViewServerAdaptor(this);
}

void JobViewServer::init() {
  setupUi();
  readConfiguration();
  startServer();
}

void JobViewServer::readConfiguration() {
  setMinimumWidth(JobViewConfig::minimumWidth());
  setPreferredWidth(JobViewConfig::preferredWidth());
}

void JobViewServer::setupUi() {

  // Paar allgemeine Parameter
  setBackgroundHints(DefaultBackground);
  setHasConfigurationInterface(true);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  resize(400, 80);

  // Rahmen, der das Hauptlayout enthält
  Plasma::Frame* frame = new Plasma::Frame();
  frame->setFrameShadow(Plasma::Frame::Sunken);
  QGraphicsLinearLayout* fullSpaceLayout = new QGraphicsLinearLayout();
  fullSpaceLayout->setContentsMargins(0,0,0,0);
  setLayout(fullSpaceLayout);
  fullSpaceLayout->addItem(frame);
  QGraphicsLinearLayout* layout = new QGraphicsLinearLayout();
  layout->setContentsMargins(0,0,0,0);
  frame->setLayout(layout);

  // Icon mit History und Queue Indicator
  iconWidget = new Plasma::IconWidget();
  iconWidget->setPreferredWidth(25);
  iconWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  iconWidget->setIcon(KIcon("arrow-down"));
  layout->addItem(iconWidget);

  // Layout mit Progressbars
  meterContainer = new Plasma::Label();
  meterContainer->nativeWidget()->setWordWrap(true);
  meterLayout = new QGraphicsLinearLayout();
  meterContainer->setLayout(meterLayout);
  layout->addItem(meterContainer);

  // Popup erzeugen
  popup = new Plasma::Dialog();
  QVBoxLayout* popupLayout = new QVBoxLayout();
  popupLayout->setContentsMargins(0,0,0,0);
  popup->setLayout(popupLayout);
  popup->setWindowFlags(Qt::Popup);
  popup->resize(400, 300);

  // GraphicsView und GraphicsScene erzeugen
  QGraphicsScene* popupScene = new QGraphicsScene();
  QGraphicsView* popupView = new QGraphicsView();
  popupView->setScene(popupScene);
  popupView->setFrameStyle(QFrame::NoFrame);
  popupView->setStyleSheet("background-color: transparent;");
  popupView->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  popupView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  popupView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  popupLayout->addWidget(popupView);

  // Container mit dem GridLayout zur Scene hinzufügen
  jobViewContainer = new QGraphicsWidget();
  jobViewLayout = new QGraphicsLinearLayout(Qt::Vertical);
  jobViewLayout->setContentsMargins(5,5,5,5);
  jobViewContainer->setLayout(jobViewLayout);
  popupScene->addItem(jobViewContainer);

  // Überschrift-/Nachrichtenbereich
  popupHeadline = new Plasma::Label();
  popupHeadline->nativeWidget()->setAlignment(Qt::AlignCenter);
  jobViewLayout->addItem(popupHeadline);

  QObject::connect(iconWidget, SIGNAL(clicked()), this, SLOT(showPopup()));
}

void JobViewServer::startServer() {
  QDBusConnection bus = QDBusConnection::sessionBus();
  if (bus.registerService("org.kde.JobViewServer") && bus.registerObject("/JobViewServer", this)) {
    connected = true;
  } else {
    connected = false;
    QDBusServiceWatcher* watcher = new QDBusServiceWatcher("org.kde.JobViewServer", bus, QDBusServiceWatcher::WatchForUnregistration, this);
    QObject::connect(watcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(startServer()));
    QObject::connect(watcher, SIGNAL(serviceUnregistered(QString)), watcher, SLOT(deleteLater()));
  }
  updateStatusIndicators();
}

void JobViewServer::showPopup() {
  if (!connected) return;
  jobViewContainer->setMinimumWidth(360);
  jobViewContainer->setMaximumWidth(360);
  jobViewContainer->resize(jobViewContainer->effectiveSizeHint(Qt::MinimumSize));
  jobViewContainer->scene()->setSceneRect(jobViewContainer->geometry());
  popup->move(popupPosition(popup->size()));
  popup->animatedShow(Plasma::Direction(0));
}

void JobViewServer::showConfigurationInterface() {
  if (KConfigDialog::showDialog("jobviewsrc")) return;
  KConfigDialog* dialog = new KConfigDialog(0, "jobviewsrc", JobViewConfig::self());
  QObject::connect(dialog, SIGNAL(settingsChanged(const QString&)), this, SLOT(readConfiguration()));
  dialog->addPage(new SettingsPage(), "Settings", "configure");
  dialog->show();
}

void JobViewServer::updateStatusIndicators() {
  if (!connected) {
    iconWidget->setIcon(KIcon("dialog-cancel"));
    popupHeadline->setText("Not connected to DBus.");
    meterContainer->setText("D-Bus service occupied - open configuration for instructions.");
  } else if (activeJobViews.count() == 1) {
    QIcon icon = activeJobViews.values().first()->icon();
    iconWidget->setIcon(!icon.isNull() ? icon : QIcon(":jobviews-active.png"));
    popupHeadline->setText("One active job:");
    meterContainer->setText("");
  } else if (activeJobViews.count() > 1) {
    iconWidget->setIcon(QIcon(":jobviews-active.png"));
    popupHeadline->setText(QString("%1 active jobs:").arg(activeJobViews.count()));
    meterContainer->setText("");
  } else {
    iconWidget->setIcon(QIcon(":jobviews-inactive.png"));
    popupHeadline->setText("No active jobs.");
    meterContainer->setText("");
  }
}

QDBusObjectPath JobViewServer::requestView(const QString &appName, const QString &appIconName, int capabilities) {
  QString objectPath = QString("/JobViewServer/JobView_%1").arg(++lastID);
  JobView* jobview = new JobView(objectPath, appName, appIconName, capabilities);
  Plasma::Meter* meter = new Plasma::Meter();
  meter->setMeterType(Plasma::Meter::BarMeterHorizontal);
  meter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  activeMeters.insert(jobview->objectPath(), meter);
  activeJobViews.insert(jobview->objectPath(), jobview);
  meterLayout->addItem(meter);
  jobViewLayout->addItem(jobview);
  connect(jobview, SIGNAL(progressChanged(uint)), this, SLOT(updateMeter(uint)));
  connect(jobview, SIGNAL(terminated()), this, SLOT(jobViewTerminated()));
  updateStatusIndicators();
  return QDBusObjectPath(objectPath);
}

void JobViewServer::updateMeter(uint percent) {
  QString objectPath = static_cast<JobView*>(sender())->objectPath();
  activeMeters.value(objectPath)->setValue(percent);
}

void JobViewServer::jobViewTerminated() {
  QString objectPath = static_cast<JobView*>(sender())->objectPath();
  if (popup->isVisible()) {
    Plasma::Meter* meter = activeMeters.take(objectPath);
    JobView* jobview = activeJobViews.take(objectPath);
    connect(popup, SIGNAL(dialogVisible(bool)), meter, SLOT(deleteLater()));
    connect(popup, SIGNAL(dialogVisible(bool)), jobview, SLOT(deleteLater()));
  } else {
    delete activeMeters.take(objectPath);
    delete activeJobViews.take(objectPath);
  }
  updateStatusIndicators();
}

SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent) {
  setupUi(this);
}

#include "jobviewserver.moc"
K_EXPORT_PLASMA_APPLET(smartjobviews, JobViewServer)