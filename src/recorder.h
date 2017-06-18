#ifndef RECORDER_H
#define RECORDER_H

#include <QAudioInput>
#include <QDebug>
#include <QTimer>
#include <QBuffer>
#include <QStringList>
#include <QDataStream>
#include <exception>
#include <complex>

using std::exception;
/**
 * @brief Klasa odpowiadająca za nagrywanie dźwięku z wybranego urządzenia wejścia.
 * @author Kamil Wasilewski
 */
class Recorder : public QObject
{
    Q_OBJECT
    QAudioFormat format;
    QAudioInput *audio;
    QBuffer buffer;
    QTimer timer;
	QVector<std::complex<double> > complexData;

	void setupTimer();
	void setFormatSettings();
    void printFormat() const;
	void parseBufferContent(const QByteArray &data);
	void parse(QDataStream &stream);
public:
	Recorder();
    ~Recorder();
    void Start();
	QStringList GetAvailableDevices() const;
    void LoadAudioDataFromFile(const QString &fileName);
public slots:
	void Stop();
	void InitialiseRecorder(const QString &deviceName = "");
signals:

   /**
    * @brief Sygnał kończący nagrywanie.
    * @authors Kamil Wasilewski
    */
	void recordingStopped(const QVector<std::complex<double> > &complexData);
};

#endif // RECORDER_H
