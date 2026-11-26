#include "video_tab_plugin.h"

#include "core/input/input_event.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/object/callable_mp.h"
#include "editor/docks/editor_dock_manager.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/main/timer.h"
#include "scene/resources/font.h"
#include "scene/scene_string_names.h"
#include "servers/display/display_server.h"
#include "video_preview.h"
#include "video_render_panel.h"

static const char *SCENE_PATH = "res://scene.json";

// ---------- Timeline ----------

void VideoTimeline::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW || dock == nullptr) {
		return;
	}
	Ref<Font> font = get_theme_default_font();
	const double dur = MAX(dock->get_duration(), 0.1);
	const float w = get_size().x;

	draw_rect(Rect2(Vector2(), get_size()), Color(0.12, 0.12, 0.14));

	double step = 1.0;
	if (dur > 30.0) {
		step = 5.0;
	} else if (dur > 12.0) {
		step = 2.0;
	}
	for (double t = 0.0; t <= dur; t += step) {
		const float x = w * t / dur;
		draw_line(Vector2(x, 0), Vector2(x, get_size().y), Color(1, 1, 1, 0.08));
		draw_string(font, Vector2(x + 2, 12), itos((int)t), HORIZONTAL_ALIGNMENT_LEFT, -1, 11, Color(1, 1, 1, 0.5));
	}

	Array &items = dock->get_items();
	for (int i = 0; i < items.size(); i++) {
		Dictionary it = items[i];
		const float y = TOP + i * ROW_H;
		const float x1 = w * (double)it.get("start", 0.0) / dur;
		const float x2 = w * (double)it.get("end", dur) / dur;
		Color col = Color(0.3, 0.6, 1.0);
		if (i == dock->get_selected()) {
			col = Color(1.0, 0.8, 0.2);
		}
		draw_rect(Rect2(x1, y, MAX(x2 - x1, 4.0f), ROW_H - 6.0f), col);
		draw_string(font, Vector2(x1 + 4, y + 16), dock->item_label(it), HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(0, 0, 0));
	}

	const float px = w * CLAMP(dock->get_time() / dur, 0.0, 1.0);
	draw_line(Vector2(px, 0), Vector2(px, get_size().y), Color(1.0, 0.3, 0.3), 2.0f);
}

void VideoTimeline::gui_input(const Ref<InputEvent> &p_event) {
	if (dock == nullptr) {
		return;
	}
	const double dur = MAX(dock->get_duration(), 0.1);
	Array &items = dock->get_items();

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT) {
		if (mb->is_pressed()) {
			if (mb->get_position().y < TOP) {
				drag_row = -2;
				dock->set_time(CLAMP(mb->get_position().x / get_size().x * dur, 0.0, dur));
			} else {
				const int row = (int)((mb->get_position().y - TOP) / ROW_H);
				if (row >= 0 && row < items.size()) {
					dock->select_item(row);
					Dictionary it = items[row];
					drag_row = row;
					drag_dx = mb->get_position().x / get_size().x * dur - (double)it.get("start", 0.0);
				}
			}
		} else {
			drag_row = -1;
		}
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && drag_row == -2) {
		dock->set_time(CLAMP(mm->get_position().x / get_size().x * dur, 0.0, dur));
		return;
	}
	if (mm.is_valid() && drag_row >= 0 && drag_row < items.size()) {
		Dictionary it = items[drag_row];
		const double start = it.get("start", 0.0);
		const double length = (double)it.get("end", dur) - start;
		double new_start = CLAMP(mm->get_position().x / get_size().x * dur - drag_dx, 0.0, MAX(dur - length, 0.0));
		new_start = Math::snapped(new_start, 0.1);
		it["start"] = new_start;
		it["end"] = Math::snapped(new_start + length, 0.1);
		dock->timeline_changed();
	}
}

// ---------- Dock ----------

static Label *_form_label(GridContainer *p_form, const String &p_text) {
	Label *l = memnew(Label);
	l->set_text(p_text);
	p_form->add_child(l);
	return l;
}

static Button *_make_button(const String &p_text, const Callable &p_action) {
	Button *b = memnew(Button);
	b->set_text(p_text);
	b->connect(SceneStringName(pressed), p_action);
	return b;
}

SpinBox *VideoDock::_make_spin(double p_min, double p_max, double p_step, int p_field) {
	SpinBox *s = memnew(SpinBox);
	s->set_min(p_min);
	s->set_max(p_max);
	s->set_step(p_step);
	s->set_allow_greater(true);
	s->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	s->connect(SceneStringName(value_changed), callable_mp(this, &VideoDock::_on_num).bind(p_field));
	return s;
}

VideoDock::VideoDock() {
	set_name("Video");
	set_icon_name("Animation");
	set_available_layouts(EditorDock::DOCK_LAYOUT_MAIN_SCREEN | EditorDock::DOCK_LAYOUT_FLOATING);
	set_default_slot(EditorDock::DOCK_SLOT_MAIN_SCREEN);

	meta["duration"] = 10.0;
	meta["background"] = "#101A2B";
	meta["language"] = "id";

	// Seluruh isi tab bisa digulir, supaya muat di layar HP.
	ScrollContainer *page = memnew(ScrollContainer);
	page->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	add_child(page);
	VBoxContainer *root = memnew(VBoxContainer);
	root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	page->add_child(root);

	HFlowContainer *bar = memnew(HFlowContainer);
	root->add_child(bar);
	bar->add_child(_make_button("Muat", callable_mp(this, &VideoDock::_load_scene)));
	bar->add_child(_make_button("Simpan", callable_mp(this, &VideoDock::_save_scene)));
	bar->add_child(_make_button("Salin JSON", callable_mp(this, &VideoDock::_copy_json)));
	bar->add_child(_make_button("+ Teks", callable_mp(this, &VideoDock::_add_item).bind((int)KIND_TEXT)));
	bar->add_child(_make_button("+ Kotak", callable_mp(this, &VideoDock::_add_item).bind((int)KIND_RECT)));
	bar->add_child(_make_button("+ Gambar", callable_mp(this, &VideoDock::_add_item).bind((int)KIND_IMAGE)));
	bar->add_child(_make_button("Hapus", callable_mp(this, &VideoDock::_delete_item)));
	status = memnew(Label);
	bar->add_child(status);

	HBoxContainer *dur_row = memnew(HBoxContainer);
	root->add_child(dur_row);
	Label *dur_label = memnew(Label);
	dur_label->set_text("Durasi (detik)");
	dur_row->add_child(dur_label);
	f_duration = memnew(SpinBox);
	f_duration->set_min(1.0);
	f_duration->set_max(600.0);
	f_duration->set_step(0.5);
	f_duration->connect(SceneStringName(value_changed), callable_mp(this, &VideoDock::_on_duration));
	dur_row->add_child(f_duration);

	preview = memnew(VideoPreview);
	preview->set_dock(this);
	preview->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(preview);

	HBoxContainer *play_row = memnew(HBoxContainer);
	root->add_child(play_row);
	play_button = _make_button("Putar", callable_mp(this, &VideoDock::_on_play));
	play_row->add_child(play_button);
	time_slider = memnew(HSlider);
	time_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	time_slider->set_min(0.0);
	time_slider->set_max(10.0);
	time_slider->set_step(0.05);
	time_slider->connect(SceneStringName(value_changed), callable_mp(this, &VideoDock::_on_slider));
	play_row->add_child(time_slider);
	time_label = memnew(Label);
	play_row->add_child(time_label);

	play_timer = memnew(Timer);
	play_timer->set_wait_time(1.0 / 30.0);
	play_timer->connect("timeout", callable_mp(this, &VideoDock::_on_tick));
	add_child(play_timer);

	ScrollContainer *scroll_t = memnew(ScrollContainer);
	scroll_t->set_custom_minimum_size(Size2(0, 190));
	scroll_t->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	root->add_child(scroll_t);
	timeline = memnew(VideoTimeline);
	timeline->set_dock(this);
	timeline->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_t->add_child(timeline);

	list = memnew(ItemList);
	list->set_custom_minimum_size(Size2(0, 120));
	list->connect(SceneStringName(item_selected), callable_mp(this, &VideoDock::_on_list_selected));
	root->add_child(list);

	form_scroll = memnew(ScrollContainer);
	form_scroll->set_custom_minimum_size(Size2(0, 260));
	form_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	root->add_child(form_scroll);

	GridContainer *form = memnew(GridContainer);
	form->set_columns(2);
	form->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	form_scroll->add_child(form);

	f_text_label = _form_label(form, "Teks");
	f_text = memnew(LineEdit);
	f_text->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	f_text->connect(SceneStringName(text_changed), callable_mp(this, &VideoDock::_on_text));
	form->add_child(f_text);

	_form_label(form, "Mulai (detik)");
	f_start = _make_spin(0.0, 600.0, 0.1, FIELD_START);
	form->add_child(f_start);
	_form_label(form, "Selesai (detik)");
	f_end = _make_spin(0.0, 600.0, 0.1, FIELD_END);
	form->add_child(f_end);
	_form_label(form, "Posisi x");
	f_x = _make_spin(-2000.0, 4000.0, 1.0, FIELD_X);
	form->add_child(f_x);
	_form_label(form, "Posisi y");
	f_y = _make_spin(-2000.0, 4000.0, 1.0, FIELD_Y);
	form->add_child(f_y);
	_form_label(form, "Ukuran huruf");
	f_size = _make_spin(8.0, 400.0, 1.0, FIELD_SIZE);
	form->add_child(f_size);

	_form_label(form, "Warna");
	f_color = memnew(ColorPickerButton);
	f_color->set_custom_minimum_size(Size2(0, 40));
	f_color->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	f_color->connect("color_changed", callable_mp(this, &VideoDock::_on_color));
	form->add_child(f_color);

	root->add_child(memnew(VideoRenderPanel(this)));

	_load_scene();
}

// ---------- Data ----------

double VideoDock::get_duration() const {
	return (double)meta.get("duration", 10.0);
}

Color VideoDock::get_background() const {
	return Color::html(String(meta.get("background", "#000000")));
}

String VideoDock::item_label(const Dictionary &p_item) const {
	const String kind = p_item.get("type", "text");
	if (kind == "text") {
		return String(p_item.get("text", ""));
	}
	if (kind == "image") {
		return "gambar: " + String(p_item.get("src", ""));
	}
	return "kotak";
}

void VideoDock::_say(const String &p_text) {
	status->set_text(p_text);
}

String VideoDock::_to_json() const {
	Dictionary data = meta.duplicate();
	data["items"] = items;
	return JSON::stringify(data, "  ", false);
}

void VideoDock::_load_scene() {
	items.clear();
	selected = -1;
	if (FileAccess::exists(SCENE_PATH)) {
		String raw = FileAccess::get_file_as_string(SCENE_PATH);
		raw = raw.replace("\\[", "[").replace("\\]", "]");
		Variant parsed = JSON::parse_string(raw);
		if (parsed.get_type() == Variant::DICTIONARY) {
			Dictionary d = parsed;
			Array keys = d.keys();
			for (int i = 0; i < keys.size(); i++) {
				if (String(keys[i]) != "items") {
					meta[keys[i]] = d[keys[i]];
				}
			}
			Array arr = d.get("items", Array());
			for (int i = 0; i < arr.size(); i++) {
				if (arr[i].get_type() == Variant::DICTIONARY) {
					items.push_back(arr[i]);
				}
			}
			_say(vformat("Dimuat: %d item", items.size()));
		} else {
			_say("scene.json tidak valid, mulai kosong");
		}
	} else {
		_say("scene.json belum ada, mulai kosong");
	}
	preview->clear_cache();
	f_duration->set_value_no_signal(get_duration());
	time_slider->set_max(get_duration());
	_refresh();
	set_time(0.0);
}

void VideoDock::_save_scene() {
	Ref<FileAccess> f = FileAccess::open(SCENE_PATH, FileAccess::WRITE);
	if (f.is_null()) {
		_say("Gagal menyimpan");
		return;
	}
	f->store_string(_to_json());
	f->close();
	_say("Tersimpan ke scene.json");
}

void VideoDock::_copy_json() {
	DisplayServer::get_singleton()->clipboard_set(_to_json());
	_say("JSON disalin. Tempel di GitHub.");
}

void VideoDock::_add_item(int p_kind) {
	Dictionary it;
	if (p_kind == KIND_RECT) {
		it["type"] = "rect";
		it["x"] = 0;
		it["y"] = 300;
		it["w"] = 1280;
		it["h"] = 6;
		it["color"] = "#FFD400";
		it["start"] = 0.0;
		it["end"] = 3.0;
	} else if (p_kind == KIND_IMAGE) {
		it["type"] = "image";
		it["src"] = "assets/latar.jpg";
		it["x"] = 0;
		it["y"] = 0;
		it["w"] = 1280;
		it["h"] = 720;
		it["start"] = 0.0;
		it["end"] = 5.0;
	} else {
		it["type"] = "text";
		it["text"] = "Teks baru";
		it["x"] = 80;
		it["y"] = 300;
		it["w"] = 1120;
		it["size"] = 52;
		it["color"] = "#FFFFFF";
		it["start"] = 0.0;
		it["end"] = 3.0;
	}
	items.push_back(it);
	_refresh();
	select_item(items.size() - 1);
}

void VideoDock::_delete_item() {
	if (selected < 0 || selected >= items.size()) {
		return;
	}
	items.remove_at(selected);
	selected = -1;
	_refresh();
}

// ---------- Tampilan ----------

void VideoDock::_redraw_all() {
	timeline->queue_redraw();
	preview->queue_redraw();
}

void VideoDock::_refresh() {
	list->clear();
	for (int i = 0; i < items.size(); i++) {
		Dictionary it = items[i];
		list->add_item(vformat("%d. %s", i + 1, item_label(it)));
	}
	timeline->set_custom_minimum_size(Size2(0, VideoTimeline::TOP + items.size() * VideoTimeline::ROW_H + 8));
	if (selected >= items.size()) {
		selected = -1;
	}
	_fill_form();
	_redraw_all();
}

void VideoDock::select_item(int p_index) {
	selected = p_index;
	if (p_index >= 0 && p_index < list->get_item_count()) {
		list->select(p_index);
	}
	_fill_form();
	_redraw_all();
}

void VideoDock::timeline_changed() {
	_fill_form();
	_redraw_all();
}

void VideoDock::set_time(double p_time) {
	current_time = CLAMP(p_time, 0.0, get_duration());
	time_slider->set_value_no_signal(current_time);
	time_label->set_text(vformat("%.1f / %.1f s", current_time, get_duration()));
	_redraw_all();
}

void VideoDock::_fill_form() {
	updating = true;
	const bool ok = selected >= 0 && selected < items.size();
	form_scroll->set_visible(ok);
	if (ok) {
		Dictionary it = items[selected];
		const String kind = it.get("type", "text");
		f_text_label->set_text(kind == "image" ? "File gambar" : "Teks");
		f_text->set_editable(kind != "rect");
		const String text_key = kind == "image" ? "src" : "text";
		f_text->set_text(String(it.get(text_key, "")));
		f_start->set_value_no_signal((double)it.get("start", 0.0));
		f_end->set_value_no_signal((double)it.get("end", get_duration()));
		f_x->set_value_no_signal((double)it.get("x", 0));
		f_y->set_value_no_signal((double)it.get("y", 0));
		f_size->set_value_no_signal((double)it.get("size", 48));
		f_color->set_pick_color(Color::html(String(it.get("color", "#ffffff"))));
	}
	updating = false;
}

// ---------- Perubahan dari formulir ----------

void VideoDock::_on_list_selected(int p_index) {
	select_item(p_index);
}

void VideoDock::_on_duration(double p_value) {
	meta["duration"] = p_value;
	time_slider->set_max(p_value);
	set_time(MIN(current_time, p_value));
}

void VideoDock::_on_text(const String &p_text) {
	if (updating || selected < 0 || selected >= items.size()) {
		return;
	}
	Dictionary it = items[selected];
	const String key = String(it.get("type", "text")) == "image" ? "src" : "text";
	it[key] = p_text;
	list->set_item_text(selected, vformat("%d. %s", selected + 1, item_label(it)));
	if (key == "src") {
		preview->clear_cache();
	}
	_redraw_all();
}

void VideoDock::_on_num(double p_value, int p_field) {
	if (updating || selected < 0 || selected >= items.size()) {
		return;
	}
	static const char *keys[] = { "start", "end", "x", "y", "size" };
	Dictionary it = items[selected];
	it[String(keys[p_field])] = p_value;
	_redraw_all();
}

void VideoDock::_on_color(const Color &p_color) {
	if (updating || selected < 0 || selected >= items.size()) {
		return;
	}
	Dictionary it = items[selected];
	it["color"] = "#" + p_color.to_html(false);
	_redraw_all();
}

void VideoDock::_on_slider(double p_value) {
	set_time(p_value);
}

void VideoDock::_on_play() {
	if (play_timer->is_stopped()) {
		if (current_time >= get_duration() - 0.05) {
			set_time(0.0);
		}
		play_timer->start();
		play_button->set_text("Jeda");
	} else {
		play_timer->stop();
		play_button->set_text("Putar");
	}
}

void VideoDock::_on_tick() {
	set_time(current_time + 1.0 / 30.0);
	if (current_time >= get_duration()) {
		play_timer->stop();
		play_button->set_text("Putar");
	}
}

VideoTabPlugin::VideoTabPlugin() {
	dock = memnew(VideoDock);
	EditorDockManager::get_singleton()->add_dock(dock);
}
