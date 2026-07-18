#[=======================================================================[.rst:
FindGStreamer
-------------

Find GStreamer libraries and components using only CMake find_path /
find_library — no pkg-config dependency.

IMPORTED Targets
^^^^^^^^^^^^^^^^

``GStreamer::GStreamer``
  The main GStreamer library, if found.

``GStreamer::<component>``
  Individual GStreamer component libraries (e.g., GStreamer::Video).

Result Variables
^^^^^^^^^^^^^^^^

``GStreamer_FOUND``
  True if the GStreamer library was found.

``GStreamer_VERSION``
  The version of GStreamer found.

``GStreamer_INCLUDE_DIRS``
  Include directories needed to use GStreamer (includes GLib dirs).

``GStreamer_LIBRARIES``
  Libraries needed to link to GStreamer.

``GStreamer_<component>_FOUND``
  True if the specified component was found.

Components
^^^^^^^^^^

* App         (gstapp)
* Audio       (gstaudio)
* Video       (gstvideo)
* Pbutils     (gstpbutils)
* Rtp         (gstrtp)
* Rtsp        (gstrtsp)
* RtspServer  (gstrtspserver)
* Sdp         (gstsdp)
* Tag         (gsttag)
* Allocators  (gstallocators)
* Controller  (gstcontroller)
* Net         (gstnet)
* Base        (gstbase)
* Check       (gstcheck)
* Gl          (gstgl)

Example usage
^^^^^^^^^^^^^

.. code-block:: cmake

  find_package(GStreamer REQUIRED COMPONENTS Video Base)
  target_link_libraries(myapp PRIVATE GStreamer::GStreamer GStreamer::Video GStreamer::Base)

#]=======================================================================]

# ---------------------------------------------------------------------------
# 0. Locate GLib/GObject libraries — required transitive link deps of GStreamer.
#    Without pkg-config these must be found and propagated explicitly.
# ---------------------------------------------------------------------------
set(_GLib_LIB_SEARCH_PATHS
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib/aarch64-linux-gnu
)

find_library(GLib_LIBRARY     NAMES glib-2.0   PATHS ${_GLib_LIB_SEARCH_PATHS})
find_library(GObject_LIBRARY  NAMES gobject-2.0 PATHS ${_GLib_LIB_SEARCH_PATHS})
find_library(GModule_LIBRARY  NAMES gmodule-2.0 PATHS ${_GLib_LIB_SEARCH_PATHS})

set(_GLib_LIBRARIES "")
foreach(_glib_lib GLib_LIBRARY GObject_LIBRARY GModule_LIBRARY)
    if(${_glib_lib})
        list(APPEND _GLib_LIBRARIES "${${_glib_lib}}")
    endif()
endforeach()

mark_as_advanced(GLib_LIBRARY GObject_LIBRARY GModule_LIBRARY)

# ---------------------------------------------------------------------------
# 1. Locate GLib headers — required transitive dependency of GStreamer.
#    GLib has two include trees: the main headers and an arch-specific
#    directory that contains glibconfig.h.
# ---------------------------------------------------------------------------
set(_GLib_ARCH_SUFFIXES
    x86_64-linux-gnu/glib-2.0/include
    aarch64-linux-gnu/glib-2.0/include
    arm-linux-gnueabihf/glib-2.0/include
    i386-linux-gnu/glib-2.0/include
    glib-2.0/include               # fallback (some distros put it here)
    lib/glib-2.0/include           # Windows GStreamer SDK layout (<prefix>/lib/glib-2.0/include)
)

find_path(GLib_INCLUDE_DIR
    NAMES glib.h
    PATH_SUFFIXES glib-2.0
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
)

find_path(GLib_CONFIG_INCLUDE_DIR
    NAMES glibconfig.h
    PATH_SUFFIXES ${_GLib_ARCH_SUFFIXES}
    PATHS
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/local/lib64
        /usr/lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu
)

set(_GLib_INCLUDE_DIRS "")
if(GLib_INCLUDE_DIR)
    list(APPEND _GLib_INCLUDE_DIRS "${GLib_INCLUDE_DIR}")
endif()
if(GLib_CONFIG_INCLUDE_DIR)
    list(APPEND _GLib_INCLUDE_DIRS "${GLib_CONFIG_INCLUDE_DIR}")
endif()

# ---------------------------------------------------------------------------
# 2. Locate the main GStreamer include directory and library.
# ---------------------------------------------------------------------------
find_path(GStreamer_INCLUDE_DIR
    NAMES gst/gst.h
    PATH_SUFFIXES gstreamer-1.0
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
)

find_library(GStreamer_LIBRARY
    NAMES gstreamer-1.0
    PATHS
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu
)

# ---------------------------------------------------------------------------
# 3. Determine version by reading gstversion.h directly.
# ---------------------------------------------------------------------------
# gstversion.h defines values as either bare integers (1) or parenthesised
# integers ((1)), depending on the GStreamer version.  Both forms are handled.
if(GStreamer_INCLUDE_DIR AND EXISTS "${GStreamer_INCLUDE_DIR}/gst/gstversion.h")
    file(STRINGS "${GStreamer_INCLUDE_DIR}/gst/gstversion.h" _ver_major_line
        REGEX "^#define[ \t]+GST_VERSION_MAJOR")
    file(STRINGS "${GStreamer_INCLUDE_DIR}/gst/gstversion.h" _ver_minor_line
        REGEX "^#define[ \t]+GST_VERSION_MINOR")
    file(STRINGS "${GStreamer_INCLUDE_DIR}/gst/gstversion.h" _ver_micro_line
        REGEX "^#define[ \t]+GST_VERSION_MICRO")

    string(REGEX REPLACE "^#define[ \t]+GST_VERSION_MAJOR[ \t]+\\(?([0-9]+)\\)?.*" "\\1"
        _ver_major "${_ver_major_line}")
    string(REGEX REPLACE "^#define[ \t]+GST_VERSION_MINOR[ \t]+\\(?([0-9]+)\\)?.*" "\\1"
        _ver_minor "${_ver_minor_line}")
    string(REGEX REPLACE "^#define[ \t]+GST_VERSION_MICRO[ \t]+\\(?([0-9]+)\\)?.*" "\\1"
        _ver_micro "${_ver_micro_line}")

    set(GStreamer_VERSION "${_ver_major}.${_ver_minor}.${_ver_micro}")
    unset(_ver_major_line)
    unset(_ver_minor_line)
    unset(_ver_micro_line)
    unset(_ver_major)
    unset(_ver_minor)
    unset(_ver_micro)
endif()

# ---------------------------------------------------------------------------
# 4. Component table: CMake name -> library stem (without -1.0 suffix).
#    The library is looked up as  lib${stem}-1.0.so.
# ---------------------------------------------------------------------------
set(_GStreamer_COMPONENT_LIB_MAP
    "App:gstapp"
    "Audio:gstaudio"
    "Video:gstvideo"
    "Pbutils:gstpbutils"
    "Rtp:gstrtp"
    "Rtsp:gstrtsp"
    "RtspServer:gstrtspserver"
    "Sdp:gstsdp"
    "Tag:gsttag"
    "Allocators:gstallocators"
    "Controller:gstcontroller"
    "Net:gstnet"
    "Base:gstbase"
    "Check:gstcheck"
    "Gl:gstgl"
)

# Header hint per component — used by find_path to locate the component's
# own include directory (usually the same gstreamer-1.0 tree).
set(_GStreamer_COMPONENT_HEADER_MAP
    "App:gst/app/gstappsink.h"
    "Audio:gst/audio/audio.h"
    "Video:gst/video/video.h"
    "Pbutils:gst/pbutils/pbutils.h"
    "Rtp:gst/rtp/gstrtp.h"
    "Rtsp:gst/rtsp/gstrtsp.h"
    "RtspServer:gst/rtsp-server/rtsp-server.h"
    "Sdp:gst/sdp/gstsdp.h"
    "Tag:gst/tag/tag.h"
    "Allocators:gst/allocators/gstdmabuf.h"
    "Controller:gst/controller/gstinterpolationcontrolsource.h"
    "Net:gst/net/gstnet.h"
    "Base:gst/base/gstbasesrc.h"
    "Check:gst/check/gstcheck.h"
    "Gl:gst/gl/gl.h"
)

set(_GStreamer_REQUIRED_VARS GStreamer_LIBRARY GStreamer_INCLUDE_DIR)

foreach(_comp IN LISTS GStreamer_FIND_COMPONENTS)
    # Resolve library stem for this component.
    set(_lib_stem "")
    foreach(_entry IN LISTS _GStreamer_COMPONENT_LIB_MAP)
        string(REPLACE ":" ";" _pair "${_entry}")
        list(GET _pair 0 _cname)
        list(GET _pair 1 _cstem)
        if(_comp STREQUAL _cname)
            set(_lib_stem "${_cstem}")
            break()
        endif()
    endforeach()

    if(NOT _lib_stem)
        if(GStreamer_FIND_REQUIRED_${_comp})
            message(FATAL_ERROR "FindGStreamer: unknown component '${_comp}'")
        else()
            message(WARNING "FindGStreamer: unknown component '${_comp}'")
            set(GStreamer_${_comp}_FOUND FALSE)
            continue()
        endif()
    endif()

    # Resolve header hint for this component.
    set(_header_hint "")
    foreach(_entry IN LISTS _GStreamer_COMPONENT_HEADER_MAP)
        string(REPLACE ":" ";" _pair "${_entry}")
        list(GET _pair 0 _cname)
        list(GET _pair 1 _cheader)
        if(_comp STREQUAL _cname)
            set(_header_hint "${_cheader}")
            break()
        endif()
    endforeach()

    # Find include dir for this component (typically the same gstreamer-1.0 tree).
    # RtspServer lives in a separate gstreamer-rtsp-server-1.0 directory.
    if(_comp STREQUAL "RtspServer")
        find_path(GStreamer_${_comp}_INCLUDE_DIR
            NAMES "${_header_hint}"
            PATH_SUFFIXES gstreamer-rtsp-server-1.0 gstreamer-1.0
            PATHS
                /usr/include
                /usr/local/include
                /opt/local/include
        )
    else()
        find_path(GStreamer_${_comp}_INCLUDE_DIR
            NAMES "${_header_hint}"
            PATH_SUFFIXES gstreamer-1.0
            PATHS
                /usr/include
                /usr/local/include
                /opt/local/include
            HINTS "${GStreamer_INCLUDE_DIR}"
        )
    endif()

    # Find the component library.
    find_library(GStreamer_${_comp}_LIBRARY
        NAMES "${_lib_stem}-1.0"
        PATHS
            /usr/lib
            /usr/lib64
            /usr/local/lib
            /usr/lib/x86_64-linux-gnu
            /usr/lib/aarch64-linux-gnu
    )

    # Gl ships an arch-specific generated gstglconfig.h alongside the library
    # (like glibconfig.h), not under the main include tree.
    set(_extra_include_dirs "")
    if(_comp STREQUAL "Gl")
        find_path(GStreamer_Gl_CONFIG_INCLUDE_DIR
            NAMES gst/gl/gstglconfig.h
            PATH_SUFFIXES gstreamer-1.0/include lib/gstreamer-1.0/include
            PATHS
                /usr/lib/x86_64-linux-gnu
                /usr/lib/aarch64-linux-gnu
                /usr/lib
                /usr/lib64
        )
        mark_as_advanced(GStreamer_Gl_CONFIG_INCLUDE_DIR)
        if(GStreamer_Gl_CONFIG_INCLUDE_DIR)
            list(APPEND _extra_include_dirs "${GStreamer_Gl_CONFIG_INCLUDE_DIR}")
        endif()
    endif()

    if(GStreamer_${_comp}_INCLUDE_DIR AND GStreamer_${_comp}_LIBRARY)
        set(GStreamer_${_comp}_FOUND TRUE)

        if(NOT TARGET GStreamer::${_comp})
            add_library(GStreamer::${_comp} UNKNOWN IMPORTED)
            set_target_properties(GStreamer::${_comp} PROPERTIES
                IMPORTED_LOCATION "${GStreamer_${_comp}_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_${_comp}_INCLUDE_DIR};${_extra_include_dirs}"
            )
            # Pull in the main library and GLib transitively.
            if(TARGET GStreamer::GStreamer)
                set_property(TARGET GStreamer::${_comp} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES GStreamer::GStreamer)
            endif()
        endif()
    else()
        set(GStreamer_${_comp}_FOUND FALSE)
    endif()

    if(GStreamer_FIND_REQUIRED_${_comp})
        list(APPEND _GStreamer_REQUIRED_VARS
            GStreamer_${_comp}_LIBRARY
            GStreamer_${_comp}_INCLUDE_DIR)
    endif()

    mark_as_advanced(GStreamer_${_comp}_INCLUDE_DIR GStreamer_${_comp}_LIBRARY)
endforeach()

# ---------------------------------------------------------------------------
# 5. Standard result handling.
# ---------------------------------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GStreamer
    REQUIRED_VARS ${_GStreamer_REQUIRED_VARS}
    VERSION_VAR   GStreamer_VERSION
    HANDLE_COMPONENTS
)

# ---------------------------------------------------------------------------
# 6. Create the main GStreamer::GStreamer imported target.
# ---------------------------------------------------------------------------
if(GStreamer_FOUND AND NOT TARGET GStreamer::GStreamer)
    add_library(GStreamer::GStreamer UNKNOWN IMPORTED)
    set_target_properties(GStreamer::GStreamer PROPERTIES
        IMPORTED_LOCATION "${GStreamer_LIBRARY}"
        # GStreamer headers + GLib headers (main + arch-specific config).
        INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_INCLUDE_DIR};${_GLib_INCLUDE_DIRS}"
        # GLib / GObject must be propagated explicitly (no pkg-config).
        INTERFACE_LINK_LIBRARIES "${_GLib_LIBRARIES}"
    )
endif()

# ---------------------------------------------------------------------------
# 7. Populate convenience variables.
# ---------------------------------------------------------------------------
if(GStreamer_FOUND)
    set(GStreamer_INCLUDE_DIRS "${GStreamer_INCLUDE_DIR}" ${_GLib_INCLUDE_DIRS})
    set(GStreamer_LIBRARIES    "${GStreamer_LIBRARY}")

    foreach(_comp IN LISTS GStreamer_FIND_COMPONENTS)
        if(GStreamer_${_comp}_FOUND)
            list(APPEND GStreamer_LIBRARIES    "${GStreamer_${_comp}_LIBRARY}")
            list(APPEND GStreamer_INCLUDE_DIRS "${GStreamer_${_comp}_INCLUDE_DIR}")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES GStreamer_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES GStreamer_LIBRARIES)
endif()

mark_as_advanced(GStreamer_INCLUDE_DIR GStreamer_LIBRARY)
unset(_GStreamer_REQUIRED_VARS)
unset(_GStreamer_COMPONENT_LIB_MAP)
unset(_GStreamer_COMPONENT_HEADER_MAP)
unset(_GLib_ARCH_SUFFIXES)
