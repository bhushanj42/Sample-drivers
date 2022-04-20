#ifndef GUIBACKEND_H
#define GUIBACKEND_H

#include <QObject>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QDataStream>

class GuiBackEnd : public QObject
{
    Q_OBJECT
public:
    GuiBackEnd(QObject *parent = nullptr);
    Q_INVOKABLE void writeToSPI(QString s);
    Q_INVOKABLE void readFromSPI();

signals:
    void sendstatusChanged(bool b);

private:
    QDir m_Dir;
    QFile *file;
    const QString CHAR_DEVICE_NAME = "purespi-char";
    QDataStream *stream;
};

#endif // GUIBACKEND_H
