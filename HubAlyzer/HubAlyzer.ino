/*
   Based on SpectrumAnalyzerBasic by Paul Stoffregen included in the Teensy Audio Library
   Modified by Jason Coon for the SmartMatrix Library
   If you are having trouble compiling, see
   the troubleshooting instructions here:
   https://github.com/pixelmatix/SmartMatrix/#external-libraries

   Also based on the ESP32 I2S SLM project by Ivan Kostoski:
   https://github.com/ikostoski/esp32-i2s-slm

   Requires the following libraries:
   Arduino Fast Fourier Transform library: https://github.com/kosme/arduinoFFT
   FastLED v3.1 or higher: https://github.com/FastLED/FastLED/releases
   SmartMatrix v4 or higher: https://github.com/pixelmatix/SmartMatrix/releases
*/

#include "esp32-i2s-slm/filters.h"
#include "esp32-i2s-slm/i2s_mic.h"

// NOTE: Some microphones require at least a DC-Blocker filter
#define MIC_EQUALIZER INMP441 // See below for defined IIR filters or set to 'None' to disable
#define MIC_OFFSET_DB 3.0103F // Default offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration
// Customize these values from microphone datasheet
#define MIC_SENSITIVITY -26.0F // dBFS value expected at MIC_REF_DB (Sensitivity value from datasheet)
#define MIC_REF_DB 94.0F       // Value at which point sensitivity is specified in datasheet (dB)
#define MIC_OVERLOAD_DB 120.0F // dB - Acoustic overload point / Maximum Acoustic Input
#define MIC_NOISE_DB 33.0F     // dB - Noise floor
#define MIC_BITS 24            // valid number of bits in I2S data

// Calculate reference amplitude value at compile time
static constexpr float MIC_REF_AMPL = pow(10.0F, float(MIC_SENSITIVITY) / 20.0F) * ((1 << (MIC_BITS - 1)) - 1);

#define SAMPLE_RATE_HZ 48000 // Hz, fixed to design of IIR filters. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define SAMPLE_COUNT 512     // ~10ms sample time, must be power-of-two

auto mic = Microphone_I2S<SAMPLE_COUNT, 33, 32, 34, I2S_NUM_0, MIC_BITS, false, SAMPLE_RATE_HZ>(MIC_EQUALIZER);

// ------------------------------------------------------------------------------------------

#define ENABLE_OTA
#ifdef ENABLE_OTA
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#endif

//#define ENABLE_BLUETOOTH
#ifdef ENABLE_BLUETOOTH
#include "BluetoothSerial.h"
//#include "esp_gap_bt_api.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;
#endif

// Configure your WiFi and Bluetooth settings here:
#include "network_config.h"

// ------------------------------------------------------------------------------------------

#define GPIOPINOUT ESP32_FORUM_PINOUT
#include <FastLED.h>
#include <MatrixHardware_ESP32_V0.h>
#include <SmartMatrix.h>

#define COLOR_DEPTH 24                                       // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
const uint8_t kMatrixWidth = 32;                             // known working: 32, 64, 96, 128
const uint8_t kMatrixHeight = 32;                            // known working: 16, 32, 48, 64
const uint8_t kRefreshDepth = 24;                            // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 4;                            // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
const uint8_t kPanelType = SMARTMATRIX_HUB75_16ROW_MOD8SCAN; // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
const uint8_t kMatrixOptions = (SM_HUB75_OPTIONS_NONE);      // see http://docs.pixelmatix.com/SmartMatrix for options
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);

rgb24 toRGB24(const CRGB &c)
{
  return rgb24{c.r, c.g, c.b};
}

// ------------------------------------------------------------------------------------------

#define FFT_SPEED_OVER_PRECISION
#define FFT_SQRT_APPROXIMATION
#include "arduinoFFT.h" // Arduino FFT library
#include "approx.h"     // fast log10f approximation

// A way to split FFT bins logarithmicaly by frequencies
// Split FFT results into spectrum / frequency bins
// See: https://dsp.stackexchange.com/questions/49436/scale-fft-frequency-range-for-a-bars-graph
// N_fft = (F_sample * N_bars) / |(F_high - F_low)|
#define BIN_MIN_HZ 200
#define BIN_MAX_HZ 8000
#define NR_OF_BINS (SAMPLE_COUNT / 2)
#define FFT_POINTS (trunc((SAMPLE_RATE_HZ * NR_OF_BINS) / (BAR_MAX_HZ - BAR_MIN_HZ)))
#define FFT_SPACING_HZ (trunc(SAMPLE_RATE_HZ / FFT_POINTS))

// We use only the 40 lowest bins. Everything else is rather uninteresting. The lowest bin is crap / DC offset, so we don't use it.
// Then we split the remaining bins logarithmicaly, that is, the number of bins the bars use increases logarithmicaly.
#define BIN_LOG_START 0.0F // 10^0.0 = 1
#define BIN_LOG_END 1.557F // 10^1.60 ~ 40
#define BIN_LOG_RANGE (BIN_LOG_END - BIN_LOG_START)

struct BarInfo
{
  unsigned int start; // first FFT bin for bar. filled in setupBars()
  unsigned int end;   // last FFT bin for bar. filled in setupBars()
  float noiseLevel;   // system-dependent noise level for bar
};
static const unsigned int NR_OF_BARS = 32;
BarInfo bars[NR_OF_BARS];

float weighingFactors[SAMPLE_COUNT] = {0};
float real[SAMPLE_COUNT] = {0};
float imag[SAMPLE_COUNT] = {0};
auto fft = ArduinoFFT<float>(real, imag, SAMPLE_COUNT, SAMPLE_RATE_HZ, weighingFactors);
float levels[NR_OF_BARS] = {0};
float peaks[NR_OF_BARS] = {0};
static constexpr float PeakDecayPerFrame = 0.002f;

void setupFFTBins()
{
  // build bin info, spacing frequency bars even on the logarithmic x-axis
  unsigned int currentBin = 1;
  for (int i = 0; i < NR_OF_BARS; i++)
  {
    auto &bar = bars[i];
    auto barBinCount = trunc(pow(10, BIN_LOG_START + (BIN_LOG_RANGE * (i + 1)) / NR_OF_BARS) - pow(10, BIN_LOG_START + (BIN_LOG_RANGE * i) / NR_OF_BARS));
    bar.start = currentBin;
    bar.end = currentBin + barBinCount;
    bar.noiseLevel = 1.0F;
    currentBin = bar.end + 1;
    /*Serial.print(bar.start);
      Serial.print(", ");
      Serial.println(bar.end);*/
  }
}

#ifdef ENABLE_BLUETOOTH
void bluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  Serial.print("Bluetooth event: ");
  Serial.println(event);
  if (event == ESP_SPP_SRV_OPEN_EVT)
  {
    Serial.println("Client Connected");
  }
  if (event == ESP_SPP_CLOSE_EVT)
  {
    Serial.println("Client disconnected");
  }
}
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println("Running setup");
  // Generate host name for WiFi and Bluetooth
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();
  String hostName = String("Matrix-") + chipId;

#ifdef ENABLE_OTA
  // Set up WiFi
  Serial.println("Setting up WiFi");
  WiFi.hostname(hostName);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPassword);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("WiFi connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  // Set up OTA updates
  Serial.println("Setting up OTA updates");
  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                         type = "sketch";
                       else // U_SPIFFS
                         type = "filesystem";
                       // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                       Serial.println("Start updating " + type);
                     })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
                 Serial.printf("Error[%u]: ", error);
                 if (error == OTA_AUTH_ERROR)
                   Serial.println("Auth Failed");
                 else if (error == OTA_BEGIN_ERROR)
                   Serial.println("Begin Failed");
                 else if (error == OTA_CONNECT_ERROR)
                   Serial.println("Connect Failed");
                 else if (error == OTA_RECEIVE_ERROR)
                   Serial.println("Receive Failed");
                 else if (error == OTA_END_ERROR)
                   Serial.println("End Failed");
               });
  ArduinoOTA.begin();
  Serial.print("Hostname: ");
  Serial.println(hostName);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#else
  WiFi.mode(WIFI_OFF);
#endif

#ifdef ENABLE_BLUETOOTH
  //SerialBT.register_callback(bluetoothCallback);
  if (!SerialBT.begin(hostName))
  {
    Serial.println("An error occurred initializing Bluetooth!");
  }
  else
  {
    Serial.print("Bluetooth initialized. Device name is ");
    Serial.println(hostName);
  }
  // Set up Bluetooth classic for legacy pairing
  if (esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_FIXED, strlen(bluetoothPinCode), reinterpret_cast<uint8_t *>(bluetoothPinCode)) == ESP_OK)
  {
    Serial.print("Bluetooth pin code set to ");
    Serial.println(bluetoothPinCode);
  }
  else
  {
    Serial.println("Setting bluetooth pin code failed!");
  }
#endif

  // initialize LED matrix
  matrix.addLayer(&backgroundLayer);
  matrix.setBrightness(255);
  matrix.setRefreshRate(60);
  matrix.begin();

  // initialize FFT
  setupFFTBins();
  //Serial.print("Size of uint_fast16_t is "); Serial.println(sizeof(uint_fast16_t));
  //Serial.print("Size of uint_fast8_t is "); Serial.println(sizeof(uint_fast8_t));

  mic.begin();
  Serial.println("Starting sampling from mic");
  mic.startSampling();
}

void displayLine(int band, int y, rgb24 color)
{
  int xStart = band * (kMatrixWidth / NR_OF_BARS);
  for (int x = 0; x < kMatrixWidth / NR_OF_BARS; x++)
  {
    backgroundLayer.drawPixel(xStart + x, y, color);
  }
}

void displayBand(int band, float value, float peak)
{
  // color hue based on band
  rgb24 color = toRGB24(CRGB(CHSV((band * 255) / (NR_OF_BARS - 1), 255, 255)));
  int barHeight = (kMatrixHeight - 1) * value;
  barHeight = barHeight < 0 ? 0 : barHeight;
  barHeight = barHeight > (kMatrixHeight - 1) ? (kMatrixHeight - 1) : barHeight;
  for (int y = 0; y < barHeight; y++)
  {
    displayLine(band, y, color);
  }
  if (peak > 0.0)
  {
    rgb24 peakColor = toRGB24(CRGB(CHSV((band * 255) / (NR_OF_BARS - 1), 100, 200)));
    int peakY = (kMatrixHeight - 1) * peak;
    peakY = peakY < 0 ? 0 : peakY;
    peakY = peakY > (kMatrixHeight - 1) ? (kMatrixHeight - 1) : peakY;
    displayLine(band, peakY, peakColor);
  }
}

unsigned long frameTimes[4] = {0};
unsigned long frameCounter = 0;
unsigned long loopStart = 0;
unsigned long loopTime = 0;

/*float vMin;
  float vMax;
  double vAvg;
  long vNan;*/

void loop()
{
  Serial.println("Starting loop");
  while (xQueueReceive(mic.sampleQueue(), &real, portMAX_DELAY))
  {
    loopTime += micros() - loopStart;
    loopStart = micros();
    auto timeStart = loopStart;

    /*for (unsigned int k = 0; k < 10; k++)
          {
            Serial.print(real[k], 3); Serial.print(" ");
          }
          Serial.println();*/

    /*vMin = 1000000;
          vMax = -vMin;
          vAvg = 0;
          vNan = 0;
          for (unsigned int k = 0; k < SAMPLE_COUNT; k++)
          {
            if (isnan(real[k]) || isinf(real[k]))
            {
              real[k] = 0;
              vNan++;
            }
            if (real[k] < vMin)
            {
              vMin = real[k];
            }
            if (real[k] > vMax)
            {
              vMax = real[k];
            }
            vAvg += real[k];
          }
          vAvg /= SAMPLE_COUNT;*/

    memset(imag, 0, sizeof(imag));
    fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    fft.compute(FFTDirection::Forward);
    fft.complexToMagnitude();
    //fft.dcRemoval();

    frameTimes[0] += micros() - timeStart;
    timeStart = micros();

    // Calculate dB values relative to MIC_REF_AMPL and adjust for microphone reference
    // this should give values between ~[MIC_NOISE_DB, MIC_OVERLOAD_DB]
    for (unsigned int i = 0; i < SAMPLE_COUNT; i++)
    {
      real[i] = MIC_OFFSET_DB + MIC_REF_DB + 20.0F * log10f_fast(real[i] / MIC_REF_AMPL);
    }

    frameTimes[1] += micros() - timeStart;
    timeStart = micros();

    float tempLevels[NR_OF_BARS];
    for (int i = 0; i < NR_OF_BARS; i++)
    {
      auto const &bar = bars[i]; // the bar we've currently processing
      float barBinCount = 0;     // number of valid bins found
      tempLevels[i] = 0;         // accumulated levels
      for (unsigned int vi = bar.start; vi <= bar.end; vi++)
      {
        // accumulate values
        float value = real[vi];
        // normalize and clamp
        if (value >= 2.0F * MIC_NOISE_DB)
        {
          value = (value - 2.0F * MIC_NOISE_DB) * (1.0F / MIC_OVERLOAD_DB);
          value = value < 0 ? 0 : value;
          tempLevels[i] += value;
          barBinCount++;
        }
      }
      barBinCount = barBinCount == 0 ? 1 : barBinCount;
      tempLevels[i] = tempLevels[i] / barBinCount;
      /*if (tempLevels[i] < (MIC_NOISE_DB / MIC_OVERLOAD_DB)) {
            tempLevels[i] = 0;
            }*/
      levels[i] = 0.5F * levels[i] + 0.5F * tempLevels[i];
      peaks[i] = levels[i] > peaks[i] ? levels[i] : (peaks[i] > 0 ? peaks[i] - PeakDecayPerFrame : 0);
      //Serial.println(levels[i], 1);
    }

    frameTimes[2] += micros() - timeStart;
    timeStart = micros();

    backgroundLayer.fillScreen(toRGB24(CRGB(0, 0, 0)));
    for (int i = 0; i < NR_OF_BARS; i++)
    {
      displayBand(i, levels[i], peaks[i]);
    }
    backgroundLayer.swapBuffers();

    frameTimes[3] += micros() - timeStart;
    timeStart = micros();
    frameCounter++;

    /*if (frameCounter >= 20)
        {
          frameCounter *= 1000;
          Serial.print("FFT: ");
          Serial.print((float)frameTimes[0] / float(frameCounter), 2); Serial.print(" ms, dB calculation: ");
          Serial.print((float)frameTimes[1] / float(frameCounter), 2); Serial.print(" ms, Levels: ");
          Serial.print((float)frameTimes[2] / float(frameCounter), 2); Serial.print(" ms, Display: ");
          Serial.print((float)frameTimes[3] / float(frameCounter), 2); Serial.print(" ms, Loop called every: ");
          Serial.print((float)loopTime / float(frameCounter), 2); Serial.println(" ms");
          frameCounter = 0;
          frameTimes[0] = 0;
          frameTimes[1] = 0;
          frameTimes[2] = 0;
          frameTimes[3] = 0;
          loopTime = 0;
          //Serial.print("Min: "); Serial.print(vMin, 3);
          //Serial.print(", Max: "); Serial.print(vMax, 3);
          //Serial.print(", Avg: "); Serial.print(vAvg, 3);
          //Serial.print(", NAN or INF: "); Serial.println(vNan);
        }*/
#ifdef ENABLE_OTA
    EVERY_N_MILLISECONDS(1000)
    {
      ArduinoOTA.handle();
    }
#endif
  }
}
