#include "tempReader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

tempReader::tempReader(int sID, int epoch, QObject *parent) : QObject(parent)
    , isRunning(false)
    , runCntr(0)
{
    // NOTE: IMPORTANT, don't create any heap object here!!!!
    myID = sID;
    iteration = epoch;
}

void tempReader::run()
{
    QString sFile;
    switch(myID) {
    case 1:
        sFile = "/sys/bus/w1/devices/28-000007a08e92/w1_slave"; break;
    case 2:
        sFile = "/sys/bus/w1/devices/28-000007a0b647/w1_slave"; break;
    case 3:
        sFile = "/sys/bus/w1/devices/28-000007a0bd43/w1_slave"; break;
    case 4:
        sFile = "/sys/bus/w1/devices/28-000007a1dda4/w1_slave"; break;
    }

    isRunning = true;
    // which sensor I'm supposed to read?
    while(isRunning) {
        // open the device
        QFile file(sFile);
        QString line;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // read the device file
            QTextStream in(&file);
            // skip the first line
            line = in.readLine();
            // read the second line
            line = in.readLine();

            // close the device file
            file.close();

            // extract the value
            QStringList lst = line.split("=");
            // qDebug() << lst.at(1);
            myT = lst.at(1).toInt();
            emit newTval(myID, myT);
        }

      // number of iteration is reached?
      if(iteration>0) {
          runCntr++;
          if(runCntr>=iteration) {
              isRunning = false;
          }
      }
    } // end of while(isRunning)
    emit finished();
}

void tempReader::stop()
{
    isRunning = false;
}
