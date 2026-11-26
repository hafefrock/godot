#pragma once

#include "editor/plugins/editor_plugin.h"

// Menyembunyikan tab khusus game dan menjadikan tab Video tampilan utama.
class VideoLayoutPlugin : public EditorPlugin {
	GDCLASS(VideoLayoutPlugin, EditorPlugin);

	void _apply_layout();

protected:
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "VideoLayout"; }
};
