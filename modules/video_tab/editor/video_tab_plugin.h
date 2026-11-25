#pragma once

#include "editor/docks/editor_dock.h"
#include "editor/plugins/editor_plugin.h"

class Label;

class VideoDock : public EditorDock {
	GDCLASS(VideoDock, EditorDock);

	Label *info = nullptr;
	int counter = 0;

	void _on_button_pressed();

public:
	VideoDock();
};

class VideoTabPlugin : public EditorPlugin {
	GDCLASS(VideoTabPlugin, EditorPlugin);

	VideoDock *dock = nullptr;

public:
	virtual String get_plugin_name() const override { return "Video"; }

	VideoTabPlugin();
};
