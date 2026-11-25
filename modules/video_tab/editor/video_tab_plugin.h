#pragma once

#include "editor/docks/editor_dock.h"
#include "editor/plugins/editor_plugin.h"

class ColorPickerButton;
class GridContainer;
class ItemList;
class Label;
class LineEdit;
class ScrollContainer;
class SpinBox;
class VideoDock;

// Timeline: satu baris per item, batang bisa digeser untuk memindah waktu tampil.
class VideoTimeline : public Control {
	GDCLASS(VideoTimeline, Control);

	VideoDock *dock = nullptr;
	int drag_row = -1;
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

	Label *status = nullptr;
	SpinBox *f_duration = nullptr;
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

	void _on_list_selected(int p_index);
	void _on_duration(double p_value);
	void _on_text(const String &p_text);
	void _on_num(double p_value, int p_field);
	void _on_color(const Color &p_color);

public:
	Array &get_items() { return items; }
	double get_duration() const;
	int get_selected() const { return selected; }
	String item_label(const Dictionary &p_item) const;
	void select_item(int p_index);
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
