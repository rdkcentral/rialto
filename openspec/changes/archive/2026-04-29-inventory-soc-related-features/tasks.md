## 1. Prepare Keyword Set

- [ ] 1.1 Define the complete list of vendor-specific keywords to search for: amlogic, amlhalasink, rtk, realtek, bcm, broadcom, qcom, qualcomm, mtk, mediatek, hisi, hisilicon, meson
- [ ] 1.2 Define the complete list of generic SoC capability keywords: isSocAudioFadeSupported, doAudioEasingonSoc, onSoc, SoC, soc
- [ ] 1.3 Define the complete list of configuration-level keywords: RIALTO_CONFIG_SOC_PATH, rialto-soc.json
- [ ] 1.4 Define the complete list of media pipeline keywords: amlhalasink, rdk_gstreamer_utils_soc, audio-fade

## 2. Audit Production Code

- [ ] 2.1 Search media/server/gstplayer for all defined SoC keywords and record file paths and code context
- [ ] 2.2 Search wrappers for all defined SoC keywords and record file paths and code context
- [ ] 2.3 Search serverManager for all defined SoC keywords and record file paths and code context
- [ ] 2.4 Search media/client for all defined SoC keywords and record file paths and code context
- [ ] 2.5 Search proto and common for any SoC-related markers and record file paths and code context

## 3. Categorize Findings

- [ ] 3.1 Assign each discovered marker to a category: vendor-specific, generic SoC capability, configuration-level, or media pipeline
- [ ] 3.2 Verify no runtime behavior changes are introduced as part of this categorization step
- [ ] 3.3 Note any markers found that belong to SoC vendors not yet in the keyword set and add them

## 4. Record Inventory

- [ ] 4.1 Document vendor-specific markers with file locations and code context
- [ ] 4.2 Document generic SoC capability markers with file locations and code context
- [ ] 4.3 Document configuration-level markers with file locations and code context
- [ ] 4.4 Document media pipeline markers with file locations and code context

## 5. Validate and Archive

- [ ] 5.1 Run openspec validate to confirm all artifacts are complete and well-formed
- [ ] 5.2 Review the full inventory for completeness against the specs requirements
- [ ] 5.3 Run openspec archive "inventory-soc-related-features" to promote specs to the main spec set
