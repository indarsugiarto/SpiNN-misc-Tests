/*
 TODO: validasi mapping ini:
 chip<0,0> --> R1, yang berarti ID_R1==1
 chip<1,0> --> R2, yang berarti ID_R2==2
 chip<0,1> --> R3, yang berarti ID_R3==3
 chip<1,1> --> R4, yang berarti ID_R4==4

 Ternyata yang benar adalah:
 chip<0,0> --> R2, yang berarti ID_R2==1
 chip<1,0> --> R1, yang berarti ID_R1==2
 chip<0,1> --> R4, yang berarti ID_R4==3
 chip<1,1> --> R3, yang berarti ID_R3==4

 */

#define ID_R1 1
#define ID_R2 2
#define ID_R3 3
#define ID_R4 4

#ifndef CTASK_H
#define CTASK_H

#include <QObject>
#include <QThread>
//#include <QUdpSocket>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>

#include "tempReader.h"

#define N_ITERATION 0	// repeat until Ctrl-C is pressed

// the following definition must agree with the value in constDef.py
#define TCP_PORT	30001

class cTask : public QObject
{
    Q_OBJECT
public:
    explicit cTask(QObject *parent = 0);

signals:
    void finished();

public slots:
    void run();
    void readTval(int sID, int Tval);
    void stop();
    void readerStopped();
    void processResult();
    void newClient();
    void clientDisconnected();

private:
    QTimer *timer;
//    QUdpSocket *sock;
    QTcpServer *m_Server;
    QTcpSocket *m_Client;

    QThread *T1, *T2, *T3, *T4;
    tempReader *R1, *R2, *R3, *R4;
    int allTval[4];
    int stoppedReader;
    bool isRunning;
    bool clientConnected;
};

#endif // CTASK_H

