#ifndef CTASK_H
#define CTASK_H

#include <QObject>
#include <QUdpSocket>

#define DEF_PORT    20001

class cTask : public QObject
{
    Q_OBJECT
public:
    explicit cTask(QObject *parent = 0);

signals:
    void finished();

public slots:
    void run();
private:
    QUdpSocket *sock;
};

#endif // CTASK_H
