# Linker selection. Preference order when GST_LINKER is left at its default: mold > lld > gold > platform default.
find_program(GST_MOLD_LINKER mold)
find_program(GST_LLD_LINKER ld.lld)
find_program(GST_GOLD_LINKER ld.gold)

if(GST_MOLD_LINKER)
  set(GST_LINKER_DEFAULT "mold")
elseif(GST_LLD_LINKER)
  set(GST_LINKER_DEFAULT "lld")
elseif(GST_GOLD_LINKER)
  set(GST_LINKER_DEFAULT "gold")
else()
  set(GST_LINKER_DEFAULT "default")
endif()

set(GST_LINKER
    ${GST_LINKER_DEFAULT}
    CACHE STRING "Linker to use: default, mold, lld, or gold")
set_property(CACHE GST_LINKER PROPERTY STRINGS "default" "mold" "lld" "gold")

if(MSVC)
  if(NOT GST_LINKER STREQUAL "default")
    message(AUTHOR_WARNING "GST_LINKER=${GST_LINKER} ignored: linker selection is not supported with MSVC")
  endif()
elseif(NOT GST_LINKER STREQUAL "default")
  add_link_options(-fuse-ld=${GST_LINKER})
  message(STATUS "Using ${GST_LINKER} linker")
endif()
