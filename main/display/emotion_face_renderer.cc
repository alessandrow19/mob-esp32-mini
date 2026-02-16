#include "emotion_face_renderer.h"

#include <unordered_map>

namespace {

using Style = EmotionFaceRenderer::EyeStyle;

// Presets de olhos - Foco em CLAREZA emocional
// [width, height, pupil, x_pos, y_pos, tilt_left, tilt_right, offset_y_left, offset_y_right]
const std::unordered_map<std::string, Style> kEmotionEyeStyles = {
    // ==================== BASE ====================
    {"neutral",    {64, 48, 36, -18, 16,  0,  0,  0,  0}},  // Olhos abertos, retos
    {"idle",       {64, 46, 36, -18, 16,  0,  0,  1,  0}},  // Leve assimetria (respirando)
    {"relaxed",    {64, 40, 36, -18, 17,  0,  0,  0,  0}},  // Meio fechadinho
    
    // ==================== FELIZ / POSITIVO ====================
    {"happy",      {64, 14, 36, -18, 14,  3,  3, -2, -2}},  // ^ ^ (fechado + sobe)
    {"laughing",   {64, 10, 36, -18, 13,  4,  4, -3, -3}},  // ^ ^ (mais fechado ainda)
    {"loving",     {64, 20, 36, -18, 15, -3, -3, -1, -1}},  // ♥ (inclina pra dentro)
    {"delicious",  {64, 16, 36, -18, 14,  2,  2, -2, -2}},  // ^ ^ (feliz com fome)
    {"confident",  {64, 24, 38, -18, 15, -1, -1, -1, -1}},  // Meio fechado, firme
    
    // ==================== TRISTE / NEGATIVO ====================
    {"sad",        {60, 28, 34, -18, 20, -4, -4,  3,  3}},  // \ / (cantos internos sobem)
    {"crying",     {60, 26, 34, -18, 21, -5, -5,  4,  4}},  // \ / (mais caído + tremido)
    {"embarrassed",{58, 24, 34, -20, 18, -3,  3,  2,  2}},  // Olha pros lados
    {"confused",   {60, 26, 34, -18, 17, -4,  2,  1, -1}},  // Um diferente do outro
    
    // ==================== INTENSO ====================
    {"angry",      {62, 26, 36, -18, 14,  5,  5,  2,  2}},  // V V (cantos internos descem)
    {"surprised",  {56, 56, 40, -18, 14,  0,  0,  0,  0}},  // O O (máximo aberto)
    {"shocked",    {54, 58, 38, -18, 13,  0,  0,  1, -1}},  // O O (tremido)
    {"thinking",   {60, 24, 34, -20, 16, -3,  2,  1, -1}},  // Olha pro lado + fechado
    
    // ==================== ESTADOS ====================
    {"sleepy",     {64,  8, 32, -18, 22,  0,  0,  2,  2}},  // _ _ (quase fechando)
    {"winking",    {64, 12, 36, -18, 14,  3,  0, -2, -8}},  // ^ - (um fecha)
    {"silly",      {56, 28, 36, -22, 16, -5,  5,  0,  0}},  // > < (vesgo)
    {"funny",      {58, 26, 36, -20, 16, -4,  4, -1,  1}},  // > < (menos vesgo)
};

} // namespace

void EmotionFaceRenderer::Attach(lv_obj_t* parent, lv_obj_t* content_layer)
{
    parent_ = parent;
    content_layer_ = content_layer;
    EnsureObjects();
}

const EmotionFaceRenderer::EyeStyle* EmotionFaceRenderer::ResolveStyle(const char* emotion) const
{
    if (emotion == nullptr) {
        return nullptr;
    }
    const auto it = kEmotionEyeStyles.find(emotion);
    return (it != kEmotionEyeStyles.end()) ? &it->second : nullptr;
}

void EmotionFaceRenderer::EnsureObjects()
{
    if (parent_ == nullptr || face_layer_ != nullptr) {
        return;
    }

    face_layer_ = lv_obj_create(parent_);
    lv_obj_set_size(face_layer_, LV_PCT(100), LV_PCT(100));
    lv_obj_center(face_layer_);
    lv_obj_set_style_pad_all(face_layer_, 0, 0);
    lv_obj_set_style_border_width(face_layer_, 0, 0);
    lv_obj_set_style_radius(face_layer_, 0, 0);
    lv_obj_set_style_bg_color(face_layer_, lv_color_black(), 0);

    eye_left_ = lv_obj_create(face_layer_);
    eye_right_ = lv_obj_create(face_layer_);

    for (lv_obj_t* eye : {eye_left_, eye_right_}) {
        lv_obj_set_style_border_width(eye, 0, 0);
        lv_obj_set_style_bg_color(eye, lv_color_hex(kEyeColorHex), 0);
    }

    SetVisible(false);
}

void EmotionFaceRenderer::ApplyEyes(const EyeStyle& style)
{
    const int base_left_x = -(style.eye_w / 2 + style.eye_gap);
    const int base_right_x = (style.eye_w / 2 + style.eye_gap);

    lv_obj_set_size(eye_left_, style.eye_w, style.eye_h);
    lv_obj_set_style_radius(eye_left_, LV_RADIUS_CIRCLE, 0);

    lv_obj_align(eye_left_, LV_ALIGN_CENTER, base_left_x + style.left_dx, style.eye_y + style.left_dy);

    lv_obj_set_size(eye_right_, style.eye_w, style.eye_h);
    lv_obj_set_style_radius(eye_right_, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(eye_right_, LV_ALIGN_CENTER, base_right_x + style.right_dx, style.eye_y + style.right_dy);
}

bool EmotionFaceRenderer::Render(const char* emotion)
{
    const EyeStyle* style = ResolveStyle(emotion);
    if (style == nullptr) {
        return false;
    }

    EnsureObjects();
    if (face_layer_ == nullptr) {
        return false;
    }

    if (content_layer_ != nullptr) {
        lv_obj_set_style_bg_color(content_layer_, lv_color_black(), 0);
    }

    ApplyEyes(*style);
    SetVisible(true);
    return true;
}

void EmotionFaceRenderer::SetVisible(bool visible)
{
    if (face_layer_ == nullptr) {
        return;
    }
    if (visible) {
        lv_obj_remove_flag(face_layer_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(face_layer_, LV_OBJ_FLAG_HIDDEN);
    }
}

void EmotionFaceRenderer::RestoreContentBackground(lv_color_t color)
{
    if (content_layer_ != nullptr) {
        lv_obj_set_style_bg_color(content_layer_, color, 0);
    }
}
