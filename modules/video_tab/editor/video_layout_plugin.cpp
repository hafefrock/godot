#include "video_layout_plugin.h"

#include "core/object/callable_mp.h"
#include "core/string/print_string.h"
#include "scene/gui/button.h"
#include "scene/gui/color_rect.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/style_box_flat.h"
#include "scene/scene_string_names.h"
#include "video_style.h"
#include "video_tab_plugin.h"

Ref<Theme> VideoLayoutPlugin::_make_theme() const {
	Ref<Theme> t;
	t.instantiate();
	t->set_default_font_size(26);

	t->set_stylebox("normal", "Button", VideoStyle::box(VideoStyle::panel2()));
	t->set_stylebox("hover", "Button", VideoStyle::box(VideoStyle::panel2().lightened(0.12f)));
	t->set_stylebox("pressed", "Button", VideoStyle::box(VideoStyle::panel2().darkened(0.15f)));
	t->set_stylebox("disabled", "Button", VideoStyle::box(VideoStyle::panel()));
	t->set_color("font_color", "Button", VideoStyle::text());
	t->set_color("font_hover_color", "Button", VideoStyle::text());
	t->set_color("font_pressed_color", "Button", VideoStyle::text());

	t->set_stylebox("normal", "MenuButton", VideoStyle::box(VideoStyle::panel2()));
	t->set_stylebox("hover", "MenuButton", VideoStyle::box(VideoStyle::panel2().lightened(0.12f)));
	t->set_stylebox("pressed", "MenuButton", VideoStyle::box(VideoStyle::panel2().darkened(0.15f)));
	t->set_color("font_color", "MenuButton", VideoStyle::text());

	t->set_stylebox("normal", "LineEdit", VideoStyle::box(VideoStyle::panel2(), 12, 12));
	t->set_stylebox("focus", "LineEdit", VideoStyle::box(VideoStyle::panel2().lightened(0.08f), 12, 12));
	t->set_color("font_color", "LineEdit", VideoStyle::text());
	t->set_stylebox("panel", "PanelContainer", VideoStyle::box(VideoStyle::panel(), 16, 12));
	t->set_color("font_color", "Label", VideoStyle::text());
	t->set_color("font_color", "CheckBox", VideoStyle::text());
	return t;
}

void VideoLayoutPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		print_line("[Video] plugin masuk tree");
	} else if (p_what == NOTIFICATION_READY) {
		print_line("[Video] plugin siap");
		add_tool_menu_item("Mode Video", callable_mp(this, &VideoLayoutPlugin::_show_overlay));
		_make_entry_button();
		_schedule();
	}
}

void VideoLayoutPlugin::_make_entry_button() {
	if (entry_layer != nullptr || get_tree() == nullptr) {
		return;
	}
	entry_layer = memnew(CanvasLayer);
	entry_layer->set_layer(99);
	get_tree()->get_root()->add_child(entry_layer);

	entry_button = memnew(Button);
	entry_button->set_text("Mode Video");
	entry_button->set_custom_minimum_size(Size2(200, 68));
	entry_button->add_theme_font_size_override("font_size", 26);
	VideoStyle::style_primary(entry_button);
	entry_button->connect(SceneStringName(pressed), callable_mp(this, &VideoLayoutPlugin::_show_overlay));
	entry_layer->add_child(entry_button);
	entry_button->set_anchors_and_offsets_preset(Control::PRESET_TOP_RIGHT, Control::PRESET_MODE_MINSIZE, 12);
	print_line("[Video] tombol Mode Video dibuat");
}

void VideoLayoutPlugin::_schedule() {
	if (scheduled || get_tree() == nullptr) {
		return;
	}
	scheduled = true;
	// Tunggu sebentar supaya editor selesai dimuat, lalu tampilkan Mode Video.
	get_tree()->create_timer(1.5)->connect("timeout", callable_mp(this, &VideoLayoutPlugin::_build_overlay));
}

void VideoLayoutPlugin::_build_overlay() {
	if (layer != nullptr || get_tree() == nullptr) {
		return;
	}
	layer = memnew(CanvasLayer);
	layer->set_layer(100);
	get_tree()->get_root()->add_child(layer);

	Control *root = memnew(Control);
	layer->add_child(root);
	root->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	root->set_theme(_make_theme());

	ColorRect *bg = memnew(ColorRect);
	bg->set_color(VideoStyle::bg());
	bg->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	root->add_child(bg);
	bg->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	VideoDock *dock = memnew(VideoDock);
	dock->set_exit_callback(callable_mp(this, &VideoLayoutPlugin::_hide_overlay));
	root->add_child(dock);
	dock->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	if (entry_layer != nullptr) {
		entry_layer->set_visible(false);
	}
	print_line("[Video] Mode Video ditampilkan");
}

void VideoLayoutPlugin::_show_overlay() {
	if (layer == nullptr) {
		_build_overlay();
		return;
	}
	layer->set_visible(true);
	if (entry_layer != nullptr) {
		entry_layer->set_visible(false);
	}
}

void VideoLayoutPlugin::_hide_overlay() {
	if (layer != nullptr) {
		layer->set_visible(false);
	}
	if (entry_layer != nullptr) {
		entry_layer->set_visible(true);
	}
}
