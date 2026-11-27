#pragma once

#include "scene/gui/margin_container.h"

class CheckBox;
class ColorPickerButton;
class HSlider;
class Label;
class LineEdit;
class SpinBox;
class Timer;
class VBoxContainer;
class VideoDock;
class VideoPreview;
class VideoRenderPanel;
class Button;

// Timeline: satu baris per item.
// Ketuk baris angka di atas untuk memindah waktu, tarik tengah batang untuk memindah,
// tarik ujung batang untuk memotong durasi.
class VideoTimeline : public Control {
	GDCLASS(VideoTimeline, Control);

	enum DragMode {
		DRAG_NONE,
		DRAG_SCRUB,
		DRAG_MOVE,
		DRAG_TRIM_START,
		DRAG_TRIM_END,
	};

	VideoDock *dock = nullptr;
	DragMode mode = DRAG_NONE;
	int drag_row = -1;
	double drag_dx = 0.0;

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	static constexpr float ROW_H = 44.0f;
	static constexpr float TOP = 26.0f;
	static constexpr float HANDLE = 22.0f;

	void set_dock(VideoDock *p_dock) { dock = p_dock; }
};

class VideoDock : public MarginContainer {
	GDCLASS(VideoDock, MarginContainer);

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
	Vector<String> history;
	Vector<String> redo_stack;
	uint64_t last_snap_ms = 0;
	Callable exit_callback;

	Label *status = nullptr;
	SpinBox *f_duration = nullptr;
	VideoPreview *preview = nullptr;
	HSlider *time_slider = nullptr;
	Label *time_label = nullptr;
	Button *play_button = nullptr;
	Timer *play_timer = nullptr;
	Timer *save_timer = nullptr;
	VideoTimeline *timeline = nullptr;
	VBoxContainer *props_box = nullptr;
	Label *no_selection = nullptr;
	Label *f_text_label = nullptr;
	LineEdit *f_text = nullptr;
	CheckBox *f_narrate = nullptr;
	SpinBox *f_start = nullptr;
	SpinBox *f_end = nullptr;
	SpinBox *f_x = nullptr;
	SpinBox *f_y = nullptr;
	SpinBox *f_size = nullptr;
	ColorPickerButton *f_color = nullptr;
	VideoRenderPanel *render_panel = nullptr;

	Button *_make_button(const String &p_text, const Callable &p_action, bool p_primary = false);
	Label *_make_caption(VBoxContainer *p_parent, const String &p_text, bool p_accent = false);
	SpinBox *_make_spin(VBoxContainer *p_parent, const String &p_caption, double p_min, double p_max, double p_step, int p_field);
	void _say(const String &p_text);
	String _to_json() const;
	void _apply_scene(const Dictionary &p_scene);
	void _load_scene();
	void _save_scene();
	void _copy_json();
	void _add_item(int p_kind);
	void _duplicate_item();
	void _delete_item();
	void _refresh();
	void _fill_form();
	void _redraw_all();
	void _schedule_save();
	void _snapshot();
	void _snapshot_soon();
	void _restore(const String &p_json);

	void _on_project_menu(int p_id);
	void _on_undo();
	void _on_redo();
	void _on_exit();
	void _on_render();
	void _on_duration(double p_value);
	void _on_text(const String &p_text);
	void _on_narrate(bool p_pressed);
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
	void begin_edit() { _snapshot(); }
	void set_exit_callback(const Callable &p_callback) { exit_callback = p_callback; }

	VideoDock();
};
