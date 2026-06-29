# Audio Player

## Goal

Learn audio sources, format conversion, resampling, and sinks.

## Concepts

- audiotestsrc / filesrc
- decodebin
- audioconvert
- audioresample
- autoaudiosink
- Bus messages
- EOS handling
- Dynamic pad linking (audio)

## Example Flow

### Synthetic source
```
audiotestsrc
     ↓
audioconvert
     ↓
audioresample
     ↓
autoaudiosink
```

### File source (dynamic)
```
OGG/MP3/FLAC File
      ↓
  decodebin  ──(pad-added)──▶ audioconvert
                                   ↓
                             audioresample
                                   ↓
                             autoaudiosink
```

## Variants

| Binary | Source | Approach |
|---|---|---|
| `AudioPlayer` | `main.cpp` | Imperative GStreamer C API |
| `AudioPlayerRAII` | `main_raii.cpp` | `gst::` RAII wrappers |
| `AudioPlayerDeclarative` | `main_declarative.cpp` | `gst::PipelineDesc` / `gst::build()` |
| `AudioPlayerDynamic` | `main_dynamic.cpp` | Dynamic pad linking from a file with `gst::` wrappers |

## Running

The first three variants play a synthesised tone and stop automatically.
The dynamic variant reads `sample.ogg` from the current directory:

```bash
# provide any audio file
cp /path/to/audio.ogg sample.ogg
./AudioPlayerDynamic
```
