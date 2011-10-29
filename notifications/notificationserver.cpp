#include "notificationserver.h"
#include "notificationadaptor.h"
#include "notificationproxy.h"
#include "notificationconfig.h"

#include <algorithm>

#include <KIcon>

#include <Plasma/Frame>
#include <Plasma/Label>
#include <Plasma/Theme>

NotificationServer::NotificationServer(QObject *parent, const QVariantList &args) : Plasma::Applet(parent, args) {
  animationTimer = new QTimer(this);
  color          = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
  font           = Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont);
  phase          = Inactive;
}

void NotificationServer::init() {
  setupUi();
  readConfiguration();
  startServer();
}

void NotificationServer::readConfiguration() {
  ap.scrollStep = 0.1  +  0.6 * (NotificationConfig::scrollSpeed() / 100.0);
  ap.fadeStep   = 0.03 +  0.1 * (NotificationConfig::fadeSpeed() / 100.0);
  ap.pauseTime  = (int)(0.0  + 40.0 * (NotificationConfig::pauseTime() / 100.0));

  setMinimumWidth(NotificationConfig::minimumWidth());
  setPreferredWidth(NotificationConfig::preferredWidth());
  font.setPixelSize(NotificationConfig::fontSize());
}

void NotificationServer::setupUi() {

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
  iconWidget->setIcon(QIcon(":/notifications-inactive.png"));
  layout->addItem(iconWidget);

  // Laufschrift
  QGraphicsWidget* clippingSpace = new QGraphicsWidget();
  clippingSpace->setFlags(QGraphicsItem::ItemClipsChildrenToShape);
  clippingSpace->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  movingLabel = new QGraphicsTextItem(clippingSpace);
  layout->addItem(clippingSpace);

  // Popup erzeugen
  popup = new Plasma::Dialog();
  QVBoxLayout* popupLayout = new QVBoxLayout();
  popupLayout->setContentsMargins(5,5,5,5);
  popup->setLayout(popupLayout);
  popup->setWindowFlags(Qt::Popup);

  // GraphicsView und GraphicsScene erzeugen
  QGraphicsScene* popupScene = new QGraphicsScene();
  QGraphicsView* popupView = new QGraphicsView();
  popupView->setScene(popupScene);
  popupView->setFrameStyle(QFrame::NoFrame);
  popupView->setStyleSheet("background-color: transparent;");
  popupView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  popupView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  popupView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  popupLayout->addWidget(popupView);

  // Container mit dem GridLayout zur Scene hinzufügen
  QGraphicsWidget* historyContainer = new QGraphicsWidget();
  historyLayout = new QGraphicsGridLayout();
  historyLayout->setContentsMargins(0,0,0,0);
  historyContainer->setLayout(historyLayout);
  popupScene->addItem(historyContainer);

  QObject::connect(iconWidget, SIGNAL(clicked()), this, SLOT(showPopup()));
  QObject::connect(animationTimer, SIGNAL(timeout()), this, SLOT(animationStep()));

}

void NotificationServer::startServer() {
  static NotificationsAdaptor* adaptor = new NotificationsAdaptor(this);
  QDBusConnection bus = QDBusConnection::sessionBus();
  if (bus.registerService("org.freedesktop.Notifications") &&  bus.registerObject("/org/freedesktop/Notifications", adaptor, QDBusConnection::ExportAllContents)) {
    Notify("self", 0, "", "Success", "Plasmoid successfully started. Waiting for notifications.", QStringList(), QVariantMap(), 0);
  } else {
    Notify("self", 0, "dialog-cancel", "Error", "D-Bus service occupied - open config dialog for instructions.", QStringList(), QVariantMap(), 0);
    QDBusServiceWatcher* watcher = new QDBusServiceWatcher("org.freedesktop.Notifications", bus, QDBusServiceWatcher::WatchForUnregistration, this);
    QObject::connect(watcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(startServer()));
    QObject::connect(watcher, SIGNAL(serviceUnregistered(QString)), watcher, SLOT(deleteLater()));
  }
}

void NotificationServer::showPopup() {
  while (historyLayout->count() > 0) delete historyLayout->itemAt(0)->graphicsItem();
  while (history.count() > NotificationConfig::historySize()) delete history.takeLast();

  // History reinschreiben
  for (int i = 0; i < history.count(); i++) {
    Notification* notification = history.at(i);
    Plasma::IconWidget* icon; Plasma::Label* time; Plasma::Label* mesg;
    icon = new Plasma::IconWidget();
    icon->setPreferredWidth(25);
    icon->setIcon(notification->icon.isNull() ? KIcon("bookmarks") : notification->icon);
    icon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    time = new Plasma::Label();
    time->nativeWidget()->setIndent(3);
    time->nativeWidget()->setText(notification->time.toString("hh:mm"));
    time->nativeWidget()->setWordWrap(false);
    time->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    mesg = new Plasma::Label();
    mesg->nativeWidget()->setIndent(5);
    mesg->nativeWidget()->setText(notification->plain);
    mesg->nativeWidget()->setWordWrap(false);
    mesg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    historyLayout->addItem(icon, i, 0);
    historyLayout->addItem(time, i, 1);
    historyLayout->addItem(mesg, i, 2);
  }
  historyLayout->activate();

  // Popup an der richtigen Stelle einblenden
  popup->resize(std::max(400.0, size().width()), 300);
  popup->move(popupPosition(popup->size()));
  popup->animatedShow(Plasma::Direction(0));
}

void NotificationServer::showConfigurationInterface() {
  if (KConfigDialog::showDialog("notificationsrc")) return;
  KConfigDialog* dialog = new KConfigDialog(0, "notificationsrc", NotificationConfig::self());
  QObject::connect(dialog, SIGNAL(settingsChanged(const QString&)), this, SLOT(readConfiguration()));
  dialog->addPage(new SettingsPage(), "Settings", "configure");
  dialog->addPage(new HelpPage(), "Help", "help");
  dialog->show();
}

uint NotificationServer::Notify(const QString &app_name, uint id, const QString &icon, const QString &summary, const QString &body, const QStringList &actions, const QVariantMap &hints, int timeout) {
  Q_UNUSED(app_name); Q_UNUSED(id); Q_UNUSED(timeout);
  Notification* n = new Notification;
  n->html    = body;
  n->plain   = QString(body).replace(QRegExp("<[^>]*>"), " ").replace("\n", "").trimmed();
  n->title   = summary;
  n->actions = actions;
  n->hints   = hints;
  n->time    = QTime::currentTime();
  if (hints.contains("image_data"))
    n->icon = decodeImageData(hints["image_data"].value<QDBusArgument>());
  else if (!icon.isEmpty())
    n->icon = KIcon(icon);
  else
    n->icon = QIcon();
  queue.append(n);
  history.prepend(n);
  animationTimer->start(50);
  return 0;
}

void NotificationServer::animationStep() {
  static int countdown = 0;
  if (phase == Inactive) {
    if (queue.count() > 0) {
      Notification* notification = queue.takeFirst();
      movingLabel->setFont(font);
      movingLabel->setDefaultTextColor(color);
      movingLabel->setTextWidth(movingLabel->parentObject()->boundingRect().width());
      movingLabel->setHtml(notification->html);
      double diff = movingLabel->parentObject()->boundingRect().height() - movingLabel->boundingRect().height();
      movingLabel->setPos(0, diff > 0 ? diff/2.0 : 0);
      movingLabel->setOpacity(0.0);
      iconWidget->setIcon(notification->icon.isNull() ? QIcon(":notifications-active.png") : notification->icon);
      phase = FadeIn;
    } else {
      animationTimer->stop();
    }
  } else if (phase == FadeIn) {
    if (movingLabel->opacity() < 1.0) {
      movingLabel->setOpacity(movingLabel->opacity() + ap.fadeStep);
    } else {
      countdown = ap.pauseTime*2;
      phase = PauseBeforeScroll;
    }
  } else if (phase == PauseBeforeScroll) {
    if (countdown > 0) {
      countdown--;
    } else {
      phase = Scroll;
    }
  } else if (phase == Scroll) {
    if (movingLabel->sceneBoundingRect().bottom() > movingLabel->parentObject()->sceneBoundingRect().bottom()) {
      movingLabel->moveBy(0, - ap.scrollStep);
    } else {
      countdown = ap.pauseTime;
      phase = PauseAfterScroll;
    }
  } else if (phase == PauseAfterScroll) {
    if (countdown > 0) {
      countdown--;
    } else {
      phase = FadeOut;
    }
  } else if (phase == FadeOut) {
    if (movingLabel->opacity() > 0.0) {
      movingLabel->setOpacity(movingLabel->opacity() - ap.fadeStep);
    } else {
      iconWidget->setIcon(QIcon(":notifications-inactive.png"));
      countdown = 10;
      phase = PauseAtEnd;
    }
  } else if (phase == PauseAtEnd) {
    if (countdown > 0) {
      countdown--;
    } else {
      phase = Inactive;
    }
  } else {
    animationTimer->stop();
    qWarning() << "invalid animation state";
  }
}

QIcon NotificationServer::decodeImageData(const QDBusArgument imageData) {
  int width, height, bytesPerLine, hasAlpha, bitsPerSample, channels; QByteArray pixels; QImage::Format format;
  imageData.beginStructure(); imageData >> width >> height >> bytesPerLine >> hasAlpha >> bitsPerSample >> channels >> pixels; imageData.endStructure();
  if      (bitsPerSample == 8 && channels == 4) format = QImage::Format_ARGB32;
  else if (bitsPerSample == 8 && channels == 3) format = QImage::Format_RGB32;
  else return QIcon();
  QImage image = QImage((uchar*)pixels.data(), width, height, bytesPerLine, format);
  QPixmap pixmap = QPixmap::fromImage(image);
  return QIcon(pixmap);
}

SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent) {
  setupUi(this);
  connect(notifyButton, SIGNAL(clicked()), this, SLOT(sendNotification()));
}

void SettingsPage::sendNotification() {
  org::freedesktop::Notifications dbusProxy("org.freedesktop.Notifications", "/org/freedesktop/Notifications", QDBusConnection::sessionBus());
  QString mesg = messageTextEdit->toPlainText();
  dbusProxy.Notify("self", 0, "bookmarks", "Notification test", mesg, QStringList(), QVariantMap(), 0);
}

#include "notificationserver.moc"
K_EXPORT_PLASMA_APPLET(smartnotifications, NotificationServer)