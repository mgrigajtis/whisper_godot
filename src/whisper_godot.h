#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "../whisper_cpp/whisper.h"
#include "../whisper_cpp/examples/common.h"

using namespace godot;

class WhisperGodot : public Node
{
	GDCLASS(WhisperGodot, Node);

	private:
		struct whisper_context *ctx;

	protected:
		static void _bind_methods();

	public:
		WhisperGodot();
		~WhisperGodot();
		bool init(godot::String file_location);
		godot::String convert_audio_file(godot::String file_location);
};