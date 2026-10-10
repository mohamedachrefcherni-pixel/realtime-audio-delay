#include <iostream>
#include <vector>
#include <portaudio.h>
#include "pa_asio.h"

// ---------------- Delay Line ----------------
struct DelayLine {
    std::vector<float> buffer;
    int writePos = 0;
    int delaySamples = 0;

    void init(double sampleRate, double maxDelaySeconds) {
        buffer.assign((int)(sampleRate * maxDelaySeconds) + 1, 0.0f);
        writePos = 0;
    }

    void setDelaySeconds(double sampleRate, double delaySeconds) {
        delaySamples = (int)(sampleRate * delaySeconds);
        if (delaySamples >= (int)buffer.size()) delaySamples = buffer.size() - 1;
        if (delaySamples < 0) delaySamples = 0;
    }

    float process(float input) {
        buffer[writePos] = input;

        int readPos = writePos - delaySamples;
        if (readPos < 0) readPos += buffer.size();

        float output = buffer[readPos];

        writePos = (writePos + 1) % buffer.size();
        return output;
    }
};

DelayLine delayLine;

static int passthroughCallback(const void *inputBuffer, void *outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *userData)
{
    const float *in = (const float*)inputBuffer;
    float *out = (float*)outputBuffer;

    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        float sample = (in == nullptr) ? 0.0f : in[i];
        sample = delayLine.process(sample);
        out[i * 2] = sample;
        out[i * 2 + 1] = sample;
    }
    return paContinue;
}

int main() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Pa_Initialize failed: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    int numDevices = Pa_GetDeviceCount();
    std::cout << "Available devices:\n";
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        const PaHostApiInfo* hostApi = Pa_GetHostApiInfo(info->hostApi);
        std::cout << i << ": " << info->name << " (" << hostApi->name << ")"
                  << "  in=" << info->maxInputChannels
                  << " out=" << info->maxOutputChannels << std::endl;
    }

    std::cout << "\nEnter input device index: ";
    int inputDevice;
    std::cin >> inputDevice;

    std::cout << "Enter output device index: ";
    int outputDevice;
    std::cin >> outputDevice;

    std::cout << "Enter desired ARTIFICIAL delay in ms (e.g. 500, 1000, 2000): ";
    double delayMs;
    std::cin >> delayMs;

    std::cout << "Opening ASIO control panel...\n";
    PaAsio_ShowControlPanel(inputDevice, nullptr);

    double sampleRate = 44100;
    delayLine.init(sampleRate, 3.0);  // supports up to 3 seconds of delay
    delayLine.setDelaySeconds(sampleRate, delayMs / 1000.0);

    PaStreamParameters inputParams, outputParams;
    inputParams.device = inputDevice;
    inputParams.channelCount = 1;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = 0;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    outputParams.device = outputDevice;
    outputParams.channelCount = 2;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = 0;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaStream *stream;
    err = Pa_OpenStream(&stream, &inputParams, &outputParams,
                         sampleRate, 64, paNoFlag,
                         passthroughCallback, nullptr);

    if (err != paNoError) {
        std::cerr << "Pa_OpenStream failed: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "Pa_StartStream failed: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    std::cout << "\nStreaming with ~" << delayMs << " ms artificial delay (plus small hardware latency)... press Enter to stop.\n";
    std::cin.ignore();
    std::cin.get();

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;
}