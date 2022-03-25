#ifndef GUIBACKEND_H
#define GUIBACKEND_H

#include <QObject>
#include <QDebug>
#include <QDir>
#include <QFile>

class GUIBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sometext READ sometext WRITE setSometext NOTIFY sometextChanged)
public:
    GUIBackend(QObject *parent = nullptr);
    ~GUIBackend();
    Q_INVOKABLE void redButtonClicked();
    Q_INVOKABLE void greenButtonClicked();
    QString sometext() const;
public slots:
    void setSometext(QString sometext);
signals:
    void sometextChanged(QString sometext);

private:
    QString m_sometext;
    bool m_RedLEDState, m_GreenLEDState;
    QDir m_Dir;
    QFile *file;
    const QString CHAR_DEVICE_NAME = "migpio-char";
};

#endif // GUIBACKEND_H
