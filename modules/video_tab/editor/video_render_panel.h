#pragma once

#include "scene/gui/box_container.h"

class CheckBox;
class HTTPRequest;
class Label;
class LineEdit;
class VideoDock;

// Panel "Render lewat GitHub": mengirim scene.json ke repo lalu menjalankan workflow render.
class VideoRenderPanel : public VBoxContainer {
	GDCLASS(VideoRenderPanel, VBoxContainer);

	enum Step {
		STEP_NONE,
		STEP_GET_SHA,
		STEP_PUT,
		STEP_DISPATCH,
	};

	VideoDock *dock = nullptr;
	HTTPRequest *http = nullptr;
	LineEdit *token = nullptr;
	LineEdit *repo = nullptr;
	CheckBox *narration = nullptr;
	Label *info = nullptr;
	Step step = STEP_NONE;
	String sha;
	String payload_json;

	PackedStringArray _headers(bool p_json_body) const;
	String _contents_url() const;
	void _load_settings();
	void _save_settings();
	void _say(const String &p_text);
	void _fail(const String &p_text, int p_code, const PackedByteArray &p_body);
	void _on_send();
	void _on_done(int p_result, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_body);

public:
	// Dipanggil dari tombol Render di bilah atas.
	void request_render() { _on_send(); }

	VideoRenderPanel(VideoDock *p_dock);
};
