#pragma once

#include "editor/plugins/editor_plugin.h"
#include "scene/resources/theme.h"

class CanvasLayer;

// "Mode Video": menutup tampilan editor game dengan lapisan layar penuh berisi editor video.
// Tampilan ini muncul otomatis setelah proyek terbuka. Untuk membukanya lagi dari editor:
// menu Proyek > Alat > Mode Video.
class VideoLayoutPlugin : public EditorPlugin {
	GDCLASS(VideoLayoutPlugin, EditorPlugin);

	CanvasLayer *layer = nullptr;

	Ref<Theme> _make_theme() const;
	void _build_overlay();
	void _show_overlay();
	void _hide_overlay();

protected:
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "VideoLayout"; }
};
