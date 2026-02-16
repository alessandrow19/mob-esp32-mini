#pragma once

#include <lvgl.h>
#include <string>

/**
 * Renderizador de faces geométricas em tela cheia para emoções.
 *
 * Objetivo: encapsular toda a lógica visual (olhos/boca/cores/estados)
 * fora do LcdDisplay para manter o código modular e legível.
 */
class EmotionFaceRenderer {
public:
    EmotionFaceRenderer() = default;

    // Inicializa objetos de desenho dentro de um parent (ex.: emoji_box_) e
    // usa content_layer para forçar o fundo preto quando necessário.
    void Attach(lv_obj_t* parent, lv_obj_t* content_layer);

    // Renderiza a emoção. Retorna false quando emoção não é suportada.
    bool Render(const char* emotion);

    // Exibe/oculta a camada da face.
    void SetVisible(bool visible);

    // Restaura fundo padrão de conteúdo (quando sair do modo face).
    void RestoreContentBackground(lv_color_t color);

public:
    enum class MouthStyle {
        Smile,
        Sad,
        Neutral,
        Hmm,
        Surprised,
    };

    struct FaceStyle {
        int eye_w;
        int eye_h;
        int eye_gap;
        int eye_y;
        int eye_radius;
        MouthStyle mouth_style;
        int mouth_w;
        int mouth_h;
        int mouth_y;
    };

private:
    const FaceStyle* ResolveStyle(const char* emotion) const;
    void EnsureObjects();
    void ApplyEyes(const FaceStyle& style);
    void ApplyMouth(const FaceStyle& style);

    lv_obj_t* parent_ = nullptr;
    lv_obj_t* content_layer_ = nullptr;

    lv_obj_t* face_layer_ = nullptr;
    lv_obj_t* eye_left_ = nullptr;
    lv_obj_t* eye_right_ = nullptr;

    lv_obj_t* mouth_line_ = nullptr;
    lv_obj_t* mouth_arc_ = nullptr;
    lv_obj_t* mouth_o_ = nullptr;

    static constexpr uint32_t kFaceColorHex = 0x66D9FF; // azul suave infantil
};
