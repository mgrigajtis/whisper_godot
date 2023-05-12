#include "whisper_godot.h"
#include <thread>
#include <fstream>
#include <cstdio>
#include <string>
#include <vector>
#include <cstring>

using namespace godot;

struct whisper_params {
    int32_t n_threads    =  4;
    int32_t n_processors =  1;
    int32_t offset_t_ms  =  0;
    int32_t offset_n     =  0;
    int32_t duration_ms  =  0;
    int32_t max_context  = -1;
    int32_t max_len      =  0;
    int32_t best_of      =  2;
    int32_t beam_size    = -1;

    float word_thold    =  0.01f;
    float entropy_thold =  2.40f;
    float logprob_thold = -1.00f;

    bool speed_up       = false;
    bool translate      = false;
    bool detect_language= false;
    bool diarize        = false;
    bool split_on_word  = false;
    bool no_fallback    = false;
    bool output_txt     = false;
    bool output_vtt     = false;
    bool output_srt     = false;
    bool output_wts     = false;
    bool output_csv     = false;
    bool output_jsn     = false;
    bool output_lrc     = false;
    bool print_special  = false;
    bool print_colors   = false;
    bool print_progress = false;
    bool no_timestamps  = false;

    std::string language = "en";
    std::string prompt;
    std::string font_path = "/System/Library/Fonts/Supplemental/Courier New Bold.ttf";
    std::string model    = "models/ggml-base.en.bin";

    std::vector<std::string> fname_inp = {};
    std::vector<std::string> fname_out = {};
};

WhisperGodot::WhisperGodot() {
    // Constructor implementation
}

void WhisperGodot::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("init", "file_location"), &WhisperGodot::init);
    ClassDB::bind_method(D_METHOD("convert_audio_file", "file_location"), &WhisperGodot::convert_audio_file);
}

WhisperGodot::~WhisperGodot() {
    whisper_free(ctx);
}

bool WhisperGodot::init(godot::String file_location)
{
    ctx = whisper_init_from_file(file_location.utf8().get_data());

    return (ctx != nullptr);
}

godot::String WhisperGodot::convert_audio_file(godot::String file_location)
{
    whisper_params params;

    // mono-channel F32 PCM
    std::vector<float> pcmf32; 

    // stereo-channel F32 PCM
    std::vector<std::vector<float>> pcmf32s; 

    if (!::read_wav(file_location.utf8().get_data(), pcmf32, pcmf32s, params.diarize)) {
        return "error: failed to read WAV file '%s'\n", file_location.utf8().get_data();
    }

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.strategy = params.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY;

    wparams.print_realtime   = false;
    wparams.print_progress   = params.print_progress;
    wparams.print_timestamps = !params.no_timestamps;
    wparams.print_special    = params.print_special;
    wparams.translate        = params.translate;
    wparams.language         = params.language.c_str();
    wparams.detect_language  = params.detect_language;
    wparams.n_threads        = params.n_threads;
    wparams.n_max_text_ctx   = params.max_context >= 0 ? params.max_context : wparams.n_max_text_ctx;
    wparams.offset_ms        = params.offset_t_ms;
    wparams.duration_ms      = params.duration_ms;

    wparams.token_timestamps = params.output_wts || params.max_len > 0;
    wparams.thold_pt         = params.word_thold;
    wparams.max_len          = params.output_wts && params.max_len == 0 ? 60 : params.max_len;
    wparams.split_on_word    = params.split_on_word;

    wparams.speed_up         = params.speed_up;

    wparams.initial_prompt   = params.prompt.c_str();

    wparams.greedy.best_of        = params.best_of;
    wparams.beam_search.beam_size = params.beam_size;

    wparams.temperature_inc  = params.no_fallback ? 0.0f : wparams.temperature_inc;
    wparams.entropy_thold    = params.entropy_thold;
    wparams.logprob_thold    = params.logprob_thold;

    if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
        return "Failed to process audio";
    }

    godot::String text_output = "";
    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) { 
        const char * text = whisper_full_get_segment_text(ctx, i);
        text_output += text;
    }

    return text_output;
}