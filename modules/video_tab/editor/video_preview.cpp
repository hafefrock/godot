#include "video_preview.h"

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "scene/resources/font.h"
#include "scene/resources/image_texture.h"
#include "video_tab_plugin.h"

VideoPreview::VideoPreview() {
	set_clip_contents(true);
	set_custom_minimum_size(Size2(0, 240));
}

Ref<Texture2D> VideoPreview::_get_texture(const String &p_src) {
	if (textures.has(p_src)) {
		return textures[p_src];
	}
	Ref<Texture2D> tex;
	String path = p_src;
	if (!p_src.begins_with("/")) {
		path = ProjectSettings::get_singleton()->globalize_path("res://" + p_src.trim_prefix("res://"));
	}
	Ref<Image> img = Image::load_from_file(path);
	if (img.is_valid() && !img->is_empty()) {
		tex = ImageTexture::create_from_image(img);
	}
	textures[p_src] = tex;
	return tex;
}

void VideoPreview::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW || dock == nullptr) {
		return;
	}
	const Size2 avail = get_size();
	const float s = MIN(avail.x / 1280.0f, avail.y / 720.0f);
	if (s <= 0.0f) {
		return;
	}
	const Size2 canvas = Size2(1280, 720) * s;
	const Point2 origin = (avail - canvas) / 2.0f;

	draw_rect(Rect2(Point2(), avail), Color(0.05, 0.05, 0.06));
	draw_rect(Rect2(origin, canvas), dock->get_background());

	Ref<Font> font = get_theme_default_font();
	const double now = dock->get_time();
	const double total = dock->get_duration();
	Array &items = dock->get_items();
	for (int i = 0; i < items.size(); i++) {
		Dictionary it = items[i];
		const double start = it.get("start", 0.0);
		const double end = it.get("end", total);
		if (now < start || now >= end) {
			continue;
		}
		const double span = MAX(end - start, 0.001);
		const double t = CLAMP((now - start) / span, 0.0, 1.0);
		const double fade = it.get("fade", 0.3);
		double alpha = 1.0;
		if (fade > 0.0) {
			alpha = CLAMP(MIN((now - start) / fade, (end - now) / fade), 0.0, 1.0);
		}
		const Vector2 base((double)it.get("x", 0), (double)it.get("y", 0));
		const Vector2 target((double)it.get("to_x", base.x), (double)it.get("to_y", base.y));
		const Point2 pos = origin + base.lerp(target, t) * s;
		const float zoom = (float)Math::lerp((double)it.get("zoom_from", 1.0), (double)it.get("zoom_to", 1.0), t);
		const float a = (float)alpha;
		const String kind = it.get("type", "text");

		if (kind == "text") {
			const String text = it.get("text", "");
			const int fs = MAX(6, (int)((double)it.get("size", 48) * s * zoom));
			const float w = (float)((double)it.get("w", 1280) * s);
			HorizontalAlignment align = HORIZONTAL_ALIGNMENT_CENTER;
			const String align_name = it.get("align", "center");
			if (align_name == "left") {
				align = HORIZONTAL_ALIGNMENT_LEFT;
			} else if (align_name == "right") {
				align = HORIZONTAL_ALIGNMENT_RIGHT;
			}
			Color col = Color::html(String(it.get("color", "#ffffff")));
			col.a *= a;
			const Point2 p = pos + Vector2(0, font->get_ascent(fs));
			draw_multiline_string(font, p + Vector2(2, 2), text, align, w, fs, -1, Color(0, 0, 0, 0.8f * a));
			draw_multiline_string(font, p, text, align, w, fs, -1, col);
		} else {
			const Size2 size0 = Size2((double)it.get("w", kind == "image" ? 1280 : 100), (double)it.get("h", kind == "image" ? 720 : 100)) * s;
			const Point2 center = pos + size0 / 2.0f;
			const Size2 size1 = size0 * zoom;
			const Rect2 r(center - size1 / 2.0f, size1);
			if (kind == "rect") {
				Color col = Color::html(String(it.get("color", "#ffffff")));
				col.a *= a;
				draw_rect(r, col);
			} else {
				Ref<Texture2D> tex = _get_texture(String(it.get("src", "")));
				if (tex.is_null()) {
					draw_rect(r, Color(0.4, 0.4, 0.4, 0.5f * a));
				} else {
					const Size2 ts = tex->get_size();
					const float k = MAX(r.size.x / ts.x, r.size.y / ts.y);
					const Size2 src_size = r.size / k;
					const Rect2 src((ts - src_size) / 2.0f, src_size);
					draw_texture_rect_region(tex, r, src, Color(1, 1, 1, a));
				}
			}
		}
	}
	draw_rect(Rect2(origin, canvas), Color(1, 1, 1, 0.15), false, 1.0f);
}
