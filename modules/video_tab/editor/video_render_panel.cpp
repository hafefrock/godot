#include "video_render_panel.h"

#include "core/crypto/crypto_core.h"
#include "core/io/config_file.h"
#include "core/io/json.h"
#include "core/object/callable_mp.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/main/http_request.h"
#include "scene/scene_string_names.h"
#include "video_tab_plugin.h"

static const char *SETTINGS_PATH = "user://video_tab.cfg";
static const char *BRANCH = "main";
static const char *WORKFLOW = "render.yml";
static const char *DEFAULT_REPO = "hafefrock/godot-render";

static String _text_of(const PackedByteArray &p_body) {
	if (p_body.is_empty()) {
		return String();
	}
	return String::utf8((const char *)p_body.ptr(), p_body.size());
}

VideoRenderPanel::VideoRenderPanel(VideoDock *p_dock) {
	dock = p_dock;

	Label *title = memnew(Label);
	title->set_text("Render lewat GitHub");
	add_child(title);

	token = memnew(LineEdit);
	token->set_secret(true);
	token->set_placeholder("Token GitHub (hanya disimpan di HP ini)");
	token->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(token);

	repo = memnew(LineEdit);
	repo->set_text(DEFAULT_REPO);
	repo->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(repo);

	HBoxContainer *row = memnew(HBoxContainer);
	add_child(row);
	narration = memnew(CheckBox);
	narration->set_text("Suara narasi");
	narration->set_pressed(true);
	row->add_child(narration);
	Button *send = memnew(Button);
	send->set_text("Kirim & Render");
	send->connect(SceneStringName(pressed), callable_mp(this, &VideoRenderPanel::_on_send));
	row->add_child(send);

	info = memnew(Label);
	add_child(info);

	http = memnew(HTTPRequest);
	add_child(http);
	http->connect("request_completed", callable_mp(this, &VideoRenderPanel::_on_done));

	_load_settings();
}

void VideoRenderPanel::_load_settings() {
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	if (cfg->load(SETTINGS_PATH) == OK) {
		token->set_text(String(cfg->get_value("github", "token", "")));
		repo->set_text(String(cfg->get_value("github", "repo", DEFAULT_REPO)));
	}
}

void VideoRenderPanel::_save_settings() {
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	cfg->set_value("github", "token", token->get_text().strip_edges());
	cfg->set_value("github", "repo", repo->get_text().strip_edges());
	cfg->save(SETTINGS_PATH);
}

void VideoRenderPanel::_say(const String &p_text) {
	info->set_text(p_text);
}

PackedStringArray VideoRenderPanel::_headers(bool p_json_body) const {
	PackedStringArray h;
	h.push_back("Authorization: Bearer " + token->get_text().strip_edges());
	h.push_back("Accept: application/vnd.github+json");
	h.push_back("X-GitHub-Api-Version: 2022-11-28");
	h.push_back("User-Agent: godot-video-tab");
	if (p_json_body) {
		h.push_back("Content-Type: application/json");
	}
	return h;
}

String VideoRenderPanel::_contents_url() const {
	return "https://api.github.com/repos/" + repo->get_text().strip_edges() + "/contents/scene.json";
}

void VideoRenderPanel::_fail(const String &p_text, int p_code, const PackedByteArray &p_body) {
	step = STEP_NONE;
	String msg;
	Variant v = JSON::parse_string(_text_of(p_body));
	if (v.get_type() == Variant::DICTIONARY) {
		Dictionary d = v;
		msg = String(d.get("message", ""));
	}
	_say(vformat("%s (kode %d). %s", p_text, p_code, msg));
}

void VideoRenderPanel::_on_send() {
	if (step != STEP_NONE) {
		_say("Masih berjalan, tunggu sebentar.");
		return;
	}
	if (token->get_text().strip_edges().is_empty()) {
		_say("Isi token GitHub dulu.");
		return;
	}
	_save_settings();
	payload_json = dock->get_scene_json();
	sha = "";
	step = STEP_GET_SHA;
	_say("1/3 Memeriksa scene.json di repo...");
	Error err = http->request(_contents_url() + "?ref=" + String(BRANCH), _headers(false), HTTPClient::METHOD_GET);
	if (err != OK) {
		step = STEP_NONE;
		_say("Tidak bisa memulai permintaan.");
	}
}

void VideoRenderPanel::_on_done(int p_result, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_body) {
	if (p_result != HTTPRequest::RESULT_SUCCESS) {
		step = STEP_NONE;
		_say(vformat("Gagal terhubung (kode %d). Periksa internet.", p_result));
		return;
	}
	switch (step) {
		case STEP_GET_SHA: {
			if (p_code == 200) {
				Variant v = JSON::parse_string(_text_of(p_body));
				if (v.get_type() == Variant::DICTIONARY) {
					Dictionary d = v;
					sha = String(d.get("sha", ""));
				}
			} else if (p_code != 404) {
				_fail("Gagal membaca repo", p_code, p_body);
				return;
			}
			const CharString utf8 = payload_json.utf8();
			Dictionary body;
			body["message"] = "Update scene.json dari editor Video";
			body["content"] = CryptoCore::b64_encode_str((const uint8_t *)utf8.get_data(), utf8.length());
			body["branch"] = String(BRANCH);
			if (!sha.is_empty()) {
				body["sha"] = sha;
			}
			step = STEP_PUT;
			_say("2/3 Mengirim scene.json...");
			http->request(_contents_url(), _headers(true), HTTPClient::METHOD_PUT, JSON::stringify(body));
		} break;
		case STEP_PUT: {
			if (p_code != 200 && p_code != 201) {
				_fail("Gagal mengirim scene.json", p_code, p_body);
				return;
			}
			Dictionary inputs;
			inputs["story"] = "";
			inputs["narration"] = String(narration->is_pressed() ? "ya" : "tidak");
			inputs["scene"] = "res://scene.json";
			Dictionary body;
			body["ref"] = String(BRANCH);
			body["inputs"] = inputs;
			step = STEP_DISPATCH;
			_say("3/3 Menjalankan render...");
			const String url = "https://api.github.com/repos/" + repo->get_text().strip_edges() + "/actions/workflows/" + String(WORKFLOW) + "/dispatches";
			http->request(url, _headers(true), HTTPClient::METHOD_POST, JSON::stringify(body));
		} break;
		case STEP_DISPATCH: {
			if (p_code == 204) {
				step = STEP_NONE;
				_say("Render dimulai. Hasilnya ada di tab Actions repo.");
			} else {
				_fail("Gagal menjalankan render", p_code, p_body);
			}
		} break;
		default:
			break;
	}
}
