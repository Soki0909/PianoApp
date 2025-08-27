/**
 * @file PianoApp.c
 * @brief 3D電子ピアノアプリケーション
 * @details
 * - OpenGL/GLUTによる3D描画
 * - miniaudioによる音声出力
 * - マウスとキーボードによるカメラ操作と演奏
 * - OBJモデル、PPMテクスチャ、カスタム音色・楽譜ファイルの読み込み
 * - 自動演奏シーケンサー機能
 */

 // ============================================================================
 // インクルード
 // ============================================================================
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

// miniaudioライブラリの実装を有効化
#define MINIAUDIO_IMPLEMENTATION
#include "include/miniaudio.h"


// ============================================================================
// 定数定義
// ============================================================================

// --- オーディオ関連 ---
#define AUDIO_SAMPLE_RATE       44100
#define AUDIO_ATTACK_TIME_S     0.01
#define AUDIO_RELEASE_TIME_S    0.01
#define MIDI_NOTE_A4            69
#define FREQUENCY_A4            440.0f

// --- ピアノ・UI関連 ---
#define PIANO_KEY_COUNT         37
#define TIMBRE_BUTTON_COUNT     4
#define OCTAVE_BUTTON_COUNT     2
#define MIDI_NOTE_START         48
#define KEY_PRESSED_Y_OFFSET    -0.2f
#define OCTAVE_SHIFT_MAX        2
#define OCTAVE_SHIFT_MIN       -2

// --- カメラ・操作関連 ---
#define CAMERA_MOVE_SPEED       0.2f
#define MOUSE_SENSITIVITY       0.1f
#define CAMERA_PITCH_MAX        89.0f
#define CAMERA_PITCH_MIN       -89.0f

// --- アニメーション・描画関連 ---
#define ANIMATION_TIMER_MS      16
#define KEY_ANIMATION_SPEED     0.2f
#define RETICLE_SIZE            10
#define HUD_MARGIN_X            10
#define HUD_MARGIN_Y            30
#define HUD_LINE_HEIGHT         20

// --- メニューID ---
#define MENU_ID_SEQ_PLAY        1
#define MENU_ID_SEQ_STOP        2


// ============================================================================
// 型定義 (構造体・列挙体)
// ============================================================================

typedef enum {
    KEY_TYPE_WHITE,
    KEY_TYPE_BLACK
} key_type_e;

typedef enum {
    ENV_STATE_OFF,
    ENV_STATE_ATTACK,
    ENV_STATE_PRESSED,
    ENV_STATE_RELEASING
} envelope_state_e;

typedef struct {
    double x, y, z;
} vector_3d_t;

typedef struct {
    vector_3d_t min;
    vector_3d_t max;
} bounding_box_t;

typedef struct {
    GLuint display_list_id;
    bounding_box_t local_bbox;
} model_3d_t;

typedef struct {
    float amplitude;
    float phase_shift;
} harmonic_t;

typedef struct {
    char name[64];
    int harmonic_count;
    harmonic_t* harmonics;
} timbre_t;

typedef struct {
    key_type_e type;
    int midi_note;
    float center_pos[3];
    envelope_state_e envelope_state;
    double wave_phase;
    double current_amplitude;
    float current_y_pos;
    float target_y_pos;
} piano_key_t;

typedef struct {
    int midi_note;
    float duration_ms;
} sequence_event_t;


// ============================================================================
// グローバル変数
// ============================================================================

// --- 3Dモデル ---
model_3d_t g_model_piano_body;
model_3d_t g_model_white_key;
model_3d_t g_model_black_key;
model_3d_t g_model_timbre_button;
model_3d_t g_model_octave_button;

// --- テクスチャ ---
GLuint g_texture_wood;

// --- ピアノ・音色 ---
piano_key_t g_piano_keys[PIANO_KEY_COUNT];
timbre_t g_timbres[TIMBRE_BUTTON_COUNT];
int g_current_timbre_index = 0;
int g_current_octave_shift = 0;

// --- カメラ ---
float g_camera_pos[] = { -3.5f, 17.0f, -17.0f };
float g_camera_front[] = { 0.0f, 0.0f, -1.0f };
float g_camera_up[] = { 0.0f, 1.0f, 0.0f };
float g_camera_yaw = 90.0f;
float g_camera_pitch = -45.0f;

// --- オーディオデバイス ---
ma_device g_audio_device;

// --- シーケンサー ---
sequence_event_t* g_sequence = NULL;
int g_sequence_length = 0;
int g_sequence_index = 0;
int g_is_sequencer_playing = 0;

// --- オブジェクト配置座標 (定数) ---
const float WHITE_KEY_X_START = 7.0f;
const float BLACK_KEY_Y = 0.2f;
const float BLACK_KEY_Z = 0.8f;
const float BLACK_KEY_X_POSITIONS[] = {
    6.6f, 5.4f, 3.6f, 2.5f, 1.4f, -0.4f, -1.6f, -3.4f,
    -4.5f, -5.6f, -7.4f, -8.6f, -10.4f, -11.5f, -12.6f
};
const float BUTTON_Y = 0.8f;
const float BUTTON_Z = 3.5f;
const float TIMBRE_BUTTON_X_POSITIONS[] = { -5.0f, -8.0f, -11.0f, -14.0f };
const float OCTAVE_BUTTON_POSITIONS[][4] = {
    {6.0f, 0.8f, 3.5f, -90.0f},
    {5.0f, 0.8f, 3.5f,  90.0f}
};

// --- 材質 (マテリアル) ---
GLfloat MAT_BLACK_PLASTIC_AMBIENT[] = { 0.0f, 0.0f, 0.0f, 1.0f };
GLfloat MAT_BLACK_PLASTIC_DIFFUSE[] = { 0.01f, 0.01f, 0.01f, 1.0f };
GLfloat MAT_BLACK_PLASTIC_SPECULAR[] = { 0.50f, 0.50f, 0.50f, 1.0f };
GLfloat MAT_BLACK_PLASTIC_SHININESS = 32.0f;

GLfloat MAT_IVORY_AMBIENT[] = { 0.25f, 0.25f, 0.20f, 1.0f };
GLfloat MAT_IVORY_DIFFUSE[] = { 0.9f, 0.9f, 0.8f, 1.0f };
GLfloat MAT_IVORY_SPECULAR[] = { 0.3f, 0.3f, 0.3f, 1.0f };
GLfloat MAT_IVORY_SHININESS = 15.0f;

GLfloat MAT_BLACK_MATTE_AMBIENT[] = { 0.02f, 0.02f, 0.02f, 1.0f };
GLfloat MAT_BLACK_MATTE_DIFFUSE[] = { 0.01f, 0.01f, 0.01f, 1.0f };
GLfloat MAT_BLACK_MATTE_SPECULAR[] = { 0.1f, 0.1f, 0.1f, 1.0f };
GLfloat MAT_BLACK_MATTE_SHININESS = 5.0f;

GLfloat MAT_GREY_PLASTIC_AMBIENT[] = { 0.3f, 0.3f, 0.3f, 1.0f };
GLfloat MAT_GREY_PLASTIC_DIFFUSE[] = { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat MAT_GREY_PLASTIC_SPECULAR[] = { 0.6f, 0.6f, 0.6f, 1.0f };
GLfloat MAT_GREY_PLASTIC_SHININESS = 70.0f;


// ============================================================================
// 関数プロトタイプ宣言
// ============================================================================

// --- 初期化・終了処理 ---
void initialize_application();
void initialize_opengl();
void initialize_piano_keys();
void initialize_camera();
void cleanup_application();

// --- データ読み込み ---
model_3d_t load_obj_model(const char* filename);
GLuint load_ppm_texture(const char* filename);
void load_timbre_file(const char* filename, int timbre_index);
void load_sequence_file(const char* filename, float tempo);

// --- 描画処理 ---
void display();
void draw_floor();
void draw_piano_body();
void draw_piano_keys();
void draw_buttons();
void draw_hud();
void draw_reticle();

// --- 入力・イベント処理 ---
void on_mouse_button(int button, int state, int x, int y);
void on_keyboard_press(unsigned char key, int x, int y);
void on_mouse_move(int x, int y);
void on_menu_select(int menu_id);

// --- アニメーション・シーケンサー ---
void update_key_animation(int timer_value);
void play_next_sequence_note(int previous_midi_note);

// --- オーディオ処理 ---
void audio_callback(ma_device* p_device, void* p_output, const void* p_input, ma_uint32 frame_count);
void trigger_note_on(int midi_note);
void trigger_note_off(int midi_note);

// --- ユーティリティ ---
float midi_to_freq(int midi_note);
vector_3d_t get_world_pos_from_screen_center();
int is_point_in_box(vector_3d_t point, bounding_box_t box);


// ============================================================================
// main: プログラムのエントリーポイント
// ============================================================================
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    int screen_width = glutGet(GLUT_SCREEN_WIDTH);
    int screen_height = glutGet(GLUT_SCREEN_HEIGHT);
    glutInitWindowSize(screen_width, screen_height);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("3D電子ピアノ");

    glutCreateMenu(on_menu_select);
    glutAddMenuEntry("Play Sequence", MENU_ID_SEQ_PLAY);
    glutAddMenuEntry("Stop Sequence", MENU_ID_SEQ_STOP);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    initialize_application();

    glutDisplayFunc(display);
    glutMouseFunc(on_mouse_button);
    glutKeyboardFunc(on_keyboard_press);
    glutPassiveMotionFunc(on_mouse_move);

    glutSetCursor(GLUT_CURSOR_NONE);
    glutTimerFunc(ANIMATION_TIMER_MS, update_key_animation, 0);

    glutMainLoop();
    cleanup_application();
    return 0;
}


// ============================================================================
// 初期化・終了処理
// ============================================================================

void initialize_application() {
    printf("アプリケーションを初期化しています...\n");

    initialize_opengl();
    initialize_camera();

    g_model_piano_body = load_obj_model("object/Body.obj");
    g_model_white_key = load_obj_model("object/WhiteKey.obj");
    g_model_black_key = load_obj_model("object/BlackKey.obj");
    g_model_timbre_button = load_obj_model("object/Botton.obj");
    g_model_octave_button = load_obj_model("object/Botton2.obj");
    g_texture_wood = load_ppm_texture("textures/tile.ppm");

    load_timbre_file("timbres/neiro0.txt", 0);
    load_timbre_file("timbres/neiro1.txt", 1);
    load_timbre_file("timbres/neiro2.txt", 2);
    load_timbre_file("timbres/neiro3.txt", 3);

    load_sequence_file("gakufu/kirakira.txt", 120.0f);
    initialize_piano_keys();

    ma_device_config device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format = ma_format_f32;
    device_config.playback.channels = 2;
    device_config.sampleRate = AUDIO_SAMPLE_RATE;
    device_config.dataCallback = audio_callback;
    device_config.pUserData = g_piano_keys;

    if (ma_device_init(NULL, &device_config, &g_audio_device) != MA_SUCCESS) {
        fprintf(stderr, "エラー: 再生デバイスの初期化に失敗しました。\n");
        return;
    }
    if (ma_device_start(&g_audio_device) != MA_SUCCESS) {
        fprintf(stderr, "エラー: 再生デバイスの開始に失敗しました。\n");
        ma_device_uninit(&g_audio_device);
        return;
    }

    printf("初期化が完了しました。\n");
}

void initialize_opengl() {
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat global_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    GLfloat light_pos[] = { 2.0f, 10.0f, 5.0f, 1.0f };
    GLfloat light_dif[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_dif);
}

void initialize_piano_keys() {
    int white_key_index = 0;
    int black_key_index = 0;

    for (int i = 0; i < PIANO_KEY_COUNT; ++i) {
        piano_key_t* key = &g_piano_keys[i];
        key->midi_note = i + MIDI_NOTE_START;

        int note_in_octave = (key->midi_note - 12) % 12;
        int is_black_key = (note_in_octave == 1 || note_in_octave == 3 || note_in_octave == 6 || note_in_octave == 8 || note_in_octave == 10);

        if (is_black_key) {
            key->type = KEY_TYPE_BLACK;
            key->center_pos[0] = BLACK_KEY_X_POSITIONS[black_key_index];
            key->center_pos[1] = BLACK_KEY_Y;
            key->center_pos[2] = BLACK_KEY_Z;
            black_key_index++;
        }
        else {
            key->type = KEY_TYPE_WHITE;
            key->center_pos[0] = WHITE_KEY_X_START - white_key_index;
            key->center_pos[1] = 0.0f;
            key->center_pos[2] = 0.0f;
            white_key_index++;
        }

        key->envelope_state = ENV_STATE_OFF;
        key->wave_phase = 0.0;
        key->current_amplitude = 0.0;
        key->current_y_pos = 0.0f;
        key->target_y_pos = 0.0f;
    }
}

void initialize_camera() {
    float rad_yaw = g_camera_yaw * M_PI / 180.0f;
    float rad_pitch = g_camera_pitch * M_PI / 180.0f;

    g_camera_front[0] = cos(rad_yaw) * cos(rad_pitch);
    g_camera_front[1] = sin(rad_pitch);
    g_camera_front[2] = sin(rad_yaw) * cos(rad_pitch);

    float len = sqrt(pow(g_camera_front[0], 2) + pow(g_camera_front[1], 2) + pow(g_camera_front[2], 2));
    g_camera_front[0] /= len;
    g_camera_front[1] /= len;
    g_camera_front[2] /= len;
}

void cleanup_application() {
    ma_device_uninit(&g_audio_device);

    glDeleteLists(g_model_piano_body.display_list_id, 1);
    glDeleteLists(g_model_white_key.display_list_id, 1);
    glDeleteLists(g_model_black_key.display_list_id, 1);
    glDeleteLists(g_model_timbre_button.display_list_id, 1);
    glDeleteLists(g_model_octave_button.display_list_id, 1);
    glDeleteTextures(1, &g_texture_wood);

    for (int i = 0; i < TIMBRE_BUTTON_COUNT; ++i) {
        free(g_timbres[i].harmonics);
        g_timbres[i].harmonics = NULL;
    }
    free(g_sequence);
    g_sequence = NULL;

    printf("リソースを解放しました。\n");
}


// ============================================================================
// データ読み込み
// ============================================================================

model_3d_t load_obj_model(const char* filename) {
    model_3d_t model = { 0 };
    FILE* file;
    if (fopen_s(&file, filename, "r") != 0 || file == NULL) {
        fprintf(stderr, "エラー: モデルファイル '%s' を開けません。\n", filename);
        return model;
    }

    int vertex_count = 0, normal_count = 0, tex_coord_count = 0;
    char line_buffer[256];
    while (fgets(line_buffer, sizeof(line_buffer), file)) {
        if (strncmp(line_buffer, "v ", 2) == 0) vertex_count++;
        else if (strncmp(line_buffer, "vn ", 3) == 0) normal_count++;
        else if (strncmp(line_buffer, "vt ", 3) == 0) tex_coord_count++;
    }

    if (vertex_count == 0) {
        fclose(file);
        fprintf(stderr, "エラー: '%s' に頂点データがありません。\n", filename);
        return model;
    }

    float(*vertices)[3] = malloc(sizeof(float[3]) * vertex_count);
    float(*normals)[3] = malloc(sizeof(float[3]) * normal_count);
    float(*tex_coords)[2] = malloc(sizeof(float[2]) * tex_coord_count);

    if (!vertices || (normal_count > 0 && !normals) || (tex_coord_count > 0 && !tex_coords)) {
        fprintf(stderr, "エラー: モデルデータのメモリ確保に失敗しました。\n");
        free(vertices); free(normals); free(tex_coords);
        fclose(file);
        return model;
    }

    rewind(file);
    int v_idx = 0, vn_idx = 0, vt_idx = 0;
    model.local_bbox.min = (vector_3d_t){ 1e9, 1e9, 1e9 };
    model.local_bbox.max = (vector_3d_t){ -1e9, -1e9, -1e9 };

    while (fgets(line_buffer, sizeof(line_buffer), file)) {
        if (strncmp(line_buffer, "v ", 2) == 0) {
            float x, y, z;
            sscanf_s(line_buffer, "v %f %f %f", &x, &y, &z);
            vertices[v_idx][0] = x; vertices[v_idx][1] = y; vertices[v_idx][2] = z;
            if (x < model.local_bbox.min.x) model.local_bbox.min.x = x;
            if (x > model.local_bbox.max.x) model.local_bbox.max.x = x;
            if (y < model.local_bbox.min.y) model.local_bbox.min.y = y;
            if (y > model.local_bbox.max.y) model.local_bbox.max.y = y;
            if (z < model.local_bbox.min.z) model.local_bbox.min.z = z;
            if (z > model.local_bbox.max.z) model.local_bbox.max.z = z;
            v_idx++;
        }
        else if (strncmp(line_buffer, "vn ", 3) == 0) {
            sscanf_s(line_buffer, "vn %f %f %f", &normals[vn_idx][0], &normals[vn_idx][1], &normals[vn_idx][2]);
            vn_idx++;
        }
        else if (strncmp(line_buffer, "vt ", 3) == 0) {
            sscanf_s(line_buffer, "vt %f %f", &tex_coords[vt_idx][0], &tex_coords[vt_idx][1]);
            vt_idx++;
        }
    }

    model.display_list_id = glGenLists(1);
    if (model.display_list_id == 0) {
        fprintf(stderr, "エラー: glGenListsに失敗しました。\n");
    }
    else {
        glNewList(model.display_list_id, GL_COMPILE);
        glBegin(GL_TRIANGLES);
        rewind(file);
        while (fgets(line_buffer, sizeof(line_buffer), file)) {
            if (strncmp(line_buffer, "f ", 2) == 0) {
                int v[3], vt[3], vn[3];
                if (sscanf_s(line_buffer, "f %d/%d/%d %d/%d/%d %d/%d/%d",
                    &v[0], &vt[0], &vn[0], &v[1], &vt[1], &vn[1], &v[2], &vt[2], &vn[2]) == 9) {
                    for (int i = 0; i < 3; ++i) {
                        if (vn[i] <= normal_count)   glNormal3fv(normals[vn[i] - 1]);
                        if (vt[i] <= tex_coord_count) glTexCoord2fv(tex_coords[vt[i] - 1]);
                        if (v[i] <= vertex_count)    glVertex3fv(vertices[v[i] - 1]);
                    }
                }
            }
        }
        glEnd();
        glEndList();
    }

    fclose(file);
    free(vertices);
    free(normals);
    free(tex_coords);

    printf("情報: モデル '%s' を読み込みました (頂点: %d, 法線: %d, UV: %d)。\n", filename, vertex_count, normal_count, tex_coord_count);
    return model;
}

GLuint load_ppm_texture(const char* filename) {
    GLuint texture_id;
    int width, height, max_color;
    char buffer[256];
    FILE* file;

    if (fopen_s(&file, filename, "rb") != 0 || file == NULL) {
        fprintf(stderr, "エラー: テクスチャファイル '%s' を開けません。\n", filename);
        return 0;
    }

    fgets(buffer, sizeof(buffer), file);
    if (strncmp(buffer, "P6", 2) != 0) {
        fprintf(stderr, "エラー: '%s' はP6形式のPPMファイルではありません。\n", filename);
        fclose(file);
        return 0;
    }

    do {
        fgets(buffer, sizeof(buffer), file);
    } while (strncmp(buffer, "#", 1) == 0);

    sscanf_s(buffer, "%d %d", &width, &height);

    fgets(buffer, sizeof(buffer), file);
    sscanf_s(buffer, "%d", &max_color);
    if (max_color != 255) {
        fprintf(stderr, "警告: '%s' の最大色深度が255ではありません。\n", filename);
    }

    unsigned char* pixel_data = (unsigned char*)malloc(width * height * 3);
    if (pixel_data == NULL) {
        fprintf(stderr, "エラー: テクスチャデータのメモリ確保に失敗しました。\n");
        fclose(file);
        return 0;
    }

    fread(pixel_data, 1, width * height * 3, file);
    fclose(file);

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    free(pixel_data);
    printf("情報: PPMテクスチャ '%s' を読み込みました (ID: %d, サイズ: %dx%d)。\n", filename, texture_id, width, height);
    return texture_id;
}

void load_timbre_file(const char* filename, int timbre_index) {
    if (timbre_index < 0 || timbre_index >= TIMBRE_BUTTON_COUNT) return;

    FILE* file;
    timbre_t* timbre = &g_timbres[timbre_index];

    if (fopen_s(&file, filename, "r") != 0 || file == NULL) {
        fprintf(stderr, "警告: 音色ファイル '%s' を開けません。デフォルト音色を適用します。\n", filename);
        timbre->harmonic_count = 1;
        timbre->harmonics = (harmonic_t*)malloc(sizeof(harmonic_t));
        timbre->harmonics[0] = (harmonic_t){ 1.0f, 0.0f };
        sprintf_s(timbre->name, sizeof(timbre->name), "Default Sine");
        return;
    }

    char name_buffer[64];
    fgets(name_buffer, sizeof(name_buffer), file);
    if (sscanf_s(name_buffer, "%63[^\n]", timbre->name, (unsigned)_countof(timbre->name)) != 1) {
        sprintf_s(timbre->name, sizeof(timbre->name), "Unnamed %d", timbre_index);
    }

    if (fscanf_s(file, "%d", &timbre->harmonic_count) != 1 || timbre->harmonic_count <= 0) {
        fprintf(stderr, "エラー: '%s' の倍音数が不正です。\n", filename);
        fclose(file);
        return;
    }

    timbre->harmonics = (harmonic_t*)malloc(sizeof(harmonic_t) * timbre->harmonic_count);
    if (timbre->harmonics == NULL) {
        fprintf(stderr, "エラー: 倍音データのメモリ確保に失敗しました。\n");
        fclose(file);
        return;
    }
    for (int i = 0; i < timbre->harmonic_count; ++i) {
        if (fscanf_s(file, "%f,%f", &timbre->harmonics[i].amplitude, &timbre->harmonics[i].phase_shift) != 2) {
            fprintf(stderr, "警告: '%s' の倍音%dの読み込みに失敗しました。\n", filename, i + 1);
            timbre->harmonics[i] = (harmonic_t){ 0.0f, 0.0f };
        }
    }

    fclose(file);
    printf("情報: 音色 '%s' (%s) を読み込みました (倍音数: %d)。\n", timbre->name, filename, timbre->harmonic_count);
}

void load_sequence_file(const char* filename, float tempo) {
    FILE* file;
    if (fopen_s(&file, filename, "r") != 0 || file == NULL) {
        fprintf(stderr, "エラー: 楽譜ファイル '%s' を開けません。\n", filename);
        return;
    }

    char line[256];
    g_sequence_length = 0;
    while (fgets(line, sizeof(line), file)) {
        if (strlen(line) > 2) g_sequence_length++;
    }
    if (g_sequence_length == 0) { fclose(file); return; }

    g_sequence = (sequence_event_t*)malloc(sizeof(sequence_event_t) * g_sequence_length);
    if (g_sequence == NULL) {
        fprintf(stderr, "エラー: シーケンスデータのメモリ確保に失敗しました。\n");
        fclose(file);
        return;
    }

    rewind(file);
    int event_index = 0;
    while (fgets(line, sizeof(line), file) && event_index < g_sequence_length) {
        if (strlen(line) < 3) continue;

        char note_char = line[0];
        char accidental_char = line[1];
        int octave = line[2] - '0';
        char dot_char = line[3];
        char duration_char = line[4];

        int midi_note = 0;
        if (note_char == 'M') {
            midi_note = 0;
        }
        else {
            int base_note = 0;
            switch (note_char) {
            case 'C': base_note = 0; break; case 'D': base_note = 2; break;
            case 'E': base_note = 4; break; case 'F': base_note = 5; break;
            case 'G': base_note = 7; break; case 'A': base_note = 9; break;
            case 'B': base_note = 11; break;
            default: continue;
            }
            int accidental = (accidental_char == '#') ? 1 : ((accidental_char == 'b') ? -1 : 0);
            midi_note = 12 + (octave * 12) + base_note + accidental;
        }
        g_sequence[event_index].midi_note = midi_note;

        float beat_duration_ms = (60.0f / tempo) * 1000.0f;
        float base_duration = beat_duration_ms;
        switch (duration_char) {
        case '2': base_duration = beat_duration_ms * 4.0f; break;
        case '3': base_duration = beat_duration_ms * 2.0f; break;
        case '4': base_duration = beat_duration_ms; break;
        case '5': base_duration = beat_duration_ms / 2.0f; break;
        case '6': base_duration = beat_duration_ms / 4.0f; break;
        case '7': base_duration = beat_duration_ms / 8.0f; break;
        }

        g_sequence[event_index].duration_ms = (dot_char == '.') ? (base_duration * 1.5f) : base_duration;
        event_index++;
    }

    fclose(file);
    printf("情報: 楽譜 '%s' を読み込みました (イベント数: %d)。\n", filename, g_sequence_length);
}


// ============================================================================
// 描画処理
// ============================================================================

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int window_width = glutGet(GLUT_WINDOW_WIDTH);
    int window_height = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (GLfloat)window_width / (window_height == 0 ? 1 : window_height), 0.1, 150.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(g_camera_pos[0], g_camera_pos[1], g_camera_pos[2],
        g_camera_pos[0] + g_camera_front[0],
        g_camera_pos[1] + g_camera_front[1],
        g_camera_pos[2] + g_camera_front[2],
        g_camera_up[0], g_camera_up[1], g_camera_up[2]);

    GLfloat light_pos[] = { 0.0f, 5.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    draw_floor();
    draw_piano_body();
    draw_piano_keys();
    draw_buttons();
    draw_hud();
    draw_reticle();

    glutSwapBuffers();
}

void draw_floor() {
    GLfloat floor_mat_ambient[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    GLfloat floor_mat_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_texture_wood);

    glMaterialfv(GL_FRONT, GL_AMBIENT, floor_mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, floor_mat_diffuse);

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(-100.0f, -1.8f, -100.0f);
    glTexCoord2f(20.0f, 0.0f);  glVertex3f(100.0f, -1.8f, -100.0f);
    glTexCoord2f(20.0f, 20.0f); glVertex3f(100.0f, -1.8f, 100.0f);
    glTexCoord2f(0.0f, 20.0f);  glVertex3f(-100.0f, -1.8f, 100.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void draw_piano_body() {
    glMaterialfv(GL_FRONT, GL_AMBIENT, MAT_BLACK_PLASTIC_AMBIENT);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, MAT_BLACK_PLASTIC_DIFFUSE);
    glMaterialfv(GL_FRONT, GL_SPECULAR, MAT_BLACK_PLASTIC_SPECULAR);
    glMaterialf(GL_FRONT, GL_SHININESS, MAT_BLACK_PLASTIC_SHININESS);

    glPushMatrix();
    glTranslatef(-3.5f, -0.8f, 1.5f);
    glCallList(g_model_piano_body.display_list_id);
    glPopMatrix();
}

void draw_piano_keys() {
    for (int i = 0; i < PIANO_KEY_COUNT; ++i) {
        piano_key_t* key = &g_piano_keys[i];
        glPushMatrix();

        glTranslatef(key->center_pos[0], key->center_pos[1], key->center_pos[2]);
        glTranslatef(0.0f, key->current_y_pos, 0.0f);

        if (key->type == KEY_TYPE_WHITE) {
            glMaterialfv(GL_FRONT, GL_AMBIENT, MAT_IVORY_AMBIENT);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, MAT_IVORY_DIFFUSE);
            glMaterialfv(GL_FRONT, GL_SPECULAR, MAT_IVORY_SPECULAR);
            glMaterialf(GL_FRONT, GL_SHININESS, MAT_IVORY_SHININESS);
            glCallList(g_model_white_key.display_list_id);
        }
        else {
            glMaterialfv(GL_FRONT, GL_AMBIENT, MAT_BLACK_MATTE_AMBIENT);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, MAT_BLACK_MATTE_DIFFUSE);
            glMaterialfv(GL_FRONT, GL_SPECULAR, MAT_BLACK_MATTE_SPECULAR);
            glMaterialf(GL_FRONT, GL_SHININESS, MAT_BLACK_MATTE_SHININESS);
            glCallList(g_model_black_key.display_list_id);
        }

        glPopMatrix();
    }
}

void draw_buttons() {
    glMaterialfv(GL_FRONT, GL_AMBIENT, MAT_GREY_PLASTIC_AMBIENT);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, MAT_GREY_PLASTIC_DIFFUSE);
    glMaterialfv(GL_FRONT, GL_SPECULAR, MAT_GREY_PLASTIC_SPECULAR);
    glMaterialf(GL_FRONT, GL_SHININESS, MAT_GREY_PLASTIC_SHININESS);

    for (int i = 0; i < TIMBRE_BUTTON_COUNT; ++i) {
        glPushMatrix();
        glTranslatef(TIMBRE_BUTTON_X_POSITIONS[i], BUTTON_Y, BUTTON_Z);
        glCallList(g_model_timbre_button.display_list_id);
        glPopMatrix();
    }

    for (int i = 0; i < OCTAVE_BUTTON_COUNT; ++i) {
        glPushMatrix();
        glTranslatef(OCTAVE_BUTTON_POSITIONS[i][0], OCTAVE_BUTTON_POSITIONS[i][1], OCTAVE_BUTTON_POSITIONS[i][2]);
        glRotatef(OCTAVE_BUTTON_POSITIONS[i][3], 0.0f, 1.0f, 0.0f);
        glCallList(g_model_octave_button.display_list_id);
        glPopMatrix();
    }
}

void draw_hud() {
    int window_width = glutGet(GLUT_WINDOW_WIDTH);
    int window_height = glutGet(GLUT_WINDOW_HEIGHT);
    char text_buffer[256];

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, window_width, 0, window_height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf_s(text_buffer, sizeof(text_buffer), "Octave: %+d", g_current_octave_shift);
    glRasterPos2i(HUD_MARGIN_X, window_height - HUD_MARGIN_Y);
    for (int i = 0; text_buffer[i] != '\0'; ++i) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text_buffer[i]);
    }

    sprintf_s(text_buffer, sizeof(text_buffer), "Timbre: %s", g_timbres[g_current_timbre_index].name);
    glRasterPos2i(HUD_MARGIN_X, window_height - HUD_MARGIN_Y - HUD_LINE_HEIGHT);
    for (int i = 0; text_buffer[i] != '\0'; ++i) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text_buffer[i]);
    }

    if (g_is_sequencer_playing) {
        glColor3f(0.0f, 1.0f, 0.0f);
        sprintf_s(text_buffer, sizeof(text_buffer), "Sequencer: Playing");
    }
    else {
        glColor3f(1.0f, 1.0f, 1.0f);
        sprintf_s(text_buffer, sizeof(text_buffer), "Sequencer: Stopped");
    }
    glRasterPos2i(HUD_MARGIN_X, window_height - HUD_MARGIN_Y - (HUD_LINE_HEIGHT * 2));
    for (int i = 0; text_buffer[i] != '\0'; ++i) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text_buffer[i]);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void draw_reticle() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex2f(w / 2.0f - RETICLE_SIZE, h / 2.0f);
    glVertex2f(w / 2.0f + RETICLE_SIZE, h / 2.0f);
    glVertex2f(w / 2.0f, h / 2.0f - RETICLE_SIZE);
    glVertex2f(w / 2.0f, h / 2.0f + RETICLE_SIZE);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


// ============================================================================
// 入力・イベント処理
// ============================================================================

void on_mouse_button(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON) return;

    if (state == GLUT_DOWN) {
        vector_3d_t click_pos = get_world_pos_from_screen_center();
        int is_object_found = 0;

        for (int i = 0; i < PIANO_KEY_COUNT; ++i) {
            piano_key_t* key = &g_piano_keys[i];
            model_3d_t* model = (key->type == KEY_TYPE_WHITE) ? &g_model_white_key : &g_model_black_key;
            bounding_box_t world_bbox;
            world_bbox.min.x = key->center_pos[0] + model->local_bbox.min.x;
            world_bbox.max.x = key->center_pos[0] + model->local_bbox.max.x;
            world_bbox.min.y = key->center_pos[1] + model->local_bbox.min.y;
            world_bbox.max.y = key->center_pos[1] + model->local_bbox.max.y;
            world_bbox.min.z = key->center_pos[2] + model->local_bbox.min.z;
            world_bbox.max.z = key->center_pos[2] + model->local_bbox.max.z;

            if (is_point_in_box(click_pos, world_bbox)) {
                trigger_note_on(key->midi_note);
                is_object_found = 1;
                break;
            }
        }
        if (is_object_found) { glutPostRedisplay(); return; }

        for (int i = 0; i < TIMBRE_BUTTON_COUNT; ++i) {
            bounding_box_t world_bbox;
            world_bbox.min.x = TIMBRE_BUTTON_X_POSITIONS[i] + g_model_timbre_button.local_bbox.min.x;
            world_bbox.max.x = TIMBRE_BUTTON_X_POSITIONS[i] + g_model_timbre_button.local_bbox.max.x;
            world_bbox.min.y = BUTTON_Y + g_model_timbre_button.local_bbox.min.y;
            world_bbox.max.y = BUTTON_Y + g_model_timbre_button.local_bbox.max.y;
            world_bbox.min.z = BUTTON_Z + g_model_timbre_button.local_bbox.min.z;
            world_bbox.max.z = BUTTON_Z + g_model_timbre_button.local_bbox.max.z;

            if (is_point_in_box(click_pos, world_bbox)) {
                printf("情報: 音色を「%s」に変更しました。\n", g_timbres[i].name);
                g_current_timbre_index = i;
                is_object_found = 1;
                break;
            }
        }
        if (is_object_found) { glutPostRedisplay(); return; }

        for (int i = 0; i < OCTAVE_BUTTON_COUNT; ++i) {
            bounding_box_t world_bbox;
            world_bbox.min.x = OCTAVE_BUTTON_POSITIONS[i][0] + g_model_octave_button.local_bbox.min.x;
            world_bbox.max.x = OCTAVE_BUTTON_POSITIONS[i][0] + g_model_octave_button.local_bbox.max.x;
            world_bbox.min.y = OCTAVE_BUTTON_POSITIONS[i][1] + g_model_octave_button.local_bbox.min.y;
            world_bbox.max.y = OCTAVE_BUTTON_POSITIONS[i][1] + g_model_octave_button.local_bbox.max.y;
            world_bbox.min.z = OCTAVE_BUTTON_POSITIONS[i][2] + g_model_octave_button.local_bbox.min.z;
            world_bbox.max.z = OCTAVE_BUTTON_POSITIONS[i][2] + g_model_octave_button.local_bbox.max.z;

            if (is_point_in_box(click_pos, world_bbox)) {
                if (i == 0) { // Octave Down
                    if (g_current_octave_shift > OCTAVE_SHIFT_MIN) {
                        g_current_octave_shift--;
                        printf("情報: オクターブを下げました (%+d)。\n", g_current_octave_shift);
                    }
                    
                }
                else { // Octave Up
                    if (g_current_octave_shift < OCTAVE_SHIFT_MAX) {
                        g_current_octave_shift++;
                        printf("情報: オクターブを上げました (%+d)。\n", g_current_octave_shift);
                    }
                }
                is_object_found = 1;
                break;
            }
        }
        if (is_object_found) { glutPostRedisplay(); return; }

    }
    else if (state == GLUT_UP) {
        for (int i = 0; i < PIANO_KEY_COUNT; ++i) {
            trigger_note_off(g_piano_keys[i].midi_note);
        }
        glutPostRedisplay();
    }
}

void on_keyboard_press(unsigned char key, int x, int y) {
    float front_h_x = g_camera_front[0];
    float front_h_z = g_camera_front[2];
    float len = sqrt(front_h_x * front_h_x + front_h_z * front_h_z);
    if (len > 0) {
        front_h_x /= len;
        front_h_z /= len;
    }
    float right_h_x = -front_h_z;
    float right_h_z = front_h_x;

    switch (key) {
    case 'w': g_camera_pos[0] += front_h_x * CAMERA_MOVE_SPEED; g_camera_pos[2] += front_h_z * CAMERA_MOVE_SPEED; break;
    case 's': g_camera_pos[0] -= front_h_x * CAMERA_MOVE_SPEED; g_camera_pos[2] -= front_h_z * CAMERA_MOVE_SPEED; break;
    case 'a': g_camera_pos[0] -= right_h_x * CAMERA_MOVE_SPEED; g_camera_pos[2] -= right_h_z * CAMERA_MOVE_SPEED; break;
    case 'd': g_camera_pos[0] += right_h_x * CAMERA_MOVE_SPEED; g_camera_pos[2] += right_h_z * CAMERA_MOVE_SPEED; break;
    case 'q': g_camera_pos[1] += CAMERA_MOVE_SPEED; break;
    case 'e': g_camera_pos[1] -= CAMERA_MOVE_SPEED; break;
    case 27:  exit(0); break;
    }
    glutPostRedisplay();
}

void on_mouse_move(int x, int y) {
    int center_x = glutGet(GLUT_WINDOW_WIDTH) / 2;
    int center_y = glutGet(GLUT_WINDOW_HEIGHT) / 2;

    if (x == center_x && y == center_y) return;

    float offset_x = (float)(x - center_x) * MOUSE_SENSITIVITY;
    float offset_y = (float)(center_y - y) * MOUSE_SENSITIVITY;

    g_camera_yaw += offset_x;
    g_camera_pitch += offset_y;

    if (g_camera_pitch > CAMERA_PITCH_MAX) g_camera_pitch = CAMERA_PITCH_MAX;
    if (g_camera_pitch < CAMERA_PITCH_MIN) g_camera_pitch = CAMERA_PITCH_MIN;

    float rad_yaw = g_camera_yaw * M_PI / 180.0f;
    float rad_pitch = g_camera_pitch * M_PI / 180.0f;
    g_camera_front[0] = cos(rad_yaw) * cos(rad_pitch);
    g_camera_front[1] = sin(rad_pitch);
    g_camera_front[2] = sin(rad_yaw) * cos(rad_pitch);

    float len = sqrt(pow(g_camera_front[0], 2) + pow(g_camera_front[1], 2) + pow(g_camera_front[2], 2));
    g_camera_front[0] /= len;
    g_camera_front[1] /= len;
    g_camera_front[2] /= len;

    glutWarpPointer(center_x, center_y);
    glutPostRedisplay();
}

void on_menu_select(int menu_id) {
    switch (menu_id) {
    case MENU_ID_SEQ_PLAY:
        if (!g_is_sequencer_playing && g_sequence_length > 0) {
            g_is_sequencer_playing = 1;
            g_sequence_index = 0;
            play_next_sequence_note(0);
            printf("情報: シーケンスの再生を開始しました。\n");
        }
        break;
    case MENU_ID_SEQ_STOP:
        if (g_is_sequencer_playing) {
            g_is_sequencer_playing = 0;
            play_next_sequence_note(0);
            printf("情報: シーケンスを停止しました。\n");
        }
        break;
    }
}


// ============================================================================
// アニメーション・シーケンサー
// ============================================================================

void update_key_animation(int timer_value) {
    int needs_redisplay = 0;

    for (int i = 0; i < PIANO_KEY_COUNT; ++i) {
        piano_key_t* key = &g_piano_keys[i];
        float diff = key->target_y_pos - key->current_y_pos;

        if (fabs(diff) > 0.001f) {
            key->current_y_pos += diff * KEY_ANIMATION_SPEED;
            needs_redisplay = 1;
        }
        else {
            key->current_y_pos = key->target_y_pos;
        }
    }

    if (needs_redisplay) {
        glutPostRedisplay();
    }

    glutTimerFunc(ANIMATION_TIMER_MS, update_key_animation, 0);
}

void play_next_sequence_note(int previous_midi_note) {
    if (!g_is_sequencer_playing) {
        for (int i = 0; i < PIANO_KEY_COUNT; ++i) {
            trigger_note_off(g_piano_keys[i].midi_note);
        }
        glutPostRedisplay();
        return;
    }

    if (g_sequence_index >= g_sequence_length) {
        trigger_note_off(previous_midi_note);
        g_is_sequencer_playing = 0;
        printf("情報: シーケンスの再生が終了しました。\n");
        glutPostRedisplay();
        return;
    }

    sequence_event_t* current_event = &g_sequence[g_sequence_index];

    if (previous_midi_note > 0 && previous_midi_note != current_event->midi_note) {
        trigger_note_off(previous_midi_note);
    }

    trigger_note_on(current_event->midi_note);

    g_sequence_index++;
    glutPostRedisplay();

    glutTimerFunc((unsigned int)current_event->duration_ms, play_next_sequence_note, current_event->midi_note);
}


// ============================================================================
// オーディオ処理
// ============================================================================

void audio_callback(ma_device* p_device, void* p_output, const void* p_input, ma_uint32 frame_count) {
    piano_key_t* keys = (piano_key_t*)p_device->pUserData;
    float* output_buffer = (float*)p_output;
    (void)p_input;

    const double attack_increment = 1.0 / (AUDIO_SAMPLE_RATE * AUDIO_ATTACK_TIME_S);
    const double release_decrement = 1.0 / (AUDIO_SAMPLE_RATE * AUDIO_RELEASE_TIME_S);
    timbre_t* current_timbre = &g_timbres[g_current_timbre_index];

    for (ma_uint32 i = 0; i < frame_count; i++) {
        double mixed_sample = 0.0;

        for (int k = 0; k < PIANO_KEY_COUNT; ++k) {
            piano_key_t* key = &keys[k];
            if (key->envelope_state == ENV_STATE_OFF) continue;

            switch (key->envelope_state) {
            case ENV_STATE_ATTACK:
                key->current_amplitude += attack_increment;
                if (key->current_amplitude >= 1.0) {
                    key->current_amplitude = 1.0;
                    key->envelope_state = ENV_STATE_PRESSED;
                }
                break;
            case ENV_STATE_RELEASING:
                key->current_amplitude -= release_decrement;
                if (key->current_amplitude <= 0.0) {
                    key->current_amplitude = 0.0;
                    key->envelope_state = ENV_STATE_OFF;
                }
                break;
            default: break;
            }

            if (key->envelope_state != ENV_STATE_OFF) {
                double frequency = midi_to_freq(key->midi_note + (g_current_octave_shift * 12));
                double key_sample = 0.0;

                for (int h = 0; h < current_timbre->harmonic_count; ++h) {
                    harmonic_t* harmonic = &current_timbre->harmonics[h];
                    double harmonic_freq_multiplier = (double)(h + 1);
                    key_sample += harmonic->amplitude * sin(key->wave_phase * harmonic_freq_multiplier + harmonic->phase_shift);
                }

                mixed_sample += key->current_amplitude * key_sample;

                key->wave_phase += 2.0 * M_PI * frequency / AUDIO_SAMPLE_RATE;
                if (key->wave_phase >= 2.0 * M_PI) {
                    key->wave_phase -= 2.0 * M_PI;
                }
            }
        }

        if (mixed_sample > 1.0) mixed_sample = 1.0;
        if (mixed_sample < -1.0) mixed_sample = -1.0;

        output_buffer[0] = (float)mixed_sample;
        output_buffer[1] = (float)mixed_sample;
        output_buffer += 2;
    }
}

void trigger_note_on(int midi_note) {
    if (midi_note <= 0) return;

    for (int i = 0; i < PIANO_KEY_COUNT; ++i) {
        if (g_piano_keys[i].midi_note == midi_note) {
            piano_key_t* key = &g_piano_keys[i];
            if (key->envelope_state == ENV_STATE_OFF) {
                key->envelope_state = ENV_STATE_ATTACK;
                key->current_amplitude = 0.0;
                key->wave_phase = 0.0;
            }
            key->target_y_pos = KEY_PRESSED_Y_OFFSET;
            return;
        }
    }
}

void trigger_note_off(int midi_note) {
    if (midi_note <= 0) return;

    for (int i = 0; i < PIANO_KEY_COUNT; ++i) {
        if (g_piano_keys[i].midi_note == midi_note) {
            piano_key_t* key = &g_piano_keys[i];
            if (key->envelope_state == ENV_STATE_ATTACK || key->envelope_state == ENV_STATE_PRESSED) {
                key->envelope_state = ENV_STATE_RELEASING;
            }
            key->target_y_pos = 0.0f;
            return;
        }
    }
}


// ============================================================================
// ユーティリティ
// ============================================================================

float midi_to_freq(int midi_note) {
    return FREQUENCY_A4 * powf(2.0f, (midi_note - MIDI_NOTE_A4) / 12.0f);
}

vector_3d_t get_world_pos_from_screen_center() {
    GLdouble modelview[16], projection[16];
    GLint viewport[4];
    float win_z;
    vector_3d_t world_pos;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    int win_x = viewport[2] / 2;
    int win_y = viewport[3] / 2;

    glReadPixels(win_x, win_y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &win_z);

    gluUnProject((GLdouble)win_x, (GLdouble)win_y, (GLdouble)win_z,
        modelview, projection, viewport,
        &world_pos.x, &world_pos.y, &world_pos.z);
    return world_pos;
}

int is_point_in_box(vector_3d_t point, bounding_box_t box) {
    return (point.x >= box.min.x && point.x <= box.max.x &&
        point.y >= box.min.y && point.y <= box.max.y &&
        point.z >= box.min.z && point.z <= box.max.z);
}
