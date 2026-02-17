#include "emotion_face_renderer.h"

#include <unordered_map>

namespace {

using Style = EmotionFaceRenderer::EyeStyle;

// Presets de olhos - Foco em CLAREZA emocional
// [width, height, radius, x_pos, y_pos, tilt_left, tilt_right, offset_y_left, offset_y_right]
const std::unordered_map<std::string, Style> kEmotionEyeStyles = {
    // ==================== BASE ====================
    {"neutral",    {54, 54, LV_RADIUS_CIRCLE, -18, 16,  0,  0,  0,  0}},  // O O (base redonda)
    {"idle",       {52, 50, LV_RADIUS_CIRCLE, -18, 16,  0,  0,  1,  0}},  // Leve assimetria (respirando)
    {"relaxed",    {54, 42, 22,               -18, 17,  0,  0,  0,  0}},  // Arredondado, meio fechado
    
    // ==================== FELIZ / POSITIVO ====================
    {"happy",      {56, 22, 12,               -18, 14,  3,  3, -2, -2}},  // ^ ^ (redondo nas pontas)
    {"laughing",   {56, 16, 10,               -18, 13,  4,  4, -3, -3}},  // ^ ^ (mais fechado ainda)
    {"loving",     {54, 30, 15,               -18, 15, -3, -3, -1, -1}},  // Curva suave pra dentro
    {"delicious",  {56, 20, 12,               -18, 14,  2,  2, -2, -2}},  // Feliz com fome
    {"confident",  {56, 28, 14,               -18, 15, -1, -1, -1, -1}},  // Firme, sem perder arredondado
    
    // ==================== TRISTE / NEGATIVO ====================
    {"sad",        {52, 34, 16,               -18, 20, -4, -4,  3,  3}},  // Redondo + caído
    {"crying",     {52, 32, 16,               -18, 21, -5, -5,  4,  4}},  // Mais caído
    {"embarrassed",{50, 30, 15,               -20, 18, -3,  3,  2,  2}},  // Olha pros lados
    {"confused",   {52, 32, 16,               -18, 17, -4,  2,  1, -1}},  // Assimétrico
    
    // ==================== INTENSO ====================
    {"angry",      {54, 30, 15,               -18, 14,  5,  5,  2,  2}},  // Inclinado, mantendo volume
    {"surprised",  {58, 58, LV_RADIUS_CIRCLE, -18, 14,  0,  0,  0,  0}},  // O O (máximo aberto)
    {"shocked",    {56, 60, LV_RADIUS_CIRCLE, -18, 13,  0,  0,  1, -1}},  // O O (tremido)
    {"thinking",   {52, 30, 15,               -20, 16, -3,  2,  1, -1}},  // Olha pro lado
    
    // ==================== ESTADOS ====================
    {"sleepy",     {56, 12,  8,               -18, 22,  0,  0,  2,  2}},  // _ _ (quase fechando)
    {"winking",    {56, 16, 10,               -18, 14,  3,  0, -2, -8}},  // ^ - (um fecha)
    {"silly",      {50, 34, 16,               -22, 16, -5,  5,  0,  0}},  // Vesgo
    {"funny",      {52, 32, 16,               -20, 16, -4,  4, -1,  1}},  // Vesgo suave
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
    // Cada emoção controla seu raio para preservar expressão sem perder o visual arredondado.
    lv_obj_set_style_radius(eye_left_, style.eye_radius, 0);

    lv_obj_align(eye_left_, LV_ALIGN_CENTER, base_left_x + style.left_dx, style.eye_y + style.left_dy);

    lv_obj_set_size(eye_right_, style.eye_w, style.eye_h);
    lv_obj_set_style_radius(eye_right_, style.eye_radius, 0);
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
