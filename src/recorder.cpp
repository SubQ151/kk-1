#include "recorder.h"
#include <QDir>
#include <QAudioFormat>
#include <QDataStream>
#include <limits>

using std::logic_error;
/**
 * @brief Konstruktor bezparametrowy. Inicjalizuje recorder.
 * @authors Kamil Wasilewski
 */
Recorder::Recorder()
{
    audio = nullptr;
	InitialiseRecorder();
	setupTimer();
}
/**
 * @brief Destruktor.
 * @authors Kamil Wasilewski
 */
Recorder::~Recorder()
{
    if (audio != nullptr)
        delete audio;
}
/**
 * @brief Metoda przypisująca zmiennej device parametry wybranego urządzenia wejścia.
 * @param deviceName Nazwa urządzenia wejścia
 * @warning Jeśli format wybranego urządzenia nie jest obsługiwany, wybrany zostanie format najlepszego dopasowania.
 * @authors Kamil Wasilewski
 */
void Recorder::InitialiseRecorder(const QString &deviceName)
{
	// destroy old QAudioInput object (if exist)
	if (audio != nullptr)
		delete audio;

	QAudioDeviceInfo device;
    if (deviceName.isEmpty())
		device = QAudioDeviceInfo::defaultInputDevice();
	else
	{
		for (auto inputDevice : QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
		{
			if (inputDevice.deviceName() == deviceName)
			{
				device = inputDevice;
				break;
			}
		}
	}

    setFormatSettings();
	if (!device.isFormatSupported(format))
	{
		qDebug() << "Format not supported, trying to use the nearest.";
		format = device.nearestFormat(format);
	}
	printFormat();
    audio = new QAudioInput(device, format);
}
/**
 * @brief Metoda inicjalizująca timer, trwający 5 sekund.
 * @authors Kamil Wasilewski
 */
void Recorder::setupTimer()
{
	timer.setSingleShot(true);
	timer.setInterval(5000);
	connect(&timer, SIGNAL(timeout()), this, SLOT(Stop()));
}
/**
 * @brief Metoda ustawiająca format próbek.
 * @authors Kamil Wasilewski
 */
void Recorder::setFormatSettings()
{
	format.setChannelCount(1);
	format.setSampleRate(48000);
	format.setCodec("audio/pcm");
	format.setSampleSize(16);
    format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);
}
/**
 * @brief Metoda rozpoczynająca proces pobierania dźwięków z urządzenia wejścia do bufora.
 * @authors Kamil Wasilewski
 */
void Recorder::Start()
{
    // to be deleted in final release:
//	try
//	{
//		openFile("audiodata.wav");
//	}
//	catch (exception &)
//	{
//		throw;
//		return;
//	}
    buffer.buffer().clear(); // Flush data from underlying QByteArray internal buffer.
    buffer.open(QIODevice::ReadWrite);
    audio->start(&buffer);

	// Record 5 seconds.
    timer.start();
}
/**
 * @brief Metoda wczytująca ustawienia z pliku konfiguracyjnego.
 * @param fileName Nazwa pliku konfiguracyjnego.
 * @warning Jeżeli plik jest tylko do odczytu lub istnieje już katalog o takiej samej nazwie, wyświetlony zostanie błąd.
 * @authors Kamil Wasilewski
 */
void Recorder::openFile(const QString &fileName)
{
	// Check whether some directory with the same name already exist.
	QDir testDir(fileName);
	if (testDir.exists())
	{
		QString msg = QString("Nie udało się utworzyć pliku. Istnieje już katalog o tej samej nazwie (%1).").arg(fileName);
		throw logic_error(msg.toStdString());
		return;
	}

	QDir dir;
	QString path = dir.absoluteFilePath(fileName);
	file.setFileName(path);
	file.setAudioFormat(format);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QString msg = "Nie udało się otworzyć pliku. ";
		msg.append(file.errorString());
		throw logic_error(msg.toStdString());
		return;
	}
}
/**
 * @brief Metoda kończąca przechwytywanie danych do buforu.
 * @authors Kamil Wasilewski
 */
void Recorder::Stop()
{
    timer.stop(); // Stop a timer in case user aborts recording.
    audio->stop();
    buffer.close();

    parseBufferContent(buffer.data());
	emit recordingStopped(complexData);
}
/**
 * @brief Metoda zamykająca plik konfiguracyjny.
 * @authors Kamil Wasilewski
 */
void Recorder::closeFile()
{
    file.close();
}
/**
 * @brief Metoda wyświetlająca format pobieranych danych.
 * @authors Kamil Wasilewski
 */
void Recorder::printFormat() const
{
	qDebug() << "Channel count: " << format.channelCount();
	qDebug() << "Byte order: " << format.byteOrder();
	qDebug() << "Sample type: " << format.sampleType();
	qDebug() << "Sample size: " << format.sampleSize();
	qDebug() << "Sample rate: " << format.sampleRate();
    qDebug() << "";
}
/**
 * @brief Metoda zwracająca listę dostępnych urządzeń wejścia.
 * @return devicesNames lista nazw dostępnych urządzeń wejścia.
 * @authors Kamil Wasilewski
 */
QStringList Recorder::GetAvailableDevices() const
{
    auto devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
	QStringList devicesNames;
    for (auto device : devices)
    {
		devicesNames.append(device.deviceName());
    }
	return devicesNames;
}
/**
 * @brief Metoda parsująca dane zawarte w buforze.
 * @authors Kamil Wasilewski
 */
void Recorder::parseBufferContent(const QByteArray &data)
{
	complexData.clear();
	QDataStream stream(data);
	while (!stream.atEnd())
	{
        short i;
		stream >> i;
		complexData.push_back(std::complex<double>((double)i, 0.0));
    }
}
/**
 * @brief Metoda wczytująca dane Audio z pliku.
 * @param fileName Nazwa pliku.
 * @authors Kamil Wasilewski
 */
void Recorder::LoadAudioDataFromFile(const QString &fileName)
{
    complexData.clear();
    QFile file(fileName);
    file.open(QFile::ReadOnly);
    QDataStream fstream(&file);
    while (!fstream.atEnd())
    {
        short i;
        fstream >> i;
        i /= std::numeric_limits<short>::max(); // Scale to [-1,1] range.
        complexData.push_back(std::complex<double>((double)i, 0.0));
    }
    file.close();
    emit recordingStopped(complexData);
}
