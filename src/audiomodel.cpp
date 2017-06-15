#define _USE_MATH_DEFINES

#include <functional>
#include <cstdlib>
#include <fftw3.h>

#include "audiomodel.h"

const complex<double> AudioModel::ZERO = complex<double>(0, 0);
AudioModel::AudioModel(QObject *parent) : QObject(parent)
{

}

/**
 *  @brief Metoda korzystająca z szybkiej transformaty Fourier'a, która zwraca FFT tablicy liczb zespolonych.
 *
 *  @param  x tablica liczb zespolonych
 *  @return Zwraca FFT tablicy liczb zespolonych <tt>x</tt>
 * @authors Adrian Borucki Magdalena Buczyńska Adrianna Łuczak Kamil Wasilewski
 */
QVector<complex<double>> AudioModel::fft(QVector<complex<double> > x)
{
    int N = x.length();

    fftw_complex *in, *out;
    fftw_plan p;
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    for (int i = 0; i < N; i++) {
        in[i][0] = x[i].real();
        in[i][1] = x[i].imag();
    }

    fftw_execute(p);

    auto y = QVector<complex<double>>(N);
    for (int i = 0; i < N; i++) {
        y[i] = complex<double>(out[i][0], out[i][1]);
    }

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
    return y;
}

/**
 *  Returns the inverse FFT of the specified complex array.
 *
 *  @param  x the complex array
 *  @return the inverse FFT of the complex array <tt>x</tt>
 */
QVector<complex<double>> AudioModel::ifft(QVector<complex<double> > x)
{
    int N = x.length();
    QVector<complex<double>> y = QVector<complex<double>>(N);

    // take conjugate
    for (int i = 0; i < N; i++) {
        y[i] = conj(x[i]);
    }

    // compute forward FFT
    y = fft(y);

    // take conjugate again
    for (int i = 0; i < N; i++) {
        y[i] = conj(y[i]);
    }

    // divide by N
    for (int i = 0; i < N; i++) {
        y[i] = y[i] * (1.0 / N);
    }

    return y;
}

/**
 *  Returns the linear convolution of the two specified complex arrays.
 *
 *  @param  x one complex array
 *  @param  y the other complex array
 *  @return the linear convolution of <tt>x</tt> and <tt>y</tt>
 */
QVector<complex<double>> AudioModel::convolve(QVector<std::complex<double> > x, QVector<std::complex<double> > y)
{
    QVector<complex<double>> a = QVector<complex<double>>(2*x.length());
    for (int i = 0; i < x.length(); i++)
        a[i] = x[i];
    for (int i = x.length(); i < 2*x.length(); i++)
        a[i] = ZERO;

    QVector<complex<double>> b = QVector<complex<double>>(2*y.length());
    for (int i = 0; i < y.length(); i++)
        b[i] = y[i];
    for (int i = y.length(); i < 2*y.length(); i++)
        b[i] = ZERO;

    return cconvolve(a, b);
}

/**
 *  Returns the circular convolution of the two specified complex arrays.
 *
 *  @param  x one complex array
 *  @param  y the other complex array
 *  @return the circular convolution of <tt>x</tt> and <tt>y</tt>
 */
QVector<complex<double>> AudioModel::cconvolve(QVector<std::complex<double> > x, QVector<std::complex<double> > y)
{
    // TODO: should probably pad x and y with 0s so that they have same length
    // and are powers of 2
    if (x.length() != y.length()) {
       throw new invalid_argument("Dimensions don't agree.");
    }

    int N = x.length();

    // compute FFT of each sequence
    QVector<complex<double>> a = fft(x);
    QVector<complex<double>> b = fft(y);

    // point-wise multiply
    QVector<complex<double>> c = QVector<complex<double>>(N);
    for (int i = 0; i < N; i++) {
        c[i] = a[i] * b[i];
    }

    // compute inverse FFT
    return ifft(c);
}
/**
 *  @brief Metoda obliczająca charakterystykę mikrofonu i głośność w decybelach orginalnego sygnału z urządzenia wejścia przy pomocy twierdzenia Parsevala.
 *
 *  @param x Orginalny sygnał z urządzenia wejścia
 *  @param calibrationOffset
 *  @param referencePower Głośność w decybelach punktu odniesienia (kalibracji).
 *  @return Głośność w decybelach obliczona przy pomocy twierdzenia Parsevala.
 *  @authors Adrian Borucki Magdalena Buczyńska Adrianna Łuczak Kamil Wasilewski
 */
double AudioModel::power(QVector<std::complex<double>> x, double calibrationOffset)
{
    int original_length = x.length();
    auto xfft = fft(x);
    transform(xfft.begin(), xfft.end(), xfft.begin(), [=](complex<double> z){ return z + calibrationOffset; });
    //transform(xfft.begin(), xfft.end(), xfft.begin(), [=](complex<double> z){ return 20*log10(z.real()) + calibrationOffset; });

    double result = 0;
    double fraction;
    for(auto z: xfft)
       result += abs(z) * abs(z);
    fraction = (double)xfft.length() / (double)original_length;
    result = result*fraction;

    return result;
}

/**
 *  @brief Metoda obliczająca charakterystykę mikrofonu i głośność w decybelach orginalnego sygnału z urządzenia wejścia przy pomocy twierdzenia Parsevala.
 *
 *  @param x Orginalny sygnał z urządzenia wejścia
 *  @param calibrationOffset
 *  @param referencePower Głośność w decybelach punktu odniesienia (kalibracji).
 *
 *  @return Głośność w decybelach obliczona przy pomocy twierdzenia Parsevala.
 *  @authors Adrian Borucki Magdalena Buczyńska Adrianna Łuczak Kamil Wasilewski
 */
double AudioModel::computeLevel(QVector<std::complex<double>> x, double calibrationOffset, double referencePower)
{
    double p = power(x, calibrationOffset);
    double f = 48000.0;
    double R_A = 12194*12194*pow(f, 4.0) / ((f*f + 20.6*20.6) * sqrt((f*f + 107.7*107.7)*(f*f + 737.9*737.9)) * (f*f + 12194*12194));
    double A = 20*log10(R_A) + 2.0;
    return log10(p / referencePower) + A;
}
