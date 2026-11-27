#pragma once

#include "scene/gui/button.h"
#include "scene/resources/style_box_flat.h"

// Warna dan gaya bersama untuk tampilan editor video.
namespace VideoStyle {

inline Color bg() { return Color(0.059f, 0.067f, 0.082f); }
inline Color panel() { return Color(0.09f, 0.10f, 0.13f); }
inline Color panel2() { return Color(0.125f, 0.14f, 0.18f); }
inline Color accent() { return Color(1.0f, 0.83f, 0.0f); }
inline Color text() { return Color(0.91f, 0.92f, 0.93f); }
inline Color muted() { return Color(0.55f, 0.58f, 0.63f); }

inline Ref<StyleBoxFlat> box(const Color &p_color, int p_radius = 14, float p_margin = 12.0f) {
	Ref<StyleBoxFlat> sb;
	sb.instantiate();
	sb->set_bg_color(p_color);
	sb->set_corner_radius_all(p_radius);
	sb->set_content_margin_all(p_margin);
	return sb;
}

inline void style_primary(Button *p_button) {
	p_button->add_theme_style_override("normal", box(accent()));
	p_button->add_theme_style_override("hover", box(accent().lightened(0.12f)));
	p_button->add_theme_style_override("pressed", box(accent().darkened(0.15f)));
	p_button->add_theme_color_override("font_color", Color(0, 0, 0));
	p_button->add_theme_color_override("font_hover_color", Color(0, 0, 0));
	p_button->add_theme_color_override("font_pressed_color", Color(0, 0, 0));
}

} // namespace VideoStyle
