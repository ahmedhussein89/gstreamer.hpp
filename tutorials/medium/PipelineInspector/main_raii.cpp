#include <cstdlib>

#include <fmt/core.h>

#include "gstreamer.hpp"

namespace {

void print_pad_templates(GstElementFactory* factory) {
  const GList* templates = gst_element_factory_get_static_pad_templates(factory);
  for(const GList* it = templates; it != nullptr; it = it->next) {
    const auto* tmpl    = static_cast<GstStaticPadTemplate*>(it->data);
    const char* dir_str = (tmpl->direction == GST_PAD_SRC) ? "src" : "sink";
    const char* pres    = (tmpl->presence == GST_PAD_ALWAYS)    ? "always"
                        : (tmpl->presence == GST_PAD_SOMETIMES) ? "sometimes"
                                                                 : "request";
    fmt::println(stdout, "    pad[{}] direction={} presence={}", tmpl->name_template, dir_str, pres);
  }
}

void print_factory(GstElementFactory* factory) {
  const auto* name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
  const auto* desc = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_LONGNAME);
  const auto* klas = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS);

  fmt::println(stdout, "{}", name);
  fmt::println(stdout, "  class:       {}", klas != nullptr ? klas : "(none)");
  fmt::println(stdout, "  description: {}", desc != nullptr ? desc : "(none)");
  print_pad_templates(factory);
}

bool matches(const char* name, const char* filter) {
  if(nullptr == filter) {
    return true;
  }
  return g_strstr_len(name, -1, filter) != nullptr;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  const char* filter = (argc > 1) ? argv[1] : nullptr;
  if(filter != nullptr) {
    fmt::println(stdout, "Filtering by: '{}'\n", filter);
  }

  GstRegistry* registry = gst_registry_get();
  GList*       plugins  = gst_registry_get_plugin_list(registry);

  int element_count = 0;
  for(GList* pit = plugins; pit != nullptr; pit = pit->next) {
    auto*  plugin    = static_cast<GstPlugin*>(pit->data);
    GList* factories = gst_registry_get_feature_list_by_plugin(registry, gst_plugin_get_name(plugin));

    for(GList* fit = factories; fit != nullptr; fit = fit->next) {
      auto* feature = static_cast<GstPluginFeature*>(fit->data);
      if(!GST_IS_ELEMENT_FACTORY(feature)) {
        continue;
      }
      auto*       factory = GST_ELEMENT_FACTORY(feature);
      const auto* name    = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
      if(!matches(name, filter)) {
        continue;
      }
      print_factory(factory);
      fmt::println(stdout, "");
      ++element_count;
    }

    gst_plugin_feature_list_free(factories);
  }

  gst_plugin_list_free(plugins);

  fmt::println(stdout, "Total elements found: {}", element_count);
  return EXIT_SUCCESS;
}
