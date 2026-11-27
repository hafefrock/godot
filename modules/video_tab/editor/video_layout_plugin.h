#pragma once

#include "editor/plugins/editor_plugin.h"
#include "scene/resources/theme.h"

class Button;
class CanvasLayer;

// "Mode Video": menutup tampilan editor game dengan lapisan layar penuh berisi editor video.
// Ada tombol kuning "Mode Video" di pojok kanan atas editor (muncul selama lapisan tertutup),
// dan pilihan di menu Project > Tools > Mode Video.
class VideoLayoutPlugin : public EditorPlugin {
	GDCLASS(VideoLayoutPlugin, EditorPlugin);

	CanvasLayer *layer = nullptr;
	CanvasLayer *entry_layer = nullptr;
	Button *entry_button = nullptr;
	bool scheduled = false;

	Ref<Theme> _make_theme() const;
	void _make_entry_button();
	void _schedule();
	void _build_overlay();
	void _show_overlay();
	void _hide_overlay();

protected:
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "VideoLayout"; }
};
