#pragma once

#include <lvgl.h>
#include <string>

/**
 * Renderizador de olhos geométricos em tela cheia para emoções.
 *
 * Estratégia: visual no estilo Cozmo (somente olhos), sem boca e sem imagens,
 * para economizar memória e manter o desenho expressivo.
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
    struct EyeStyle {
        int eye_w;
        int eye_h;
        int eye_gap;
        int eye_y;
        int eye_radius;
        int left_dx;
        int right_dx;
        int left_dy;
        int right_dy;
    };

private:
    const EyeStyle* ResolveStyle(const char* emotion) const;
    void EnsureObjects();
    void ApplyEyes(const EyeStyle& style);

    lv_obj_t* parent_ = nullptr;
    lv_obj_t* content_layer_ = nullptr;

    lv_obj_t* face_layer_ = nullptr;
    lv_obj_t* eye_left_ = nullptr;
    lv_obj_t* eye_right_ = nullptr;

    static constexpr uint32_t kEyeColorHex = 0x66D9FF; // azul suave infantil
};
