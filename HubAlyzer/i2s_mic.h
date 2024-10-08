/*
 * Display A-weighted sound level measured by I2S Microphone
 *
 * (c)2019 Ivan Kostoski (original version)
 * (c)2021 Bim Overbohm (split into files, template)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "sos-iir-filter.h"

#define SERIAL_OUTPUT

// I2S microphone connnection
// SAMPLE_COUNT = Number of microphone samples to take and return in queue
// I2S pins - Can be routed to almost any (unused) ESP32 pin.
//            SD can be any pin, including input only pins (36-39).
//            SCK (i.e. BCLK) and WS (i.e. L/R CLK) must be output capable pins
// PIN_WS = I2S word select pin
// PIN_SCK = I2S clock pin
// PIN_SD = I2S data pin
// I2S_PORT = I2S port to use
// MIC_BITS = Number of valid bits in microphone data
// MSB_SHIFT = Set to true to fix MSB timing for some microphones, i.e. SPH0645LM4H-x
// SAMPLE_RATE_HZ = Microphone sample rate in Hz. must be 48kHz to fit filter design
template <unsigned SAMPLE_COUNT, int PIN_WS = 18, int PIN_SCK = 23, int PIN_SD = 19, i2s_port_t I2S_PORT = I2S_NUM_0, unsigned MIC_BITS = 24, bool MSB_SHIFT = false, unsigned SAMPLE_RATE_HZ = 48000>
class Microphone_I2S
{
  static constexpr unsigned TASK_PRIO = 4;     // FreeRTOS priority
  static constexpr unsigned TASK_STACK = 4096; // FreeRTOS stack size (in 32-bit words)

public:
  using SAMPLE_T = int32_t;
  using SampleBuffer = float[SAMPLE_COUNT];
  static const constexpr uint32_t SAMPLE_BITS = sizeof(SAMPLE_T) * 8;
  static const constexpr uint32_t BUFFER_SIZE = SAMPLE_COUNT * sizeof(SAMPLE_T);

  /// @brief Create new I2S microphone.
  /// @param filter Microphone IIR filter function to apply to samples
  Microphone_I2S(const SOS_IIR_Filter &filter)
      : m_filter(filter)
  {
  }

  void begin()
  {
    // Setup I2S to sample mono channel for SAMPLE_RATE_HZ * SAMPLE_BITS
    // NOTE: Recent update to Arduino_esp32 (1.0.2 -> 1.0.3)
    //       seems to have swapped ONLY_LEFT and ONLY_RIGHT channels
    const i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE_HZ,
        .bits_per_sample = i2s_bits_per_sample_t(SAMPLE_BITS),
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = 0, // ESP_INTR_FLAG_LEVEL1,                             // default interrupt priority
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0};
    if (auto i2sError = i2s_driver_install(I2S_PORT, &i2s_config, 0, nullptr); i2sError != ESP_OK)
    {
      Serial.println("Failed to install microphone I2S driver: ");
      Serial.println(i2sError);
    }
#ifdef SERIAL_OUTPUT
    else
    {
      Serial.print("Installed microphone I2S driver at ");
      if (I2S_PORT == I2S_NUM_0)
      {
        Serial.println("I2S0");
      }
      else if (I2S_PORT == I2S_NUM_1)
      {
        Serial.println("I2S1");
      }
      else
      {
        Serial.println("unknown port");
      }
    }
#endif

    // I2S pin mapping
    const i2s_pin_config_t pin_config = {
        .bck_io_num = PIN_SCK,
        .ws_io_num = PIN_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = PIN_SD};
    if (auto i2sError = i2s_set_pin(I2S_PORT, &pin_config); i2sError != ESP_OK)
    {
      Serial.print("Failed to set microphone I2S pin mapping: ");
      Serial.println(i2sError);
    }
#ifdef SERIAL_OUTPUT
    else
    {
      Serial.println("Installed microphone I2S pin mapping");
    }
#endif

    // FIXME: There is a known issue with esp-idf and sampling rates, see:
    //        https://github.com/espressif/esp-idf/issues/2634
    //        In the meantime, the below line seems to set sampling rate at ~47999.992Hz
    //        fifs_req=24576000, sdm0=149, sdm1=212, sdm2=5, odir=2 -> fifs_reached=24575996
    // NOTE:  This seems to be fixed in ESP32 Arduino 1.0.4, esp-idf 3.2
    //        Should be safe to remove...
    // #include <soc/rtc.h>
    // rtc_clk_apll_enable(1, 149, 212, 5, 2);
    //  Create FreeRTOS queue
    if (m_sampleQueue = xQueueCreate(2, BUFFER_SIZE); m_sampleQueue == nullptr)
    {
      Serial.println("Failed to create microphone sample queue");
    }
#ifdef SERIAL_OUTPUT
    else
    {
      Serial.println("Created microphone sample queue");
    }
#endif
    // Create the I2S reader FreeRTOS task
    // NOTE: Current version of ESP-IDF will pin the task
    //       automatically to the first core it happens to run on
    //       (due to using the hardware FPU instructions).
    //       For manual control see: xTaskCreatePinnedToCore
    TaskHandle_t xHandle = nullptr;
    if (xTaskCreatePinnedToCore(readerTask, "Microphone_I2S reader", TASK_STACK, this, TASK_PRIO, &xHandle, 0) != pdPASS || xHandle == nullptr)
    {
      Serial.println("Failed to create microphone I2S reader task");
    }
    else
    {
#ifdef SERIAL_OUTPUT
      Serial.println("Created microphone I2S reader task");
#endif
    }
  }

  /// @brief Get the queue that stores new sample buffers.
  QueueHandle_t sampleQueue() const
  {
    return m_sampleQueue;
  }

  /// @brief Start sampling from microphone.
  void startSampling()
  {
    m_isSampling = true;
  }

  /// @brief Stop sampling from microphone.
  void stopSampling()
  {
    m_isSampling = false;
  }

private:
  static void readerTask(void *parameter)
  {
#ifdef SERIAL_OUTPUT
    Serial.println("Mic reader task started");
#endif
    auto object = reinterpret_cast<Microphone_I2S *>(parameter);
    // Discard first blocks, microphone may have startup time (i.e. INMP441 up to 83ms)
    size_t bytes_read = 0;
    for (int i = 0; i < 5; i++)
    {
      if (auto i2sError = i2s_read(I2S_PORT, &object->m_sampleBuffer, BUFFER_SIZE, &bytes_read, portMAX_DELAY); i2sError != ESP_OK || bytes_read != BUFFER_SIZE)
      {
        Serial.print("Failed to read from I2S: ");
        Serial.println(i2sError);
      }
    }
    while (true)
    {
      if (object->m_isSampling)
      {
        // Block and wait for microphone values from I2S
        // Data is moved from DMA buffers to our m_sampleBuffer by the driver ISR
        // and when there is requested amount of data, task is unblocked
        //
        // Note: i2s_read does not care it is writing in float[] buffer, it will write
        //       integer values to the given address, as received from the hardware peripheral.
        i2s_read(I2S_PORT, &object->m_sampleBuffer, BUFFER_SIZE, &bytes_read, portMAX_DELAY);

        // Debug only. Ticks we spent filtering and summing block of I2S data
        // TickType_t start_tick = xTaskGetTickCount();

        // Convert (including shifting) integer microphone values to floats,
        // using the same buffer (assumed sample size is same as size of float),
        // to save a bit of memory
        auto int_samples = reinterpret_cast<const SAMPLE_T *>(&object->m_sampleBuffer);
        for (int i = 0; i < SAMPLE_COUNT; i++)
        {
          object->m_sampleBuffer[i] = int_samples[i] >> (SAMPLE_BITS - MIC_BITS);
        }

        // filter values and apply gain setting
        object->m_filter.applyFilters(object->m_sampleBuffer, object->m_sampleBuffer, SAMPLE_COUNT);
        object->m_filter.applyGain(object->m_sampleBuffer, object->m_sampleBuffer, SAMPLE_COUNT);

        // Debug only. Ticks we spent filtering and summing block of I2S data
        // auto proc_ticks = xTaskGetTickCount() - start_tick;

        // Send the sums to FreeRTOS queue where main task will pick them up
        // and further calculate decibel values (division, logarithms, etc...)
        xQueueSend(object->m_sampleQueue, &object->m_sampleBuffer, portMAX_DELAY);

        // Debug only. Print raw microphone sample values
        /*int vMin = 1000000;
                int vMax = -vMin;
                int vAvg = 0;
                int vNan = 0;
                for (unsigned int k = 0; k < bytes_read; k++)
                {
                    if (isnan(object->m_sampleBuffer[k]) || isinf(object->m_sampleBuffer[k]))
                    {
                      object->m_sampleBuffer[k] = 0;
                      vNan++;
                    }
                    if (object->m_sampleBuffer[k] < vMin)
                    {
                      vMin = object->m_sampleBuffer[k];
                    }
                    if (object->m_sampleBuffer[k] > vMax)
                    {
                      vMax = object->m_sampleBuffer[k];
                    }
                    vAvg += object->m_sampleBuffer[k];
                }
                vAvg /= bytes_read;
                Serial.print("Min: "); Serial.print(vMin, 3);
                Serial.print(", Max: "); Serial.print(vMax, 3);
                Serial.print(", Avg: "); Serial.print(vAvg, 3);
                Serial.print(", NAN or INF: "); Serial.println(vNan);*/
      }
    }
  }

  SOS_IIR_Filter m_filter;
  QueueHandle_t m_sampleQueue;
  SampleBuffer m_sampleBuffer __attribute__((aligned(4)));
  bool m_isSampling = false;
};
