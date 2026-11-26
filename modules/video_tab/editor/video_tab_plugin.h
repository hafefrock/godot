#pragma once

#include "editor/docks/editor_dock.h"
#include "editor/plugins/editor_plugin.h"

class Button;
class ColorPickerButton;
class GridContainer;
class HSlider;
class ItemList;
class Label;
class LineEdit;
class ScrollContainer;
class SpinBox;
class Timer;
class VideoDock;
class VideoPreview;

// Timeline: satu baris per item, batang bisa digeser untuk memindah waktu tampil.
// Ketuk baris angka di atas untuk memindah posisi waktu (garis merah).
class VideoTimeline : public Control {
	GDCLASS(VideoTimeline, Control);

	VideoDock *dock = nullptr;
	int drag_row = -1; // -1: tidak menyeret, -2: menggeser posisi waktu.
	double drag_dx = 0.0;

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	static constexpr float ROW_H = 34.0f;
	static constexpr float TOP = 18.0f;

	void set_dock(VideoDock *p_dock) { dock = p_dock; }
};

class VideoDock : public EditorDock {
	GDCLASS(VideoDock, EditorDock);

	enum Field {
		FIELD_START,
		FIELD_END,
		FIELD_X,
		FIELD_Y,
		FIELD_SIZE,
	};

	enum ItemKind {
		KIND_TEXT,
		KIND_RECT,
		KIND_IMAGE,
	};

	Dictionary meta; // Semua kunci adegan selain "items".
	Array items; // Daftar Dictionary.
	int selected = -1;
	bool updating = false;
	double current_time = 0.0;

	Label *status = nullptr;
	SpinBox *f_duration = nullptr;
	VideoPreview *preview = nullptr;
	HSlider *time_slider = nullptr;
	Label *time_label = nullptr;
	Button *play_button = nullptr;
	Timer *play_timer = nullptr;
	ItemList *list = nullptr;
	VideoTimeline *timeline = nullptr;
	ScrollContainer *form_scroll = nullptr;
	Label *f_text_label = nullptr;
	LineEdit *f_text = nullptr;
	SpinBox *f_start = nullptr;
	SpinBox *f_end = nullptr;
	SpinBox *f_x = nullptr;
	SpinBox *f_y = nullptr;
	SpinBox *f_size = nullptr;
	ColorPickerButton *f_color = nullptr;

	SpinBox *_make_spin(double p_min, double p_max, double p_step, int p_field);
	void _say(const String &p_text);
	String _to_json() const;
	void _load_scene();
	void _save_scene();
	void _copy_json();
	void _add_item(int p_kind);
	void _delete_item();
	void _refresh();
	void _fill_form();
	void _redraw_all();

	void _on_list_selected(int p_index);
	void _on_duration(double p_value);
	void _on_text(const String &p_text);
	void _on_num(double p_value, int p_field);
	void _on_color(const Color &p_color);
	void _on_slider(double p_value);
	void _on_play();
	void _on_tick();

public:
	Array &get_items() { return items; }
	double get_duration() const;
	double get_time() const { return current_time; }
	Color get_background() const;
	int get_selected() const { return selected; }
	String get_scene_json() const { return _to_json(); }
	String item_label(const Dictionary &p_item) const;
	void select_item(int p_index);
	void set_time(double p_time);
	void timeline_changed();

	VideoDock();
};

class VideoTabPlugin : public EditorPlugin {
	GDCLASS(VideoTabPlugin, EditorPlugin);

	VideoDock *dock = nullptr;

public:
	virtual String get_plugin_name() const override { return "Video"; }

	VideoTabPlugin();
};
