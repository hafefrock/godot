#include "register_types.h"

#ifdef TOOLS_ENABLED
#include "core/object/class_db.h"
#include "editor/video_tab_plugin.h"
#endif

void initialize_video_tab_module(ModuleInitializationLevel p_level) {
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_VIRTUAL_CLASS(VideoTabPlugin);
		EditorPlugins::add_by_type<VideoTabPlugin>();
	}
#endif
}

void uninitialize_video_tab_module(ModuleInitializationLevel p_level) {
}
