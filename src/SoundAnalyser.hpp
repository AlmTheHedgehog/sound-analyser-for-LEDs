#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <cmath>
#include <cfloat>
#include <thread>

#include <fftw3.h>
#include "portaudio.h"
#include "BluetoothCommunicator.hpp"


#define SAMPLE_RATE  (48000) // 44100 was - 48000 in system
#define FRAMES_PER_BUFFER (1024) //512 was (93.75HZ analizing) - 4096 good - 1024 in system propably // better 2048 || 1024
#define NUM_CHANNELS    (1)
#define SUGGESTED_LATTENCY (0.01) // 0.01 seconds LATTENCY dellay (according to 100Hz refreshing)
#define FREQ_MAGNITUDES_BUFFER_SIZE ((FRAMES_PER_BUFFER / 2) + 1)
#define SILENT_DBFS (-120)
/* #define DITHER_FLAG     (paDitherOff) */
#define DITHER_FLAG     (0) 
#define DEVICE_NOT_FOUND_ERROR (404)

/* Sample format. */
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"

/*Sound analize*/
#define LOW_MIDDLE_SEPARATION_FREQUENCY (500)  // 250HZ - only basses :) 500 - normal
#define MIDDLE_HIGH_SEPARATION_FREQUENCY (2500)
// #define MIN_LOW_DBFS_FOR_ZERO_BRIGHTNESS -350
// #define MIN_MID_DBFS_FOR_ZERO_BRIGHTNESS -2500
// #define MIN_HIGH_DBFS_FOR_ZERO_BRIGHTNESS -35000

#define MIN_LOW_DBFS_FOR_ZERO_BRIGHTNESS -600
#define MIN_MID_DBFS_FOR_ZERO_BRIGHTNESS -3000
#define MIN_HIGH_DBFS_FOR_ZERO_BRIGHTNESS -40000
// #define MIN_LOW_DBFS_FOR_ZERO_BRIGHTNESS -500
// #define MIN_MID_DBFS_FOR_ZERO_BRIGHTNESS -4000
// #define MIN_HIGH_DBFS_FOR_ZERO_BRIGHTNESS -80000


/*
Should not be more than 1 instance
*/
class SoundAnalyser{
    public:
        double*                 fqDBFS_linearSpectrum;
        BluetoothCommunicator   bluetoothCommunicator;

        SoundAnalyser(std::string bluetoothMACaddress);
        SoundAnalyser(const SoundAnalyser &parent) = delete;
        void operator=(const SoundAnalyser &parent) = delete;
        ~SoundAnalyser();

        void reset_dbfsRefference();

    protected:
        class SamplesBuffer{
            public:
                SAMPLE *arrPtr;
                size_t buffSize;
                SamplesBuffer(const SamplesBuffer & parent);
                SamplesBuffer(const SAMPLE *samplesBuffer, size_t buffSize);
                ~SamplesBuffer();
        };

    private:
        PaStreamParameters  inputParameters;
        PaStream*           stream;
        PaError             err;
        SoundAnalyser*      analyzerPtr;
        fftw_complex        *inAnaliseBuffer, *outAnaliseBuffer;
        fftw_plan           ftransPlan;
        double              dbfsRefference;

        struct FilteredValues{
            size_t frequencesCounter;
            double DBFS;
            double maxDBFS = 0;
            uint8_t ledBrightness;
        };
        struct Filters{
            struct FilteredValues LowFilter;
            struct FilteredValues MidFilter;
            struct FilteredValues HighFilter;
        }filtersValues;
        struct ArrWithSize{
            uint8_t* array; // 1byte - op numb + 2bytes size of arr + 3*size bytes ledarray
            uint16_t size;
        }preparedLedArray;

        void checkIfErrorOccured();
        void analizeSamples(SamplesBuffer samplesBuffer);  //TODO have to be a thread which will have while(1) loop?, at the end of the loop thread will be blocked. recordCallback will unblockthread and update information(samplesBuffer)in it
        void convertRelativeMagsToBrightnessInFilters();
        void prepareSoundLedArray();
        double calcMagnitude(const fftw_complex& realSample);
        void resetFiltersValues();

        /* This routine will be called by the PortAudio engine when audio is needed.
        ** It may be called at interrupt level on some machines so don't do anything
        ** that could mess up the system like calling malloc() or free().
        */
        static int recordCallback( const void *inputBuffer, void *outputBuffer,
                                    unsigned long framesPerBuffer,
                                    const PaStreamCallbackTimeInfo* timeInfo,
                                    PaStreamCallbackFlags statusFlags,
                                    void *userData );
};

class PortAudioException{
    public:
        int errorCode;
        PortAudioException(int errorCode):errorCode(errorCode){};
};



