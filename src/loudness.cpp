#include "loudness.hpp"
#include <glibmm/main.h>
#include "util.hpp"

Loudness::Loudness(const std::string& tag, const std::string& schema, const std::string& schema_path)
    : PluginBase(tag, "loudness", schema, schema_path) {
  loudness = gst_element_factory_make("lsp-plug-in-plugins-lv2-loud-comp-stereo", nullptr);

  if (is_installed(loudness)) {
    auto* in_level = gst_element_factory_make("level", "loudness_input_level");
    auto* out_level = gst_element_factory_make("level", "loudness_output_level");
    auto* audioconvert_in = gst_element_factory_make("audioconvert", "loudness_audioconvert_in");
    auto* audioconvert_out = gst_element_factory_make("audioconvert", "loudness_audioconvert_out");

    gst_bin_add_many(GST_BIN(bin), in_level, audioconvert_in, loudness, audioconvert_out, out_level, nullptr);
    gst_element_link_many(in_level, audioconvert_in, loudness, audioconvert_out, out_level, nullptr);

    auto* pad_sink = gst_element_get_static_pad(in_level, "sink");
    auto* pad_src = gst_element_get_static_pad(out_level, "src");

    gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad_sink));
    gst_element_add_pad(bin, gst_ghost_pad_new("src", pad_src));

    gst_object_unref(GST_OBJECT(pad_sink));
    gst_object_unref(GST_OBJECT(pad_src));

    g_object_set(loudness, "enabled", 1, nullptr);
    g_object_set(loudness, "refer", 0, nullptr);
    g_object_set(loudness, "hclip", 0, nullptr);

    bind_to_gsettings();

    g_settings_bind(settings, "post-messages", in_level, "post-messages", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(settings, "post-messages", out_level, "post-messages", G_SETTINGS_BIND_DEFAULT);

    // useless write just to force callback call

    auto enable = g_settings_get_boolean(settings, "state");

    g_settings_set_boolean(settings, "state", enable);
  }
}

Loudness::~Loudness() {
  util::debug(log_tag + name + " destroyed");
}

void Loudness::bind_to_gsettings() {
  g_settings_bind_with_mapping(settings, "input", loudness, "input", G_SETTINGS_BIND_DEFAULT, util::db20_gain_to_linear,
                               util::linear_gain_to_db20, nullptr, nullptr);

  g_settings_bind_with_mapping(settings, "volume", loudness, "volume", G_SETTINGS_BIND_GET, util::double_to_float,
                               nullptr, nullptr, nullptr);

  g_settings_bind(settings, "fft", loudness, "fft", G_SETTINGS_BIND_DEFAULT);

  g_settings_bind(settings, "std", loudness, "std", G_SETTINGS_BIND_DEFAULT);

  g_settings_bind(settings, "reference-signal", loudness, "refer", G_SETTINGS_BIND_DEFAULT);
}
