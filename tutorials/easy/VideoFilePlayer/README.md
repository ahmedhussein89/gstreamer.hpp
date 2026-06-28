# Video File Player

## Goal

Learn sources, decoders, sinks.

## Concepts

- filesrc
- decodebin
- autovideosink
- Bus messages
- EOS handling

## Example Flow

```
MP4 File
   ↓
decodebin
   ↓
autovideosink
```
