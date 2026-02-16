#include "emotion_face_renderer.h"

#include <unordered_map>

namespace {

using Style = EmotionFaceRenderer::FaceStyle;
using Mouth = EmotionFaceRenderer::MouthStyle;

const std::unordered_map<std::string, Style> kEmotionStyles = {
    {"happy",      {56, 56, 36, -30, 28, Mouth::Smile,    88, 48,  36}},
    {"laughing",   {58, 58, 36, -30, 29, Mouth::Smile,    94, 52,  36}},
    {"funny",      {54, 54, 38, -28, 27, Mouth::Smile,    84, 44,  38}},
    {"loving",     {58, 58, 36, -28, 29, Mouth::Smile,    82, 42,  38}},
    {"embarrassed",{52, 52, 36, -26, 26, Mouth::Hmm,      62, 16,  42}},
    {"confident",  {56, 50, 36, -30, 25, Mouth::Smile,    78, 34,  40}},
    {"delicious",  {56, 56, 34, -28, 28, Mouth::Smile,    90, 46,  40}},
    {"sad",        {52, 52, 36, -26, 26, Mouth::Sad,      78, 34,  46}},
    {"crying",     {52, 52, 36, -26, 26, Mouth::Sad,      78, 34,  46}},
    {"sleepy",     {56, 32, 34, -28, 16, Mouth::Neutral,  64, 10,  42}},
    {"silly",      {52, 52, 38, -28, 26, Mouth::Hmm,      66, 18,  44}},
    {"angry",      {52, 48, 36, -28, 24, Mouth::Sad,      76, 26,  44}},
    {"surprised",  {50, 50, 38, -24, 25, Mouth::Surprised,34, 34,  40}},
    {"shocked",    {50, 50, 38, -24, 25, Mouth::Surprised,34, 34,  40}},
    {"thinking",   {50, 50, 36, -26, 25, Mouth::Hmm,      58, 16,  42}},
    {"winking",    {56, 28, 36, -28, 14, Mouth::Smile,    78, 36,  40}},
    {"relaxed",    {54, 40, 34, -28, 20, Mouth::Neutral,  66, 10,  42}},
    {"confused",   {52, 48, 36, -28, 24, Mouth::Hmm,      60, 16,  42}},
    {"neutral",    {54, 48, 34, -28, 24, Mouth::Neutral,  64, 10,  42}},
    {"idle",       {54, 48, 34, -28, 24, Mouth::Neutral,  64, 10,  42}},
};

} // namespace

void EmotionFaceRenderer::Attach(lv_obj_t* parent, lv_obj_t* content_layer)
{
    parent_ = parent;
    content_layer_ = content_layer;
    EnsureObjects();
}

const EmotionFaceRenderer::FaceStyle* EmotionFaceRenderer::ResolveStyle(const char* emotion) const
{
    if (emotion == nullptr) {
        return nullptr;
    }
    const auto it = kEmotionStyles.find(emotion);
    return (it != kEmotionStyles.end()) ? &it->second : nullptr;
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
        lv_obj_set_style_bg_color(eye, lv_color_hex(kFaceColorHex), 0);
    }

    mouth_line_ = lv_obj_create(face_layer_);
    lv_obj_set_style_border_width(mouth_line_, 0, 0);
    lv_obj_set_style_bg_color(mouth_line_, lv_color_hex(kFaceColorHex), 0);

    mouth_arc_ = lv_arc_create(face_layer_);
    lv_obj_remove_style(mouth_arc_, nullptr, LV_PART_KNOB);
    lv_obj_set_style_arc_width(mouth_arc_, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(mouth_arc_, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(mouth_arc_, lv_color_hex(kFaceColorHex), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(mouth_arc_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(mouth_arc_, LV_OBJ_FLAG_CLICKABLE);

    mouth_o_ = lv_obj_create(face_layer_);
    lv_obj_set_style_bg_opa(mouth_o_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mouth_o_, 8, 0);
    lv_obj_set_style_border_color(mouth_o_, lv_color_hex(kFaceColorHex), 0);

    SetVisible(false);
}

void EmotionFaceRenderer::ApplyEyes(const FaceStyle& style)
{
    lv_obj_set_size(eye_left_, style.eye_w, style.eye_h);
    lv_obj_set_style_radius(eye_left_, style.eye_radius, 0);
    lv_obj_align(eye_left_, LV_ALIGN_CENTER, -(style.eye_w / 2 + style.eye_gap), style.eye_y);

    lv_obj_set_size(eye_right_, style.eye_w, style.eye_h);
    lv_obj_set_style_radius(eye_right_, style.eye_radius, 0);
    lv_obj_align(eye_right_, LV_ALIGN_CENTER, (style.eye_w / 2 + style.eye_gap), style.eye_y);
}

void EmotionFaceRenderer::ApplyMouth(const FaceStyle& style)
{
    lv_obj_add_flag(mouth_line_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mouth_arc_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mouth_o_, LV_OBJ_FLAG_HIDDEN);

    switch (style.mouth_style) {
    case MouthStyle::Smile:
        lv_obj_set_size(mouth_arc_, style.mouth_w, style.mouth_h);
        lv_obj_align(mouth_arc_, LV_ALIGN_CENTER, 0, style.mouth_y);
        lv_arc_set_rotation(mouth_arc_, 0);
        lv_arc_set_bg_angles(mouth_arc_, 0, 360);
        lv_arc_set_angles(mouth_arc_, 30, 150);
        lv_obj_remove_flag(mouth_arc_, LV_OBJ_FLAG_HIDDEN);
        break;

    case MouthStyle::Sad:
        lv_obj_set_size(mouth_arc_, style.mouth_w, style.mouth_h);
        lv_obj_align(mouth_arc_, LV_ALIGN_CENTER, 0, style.mouth_y);
        lv_arc_set_rotation(mouth_arc_, 180);
        lv_arc_set_bg_angles(mouth_arc_, 0, 360);
        lv_arc_set_angles(mouth_arc_, 30, 150);
        lv_obj_remove_flag(mouth_arc_, LV_OBJ_FLAG_HIDDEN);
        break;

    case MouthStyle::Neutral:
        lv_obj_set_size(mouth_line_, style.mouth_w, style.mouth_h);
        lv_obj_set_style_radius(mouth_line_, style.mouth_h / 2, 0);
        lv_obj_align(mouth_line_, LV_ALIGN_CENTER, 0, style.mouth_y);
        lv_obj_remove_flag(mouth_line_, LV_OBJ_FLAG_HIDDEN);
        break;

    case MouthStyle::Hmm:
        lv_obj_set_size(mouth_line_, style.mouth_w, style.mouth_h);
        lv_obj_set_style_radius(mouth_line_, style.mouth_h / 2, 0);
        lv_obj_align(mouth_line_, LV_ALIGN_CENTER, 8, style.mouth_y);
        lv_obj_remove_flag(mouth_line_, LV_OBJ_FLAG_HIDDEN);
        break;

    case MouthStyle::Surprised:
        lv_obj_set_size(mouth_o_, style.mouth_w, style.mouth_h);
        lv_obj_set_style_radius(mouth_o_, LV_RADIUS_CIRCLE, 0);
        lv_obj_align(mouth_o_, LV_ALIGN_CENTER, 0, style.mouth_y);
        lv_obj_remove_flag(mouth_o_, LV_OBJ_FLAG_HIDDEN);
        break;
    }
}

bool EmotionFaceRenderer::Render(const char* emotion)
{
    const FaceStyle* style = ResolveStyle(emotion);
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
    ApplyMouth(*style);
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
