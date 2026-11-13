# RDKAI-66 / RDKAI-67 PoC: tee + orig-vol + bed-tap at the playbin audio-sink

PROTOTYPE. Raw gst_* calls (no wrapper classes), no unit tests, env-var gated.
With no RDKAI_* variable set, behaviour is unchanged (one extra log line in loadInternal).
Tested on Rialto v0.25.0.

## What it does
GstGenericPlayer::initMsePipeline() sets playbin's `audio-sink` to a test bin:

    RDKAI_PASSTHROUGH=1 : ghost sink -> identity -> <platform sink>
    RDKAI_PASSTHROUGH=2 : ghost sink -> tee -+-> orig-vol -> <platform sink>
                                             +-> leaky queue -> audioconvert -> audioresample
                                                 -> capsfilter (S16LE mono 16k) -> appsink "bed-tap"

Rialto never sets `audio-sink`; playbin picks it. The inner sink keeps its default name
(amlhalasink0 / brcmaudiosink0) so SetupElement's name-prefix configuration still applies.

## Environment variables (set on the RialtoServer process)
| Variable | Meaning |
|---|---|
| RDKAI_PASSTHROUGH | 1 = identity bin, 2 = tee bin. Unset = off |
| RDKAI_SINK_NAME | Override sink element. Default: first of brcmaudiosink, amlhalasink in the registry |
| RDKAI_NO_NATIVE_AUDIO | 1 = do not set the playbin `native-audio` flag (Broadcom) |
| RDKAI_BIN_CONVERT | 1 = add audioconvert/audioresample/S16LE 48k stereo before the tee (fallback) |
| RDKAI_TAP_RATE | bed-tap rate, 8000-48000, default 16000 |
| RDKAI_TAP_DUMP | 1 = write first 30 s of tap PCM to /tmp/rdkai_bedtap.raw |
| RDKAI_TAP_SLOW_MS | Test hook: sleep in the appsink callback (slow consumer) |
| GST_DEBUG_DUMP_DOT_DIR | e.g. /tmp: pipeline .dot graphs are written on each state change |

## Platform notes
Amlogic: playbin picks avdec_aac, so S16LE PCM reaches the bin. No extra setup.

Broadcom: the default route is brcmaudfilter -> brcmaudiodecoder (hardware) -> `audio/x-raw, format=brcm`.
That carries no samples, so the tee bin cannot work. For PCM BOTH are needed:
1. Lower the hardware decoder rank (merge with any existing value, the variable can be set once):
   GST_PLUGIN_FEATURE_RANK=ac3parse:0,brcmaudiodecoder:0,brcmaudfilter:0
2. RDKAI_NO_NATIVE_AUDIO=1 (with native-audio on, playsink adds no audioconvert and avdec_aac's F32LE is refused: not-negotiated).
This replaces Broadcom's hardware audio route while enabled.

Example drop-in /etc/systemd/system/wpeframework.service.d/99-rdkai.conf (Broadcom):

    [Service]
    Environment="GST_PLUGIN_FEATURE_RANK=ac3parse:0,brcmaudiodecoder:0,brcmaudfilter:0"
    Environment="RDKAI_NO_NATIVE_AUDIO=1"
    Environment="RDKAI_PASSTHROUGH=2"
    Environment="GST_DEBUG_DUMP_DOT_DIR=/tmp"

Then: systemctl daemon-reload; reboot. To undo: delete the file, daemon-reload, reboot.

## Build
Recipe lib32-rialto. `devtool build` once skipped the compile, so force it and check the binary:

    bitbake lib32-rialto -c compile -f
    bitbake lib32-rialto -c install -f
    strings <D>/usr/bin/RialtoServer | grep -c RDKAI     # must be > 0

The recipe builds with -Werror.

## Playback test (nativeplayer + aamp-cli)
1. Install the base and nativeplayer .bolt packages (AppPackageManager.install worked on the middleware images), then
   org.rdk.AppManager.launchApp {"appId":"com.rdkcentral.nativeplayer"}
2. Put reference.mp4 where the container sees it as /data:
   Amlogic  /opt/persistent/storageManager/com.rdkcentral.nativeplayer/
   Broadcom /mnt/media/apps/dac_apps/storageManager/com.rdkcentral.nativeplayer/  (chmod 644)
3. Check the server has the variables:
   for p in $(pidof RialtoServer); do tr '\0' '\n' < /proc/$p/environ | grep -E "RDKAI|FEATURE_RANK|GST_DEBUG_DUMP"; done
4. Monitor: tail -n 0 -F /opt/logs/wpeframework.log | grep -E "RDKAI|failed to ping|Audio source removed|State change to NULL|pipeline terminated"
5. Play:
       export CID=$(DobbyTool list | grep nativeplayer | awk '{print $3}')
       crun --root /var/run/rdk/crun/ exec $CID /bin/sh
       aamp-cli
       file:///data/reference.mp4
       progress
   then pause, play, seek 60, stop, exit, exit.
Use ONE playback per server (see Known issues).

## Expected log lines
- RDKAI: audio sink element = brcmaudiosink (or amlhalasink)
- RDKAI: test audio-sink bin installed (mode 2)
- RDKAI: caps entering bin = audio/x-raw, rate=48000, format=S16LE, channels=2, layout=interleaved
  (if F32LE, set RDKAI_BIN_CONVERT=1)
- RDKAI-67: orig-vol set test -> 0.80, then restored -> 1.00
- RDKAI-67 bed-tap: first buffer caps=audio/x-raw, rate=16000, format=S16LE, channels=1
- RDKAI-67 bed-tap: buffers=N bytes=M orig-vol=1.00 every 100 buffers (about 47 buffers/s, 32000 bytes/s)
- On stop: Audio source removed, State change to NULL requested, bed-tap: closed, pipeline terminated

## Debugging
- Graphs: with GST_DEBUG_DUMP_DOT_DIR=/tmp, .dot files appear in /tmp (cleared on reboot; copy them).
  Render: dot -Tsvg <file>.dot -o out.svg. Check audio-sink=(GstBin) rdkai_tee_bin, avdec_aac, aconv,
  and no brcmaudiodecoder (Broadcom).
- Tap content: RDKAI_TAP_DUMP=1 writes raw S16LE mono 16 kHz; convert with Python's `wave` module to listen.
- Slow consumer: RDKAI_TAP_SLOW_MS=100 gives about 10 buffers/s, 1000 about 1 buffer/s. Main path must be
  unaffected (same PID, no `failed to ping`, clean stop).
- Server logs go to /opt/logs/wpeframework.log on these images, not the journal.
- Tune fails with `Rialto server playback failed` / not-negotiated: Broadcom needs BOTH the rank override and RDKAI_NO_NATIVE_AUDIO=1.
- No RDKAI strings in the binary: the build skipped the compile (see Build).
- BusyBox: grep has no --line-buffered; od has no -t/-j/-N (use dd bs=1 skip=N count=M | od -d).

## Results
| | Amlogic | Broadcom |
|---|---|---|
| Audio into the bin | S16LE 48k stereo | S16LE 48k stereo (rank override + native-audio off) |
| Tap caps | S16LE 16k mono | S16LE 16k mono |
| Tap rate, normal | about 47 buffers/s | about 47 buffers/s |
| Slow consumer 100 / 1000 ms | 823 / 77 buffers | 396 / 37 buffers |
| Main path in slow runs | same PID, no ping failure | same PID, no ping failure |
| A/V sync | in sync | in sync (fullstack image) |

## Known issues / not covered
- orig-vol was rewritten from outside at tune/seek on Amlogic (likely AAMP's volume call, not proven); none seen on Broadcom.
  Ducking should not rely on orig-vol alone.
- getSink(AUDIO) returns the bin, so SetVolume audio-fade, codec switch and playback-rate paths that look for the sink by name skip their platform code.
- amlhalasink has its own volume/mute/stream-volume.
- A second playback in one RialtoServer hung at stop on the Amlogic middleware image (also with the patch off). Not tested on Broadcom.
- Amlogic results come from an earlier hard-coded-sink version of this code; the unified sink auto-detect was run on Broadcom only.
- Broadcom's hardware audio route is not used while the rank override and RDKAI_NO_NATIVE_AUDIO are on.
- session_id: agreed to be an opaque per-playback identifier, not Rialto-specific.
