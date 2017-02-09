#ifndef TEMPREADER_H
#define TEMPREADER_H

/* This temperature reader will read sensor value as fast
 * as possible. It is the responsibility of cTask to manage
 * the timer tick (eg. every 1 second to display).
 */

#include <QObject>

class tempReader : public QObject
{
    Q_OBJECT
public:
    explicit tempReader(int sID, int epoch = 0, QObject *parent = 0);
signals:
    void finished();
    void newTval(int sID, int Tval);
public slots:
    void run();
    void stop();
private:
    int myID;
    int iteration;
    volatile bool isRunning;
    int runCntr;
    int myT;
};


#endif
