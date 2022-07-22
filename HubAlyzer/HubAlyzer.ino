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
#include "approx.h" // fast log10f and sincosf approximation
#include <cmath>

static constexpr unsigned SAMPLE_RATE_HZ = 48000; // Hz, fixed to design of IIR filters. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
static constexpr unsigned SAMPLE_COUNT = 512;     // ~10ms sample time, must be power-of-two
float samples[SAMPLE_COUNT];                      // Raw microphone sample storage

// NOTE: Some microphones require at least a DC-Blocker filter
#define MIC_EQUALIZER INMP441                   // See below for defined IIR filters or set to 'None' to disable
static constexpr float MIC_OFFSET_DB = 3.0103f; // Default offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration
// Customize these values from microphone datasheet
static constexpr int MIC_SENSITIVITY = -26.0f; // dBFS value expected at MIC_REF_DB (Sensitivity value from datasheet)
static constexpr int MIC_REF_DB = 94.0f;       // Value at which point sensitivity is specified in datasheet (dB)
static constexpr int MIC_OVERLOAD_DB = 120.0f; // dB - Acoustic overload point / Maximum Acoustic Input
static constexpr int MIC_NOISE_DB = 33.0f;     // dB - Noise floor
static constexpr unsigned MIC_BITS = 24;       // valid number of bits in I2S data

constexpr float MIC_REF_AMPL = powf(10.0f, float(MIC_SENSITIVITY) / 20.0f) * ((1 << (MIC_BITS - 1)) - 1); // Microphone reference amplitude value

// Convert microphone amplitude to dB values
float MicAmplitudeToDb(float v)
{
  return MIC_OFFSET_DB + MIC_REF_DB + 20.0f * log10f_fast(v / MIC_REF_AMPL);
}

auto mic = Microphone_I2S<SAMPLE_COUNT, 33, 32, 34, I2S_NUM_0, MIC_BITS, false, SAMPLE_RATE_HZ>(MIC_EQUALIZER);

// ------------------------------------------------------------------------------------------

#include "fft.h"
#include "spectrum.h"

static constexpr unsigned NR_OF_BANDS = 32;

auto fft = FFT<SAMPLE_COUNT, SAMPLE_RATE_HZ>(&samples);
auto spectrum = Spectrum<SAMPLE_COUNT, MIC_NOISE_DB, MIC_OVERLOAD_DB, NR_OF_BANDS>(&samples, MicAmplitudeToDb);

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

#include <FastLED.h>
#define GPIOPINOUT ESP32_FORUM_PINOUT
#include <MatrixHardware_ESP32_V0.h>
#include <SmartMatrix.h>

#define COLOR_DEPTH 24                                                   // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
static constexpr unsigned kMatrixWidth = 32;                             // known working: 32, 64, 96, 128
static constexpr unsigned kMatrixHeight = 32;                            // known working: 16, 32, 48, 64
static constexpr unsigned kRefreshDepth = 24;                            // known working: 24, 36, 48
static constexpr unsigned kDmaBufferRows = 4;                            // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
static constexpr unsigned kPanelType = SMARTMATRIX_HUB75_16ROW_MOD8SCAN; // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
static constexpr unsigned kMatrixOptions = (SM_HUB75_OPTIONS_NONE);      // see http://docs.pixelmatix.com/SmartMatrix for options
static constexpr unsigned kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);

// ------------------------------------------------------------------------------------------

#include "draw.h"

auto draw = Draw<kMatrixWidth, kMatrixHeight, kBackgroundLayerOptions>(backgroundLayer);

// ------------------------------------------------------------------------------------------

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
                       Serial.println("Start updating " + type); })
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
                   Serial.println("End Failed"); });
  ArduinoOTA.begin();
  Serial.print("Hostname: ");
  Serial.println(hostName);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#else
  WiFi.mode(WIFI_OFF);
#endif

#ifdef ENABLE_BLUETOOTH
  // SerialBT.register_callback(bluetoothCallback);
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
  // initialize microphone
  mic.begin();
  Serial.println("Starting sampling from mic");
  mic.startSampling();
}

void loop()
{
  Serial.println("Starting loop");
  // Get samples from other ESP32 core that receives the I2S audio data
  while (xQueueReceive(mic.sampleQueue(), &samples, portMAX_DELAY))
  {
    // apply A-Weighting filter for perceptive loudness. See: https://www.noisemeters.com/help/faq/frequency-weighting/
    A_weighting.applyFilters(samples, samples, SAMPLE_COUNT);
    A_weighting.applyGain(samples, samples, SAMPLE_COUNT);
    // apply FFT to samples
    fft.update();
    spectrum.update();
    // draw.spectrumRays<NR_OF_BANDS>(spectrum.levels(), spectrum.peaks(), true);
    draw.spectrumCentered<NR_OF_BANDS>(spectrum.levels(), spectrum.peaks());
    // Enable over-the-air updates
#ifdef ENABLE_OTA
    EVERY_N_MILLISECONDS(1000)
    {
      ArduinoOTA.handle();
    }
#endif
  }
}
