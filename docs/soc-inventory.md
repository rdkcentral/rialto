# SoC Feature Inventory

This document lists all SoC-related markers identified in the Rialto codebase as of April 2026.
It is grouped by category to support platform audits, onboarding, and future change planning.

Produced as part of change `create-soc-inventory-document`.  
Requirements defined in: `openspec/specs/soc-feature-inventory/spec.md`

---

## 1. Vendor-Specific Markers

Markers that name or reference a specific chip vendor.

| Keyword | File | Notes |
|---------|------|-------|
| `amlhalasink` | `media/server/gstplayer/source/GstGenericPlayer.cpp` | Amlogic audio sink; used in pipeline element selection |
| `amlhalasink` | `media/server/gstplayer/source/tasks/generic/SetupElement.cpp` | Element setup for Amlogic audio sink |
| `amlhalasink` | `media/server/gstplayer/source/tasks/generic/SetPlaybackRate.cpp` | Playback rate handling for Amlogic sink |
| `amlhalasink` | `media/server/gstplayer/source/GstCapabilities.cpp` | Capability check for Amlogic audio element |
| `amlhalasink` | `media/server/gstplayer/source/GstWebAudioPlayer.cpp` | Web audio player Amlogic path |

---

## 2. Generic SoC Capability Markers

Abstract capability checks not tied to a single vendor but representing SoC-level features.

| Keyword | File | Notes |
|---------|------|-------|
| `isSocAudioFadeSupported` | `wrappers/source/RdkGstreamerUtilsWrapper.cpp` | Queries whether the SoC supports audio fade |
| `isSocAudioFadeSupported` | `media/server/gstplayer/source/GstCapabilities.cpp` | Capability flag check |
| `isSocAudioFadeSupported` | `media/server/gstplayer/source/tasks/generic/SetVolume.cpp` | Conditional fade path in volume task |
| `doAudioEasingonSoc` | `wrappers/source/RdkGstreamerUtilsWrapper.cpp` | Triggers SoC-level audio easing |
| `doAudioEasingonSoc` | `media/server/gstplayer/source/tasks/generic/SetVolume.cpp` | Audio easing call in volume task |

---

## 3. Configuration-Level Markers

Environment variables and configuration files that control SoC-specific behaviour at deployment time.

| Keyword | File | Notes |
|---------|------|-------|
| `RIALTO_CONFIG_SOC_PATH` | `serverManager/service/CMakeLists.txt` | CMake variable defining the path to the SoC config file; defaults to `/etc/rialto-soc.json` |
| `rialto-soc.json` | `serverManager/service/source/ConfigHelper.cpp` | Loaded at runtime to apply SoC-specific server manager configuration |

---

## 4. Media Pipeline SoC Hooks

SoC-specific hooks embedded in the GStreamer media pipeline layer.

| Keyword | File | Notes |
|---------|------|-------|
| `rdk_gstreamer_utils_soc` | `media/server/gstplayer/include/GstGenericPlayer.h` | Comment reference to SoC utility library used by the generic player |

---

## Maintenance

When new SoC-specific markers are introduced:
1. Add an entry to the relevant section above.
2. Open a PR referencing the `soc-feature-inventory` spec.
3. Update the date in this document's header.
