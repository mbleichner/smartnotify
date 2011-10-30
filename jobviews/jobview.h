#ifndef JOBVIEW_H
#define JOBVIEW_H

#include <QtCore>
#include <QtGui>
#include <QtDBus>
#include <QtDebug>

#include <Plasma/Label>
#include <Plasma/Frame>
#include <Plasma/Meter>
#include <Plasma/IconWidget>

class JobView : public Plasma::Frame {
  Q_OBJECT

  public:
    JobView(QString objectPath, QString appName, QString appIconName, int capabilities);
    void setupUi();
    QString objectPath();
    QIcon icon();

  protected:
    QString myObjectPath;
    QString myAppName;
    QIcon myIcon;
    bool populated;
    bool suspended;
    bool cancelled;
    bool stoppable;
    bool suspendable;
    QMap<QString, ulong> processedAmount;
    QMap<QString, ulong> totalAmount;

  protected:
    Plasma::Label* titleLabel;
    Plasma::IconWidget* stopButton;
    Plasma::IconWidget* suspendButton;
    Plasma::Label* processedAmountLabel;
    QGraphicsGridLayout* descriptionLayout;
    QMap< int, QPair<Plasma::Label*, Plasma::Label*> > descriptionLabels;
    Plasma::Meter* meter;
    Plasma::Label* speedLabel;

  protected:
    void writeProcessedAmount();
    QPair<double, QString> humanReadable(QPair<double, QString> raw);

  public Q_SLOTS:
    void clearDescriptionField(uint number);
    bool setDescriptionField(uint number, const QString &name, const QString &value);
    void setInfoMessage(const QString &message);
    void setPercent(uint percent);
    void setProcessedAmount(qulonglong amount, const QString &unit);
    void setSpeed(qulonglong bytesPerSecond);
    void setSuspended(bool suspended);
    void setTotalAmount(qulonglong amount, const QString &unit);
    void setDestUrl(const QDBusVariant &destUrl);
    void terminate(const QString &errorMessage);

  protected Q_SLOTS:
    void terminateIfStillEmpty();
    void requestSuspendToggle();
    void requestCancel();

  Q_SIGNALS:
    void cancelRequested();
    void resumeRequested();
    void suspendRequested();
    void progressChanged(uint percent);
    void terminated();
};

#endif