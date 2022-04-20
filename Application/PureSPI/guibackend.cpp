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
            if (file->open(QIODevice::ReadWrite | QIODevice::Unbuffered)){// | QIODevice::Text)){
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

    data[0] = '1';
    data[1] = '2';
    data[2] = '3';
    data[3] = '4';
    data[4] = '5';
    data[5] = '6';
    data[6] = '7';
    data[7] = '8';

    dataWritten = stream->writeRawData(data, 8);
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
