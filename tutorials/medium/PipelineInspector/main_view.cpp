#include <cstdlib>

#include <fmt/core.h>

#include "gstreamer.hpp"

namespace {

void print_pad_templates(GstElementFactory* factory) {
  const GList* templates = gst::element_factory_get_static_pad_templates(factory);
  for(const GList* it = templates; it != nullptr; it = it->next) {
    const auto* tmpl    = static_cast<GstStaticPadTemplate*>(it->data);
    const char* dir_str = (tmpl->direction == GST_PAD_SRC) ? "src" : "sink";
    const char* pres    = (tmpl->presence == GST_PAD_ALWAYS)    ? "always"
                        : (tmpl->presence == GST_PAD_SOMETIMES) ? "sometimes"
                                                                 : "request";
    fmt::print(stdout, "    pad[{}] direction={} presence={}\n", tmpl->name_template, dir_str, pres);
  }
}

void print_factory(GstElementFactory* factory) {
  const auto name = gst::plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
  const auto desc = gst::element_factory_get_metadata(factory, GST_ELEMENT_METADATA_LONGNAME);
  const auto klas = gst::element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS);

  fmt::print(stdout, "{}\n", name);
  fmt::print(stdout, "  class:       {}\n", !klas.empty() ? klas : "(none)");
  fmt::print(stdout, "  description: {}\n", !desc.empty() ? desc : "(none)");
  print_pad_templates(factory);
}

bool matches(std::string_view name, const char* filter) {
  if(nullptr == filter) {
    return true;
  }
  return name.find(filter) != std::string_view::npos;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  const char* filter = (argc > 1) ? argv[1] : nullptr;
  if(filter != nullptr) {
    fmt::print(stdout, "Filtering by: '{}'\n\n", filter);
  }

  gst::Registry registry = gst::registry_get();
  auto          plugins  = gst::registry_get_plugin_list(registry);

  int element_count = 0;
  for(const auto& plugin : plugins) {
    const auto plugin_name = gst::plugin_get_name(plugin.get());
    auto       factories   = gst::registry_get_feature_list_by_plugin(registry, plugin_name);

    for(const auto& feature : factories) {
      if(!GST_IS_ELEMENT_FACTORY(feature.get())) {
        continue;
      }
      auto*      factory = GST_ELEMENT_FACTORY(feature.get());
      const auto name    = gst::plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
      if(!matches(name, filter)) {
        continue;
      }
      print_factory(factory);
      fmt::print(stdout, "\n");
      ++element_count;
    }
  }

  fmt::print(stdout, "Total elements found: {}\n", element_count);
  return EXIT_SUCCESS;
}
