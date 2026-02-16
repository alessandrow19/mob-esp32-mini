#include "emotion_face_renderer.h"

#include <unordered_map>

namespace {

using Style = EmotionFaceRenderer::EyeStyle;

// Presets de olhos no estilo Cozmo (sem boca), usando apenas retângulos
// arredondados e pequenas variações de altura/offset para expressividade.
const std::unordered_map<std::string, Style> kEmotionEyeStyles = {
    {"neutral",    {58, 42, 34, -18, 14,  0,  0,  0,  0}},
    {"idle",       {58, 40, 34, -18, 14,  0,  0,  0,  0}},
    {"happy",      {56, 22, 34, -20, 10,  0,  0, -3, -3}},
    {"laughing",   {60, 20, 34, -20, 10,  0,  0, -4, -4}},
    {"funny",      {54, 24, 36, -20, 11, -2,  2, -1,  1}},
    {"loving",     {58, 26, 34, -20, 12,  0,  0, -2, -2}},
    {"embarrassed",{52, 24, 34, -18, 10, -3,  3,  2,  2}},
    {"confident",  {60, 18, 36, -20,  9, -1,  1, -2, -2}},
    {"delicious",  {58, 24, 34, -20, 11,  0,  0, -2, -2}},
    {"sad",        {54, 20, 34, -12, 10, -2,  2,  5,  1}},
    {"crying",     {54, 18, 34, -10,  9, -2,  2,  6,  2}},
    {"sleepy",     {60, 12, 32, -16,  7,  0,  0,  2,  2}},
    {"silly",      {50, 24, 36, -18, 11, -4,  4, -1,  1}},
    {"angry",      {56, 20, 34, -18, 10, -2,  2,  0, -2}},
    {"surprised",  {52, 52, 36, -16, 18,  0,  0,  0,  0}},
    {"shocked",    {52, 52, 36, -16, 18,  0,  0,  0,  0}},
    {"thinking",   {54, 20, 34, -18, 10, -2,  2,  1, -1}},
    {"winking",    {58, 16, 34, -18,  8,  0,  0,  0, -6}},
    {"relaxed",    {58, 16, 34, -18,  8,  0,  0,  0,  0}},
    {"confused",   {54, 20, 34, -18, 10, -3,  3, -2,  2}},
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
