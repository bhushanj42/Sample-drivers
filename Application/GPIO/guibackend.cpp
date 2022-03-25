#include "guibackend.h"

GUIBackend::GUIBackend(QObject *parent): QObject(parent)
{
//    char c = '5';
    qDebug()<<"Backend GUI controller started!!!";
    m_sometext = "A text set in backend class";
    m_RedLEDState = 0;
    m_GreenLEDState = 0;
    qDebug()<<"Current path = "<<QDir::currentPath();
    if(m_Dir.setCurrent("/dev")){

        qDebug()<<"New Current path = "<<QDir::currentPath();
        qDebug()<<"Count = "<<m_Dir.count();
        qDebug()<<"New Current dir = "<<m_Dir.currentPath();

        file = new QFile(CHAR_DEVICE_NAME);
        if (file->exists()){
            if (file->open(QIODevice::ReadWrite)){// | QIODevice::Text)){
                qDebug()<<"File opened successfully";
                if (file->putChar('5')){
                    qDebug()<<"Data written successfully!!!";
                    file->flush();
                }
                else {
                    qDebug()<<"DATA not written";
                }
            }
            else {
                qDebug()<<"File was not opened";
            }
        }
        else{
            qDebug()<<"File does not exists";
        }
    }
    else{
        qWarning("There was a problem in changing the directory");
    }
}

GUIBackend::~GUIBackend()
{
    qDebug()<<"Did soemeone call me ehh ??";
}

void GUIBackend::redButtonClicked()
{
    qDebug()<<"Red Button CLICKED!!!!";
    m_RedLEDState = m_RedLEDState ^ 1;
    if(m_RedLEDState){
        if (file->putChar('1')){
            qDebug()<<"Data written successfully!!!";
            file->flush();
        }
    }
    else {
        if (file->putChar('2')){
            qDebug()<<"Data written successfully!!!";
            file->flush();
        }
    }
    setSometext("Red Button CLICKED!!!!");
}

void GUIBackend::greenButtonClicked()
{
    qDebug()<<"Green Button CLICKED!!!!";
    m_GreenLEDState = m_GreenLEDState ^ 1;
    if(m_GreenLEDState){
        if (file->putChar('3')){
            qDebug()<<"Data written successfully!!!";
            file->flush();
        }
    }
    else {
        if (file->putChar('4')){
            qDebug()<<"Data written successfully!!!";
            file->flush();
        }
    }
    setSometext("Green Button CLICKED!!!!");
}

QString GUIBackend::sometext() const
{
//    qDebug()<<"Property read operation";
    return m_sometext;
}

void GUIBackend::setSometext(QString sometext)
{
    if (m_sometext == sometext)
        return;

    m_sometext = sometext;
    emit sometextChanged(m_sometext);
//    qDebug()<<"property write operation";
}
