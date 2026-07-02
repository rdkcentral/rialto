/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIREBOLT_RIALTO_SERVER_GST_PLAYER_CONFIG_H_
#define FIREBOLT_RIALTO_SERVER_GST_PLAYER_CONFIG_H_

/**
 * @file GstPlayerConfig.h
 *
 * Configuration constants for GStreamer player buffer management.
 *
 * These constants control the appsrc queue limits for different media types.
 * They support both byte-based limits (GStreamer < 1.20) and buffer-based
 * limits (GStreamer >= 1.20).
 *
 * IMPORTANT: The buffer counts are based on estimated average buffer sizes.
 * These estimates may need adjustment based on your actual content:
 *   - Video buffers vary by codec, bitrate, GOP structure, resolution
 *   - Audio buffers vary by codec, bitrate, sample rate, chunk duration
 *   - Subtitle buffers vary by format complexity and styling
 *
 * To validate and tune these values:
 *   1. Build with -DRIALTO_ENABLE_BUFFER_SIZE_LOGGING
 *   2. Run typical playback scenarios
 *   3. Analyze actual buffer sizes in logs
 *   4. Adjust these constants if needed
 */

#include <stdint.h>

namespace firebolt::rialto::server
{
/**
 * @brief Video appsrc max queue size in bytes (for GStreamer < 1.20)
 */
constexpr uint32_t kVideoMaxBytes = 8 * 1024 * 1024; // 8 MB

/**
 * @brief Audio appsrc max queue size in bytes (for GStreamer < 1.20)
 */
constexpr uint32_t kAudioMaxBytes = 512 * 1024; // 512 KB

/**
 * @brief Subtitle appsrc max queue size in bytes (for GStreamer < 1.20)
 */
constexpr uint32_t kSubtitleMaxBytes = 256 * 1024; // 256 KB

/**
 * @brief WebAudio appsrc max queue size in bytes (for GStreamer < 1.20)
 */
constexpr uint32_t kWebAudioMaxBytes = 10 * 1024; // 10 KB

/**
 * @brief Video appsrc max queue size in buffer count (for GStreamer >= 1.20)
 *
 * Based on: 8MB ÷ ~200KB avg video buffer = 40 buffers
 * Estimated avg buffer size: ~200 KB (HD video with mixed I/P/B frames)
 *
 * Adjust based on your content characteristics:
 *   - Higher for low bitrate content (smaller buffers)
 *   - Lower for high bitrate 4K content (larger buffers)
 */
constexpr uint32_t kVideoMaxBuffers = 40;

/**
 * @brief Audio appsrc max queue size in buffer count (for GStreamer >= 1.20)
 *
 * Based on: 512KB ÷ ~16KB avg audio buffer = 32 buffers
 * Estimated avg buffer size: ~16 KB (compressed audio at 128-256 kbps)
 *
 * Adjust based on your audio format:
 *   - Higher for lower bitrate audio (smaller buffers)
 *   - Lower for high bitrate or lossless audio (larger buffers)
 */
constexpr uint32_t kAudioMaxBuffers = 32;

/**
 * @brief Subtitle appsrc max queue size in buffer count (for GStreamer >= 1.20)
 *
 * Based on: 256KB ÷ ~4KB avg subtitle buffer = 64 buffers
 * Estimated avg buffer size: ~4 KB (text subtitles with minimal styling)
 *
 * Adjust based on subtitle format:
 *   - Higher for simple text subtitles (smaller buffers)
 *   - Lower for complex styled or image-based subtitles (larger buffers)
 */
constexpr uint32_t kSubtitleMaxBuffers = 64;

/**
 * @brief WebAudio appsrc max queue size in buffer count (for GStreamer >= 1.20)
 *
 * Based on: 10KB ÷ ~512 bytes avg buffer = 20 buffers
 * Estimated avg buffer size: ~512 bytes (small chunks for low latency)
 *
 * Adjust based on latency requirements:
 *   - Higher for lower latency (smaller chunk sizes)
 *   - Lower if larger chunks acceptable
 */
constexpr uint32_t kWebAudioMaxBuffers = 20;

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_PLAYER_CONFIG_H_
