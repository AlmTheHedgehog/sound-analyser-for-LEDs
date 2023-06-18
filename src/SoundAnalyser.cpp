#include "SoundAnalyser.hpp"

SoundAnalyser::SoundAnalyser(std::string bluetoothMACaddress):
                            bluetoothCommunicator(bluetoothMACaddress), dbfsRefference(DBL_EPSILON){
    inAnaliseBuffer = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FRAMES_PER_BUFFER);
    outAnaliseBuffer = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FRAMES_PER_BUFFER);
    ftransPlan = fftw_plan_dft_1d(FRAMES_PER_BUFFER, inAnaliseBuffer, outAnaliseBuffer, FFTW_FORWARD, FFTW_MEASURE); // probably fftw_backward
    fqDBFS_linearSpectrum = new double[FREQ_MAGNITUDES_BUFFER_SIZE];
    preparedLedArray.size = NUMBER_OF_LEDS*NUMBER_OF_COLORS + SIZE_OF_ARRAY_LENGTH + SIZE_OF_COMCODE;
    preparedLedArray.array = new uint8_t[preparedLedArray.size];

    err = paNoError;
    err = Pa_Initialize();
    checkIfErrorOccured();
    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    if (inputParameters.device == paNoDevice) {
        std::cerr << "Error: No default input device." << std::endl;
        throw PortAudioException(DEVICE_NOT_FOUND_ERROR);
    }

    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = SUGGESTED_LATTENCY;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    analyzerPtr = this;

    /* Record some audio. -------------------------------------------- */
    err = Pa_OpenStream(&stream,
                        &inputParameters,
                        NULL,                  /* &outputParameters, */
                        SAMPLE_RATE,
                        FRAMES_PER_BUFFER,
                        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                        recordCallback,
                        analyzerPtr);
    checkIfErrorOccured();

    err = Pa_StartStream( stream );
    checkIfErrorOccured();

    std::cout << std::endl << "=== Now recording!! Please speak into the microphone. ===" << std::endl;
}

SoundAnalyser::~SoundAnalyser(){
    err = Pa_CloseStream(stream);
    Pa_Terminate();
    if( err != paNoError ){
        std::cerr << "An error occurred while using the portaudio stream" << std::endl;
        std::cerr << "Error number: " << err << std::endl;
        std::cerr << "Error message: " << Pa_GetErrorText(err) << std::endl;
    }

    fftw_destroy_plan(ftransPlan);
    fftw_free(inAnaliseBuffer); fftw_free(outAnaliseBuffer);
    delete[] fqDBFS_linearSpectrum;
    delete[] preparedLedArray.array;
}

void SoundAnalyser::reset_dbfsRefference(){
    dbfsRefference = DBL_EPSILON;
}

void SoundAnalyser::analizeSamples(SamplesBuffer samplesBuffer){
    for(int i = 0; i < FRAMES_PER_BUFFER; i++){
        inAnaliseBuffer[i][0] = static_cast<double>(samplesBuffer.arrPtr[i]);
        inAnaliseBuffer[i][1] = 0; //second chanel
    }

    fftw_execute(ftransPlan);

    double sampleMagnitude = 0, relativeMagnitude, frequence;
    resetFiltersValues();    
    for(int i = 0; i < FREQ_MAGNITUDES_BUFFER_SIZE; i++){
        sampleMagnitude = calcMagnitude(outAnaliseBuffer[i]);
        if(sampleMagnitude > dbfsRefference){
            dbfsRefference = sampleMagnitude;
        }
        relativeMagnitude = sampleMagnitude / dbfsRefference;

        if(relativeMagnitude != 0){
            fqDBFS_linearSpectrum[i] = 20 * std::log10(relativeMagnitude);
        }else{
            fqDBFS_linearSpectrum[i] = SILENT_DBFS;
        }

        frequence = i * ((SAMPLE_RATE/2)/FREQ_MAGNITUDES_BUFFER_SIZE);

        if(frequence <= LOW_MIDDLE_SEPARATION_FREQUENCY){
            filtersValues.LowFilter.DBFS += fqDBFS_linearSpectrum[i];
            filtersValues.LowFilter.frequencesCounter++;
            if(fqDBFS_linearSpectrum[i] > filtersValues.LowFilter.maxDBFS){
                filtersValues.LowFilter.maxDBFS = fqDBFS_linearSpectrum[i];
            }
        }else if(frequence <= MIDDLE_HIGH_SEPARATION_FREQUENCY){
            filtersValues.MidFilter.DBFS += fqDBFS_linearSpectrum[i];
            filtersValues.MidFilter.frequencesCounter++;
            if(fqDBFS_linearSpectrum[i] > filtersValues.MidFilter.maxDBFS){
                filtersValues.MidFilter.maxDBFS = fqDBFS_linearSpectrum[i];
            }
        }else{
            filtersValues.HighFilter.DBFS += fqDBFS_linearSpectrum[i];
            filtersValues.HighFilter.frequencesCounter++;
            if(fqDBFS_linearSpectrum[i] > filtersValues.HighFilter.maxDBFS){
                filtersValues.HighFilter.maxDBFS = fqDBFS_linearSpectrum[i];
            }
        }
    }
    printf("low = %f, mid = %f, high = %f\n", filtersValues.LowFilter.DBFS, 
                                        filtersValues.MidFilter.DBFS, 
                                        filtersValues.HighFilter.DBFS);
    convertRelativeMagsToBrightnessInFilters();
    prepareSoundLedArray();
    bluetoothCommunicator.sendData(preparedLedArray.array, preparedLedArray.size);  // TODO make a prepare data function. it will prepare the array with leds colors + command code+ length. Then send it with btComm.sendData()

    //Sum all magnitudes for low, mid, high frequ. then devide it by number of them(avarage)



    // double max_mag_db = -DBL_MAX, max_mag_db_index = 0, sampleMagnitude = 0, relativeMagnitude;
    // for(int i = 0; i < FREQ_MAGNITUDES_BUFFER_SIZE; i++){
    //     sampleMagnitude = calcMagnitude(outAnaliseBuffer[i]);
    //     if(sampleMagnitude > dbfsRefference){
    //         dbfsRefference = sampleMagnitude;
    //     }
    //     relativeMagnitude = sampleMagnitude / dbfsRefference;

    //     if(relativeMagnitude != 0){
    //         fqDBFS_linearSpectrum[i] = 20 * std::log10(relativeMagnitude);
    //     }else{
    //         fqDBFS_linearSpectrum[i] = SILENT_DBFS;
    //     }

    //     if(max_mag_db < fqDBFS_linearSpectrum[i]){
    //         max_mag_db = fqDBFS_linearSpectrum[i];
    //         max_mag_db_index = i;
    //     }
    // }
    // printf("Max_mag_db = %f, at freq = %5.2f,   div = %f         ref = %f\n", 
    //         max_mag_db, max_mag_db_index * ((SAMPLE_RATE/2)/FREQ_MAGNITUDES_BUFFER_SIZE), std::log10(sampleMagnitude / dbfsRefference), dbfsRefference);
}

void SoundAnalyser::convertRelativeMagsToBrightnessInFilters(){
    filtersValues.LowFilter.ledBrightness = (uint8_t)std::round(((filtersValues.LowFilter.DBFS - MIN_LOW_DBFS_FOR_ZERO_BRIGHTNESS)
                                                        /(filtersValues.LowFilter.maxDBFS - MIN_LOW_DBFS_FOR_ZERO_BRIGHTNESS)) * MAX_BRIGHTNESS);
    filtersValues.MidFilter.ledBrightness = (uint8_t)std::round(((filtersValues.MidFilter.DBFS - MIN_MID_DBFS_FOR_ZERO_BRIGHTNESS)
                                                        /(filtersValues.MidFilter.maxDBFS - MIN_MID_DBFS_FOR_ZERO_BRIGHTNESS)) * MAX_BRIGHTNESS);
    filtersValues.HighFilter.ledBrightness = (uint8_t)std::round(((filtersValues.HighFilter.DBFS - MIN_HIGH_DBFS_FOR_ZERO_BRIGHTNESS)
                                                        /(filtersValues.HighFilter.maxDBFS - MIN_HIGH_DBFS_FOR_ZERO_BRIGHTNESS)) * MAX_BRIGHTNESS);
    printf("Colors: %d - %d - %d\n", filtersValues.LowFilter.ledBrightness, filtersValues.MidFilter.ledBrightness, filtersValues.HighFilter.ledBrightness);
}

void SoundAnalyser::prepareSoundLedArray(){
    preparedLedArray.array[0] = COMCODE_SOUND_LEDS;
    // preparedLedArray.array[SIZE_OF_COMCODE] = preparedLedArray.size;
    preparedLedArray.array[1] = 0;
    preparedLedArray.array[2] = 100;
    for(int i = 0; i < NUMBER_OF_LEDS * NUMBER_OF_COLORS; i += 5*NUMBER_OF_COLORS){
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i] = filtersValues.LowFilter.ledBrightness; //R
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i+1] = 0; //G
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i+2] = 0; //B
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (1*NUMBER_OF_COLORS)] = 0;//R
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (1*NUMBER_OF_COLORS)+1] = filtersValues.MidFilter.ledBrightness;//G
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (1*NUMBER_OF_COLORS)+2] = 0;//B
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (2*NUMBER_OF_COLORS)] = 0;//R
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (2*NUMBER_OF_COLORS)+1] = 0;//G
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (2*NUMBER_OF_COLORS)+2] = filtersValues.HighFilter.ledBrightness;//B
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (3*NUMBER_OF_COLORS)] = 0;//R
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (3*NUMBER_OF_COLORS)+1] = filtersValues.MidFilter.ledBrightness;//G
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (3*NUMBER_OF_COLORS)+2] = 0;//B
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (4*NUMBER_OF_COLORS)] = filtersValues.LowFilter.ledBrightness;//R
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (4*NUMBER_OF_COLORS)+1] = 0;//G
        preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (4*NUMBER_OF_COLORS)+2] = 0;//B
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i] = 255; //R
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i+1] = 0; //G
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i+2] = 0; //B
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (1*NUMBER_OF_COLORS)] = 0;//R
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (1*NUMBER_OF_COLORS)+1] = 255;//G
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (1*NUMBER_OF_COLORS)+2] = 0;//B
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (2*NUMBER_OF_COLORS)] = 0;//R
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (2*NUMBER_OF_COLORS)+1] = 0;//G
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (2*NUMBER_OF_COLORS)+2] = 255;//B
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (3*NUMBER_OF_COLORS)] = 0;//R
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (3*NUMBER_OF_COLORS)+1] = 255;//G
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (3*NUMBER_OF_COLORS)+2] = 0;//B
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (4*NUMBER_OF_COLORS)] = 255;//R
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (4*NUMBER_OF_COLORS)+1] = 0;//G
        // preparedLedArray.array[SIZE_OF_COMCODE + SIZE_OF_ARRAY_LENGTH + i + (4*NUMBER_OF_COLORS)+2] = 0;//B
    }
}

double SoundAnalyser::calcMagnitude(const fftw_complex &realSample){
    return std::sqrt(realSample[0]*realSample[0] + realSample[1]*realSample[1]);
}

void SoundAnalyser::resetFiltersValues(){
    filtersValues.LowFilter.frequencesCounter = 0;
    filtersValues.MidFilter.frequencesCounter = 0;
    filtersValues.HighFilter.frequencesCounter = 0;
    filtersValues.LowFilter.DBFS = 0;
    filtersValues.MidFilter.DBFS = 0;
    filtersValues.HighFilter.DBFS = 0;
    filtersValues.LowFilter.ledBrightness = 0;
    filtersValues.MidFilter.ledBrightness = 0;
    filtersValues.HighFilter.ledBrightness = 0;
}

void SoundAnalyser::checkIfErrorOccured(){
    if(err != paNoError){
        throw PortAudioException(err);
    }
}

int SoundAnalyser::recordCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData ){
    const SAMPLE *samplesBuffer = (const SAMPLE*)inputBuffer;
    SoundAnalyser* analizerPtr = (SoundAnalyser*) userData;
    int finished;

    (void) outputBuffer; /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;

    std::thread analyzingThread(&SoundAnalyser::analizeSamples, analizerPtr, SamplesBuffer(samplesBuffer, FRAMES_PER_BUFFER));
    analyzingThread.detach();
    // analizerPtr->analizeSamples(SamplesBuffer(samplesBuffer, FRAMES_PER_BUFFER));

    //finished = paComplete;  // to stop reccording
    finished = paContinue;  // to continue reccording
    return finished;
}

SoundAnalyser::SamplesBuffer::SamplesBuffer(const SamplesBuffer & parent) : buffSize(parent.buffSize){
    arrPtr = new SAMPLE[buffSize]{0};
    for(size_t i = 0; i < buffSize; i++){
        arrPtr[i] = parent.arrPtr[i];
    }
}

SoundAnalyser::SamplesBuffer::SamplesBuffer(const SAMPLE *samplesBuffer, size_t buffSize) : buffSize(buffSize){
    arrPtr = new SAMPLE[buffSize]{0};
    for(size_t i = 0; i < buffSize; i++){
        arrPtr[i] = samplesBuffer[i];
    }
}

SoundAnalyser::SamplesBuffer::~SamplesBuffer(){
    delete[] arrPtr;
}
