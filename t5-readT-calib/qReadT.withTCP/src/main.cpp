#include <QtCore>
#include <QCoreApplication>
#include <QTimer>
#include "ctask.h"
#include <signal.h>
#include <QDebug>

void setShutDownSignal( int signalId );
void handleShutDownSignal( int signalId );

cTask *task;

int main(int argc, char *argv[])
{
    setShutDownSignal( SIGINT ); // shut down on ctrl-c
    setShutDownSignal( SIGTERM ); // shut down on killall
    QCoreApplication a(argc, argv);

    // Task parented to the application so that it
    // will be deleted by the application.
    task = new cTask(&a);

    // This will cause the application to exit when
    // the task signals finished.
    QObject::connect(task, SIGNAL(finished()), task, SLOT(deleteLater()));
    QObject::connect(task, SIGNAL(finished()), &a, SLOT(quit()));

    // This will run the task from the application event loop.
    QTimer::singleShot(0, task, SLOT(run()));

    return a.exec();
}

void setShutDownSignal( int signalId )
{
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handleShutDownSignal;
    if (sigaction(signalId, &sa, NULL) == -1)
    {
        perror("setting up termination signal");
        exit(1);
    }
}

void handleShutDownSignal( int signalId )
{
    // QApplication::exit(0);
    //qDebug() << "Ctrl-C is pressed!";
    qDebug() << "Terminating readers...";
    //TODO: tell cTask to terminate gracefully
    task->stop();
}
