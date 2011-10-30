#include "jobview.h"
#include "jobviewadaptor.h"
#include <KIcon>

JobView::JobView(QString objectPath, QString appName, QString appIconName, int capabilities) : Plasma::Frame() {
  new JobViewV2Adaptor(this);
  myObjectPath = objectPath;
  myAppName    = appName;
  myIcon       = KIcon(appIconName);
  stoppable    = capabilities & 1;
  suspendable  = capabilities & 2;
  populated    = false;
  suspended    = false;
  cancelled    = false;
  setupUi();
  QDBusConnection::sessionBus().registerObject(objectPath, this);
  QTimer::singleShot(5000, this, SLOT(terminateIfStillEmpty()));
}

void JobView::setupUi() {

  // Rahmen und Hauptlayout
  setFrameShadow(Plasma::Frame::Sunken);
  QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(Qt::Vertical);
  setLayout(layout);

  // Titelleisten-Layout mit Label und Buttons
  QGraphicsLinearLayout* titleLayout = new QGraphicsLinearLayout();
  layout->addItem(titleLayout);
  Plasma::IconWidget* titleIcon = new Plasma::IconWidget();
  titleIcon->setMaximumSize(16, 16);
  titleIcon->setIcon(myIcon);
  titleLayout->addItem(titleIcon);
  titleLabel = new Plasma::Label();
  titleLabel->setText(QString("<b>%1</b>").arg(myAppName));
  titleLayout->addItem(titleLabel);
  if (stoppable) {
    stopButton = new Plasma::IconWidget();
    stopButton->setMaximumSize(16, 16);
    stopButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    stopButton->setIcon(KIcon("media-playback-stop"));
    titleLayout->addItem(stopButton);
    connect(stopButton, SIGNAL(clicked()), this, SLOT(requestCancel()));
  }
  if (suspendable) {
    suspendButton = new Plasma::IconWidget();
    suspendButton->setMaximumSize(16, 16);
    suspendButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    suspendButton->setIcon(KIcon("media-playback-pause"));
    titleLayout->addItem(suspendButton);
    connect(suspendButton, SIGNAL(clicked()), this, SLOT(requestSuspendToggle()));
  }

  // Container für Descriptions
  descriptionLayout = new QGraphicsGridLayout();
  layout->addItem(descriptionLayout);

  // Platz für ProcessedAmounts
  processedAmountLabel = new Plasma::Label();
  processedAmountLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  layout->addItem(processedAmountLabel);

  // Progressbar mit SpeedLabel
  meter = new Plasma::Meter();
  meter->setMeterType(Plasma::Meter::BarMeterHorizontal);
  meter->setPreferredHeight(22);
  layout->addItem(meter);
  QGraphicsLinearLayout* overlay = new QGraphicsLinearLayout();
  meter->setLayout(overlay);
  speedLabel = new Plasma::Label();
  speedLabel->nativeWidget()->setAlignment(Qt::AlignCenter);
  overlay->addItem(speedLabel);
}

QString JobView::objectPath() {
  return myObjectPath;
}

QIcon JobView::icon() {
  return myIcon;
}

void JobView::terminateIfStillEmpty() {
  if (!populated) terminate("timed out");
}

void JobView::setInfoMessage(const QString &message) {
  populated = true;
  titleLabel->setText(QString("<b>%1</b>").arg(message));
}

bool JobView::setDescriptionField(uint number, const QString &name, const QString &value) {
  populated = true;
  Plasma::Label* nameLabel; Plasma::Label* valueLabel;
  if (descriptionLabels.contains(number)) {
    nameLabel = descriptionLabels.value(number).first;
    valueLabel = descriptionLabels.value(number).second;
  } else {
    nameLabel = new Plasma::Label();
    valueLabel = new Plasma::Label();
    descriptionLabels.insert(number, QPair<Plasma::Label*, Plasma::Label*>(nameLabel, valueLabel));
    nameLabel->setAlignment(Qt::AlignLeft);
    nameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    valueLabel->setAlignment(Qt::AlignRight);
    valueLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    valueLabel->nativeWidget()->setWordWrap(true);
    descriptionLayout->addItem(nameLabel, number, 0);
    descriptionLayout->addItem(valueLabel, number, 1);
    descriptionLayout->activate();
  }
  descriptionLayout->activate();
  QFontMetrics fm(valueLabel->font());
  nameLabel->setText(name);
  valueLabel->setText(fm.elidedText(value, Qt::ElideMiddle, 260));
  return true;
}

void JobView::clearDescriptionField(uint number) {
  setDescriptionField(number, " ", " ");
}

void JobView::setPercent(uint percent) {
  populated = true;
  meter->setValue(percent);
  emit progressChanged(percent);
}

void JobView::setProcessedAmount(qulonglong amount, const QString &unit) {
  populated = true;
  processedAmount.insert(unit, amount);
  writeProcessedAmount();
}

void JobView::setTotalAmount(qulonglong amount, const QString &unit) {
  populated = true;
  totalAmount.insert(unit, amount);
  writeProcessedAmount();
}

void JobView::writeProcessedAmount() {
  QStringList textParts;
  QMapIterator<QString, ulong> iterator(totalAmount);
  while (iterator.hasNext()) {
    iterator.next();
    QString unit = iterator.key();
    QPair<double, QString> rawTotal          = QPair<double, QString>(iterator.value(), unit);
    QPair<double, QString> rawProcessed      = QPair<double, QString>(processedAmount.contains(unit) ? processedAmount[unit] : 0, unit);
    QPair<double, QString> readableTotal     = humanReadable(rawTotal);
    QPair<double, QString> readableProcessed = humanReadable(rawProcessed);
    if (readableTotal.second != readableProcessed.second)
      textParts << QString("%1 %2 / %3 %4").arg(readableProcessed.first, 0, 'f', 1).arg(readableProcessed.second).arg(readableTotal.first, 0, 'f', 1).arg(readableTotal.second);
    else
      textParts << QString("%1 / %2 %3").arg(readableProcessed.first, 0, 'f', 1).arg(readableTotal.first, 0, 'f', 1).arg(readableTotal.second);
    processedAmountLabel->setText(textParts.join(", ").replace(".0", ""));
  }
}

void JobView::setSpeed(qulonglong bytesPerSecond) {
  if (suspended) return;
  if (bytesPerSecond < 100) { speedLabel->setText(""); return; }
  QPair<double, QString> readableSpeed = humanReadable(QPair<double, QString>(bytesPerSecond, "bytes"));
  QString speedText = QString("%1 %2/s").arg(readableSpeed.first, 0, 'f', 1).arg(readableSpeed.second);
  if (totalAmount.contains("bytes") && processedAmount.contains("bytes")) {
    ulong remainingBytes = totalAmount.value("bytes") - processedAmount.value("bytes");
    QPair<double, QString> readableRemaining = humanReadable(QPair<double, QString>(remainingBytes / bytesPerSecond, "seconds"));
    speedText.append(QString(" (~%1 %2)").arg(readableRemaining.first, 0, 'f', 1).arg(readableRemaining.second));
  }
  speedLabel->setText(speedText.replace(".0", ""));
}

void JobView::setSuspended(bool suspended) {
  this->suspended = suspended;
  suspendButton->setIcon(KIcon(suspended ? "media-playback-start" : "media-playback-pause"));
  speedLabel->setText(suspended ? "suspended" : "resuming...");
}

void JobView::setDestUrl(const QDBusVariant &destUrl) {}

void JobView::terminate(const QString &errorMessage) {
  if (!errorMessage.isEmpty()) {
    speedLabel->setText(errorMessage);
  } else if (cancelled) {
    speedLabel->setText("cancelled");
  } else {
    speedLabel->setText("finished");
    setPercent(100);
  }
  emit terminated();
}

void JobView::requestSuspendToggle() {
  if (suspended) emit resumeRequested();
  else           emit suspendRequested();
}

void JobView::requestCancel() {
  cancelled = true;
  emit cancelRequested();
}

QPair<double, QString> JobView::humanReadable(QPair<double, QString> raw) {
  double value = raw.first;
  QString unit = raw.second;
  if (unit == "bytes") {
    if (value > pow(1024,3)) return QPair<double, QString>(value/pow(1024,3), "GByte");
    if (value > pow(1024,2)) return QPair<double, QString>(value/pow(1024,2), "MByte");
    if (value > pow(1024,1)) return QPair<double, QString>(value/pow(1024,1), "KByte");
  } else if (unit == "seconds") {
    if (value > 3600) return QPair<double, QString>(value/3600, "hours");
    if (value > 60)   return QPair<double, QString>(value/60, "minutes");
  }
  return QPair<double, QString>(value, unit);
}


#include "jobview.moc"