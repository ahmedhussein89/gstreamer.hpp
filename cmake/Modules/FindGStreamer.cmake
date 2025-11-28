#[=======================================================================[.rst:
FindGStreamer
-------------

Find GStreamer libraries and components.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``GStreamer::GStreamer``
  The main GStreamer library, if found.

``GStreamer::<component>``
  Individual GStreamer component libraries (e.g., GStreamer::App, GStreamer::Video).

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables:

``GStreamer_FOUND``
  True if the GStreamer library was found.

``GStreamer_VERSION``
  The version of GStreamer found.

``GStreamer_INCLUDE_DIRS``
  Include directories needed to use GStreamer.

``GStreamer_LIBRARIES``
  Libraries needed to link to GStreamer.

``GStreamer_<component>_FOUND``
  True if the specified component was found.

Components
^^^^^^^^^^

The following components are supported:

* App (gstapp)
* Audio (gstaudio)
* Video (gstvideo)
* Pbutils (gstpbutils)
* Rtp (gstrtp)
* Rtsp (gstrtsp)
* Sdp (gstsdp)
* Tag (gsttag)
* Allocators (gstallocators)
* Controller (gstcontroller)
* Net (gstnet)
* Base (gstbase)
* Check (gstcheck)

Example usage:

.. code-block:: cmake

  find_package(GStreamer REQUIRED COMPONENTS App Video)
  target_link_libraries(myapp PRIVATE GStreamer::GStreamer GStreamer::App GStreamer::Video)

#]=======================================================================]

# Use pkg-config to get hints about paths
find_package(PkgConfig QUIET)

# Define component mapping (CMake component name -> pkg-config name)
set(_GStreamer_COMPONENT_MAP
    App:gstreamer-app-1.0
    Audio:gstreamer-audio-1.0
    Video:gstreamer-video-1.0
    Pbutils:gstreamer-pbutils-1.0
    Rtp:gstreamer-rtp-1.0
    Rtsp:gstreamer-rtsp-1.0
    Sdp:gstreamer-sdp-1.0
    Tag:gstreamer-tag-1.0
    Allocators:gstreamer-allocators-1.0
    Controller:gstreamer-controller-1.0
    Net:gstreamer-net-1.0
    Base:gstreamer-base-1.0
    Check:gstreamer-check-1.0
)

# Find the main GStreamer library first
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GStreamer QUIET gstreamer-1.0)
endif()

# Find the main include directory
find_path(GStreamer_INCLUDE_DIR
    NAMES gst/gst.h
    HINTS
        ${PC_GStreamer_INCLUDE_DIRS}
        ${PC_GStreamer_INCLUDEDIR}
    PATH_SUFFIXES gstreamer-1.0
)

# Find the main library
find_library(GStreamer_LIBRARY
    NAMES gstreamer-1.0 gst-1.0
    HINTS
        ${PC_GStreamer_LIBRARY_DIRS}
        ${PC_GStreamer_LIBDIR}
)

# Get version from pkg-config or headers
if(PC_GStreamer_VERSION)
    set(GStreamer_VERSION ${PC_GStreamer_VERSION})
elseif(GStreamer_INCLUDE_DIR AND EXISTS "${GStreamer_INCLUDE_DIR}/gst/gstversion.h")
    file(STRINGS "${GStreamer_INCLUDE_DIR}/gst/gstversion.h" _version_major_line
        REGEX "^#define[ \t]+GST_VERSION_MAJOR[ \t]+[0-9]+")
    file(STRINGS "${GStreamer_INCLUDE_DIR}/gst/gstversion.h" _version_minor_line
        REGEX "^#define[ \t]+GST_VERSION_MINOR[ \t]+[0-9]+")
    file(STRINGS "${GStreamer_INCLUDE_DIR}/gst/gstversion.h" _version_micro_line
        REGEX "^#define[ \t]+GST_VERSION_MICRO[ \t]+[0-9]+")

    string(REGEX REPLACE "^#define[ \t]+GST_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1"
        _version_major "${_version_major_line}")
    string(REGEX REPLACE "^#define[ \t]+GST_VERSION_MINOR[ \t]+([0-9]+).*" "\\1"
        _version_minor "${_version_minor_line}")
    string(REGEX REPLACE "^#define[ \t]+GST_VERSION_MICRO[ \t]+([0-9]+).*" "\\1"
        _version_micro "${_version_micro_line}")

    set(GStreamer_VERSION "${_version_major}.${_version_minor}.${_version_micro}")
    unset(_version_major_line)
    unset(_version_minor_line)
    unset(_version_micro_line)
    unset(_version_major)
    unset(_version_minor)
    unset(_version_micro)
endif()

# Process components
set(_GStreamer_REQUIRED_VARS GStreamer_LIBRARY GStreamer_INCLUDE_DIR)

foreach(_comp IN LISTS GStreamer_FIND_COMPONENTS)
    # Find the pkg-config name for this component
    set(_pkg_name "")
    foreach(_mapping IN LISTS _GStreamer_COMPONENT_MAP)
        string(REPLACE ":" ";" _mapping_list ${_mapping})
        list(GET _mapping_list 0 _comp_name)
        list(GET _mapping_list 1 _comp_pkg)

        if(_comp STREQUAL _comp_name)
            set(_pkg_name ${_comp_pkg})
            break()
        endif()
    endforeach()

    if(NOT _pkg_name)
        if(GStreamer_FIND_REQUIRED_${_comp})
            message(FATAL_ERROR "Unknown GStreamer component: ${_comp}")
        else()
            message(WARNING "Unknown GStreamer component: ${_comp}")
            set(GStreamer_${_comp}_FOUND FALSE)
            continue()
        endif()
    endif()

    # Use pkg-config to find component
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_GStreamer_${_comp} QUIET ${_pkg_name})
    endif()

    # Find component include directory
    find_path(GStreamer_${_comp}_INCLUDE_DIR
        NAMES gst/${_comp}/gst${_comp}.h gst/app/gstappsink.h gst/video/video.h gst.h
        HINTS
            ${PC_GStreamer_${_comp}_INCLUDE_DIRS}
            ${PC_GStreamer_${_comp}_INCLUDEDIR}
            ${GStreamer_INCLUDE_DIR}
        NO_DEFAULT_PATH
    )

    # Find component library
    # Convert component name to lowercase for library name
    string(TOLOWER ${_comp} _comp_lower)
    find_library(GStreamer_${_comp}_LIBRARY
        NAMES gst${_comp_lower}-1.0 ${_pkg_name}
        HINTS
            ${PC_GStreamer_${_comp}_LIBRARY_DIRS}
            ${PC_GStreamer_${_comp}_LIBDIR}
    )

    # Mark as found if both include and library exist
    if(GStreamer_${_comp}_INCLUDE_DIR AND GStreamer_${_comp}_LIBRARY)
        set(GStreamer_${_comp}_FOUND TRUE)

        # Create imported target for component
        if(NOT TARGET GStreamer::${_comp})
            add_library(GStreamer::${_comp} UNKNOWN IMPORTED)
            set_target_properties(GStreamer::${_comp} PROPERTIES
                IMPORTED_LOCATION "${GStreamer_${_comp}_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_${_comp}_INCLUDE_DIR}"
            )

            # Add pkg-config include directories for this component
            if(PKG_CONFIG_FOUND AND PC_GStreamer_${_comp}_INCLUDE_DIRS)
                set_property(TARGET GStreamer::${_comp} APPEND PROPERTY
                    INTERFACE_INCLUDE_DIRECTORIES ${PC_GStreamer_${_comp}_INCLUDE_DIRS})
            endif()

            # Link against main GStreamer library
            if(TARGET GStreamer::GStreamer)
                set_property(TARGET GStreamer::${_comp} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES GStreamer::GStreamer)
            endif()
        endif()
    else()
        set(GStreamer_${_comp}_FOUND FALSE)
    endif()

    # Add to required vars if component is required
    if(GStreamer_FIND_REQUIRED_${_comp})
        list(APPEND _GStreamer_REQUIRED_VARS
            GStreamer_${_comp}_LIBRARY
            GStreamer_${_comp}_INCLUDE_DIR
        )
    endif()

    # Mark component variables as advanced
    mark_as_advanced(
        GStreamer_${_comp}_INCLUDE_DIR
        GStreamer_${_comp}_LIBRARY
    )
endforeach()

# Standard find_package arguments handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GStreamer
    REQUIRED_VARS ${_GStreamer_REQUIRED_VARS}
    VERSION_VAR GStreamer_VERSION
    HANDLE_COMPONENTS
)

# Create imported target for main GStreamer library
if(GStreamer_FOUND AND NOT TARGET GStreamer::GStreamer)
    add_library(GStreamer::GStreamer UNKNOWN IMPORTED)
    set_target_properties(GStreamer::GStreamer PROPERTIES
        IMPORTED_LOCATION "${GStreamer_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GStreamer_INCLUDE_DIR}"
    )

    # Add GLib and other dependencies if found via pkg-config
    if(PKG_CONFIG_FOUND)
        if(PC_GStreamer_INCLUDE_DIRS)
            set_property(TARGET GStreamer::GStreamer APPEND PROPERTY
                INTERFACE_INCLUDE_DIRECTORIES ${PC_GStreamer_INCLUDE_DIRS})
        endif()
        if(PC_GStreamer_LINK_LIBRARIES)
            set_property(TARGET GStreamer::GStreamer APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES ${PC_GStreamer_LINK_LIBRARIES})
        endif()
    endif()
endif()

# Set standard variables for compatibility
if(GStreamer_FOUND)
    set(GStreamer_LIBRARIES ${GStreamer_LIBRARY})
    set(GStreamer_INCLUDE_DIRS ${GStreamer_INCLUDE_DIR})

    # Add component libraries and includes
    foreach(_comp IN LISTS GStreamer_FIND_COMPONENTS)
        if(GStreamer_${_comp}_FOUND)
            list(APPEND GStreamer_LIBRARIES ${GStreamer_${_comp}_LIBRARY})
            list(APPEND GStreamer_INCLUDE_DIRS ${GStreamer_${_comp}_INCLUDE_DIR})
        endif()
    endforeach()

    # Remove duplicates
    list(REMOVE_DUPLICATES GStreamer_INCLUDE_DIRS)
endif()

# Mark standard variables as advanced
mark_as_advanced(
    GStreamer_INCLUDE_DIR
    GStreamer_LIBRARY
)

# Cleanup internal variables
unset(_GStreamer_REQUIRED_VARS)
unset(_GStreamer_COMPONENT_MAP)
