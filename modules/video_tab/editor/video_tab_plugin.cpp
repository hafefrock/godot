#include "video_tab_plugin.h"

#include "core/object/callable_mp.h"
#include "editor/docks/editor_dock_manager.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/scene_string_names.h"

void VideoDock::_on_button_pressed() {
	counter++;
	info->set_text(vformat("Tombol ditekan %d kali", counter));
}

VideoDock::VideoDock() {
	set_name("Video");
	set_icon_name("Animation");
	set_available_layouts(EditorDock::DOCK_LAYOUT_MAIN_SCREEN | EditorDock::DOCK_LAYOUT_FLOATING);
	set_default_slot(EditorDock::DOCK_SLOT_MAIN_SCREEN);

	VBoxContainer *vb = memnew(VBoxContainer);
	add_child(vb);

	Label *title = memnew(Label);
	title->set_text("Editor Video (v0.1)");
	vb->add_child(title);

	info = memnew(Label);
	info->set_text("Tab ini dibuat di dalam mesin Godot (modul video_tab).");
	vb->add_child(info);

	Button *button = memnew(Button);
	button->set_text("Tes tombol");
	button->connect(SceneStringName(pressed), callable_mp(this, &VideoDock::_on_button_pressed));
	vb->add_child(button);
}

VideoTabPlugin::VideoTabPlugin() {
	dock = memnew(VideoDock);
	EditorDockManager::get_singleton()->add_dock(dock);
}
