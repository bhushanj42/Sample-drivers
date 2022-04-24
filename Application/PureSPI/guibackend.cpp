#include "guibackend.h"

GuiBackEnd::GuiBackEnd(QObject *parent): QObject(parent)
{
    qDebug()<<"Backend GUI controller started!!!";
    qDebug()<<"Current path = "<<QDir::currentPath();
    if(m_Dir.setCurrent("/dev")){
        qDebug()<<"Count = "<<m_Dir.count();
        qDebug()<<"New Current dir = "<<m_Dir.currentPath();

        file = new QFile(CHAR_DEVICE_NAME);
        if (file->exists()){
            if (file->open(QIODevice::ReadWrite | QIODevice::Unbuffered)){
                stream = new QDataStream(file);
                qDebug()<<"File opened successfully";
            }
            else {
                qDebug()<<"File was not opened";
            }
        }
        else{
            qDebug()<<"File does not exist";
        }
    }
    else{
        qWarning("There was a problem in changing the directory");
    }
}

void GuiBackEnd::writeToSPI(QString s)
{
    bool writeStatus = true;
    char data[16];
    int dataWritten;
    static char count = 33;

    data[0] = '0';//count;
    data[1] = '0';//count + 1;
    data[2] = '0';//count + 2;
    data[3] = '0';//count + 3;
    data[4] = '1';//count + 4;
    data[5] = '1';//count + 5;
    data[6] = '1';//count + 6;
    data[7] = '2';//count + 7;

    count = count + 8;
    if(count >= 119){
        count = 33;
    }

    dataWritten = stream->writeRawData(data, 4);
    if(dataWritten == -1) {
        qDebug()<<"Data not written";
        writeStatus = false;
    }
    else {
        qDebug()<<"Data written ="<<dataWritten;
    }
    emit sendstatusChanged(writeStatus);
}

void GuiBackEnd::readFromSPI()
{
    qint64 dataread;
    char data[16];
    unsigned char i;

    for(i = 0; i < 16; i++){
        data[i] = '!';
    }
    dataread = stream->readRawData(data, 4);
    if(dataread == -1){
        qDebug()<<"Reading failed!!!";
    }
    else{
        qDebug()<<(int)data[0]<<(int)data[1]<<(int)data[2]<<(int)data[3];
        qDebug()<<"Read done"<<dataread;
    }
}
