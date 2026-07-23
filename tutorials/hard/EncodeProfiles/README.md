# Encode Profiles

## Problem

Wiring `encoder ! muxer` by hand for every codec/container combination is repetitive
and easy to get wrong (mismatched caps, wrong pad names). **Fix:** describe the target
codecs/container once as a `GstEncodingProfile` and hand it to `encodebin`, which
builds and links the matching sub-pipeline for you.

## New concept

`encodebin` builds an encoding/muxing sub-pipeline for you from a `GstEncodingProfile`
instead of you wiring `encoder ! muxer` by hand. A `GstEncodingContainerProfile` (the
container, e.g. Ogg) holds one or more `GstEncodingVideoProfile`/`GstEncodingAudioProfile`
stream profiles (the codecs). Set it once with `g_object_set(encodebin, "profile", ...)`
and `encodebin` internally instantiates and links the matching encoder/muxer elements,
exposing request sink pads named `video_%u` / `audio_%u` and a single static `src` pad.

This tutorial builds the profile manually (`gst_encoding_container_profile_new` +
`gst_encoding_video_profile_new`). `gst_encoding_profile_from_discoverer` is the other
constructor GStreamer offers — it derives a profile from an existing file via
`GstDiscoverer` instead of hand-specifying caps, useful for "re-encode to match this
sample" workflows; not used here to keep the pipeline self-contained (no input file).

Container/codec choice: Ogg + Theora (`gst-plugins-base`/`gst-plugins-good`) — no
`gst-plugins-ugly` dependency, unlike the `x264enc`/`mp4mux` pair used in `VideoRecorder`.

## Pipeline

```text
videotestsrc (150 frames)
      |
 videoconvert
      |
      v
  [ encodebin ]
  video_%u -> theoraenc -> oggmux -> src
      |
   filesink (output.ogv)
```

## How to run

```bash
./EncodeProfiles                  # writes output.ogv
./EncodeProfiles my_clip.ogv
./EncodeProfilesView
./EncodeProfilesRAII
```

## Expected output

```text
Encoding 150 frames to 'output.ogv' using an Ogg/Theora GstEncodingProfile.
Encoding complete.
```

## Exercises

1. Add an `audiotestsrc` branch and an `audio_%u` request pad with a matching
   `GstEncodingAudioProfile` (`video/x-vorbis`) so the Ogg file carries sound too.
2. Swap the container/codec pair for `vp8enc`/`webmmux` (`video/webm`, `video/x-vp8`)
   and compare file size and CPU use.
3. Build the profile from an existing file with `gst_encoding_profile_from_discoverer`
   instead of hand-writing the caps, and compare the resulting `GstEncodingProfile` to
   the manually-built one.

## Linking to encodebin

Do **not** request the sink pad by name. `encodebin` only hands out a `video_%u` pad if the
requester's caps match one of the profile's stream profiles, so this fails:

```c
/* returns NULL: "Couldn't find a compatible stream profile" */
GstPad* pad = gst_element_request_pad_simple(encodebin, "video_%u");
```

`gst_element_link` (and `gst::element_link`) does the caps-matched request for you, which is
why all three tracks just link `convert` to `encodebin` directly. If you need the pad itself,
request it through the template *with* caps: `gst_element_request_pad(encodebin, tmpl, NULL, caps)`.

## Wrapper gap

`gst::`/`gst::raii::` have no type for `GstEncodingProfile`/`GstEncodingContainerProfile`/
`GstEncodingVideoProfile` — all three tracks build the profile with the raw pbutils C API
(see the `// no gst:: wrapper` comment in the source). A `gst::raii::EncodingProfile` move-only
wrapper around `GstEncodingProfile*` (ref-counted via `gst_encoding_profile_ref`/`_unref`)
would be the obvious future addition; not written here — out of scope for this tutorial.
