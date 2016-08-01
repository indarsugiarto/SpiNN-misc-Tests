#include "ctask.h"
#include <QDebug>
#include <QVector>

cTask::cTask(QObject *parent) : QObject(parent)
{
    sock = new QUdpSocket(this);
    if(!sock->bind(DEF_PORT)) {
        qDebug() << QString("Fail to bind port-%1").arg(DEF_PORT);
    } else {
        qDebug() << QString("Ready to receive packets at port-%1").arg(DEF_PORT);
    }
}

void cTask::run()
{
    // Do processing here
    bool Running = true;
    qint64 sz;
    char buf[1024];
    int total = 0;
    QVector<int> cntr;
    while(Running) {
        sz = sock->pendingDatagramSize();
        if(sz != -1) {
            sz = sock->readDatagram(buf, 1024);
            if(sz > 10) {
                sz -= 10;
                total += sz;  // ignore the header
                cntr.append(sz);
            }
            else Running = false;
        }
    }
    // then print the report
    qDebug() << QString("Total packet = %1, of %2 stream").arg(total).arg(cntr.count());
    /*
    for(int i=0; i<cntr.count(); i++)
        qDebug() << QString("%1").arg(cntr.at(i));
    */
    emit finished();
}

