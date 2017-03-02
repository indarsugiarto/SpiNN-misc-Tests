#include "ctask.h"
#include <QDebug>
#include <QVector>
#include <QCoreApplication>
#include <QTextStream>
#include <QByteArray>
#include <QDataStream>

cTask::cTask(QObject *parent) : QObject(parent)
{
    /* NOTE: Don't create heap object here!!!!
    sock = new QUdpSocket(this);
    if(!sock->bind(DEF_PORT)) {
        qDebug() << QString("Fail to bind port-%1").arg(DEF_PORT);
    } else {
        qDebug() << QString("Ready to receive packets at port-%1").arg(DEF_PORT);
    }
    */
}

void cTask::run()
{

    // Prepare TCP server
    m_Server = new QTcpServer(this);
    if(!m_Server->listen(QHostAddress::Any, TCP_PORT)) {
        qDebug() << QString("Cannot start the server: %1").arg(m_Server->errorString()); 
        emit finished(); 
        return;
    }

	qDebug() << QString("Works as a TCP server on port-%1").arg(TCP_PORT);
    // initialize client handler
    clientConnected = false;
    connect(m_Server, SIGNAL(newConnection()), this, SLOT(newClient()));

    // Prepare the timer
    timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, SIGNAL(timeout()), this, SLOT(processResult()));

    // For sensor-1
    T1 = new QThread();
    R1 = new tempReader(ID_R1,N_ITERATION);
    R1->moveToThread(T1);
    connect(R1, SIGNAL(newTval(int,int)), this, SLOT(readTval(int,int)));
    connect(T1, SIGNAL(started()), R1, SLOT(run()));
    connect(R1, SIGNAL(finished()), T1, SLOT(quit()));
    connect(R1, SIGNAL(finished()), R1, SLOT(deleteLater()));
    connect(T1, SIGNAL(finished()), T1, SLOT(deleteLater()));

    // For sensor-2
    T2 = new QThread();
    R2 = new tempReader(ID_R2,N_ITERATION);
    R2->moveToThread(T2);
    connect(R2, SIGNAL(newTval(int,int)), this, SLOT(readTval(int,int)));
    connect(T2, SIGNAL(started()), R2, SLOT(run()));
    connect(R2, SIGNAL(finished()), T2, SLOT(quit()));
    connect(R2, SIGNAL(finished()), R2, SLOT(deleteLater()));
    connect(T2, SIGNAL(finished()), T2, SLOT(deleteLater()));

    // For sensor-3
    T3 = new QThread();
    R3 = new tempReader(ID_R3,N_ITERATION);
    R3->moveToThread(T3);
    connect(R3, SIGNAL(newTval(int,int)), this, SLOT(readTval(int,int)));
    connect(T3, SIGNAL(started()), R3, SLOT(run()));
    connect(R3, SIGNAL(finished()), T3, SLOT(quit()));
    connect(R3, SIGNAL(finished()), R3, SLOT(deleteLater()));
    connect(T3, SIGNAL(finished()), T3, SLOT(deleteLater()));

    // For sensor-4
    T4 = new QThread();
    R4 = new tempReader(ID_R4,N_ITERATION);
    R4->moveToThread(T4);
    connect(R4, SIGNAL(newTval(int,int)), this, SLOT(readTval(int,int)));
    connect(T4, SIGNAL(started()), R4, SLOT(run()));
    connect(R4, SIGNAL(finished()), T4, SLOT(quit()));
    connect(R4, SIGNAL(finished()), R4, SLOT(deleteLater()));
    connect(T4, SIGNAL(finished()), T4, SLOT(deleteLater()));

    connect(R1, SIGNAL(finished()), this, SLOT(readerStopped()));
    connect(R2, SIGNAL(finished()), this, SLOT(readerStopped()));
    connect(R3, SIGNAL(finished()), this, SLOT(readerStopped()));
    connect(R4, SIGNAL(finished()), this, SLOT(readerStopped()));

    // start them
    T1->start(); T2->start(); T3->start(); T4->start();
    timer->start();

    stoppedReader = 0;
    isRunning = true;

    // Do processing here
    qDebug() << "Temperature readers are started!";
    if(N_ITERATION == 0)
        qDebug() << "Press Ctrl-C to terminate!";
    //qDebug() << QString("Total packet = %1, of %2 stream").arg(total).arg(cntr.count());
    while(isRunning){
        QCoreApplication::processEvents();
    }
    emit finished();
}

void cTask::readTval(int sID, int Tval)
{
    //qDebug() << QString("T-%1 = %2").arg(sID).arg(Tval);
    allTval[sID-1] = Tval;
}

void cTask::readerStopped()
{
    stoppedReader++;
    if(stoppedReader==4)
        isRunning = false;
}

void cTask::stop()
{
    timer->stop();
    R1->stop();
    R2->stop();
    R3->stop();
    R4->stop();
}

void cTask::processResult()
{
    qDebug() << QString("T1 = %1, T2 = %2, T3 = %3, T4 = %4").arg(allTval[0]/1000.0,0,'g',4,'0')
                .arg(allTval[1]/1000.0,0,'g',4,'0').arg(allTval[2]/1000.0,0,'g',4,'0')
                .arg(allTval[3]/1000.0,0,'g',4,'0');
	if(clientConnected) {
                /*
                QByteArray block;
                QDataStream out(&block, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_0);
                */
                QTextStream out(m_Client);
                
		// QTextStream out(m_Client);
		QString txt = QString("%1,%2,%3,%4").arg(allTval[0]/1000.0,0,'g',4,'0')
                .arg(allTval[1]/1000.0,0,'g',4,'0').arg(allTval[2]/1000.0,0,'g',4,'0')
                .arg(allTval[3]/1000.0,0,'g',4,'0');
		out << txt << endl;
        qDebug() << "Data is streamed...!";
    }
}

void cTask::newClient()
{
    m_Client = m_Server->nextPendingConnection();
    connect(m_Client, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    connect(m_Client, SIGNAL(disconnected()), m_Client, SLOT(deleteLater()));
    qDebug() << "Receive a connection request. Activating the streamer...!";
	clientConnected = true;
}

void cTask::clientDisconnected()
{
    qDebug() << "Client is disconnected...!";
    clientConnected=false;
}

