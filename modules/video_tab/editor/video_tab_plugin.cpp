#include "video_tab_plugin.h"

#include "core/input/input_event.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/main/timer.h"
#include "scene/resources/font.h"
#include "scene/scene_string_names.h"
#include "servers/display/display_server.h"
#include "video_preview.h"
#include "video_render_panel.h"
#include "video_style.h"

static const char *SCENE_PATH = "res://scene.json";

// ---------- Timeline ----------

void VideoTimeline::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW || dock == nullptr) {
		return;
	}
	Ref<Font> font = get_theme_default_font();
	const double dur = MAX(dock->get_duration(), 0.1);
	const float w = get_size().x;

	draw_rect(Rect2(Vector2(), get_size()), Color(0.09, 0.10, 0.12));

	double step = 1.0;
	if (dur > 30.0) {
		step = 5.0;
	} else if (dur > 12.0) {
		step = 2.0;
	}
	for (double t = 0.0; t <= dur; t += step) {
		const float x = w * t / dur;
		draw_line(Vector2(x, 0), Vector2(x, get_size().y), Color(1, 1, 1, 0.07));
		draw_string(font, Vector2(x + 3, 17), itos((int)t) + "s", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(1, 1, 1, 0.55));
	}

	Array &items = dock->get_items();
	for (int i = 0; i < items.size(); i++) {
		Dictionary it = items[i];
		const float y = TOP + i * ROW_H;
		const float x1 = w * (double)it.get("start", 0.0) / dur;
		const float x2 = w * (double)it.get("end", dur) / dur;
		const Rect2 r(x1, y + 3.0f, MAX(x2 - x1, 10.0f), ROW_H - 8.0f);
		Color col = Color(0.30, 0.60, 1.0);
		const String kind = it.get("type", "text");
		if (kind == "rect") {
			col = Color(0.55, 0.45, 0.95);
		} else if (kind == "image") {
			col = Color(0.30, 0.78, 0.55);
		}
		if (i == dock->get_selected()) {
			col = Color(1.0, 0.83, 0.0);
		}
		draw_rect(r, col);
		if (i == dock->get_selected()) {
			draw_rect(Rect2(r.position.x, r.position.y, 8.0f, r.size.y), Color(0, 0, 0, 0.35));
			draw_rect(Rect2(r.position.x + r.size.x - 8.0f, r.position.y, 8.0f, r.size.y), Color(0, 0, 0, 0.35));
		}
		draw_string(font, Vector2(r.position.x + 12.0f, y + 29.0f), dock->item_label(it), HORIZONTAL_ALIGNMENT_LEFT, MAX(r.size.x - 20.0f, 10.0f), 16, Color(0, 0, 0));
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
			accept_event();
			const Vector2 pos = mb->get_position();
			if (pos.y < TOP) {
				mode = DRAG_SCRUB;
				dock->set_time(CLAMP(pos.x / get_size().x * dur, 0.0, dur));
				return;
			}
			const int row = (int)((pos.y - TOP) / ROW_H);
			if (row < 0 || row >= items.size()) {
				dock->select_item(-1);
				return;
			}
			Dictionary it = items[row];
			const float x1 = get_size().x * (double)it.get("start", 0.0) / dur;
			const float x2 = get_size().x * (double)it.get("end", dur) / dur;
			dock->select_item(row);
			drag_row = row;
			const double tpos = pos.x / get_size().x * dur;
			if (pos.x <= x1 + HANDLE) {
				mode = DRAG_TRIM_START;
			} else if (pos.x >= x2 - HANDLE) {
				mode = DRAG_TRIM_END;
			} else {
				mode = DRAG_MOVE;
				drag_dx = tpos - (double)it.get("start", 0.0);
			}
			dock->begin_edit();
		} else {
			mode = DRAG_NONE;
		}
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && mode != DRAG_NONE) {
		accept_event();
		const double tpos = CLAMP(mm->get_position().x / get_size().x * dur, 0.0, dur);
		if (mode == DRAG_SCRUB) {
			dock->set_time(tpos);
			return;
		}
		if (drag_row < 0 || drag_row >= items.size()) {
			return;
		}
		Dictionary it = items[drag_row];
		const double t_start = it.get("start", 0.0);
		const double t_end = it.get("end", dur);
		if (mode == DRAG_MOVE) {
			const double length = t_end - t_start;
			const double new_start = CLAMP(tpos - drag_dx, 0.0, MAX(dur - length, 0.0));
			it["start"] = Math::snapped(new_start, 0.1);
			it["end"] = Math::snapped(new_start + length, 0.1);
		} else if (mode == DRAG_TRIM_START) {
			it["start"] = Math::snapped(MIN(tpos, t_end - 0.2), 0.1);
		} else if (mode == DRAG_TRIM_END) {
			it["end"] = Math::snapped(MAX(tpos, t_start + 0.2), 0.1);
		}
		dock->timeline_changed();
	}
}

// ---------- Dock ----------

Button *VideoDock::_make_button(const String &p_text, const Callable &p_action, bool p_primary) {
	Button *b = memnew(Button);
	b->set_text(p_text);
	b->set_custom_minimum_size(Size2(0, 52));
	b->connect(SceneStringName(pressed), p_action);
	if (p_primary) {
		VideoStyle::style_primary(b);
	}
	return b;
}

Label *VideoDock::_make_caption(VBoxContainer *p_parent, const String &p_text, bool p_accent) {
	Label *l = memnew(Label);
	l->set_text(p_text);
	l->add_theme_color_override("font_color", p_accent ? VideoStyle::accent() : VideoStyle::muted());
	p_parent->add_child(l);
	return l;
}

SpinBox *VideoDock::_make_spin(VBoxContainer *p_parent, const String &p_caption, double p_min, double p_max, double p_step, int p_field) {
	_make_caption(p_parent, p_caption);
	SpinBox *s = memnew(SpinBox);
	s->set_min(p_min);
	s->set_max(p_max);
	s->set_step(p_step);
	s->set_allow_greater(true);
	s->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	s->connect(SceneStringName(value_changed), callable_mp(this, &VideoDock::_on_num).bind(p_field));
	p_parent->add_child(s);
	return s;
}

VideoDock::VideoDock() {
	set_name("Video");
	meta["duration"] = 10.0;
	meta["background"] = "#101A2B";
	meta["language"] = "id";

	add_theme_constant_override("margin_left", 10);
	add_theme_constant_override("margin_right", 10);
	add_theme_constant_override("margin_top", 10);
	add_theme_constant_override("margin_bottom", 10);

	VBoxContainer *root = memnew(VBoxContainer);
	root->add_theme_constant_override("separation", 8);
	add_child(root);

	// Bilah atas.
	HBoxContainer *top = memnew(HBoxContainer);
	top->add_theme_constant_override("separation", 8);
	root->add_child(top);
	top->add_child(_make_button("Editor", callable_mp(this, &VideoDock::_on_exit)));
	Label *title = memnew(Label);
	title->set_text("Video AI");
	title->add_theme_color_override("font_color", VideoStyle::accent());
	title->add_theme_font_size_override("font_size", 28);
	title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	top->add_child(title);
	MenuButton *project_menu = memnew(MenuButton);
	project_menu->set_text("Proyek");
	project_menu->set_custom_minimum_size(Size2(0, 52));
	project_menu->get_popup()->add_item("Muat scene.json", 0);
	project_menu->get_popup()->add_item("Simpan", 1);
	project_menu->get_popup()->add_item("Salin JSON", 2);
	project_menu->get_popup()->connect("id_pressed", callable_mp(this, &VideoDock::_on_project_menu));
	top->add_child(project_menu);
	top->add_child(_make_button("Urung", callable_mp(this, &VideoDock::_on_undo)));
	top->add_child(_make_button("Ulang", callable_mp(this, &VideoDock::_on_redo)));
	top->add_child(_make_button("Render", callable_mp(this, &VideoDock::_on_render), true));

	// Isi: kiri (pratinjau, waktu, timeline, alat) dan kanan (panel).
	HBoxContainer *body = memnew(HBoxContainer);
	body->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	body->add_theme_constant_override("separation", 10);
	root->add_child(body);

	VBoxContainer *left = memnew(VBoxContainer);
	left->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	left->add_theme_constant_override("separation", 8);
	body->add_child(left);

	preview = memnew(VideoPreview);
	preview->set_dock(this);
	preview->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	left->add_child(preview);

	HBoxContainer *transport = memnew(HBoxContainer);
	transport->add_theme_constant_override("separation", 8);
	left->add_child(transport);
	play_button = _make_button("Putar", callable_mp(this, &VideoDock::_on_play), true);
	play_button->set_custom_minimum_size(Size2(130, 48));
	transport->add_child(play_button);
	time_slider = memnew(HSlider);
	time_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	time_slider->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	time_slider->set_min(0.0);
	time_slider->set_max(10.0);
	time_slider->set_step(0.05);
	time_slider->connect(SceneStringName(value_changed), callable_mp(this, &VideoDock::_on_slider));
	transport->add_child(time_slider);
	time_label = memnew(Label);
	time_label->add_theme_color_override("font_color", VideoStyle::muted());
	transport->add_child(time_label);

	ScrollContainer *scroll_t = memnew(ScrollContainer);
	scroll_t->set_custom_minimum_size(Size2(0, 160));
	scroll_t->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	left->add_child(scroll_t);
	timeline = memnew(VideoTimeline);
	timeline->set_dock(this);
	timeline->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_t->add_child(timeline);

	HBoxContainer *tools = memnew(HBoxContainer);
	tools->add_theme_constant_override("separation", 8);
	left->add_child(tools);
	Button *b_text = _make_button("+ Teks", callable_mp(this, &VideoDock::_add_item).bind((int)KIND_TEXT));
	b_text->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tools->add_child(b_text);
	Button *b_rect = _make_button("+ Kotak", callable_mp(this, &VideoDock::_add_item).bind((int)KIND_RECT));
	b_rect->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tools->add_child(b_rect);
	Button *b_image = _make_button("+ Gambar", callable_mp(this, &VideoDock::_add_item).bind((int)KIND_IMAGE));
	b_image->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tools->add_child(b_image);
	Button *b_dup = _make_button("Duplikat", callable_mp(this, &VideoDock::_duplicate_item));
	b_dup->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tools->add_child(b_dup);
	Button *b_del = _make_button("Hapus", callable_mp(this, &VideoDock::_delete_item));
	b_del->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tools->add_child(b_del);

	// Panel kanan.
	PanelContainer *sheet = memnew(PanelContainer);
	sheet->set_custom_minimum_size(Size2(400, 0));
	body->add_child(sheet);
	ScrollContainer *scroll = memnew(ScrollContainer);
	scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	sheet->add_child(scroll);
	VBoxContainer *box = memnew(VBoxContainer);
	box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	box->add_theme_constant_override("separation", 8);
	scroll->add_child(box);

	_make_caption(box, "Proyek", true);
	_make_caption(box, "Durasi (detik)");
	f_duration = memnew(SpinBox);
	f_duration->set_min(1.0);
	f_duration->set_max(600.0);
	f_duration->set_step(0.5);
	f_duration->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	f_duration->connect(SceneStringName(value_changed), callable_mp(this, &VideoDock::_on_duration));
	box->add_child(f_duration);
	box->add_child(memnew(HSeparator));

	_make_caption(box, "Item terpilih", true);
	no_selection = _make_caption(box, "Ketuk sebuah batang di timeline.");
	props_box = memnew(VBoxContainer);
	props_box->add_theme_constant_override("separation", 8);
	box->add_child(props_box);

	f_text_label = _make_caption(props_box, "Teks");
	f_text = memnew(LineEdit);
	f_text->connect(SceneStringName(text_changed), callable_mp(this, &VideoDock::_on_text));
	props_box->add_child(f_text);
	f_narrate = memnew(CheckBox);
	f_narrate->set_text("Dibacakan (narasi)");
	f_narrate->connect("toggled", callable_mp(this, &VideoDock::_on_narrate));
	props_box->add_child(f_narrate);
	f_start = _make_spin(props_box, "Mulai (detik)", 0.0, 600.0, 0.1, FIELD_START);
	f_end = _make_spin(props_box, "Selesai (detik)", 0.0, 600.0, 0.1, FIELD_END);
	f_x = _make_spin(props_box, "Posisi x", -2000.0, 4000.0, 1.0, FIELD_X);
	f_y = _make_spin(props_box, "Posisi y", -2000.0, 4000.0, 1.0, FIELD_Y);
	f_size = _make_spin(props_box, "Ukuran huruf", 8.0, 400.0, 1.0, FIELD_SIZE);
	_make_caption(props_box, "Warna");
	f_color = memnew(ColorPickerButton);
	f_color->set_custom_minimum_size(Size2(0, 48));
	f_color->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	f_color->connect("color_changed", callable_mp(this, &VideoDock::_on_color));
	props_box->add_child(f_color);

	box->add_child(memnew(HSeparator));
	render_panel = memnew(VideoRenderPanel(this));
	box->add_child(render_panel);

	// Status.
	status = memnew(Label);
	status->add_theme_color_override("font_color", VideoStyle::muted());
	root->add_child(status);

	// Pewaktu: putar dan simpan otomatis.
	play_timer = memnew(Timer);
	play_timer->set_wait_time(1.0 / 30.0);
	play_timer->connect("timeout", callable_mp(this, &VideoDock::_on_tick));
	add_child(play_timer);
	save_timer = memnew(Timer);
	save_timer->set_one_shot(true);
	save_timer->set_wait_time(1.0);
	save_timer->connect("timeout", callable_mp(this, &VideoDock::_save_scene));
	add_child(save_timer);

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

void VideoDock::_apply_scene(const Dictionary &p_scene) {
	items.clear();
	meta.clear();
	meta["duration"] = 10.0;
	meta["background"] = "#101A2B";
	meta["language"] = "id";
	Array keys = p_scene.keys();
	for (int i = 0; i < keys.size(); i++) {
		if (String(keys[i]) != "items") {
			meta[keys[i]] = p_scene[keys[i]];
		}
	}
	Array arr = p_scene.get("items", Array());
	for (int i = 0; i < arr.size(); i++) {
		if (arr[i].get_type() == Variant::DICTIONARY) {
			items.push_back(arr[i]);
		}
	}
	selected = -1;
	preview->clear_cache();
	f_duration->set_value_no_signal(get_duration());
	time_slider->set_max(get_duration());
	_refresh();
	set_time(MIN(current_time, get_duration()));
}

void VideoDock::_load_scene() {
	if (FileAccess::exists(SCENE_PATH)) {
		String raw = FileAccess::get_file_as_string(SCENE_PATH);
		raw = raw.replace("\\[", "[").replace("\\]", "]");
		Variant parsed = JSON::parse_string(raw);
		if (parsed.get_type() == Variant::DICTIONARY) {
			_apply_scene(parsed);
			_say(vformat("Dimuat: %d item", items.size()));
			return;
		}
		_say("scene.json tidak valid, mulai kosong");
	} else {
		_say("scene.json belum ada, mulai kosong");
	}
	_apply_scene(Dictionary());
}

void VideoDock::_save_scene() {
	Ref<FileAccess> f = FileAccess::open(SCENE_PATH, FileAccess::WRITE);
	if (f.is_null()) {
		_say("Gagal menyimpan");
		return;
	}
	f->store_string(_to_json());
	f->close();
	_say("Tersimpan");
}

void VideoDock::_copy_json() {
	DisplayServer::get_singleton()->clipboard_set(_to_json());
	_say("JSON disalin.");
}

void VideoDock::_schedule_save() {
	save_timer->start();
}

// ---------- Urung / ulang ----------

void VideoDock::_snapshot() {
	history.push_back(_to_json());
	if (history.size() > 60) {
		history.remove_at(0);
	}
	redo_stack.clear();
}

void VideoDock::_snapshot_soon() {
	const uint64_t now = OS::get_singleton()->get_ticks_msec();
	if (now - last_snap_ms > 800) {
		_snapshot();
	}
	last_snap_ms = now;
}

void VideoDock::_restore(const String &p_json) {
	Variant parsed = JSON::parse_string(p_json);
	if (parsed.get_type() == Variant::DICTIONARY) {
		_apply_scene(parsed);
		_schedule_save();
	}
}

void VideoDock::_on_undo() {
	if (history.is_empty()) {
		return;
	}
	redo_stack.push_back(_to_json());
	const String prev = history[history.size() - 1];
	history.remove_at(history.size() - 1);
	_restore(prev);
}

void VideoDock::_on_redo() {
	if (redo_stack.is_empty()) {
		return;
	}
	history.push_back(_to_json());
	const String next = redo_stack[redo_stack.size() - 1];
	redo_stack.remove_at(redo_stack.size() - 1);
	_restore(next);
}

// ---------- Item ----------

void VideoDock::_add_item(int p_kind) {
	_snapshot();
	const double start = Math::snapped(current_time, 0.1);
	const double end = Math::snapped(MIN(current_time + 3.0, get_duration()), 0.1);
	Dictionary it;
	if (p_kind == KIND_RECT) {
		it["type"] = "rect";
		it["x"] = 0;
		it["y"] = 300;
		it["w"] = 1280;
		it["h"] = 6;
		it["color"] = "#FFD400";
	} else if (p_kind == KIND_IMAGE) {
		it["type"] = "image";
		it["src"] = "assets/latar.jpg";
		it["x"] = 0;
		it["y"] = 0;
		it["w"] = 1280;
		it["h"] = 720;
	} else {
		it["type"] = "text";
		it["text"] = "Teks baru";
		it["x"] = 80;
		it["y"] = 300;
		it["w"] = 1120;
		it["size"] = 52;
		it["color"] = "#FFFFFF";
	}
	it["start"] = start;
	it["end"] = end;
	items.push_back(it);
	_refresh();
	select_item(items.size() - 1);
	_schedule_save();
}

void VideoDock::_duplicate_item() {
	if (selected < 0 || selected >= items.size()) {
		return;
	}
	_snapshot();
	Dictionary src = items[selected];
	items.push_back(src.duplicate(true));
	_refresh();
	select_item(items.size() - 1);
	_schedule_save();
}

void VideoDock::_delete_item() {
	if (selected < 0 || selected >= items.size()) {
		return;
	}
	_snapshot();
	items.remove_at(selected);
	selected = -1;
	_refresh();
	_schedule_save();
}

// ---------- Tampilan ----------

void VideoDock::_redraw_all() {
	timeline->queue_redraw();
	preview->queue_redraw();
}

void VideoDock::_refresh() {
	timeline->set_custom_minimum_size(Size2(0, VideoTimeline::TOP + items.size() * VideoTimeline::ROW_H + 12));
	if (selected >= items.size()) {
		selected = -1;
	}
	_fill_form();
	_redraw_all();
}

void VideoDock::select_item(int p_index) {
	selected = p_index;
	_fill_form();
	_redraw_all();
}

void VideoDock::timeline_changed() {
	_fill_form();
	_redraw_all();
	_schedule_save();
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
	props_box->set_visible(ok);
	no_selection->set_visible(!ok);
	if (ok) {
		Dictionary it = items[selected];
		const String kind = it.get("type", "text");
		f_text_label->set_text(kind == "image" ? "File gambar" : "Teks");
		f_text->set_editable(kind != "rect");
		const String text_key = kind == "image" ? "src" : "text";
		f_text->set_text(String(it.get(text_key, "")));
		f_narrate->set_visible(kind == "text");
		f_narrate->set_pressed_no_signal((bool)it.get("narrate", false));
		f_start->set_value_no_signal((double)it.get("start", 0.0));
		f_end->set_value_no_signal((double)it.get("end", get_duration()));
		f_x->set_value_no_signal((double)it.get("x", 0));
		f_y->set_value_no_signal((double)it.get("y", 0));
		f_size->set_value_no_signal((double)it.get("size", 48));
		f_color->set_pick_color(Color::html(String(it.get("color", "#ffffff"))));
	}
	updating = false;
}

// ---------- Perubahan dari panel ----------

void VideoDock::_on_project_menu(int p_id) {
	if (p_id == 0) {
		_load_scene();
	} else if (p_id == 1) {
		_save_scene();
	} else if (p_id == 2) {
		_copy_json();
	}
}

void VideoDock::_on_exit() {
	_save_scene();
	if (exit_callback.is_valid()) {
		exit_callback.call();
	}
}

void VideoDock::_on_render() {
	_save_scene();
	render_panel->request_render();
}

void VideoDock::_on_duration(double p_value) {
	_snapshot_soon();
	meta["duration"] = p_value;
	time_slider->set_max(p_value);
	set_time(MIN(current_time, p_value));
	_schedule_save();
}

void VideoDock::_on_text(const String &p_text) {
	if (updating || selected < 0 || selected >= items.size()) {
		return;
	}
	_snapshot_soon();
	Dictionary it = items[selected];
	const String key = String(it.get("type", "text")) == "image" ? "src" : "text";
	it[key] = p_text;
	if (key == "src") {
		preview->clear_cache();
	}
	_redraw_all();
	_schedule_save();
}

void VideoDock::_on_narrate(bool p_pressed) {
	if (updating || selected < 0 || selected >= items.size()) {
		return;
	}
	_snapshot();
	Dictionary it = items[selected];
	it["narrate"] = p_pressed;
	_schedule_save();
}

void VideoDock::_on_num(double p_value, int p_field) {
	if (updating || selected < 0 || selected >= items.size()) {
		return;
	}
	_snapshot_soon();
	static const char *keys[] = { "start", "end", "x", "y", "size" };
	Dictionary it = items[selected];
	it[String(keys[p_field])] = p_value;
	_redraw_all();
	_schedule_save();
}

void VideoDock::_on_color(const Color &p_color) {
	if (updating || selected < 0 || selected >= items.size()) {
		return;
	}
	_snapshot_soon();
	Dictionary it = items[selected];
	it["color"] = "#" + p_color.to_html(false);
	_redraw_all();
	_schedule_save();
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
