#pragma once

#include "core/templates/hash_map.h"
#include "scene/gui/control.h"
#include "scene/resources/texture.h"

class VideoDock;

// Pratinjau adegan pada waktu yang dipilih. Menggambar teks, kotak, dan gambar
// dengan rumus gerak, fade, dan zoom yang sama seperti saat render.
class VideoPreview : public Control {
	GDCLASS(VideoPreview, Control);

	VideoDock *dock = nullptr;
	HashMap<String, Ref<Texture2D>> textures;

	Ref<Texture2D> _get_texture(const String &p_src);

protected:
	void _notification(int p_what);

public:
	void set_dock(VideoDock *p_dock) { dock = p_dock; }
	void clear_cache() { textures.clear(); }

	VideoPreview();
};
