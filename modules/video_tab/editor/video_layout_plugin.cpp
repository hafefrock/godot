#include "video_layout_plugin.h"

#include "core/object/callable_mp.h"
#include "core/string/print_string.h"
#include "editor/docks/editor_dock_manager.h"
#include "scene/main/scene_tree.h"

void VideoLayoutPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		// Tunggu sebentar supaya semua dock dan layout tersimpan selesai dimuat.
		get_tree()->create_timer(1.5)->connect("timeout", callable_mp(this, &VideoLayoutPlugin::_apply_layout));
	}
}

void VideoLayoutPlugin::_apply_layout() {
	EditorDockManager *dm = EditorDockManager::get_singleton();
	if (dm == nullptr) {
		return;
	}
	static const char *hidden[] = { "2D", "3D", "Script", "Game", "Asset Store", "AssetLib" };
	String closed;
	for (const char *name : hidden) {
		EditorDock *d = dm->get_dock_by_name(name);
		if (d != nullptr) {
			dm->close_dock(d);
			closed += String(name) + " ";
		}
	}
	EditorDock *video = dm->get_dock_by_name("Video");
	if (video != nullptr) {
		dm->open_dock(video);
		dm->force_focus_dock(video);
	}
	print_line("[Video] tab disembunyikan: " + closed + "| Video ditemukan: " + String(video != nullptr ? "ya" : "tidak"));
}
