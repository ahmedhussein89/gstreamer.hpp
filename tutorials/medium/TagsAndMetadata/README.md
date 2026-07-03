# Tags and Metadata

## New concept
**Tags** carry media metadata (title, artist, codec, bitrate) via `GstTagList` objects posted
as `GST_MESSAGE_TAG` bus messages. The app reads them by subscribing to the bus.

`GstDiscoverer` is a higher-level utility that probes a URI and returns all discovered
streams, caps, and tags without running a full pipeline.

## Pipeline

    videotestsrc → autovideosink
                       ↑ app posts a TAG message after PLAYING, then waits for it on the bus

## How to run
    ./TagsAndMetadata

## Expected output
    TAG message received:
      title = Tutorial Video
      artist = GStreamer Tutorial
      comment = deepstream.hpp tags tutorial
    End of stream reached.

## Exercises
1. Add a numeric tag: `GST_TAG_TRACK_NUMBER` (type `guint`). Use `gst_tag_list_add` with `GST_TAG_MERGE_APPEND`.
2. Iterate tags by index using `gst_tag_list_get_tag_index` instead of `gst_tag_list_foreach`.
3. Filter only tags whose type is `G_TYPE_STRING` using `gst_tag_get_type`.
