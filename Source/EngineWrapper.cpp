// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "EngineWrapper.hpp"
#include "keycons.h"
#include "vnconv.h"
#include <cstring>

namespace senkey {

EngineWrapper::EngineWrapper() {
    std::memset(&shared_mem, 0, sizeof(shared_mem));
    shared_mem.input.init();
    shared_mem.input.setIM(UkTelex);
    shared_mem.charsetId = CONV_CHARSET_XUTF8;
    shared_mem.vietKey = true;
    shared_mem.options.autoNonVnRestore = 1;
    shared_mem.options.freeMarking = 1;
    shared_mem.options.modernStyle = 1;
    shared_mem.options.spellCheckEnabled = 1;
    shared_mem.usrKeyMapLoaded = false;

    engine.setCtrlInfo(&shared_mem);
}

void EngineWrapper::apply_config(const SenKeyConfig& config) {
    if (config.input_method == InputMethod::VNI) {
        shared_mem.input.setIM(UkVni);
    } else if (config.input_method == InputMethod::SIMPLE_TELEX) {
        shared_mem.input.setIM(UkSimpleTelex);
    } else {
        shared_mem.input.setIM(UkTelex);
    }

    if (config.charset == Charset::TCVN3) {
        shared_mem.charsetId = CONV_CHARSET_TCVN3;
    } else if (config.charset == Charset::VNI_WINDOWS) {
        shared_mem.charsetId = CONV_CHARSET_VNIWIN;
    } else {
        shared_mem.charsetId = CONV_CHARSET_XUTF8;
    }

    shared_mem.options.modernStyle = config.modern_spelling ? 1 : 0;
    shared_mem.options.freeMarking = config.free_marking ? 1 : 0;
    shared_mem.options.spellCheckEnabled = config.spell_check ? 1 : 0;
    shared_mem.options.autoNonVnRestore = config.auto_non_vn_restore ? 1 : 0;
    macro_enabled = config.macro_enabled;

    engine.reset();
    current_raw_word.clear();
}

void EngineWrapper::load_macros(const std::string& custom_path) {
    if (!custom_path.empty()) {
        macro_mgr = MacroManager(custom_path);
    }
    macro_mgr.load();
}

bool EngineWrapper::process_key(char c, int& out_backs, std::string& out_str) {
    out_backs = 0;
    out_str.clear();

    if (macro_enabled && (c == ' ' || c == '.' || c == ',' || c == '\n')) {
        if (!current_raw_word.empty()) {
            std::string expansion = macro_mgr.lookup(current_raw_word);
            if (!expansion.empty()) {
                out_backs = static_cast<int>(current_raw_word.size());
                out_str = expansion + c;
                reset();
                return true;
            }
        }
        reset();
        return false;
    }

    current_raw_word += c;

    unsigned char outBuf[64] = {0};
    int outSize = sizeof(outBuf);
    UkOutputType outType;

    int processed = engine.process(static_cast<unsigned int>(c), out_backs, outBuf, outSize, outType);
    if (processed) {
        if (outSize > 0) {
            out_str = std::string(reinterpret_cast<char*>(outBuf), outSize);
        }
        return true;
    }
    return false;
}

bool EngineWrapper::process_backspace(int& out_backs, std::string& out_str) {
    out_backs = 0;
    out_str.clear();

    if (!current_raw_word.empty()) {
        current_raw_word.pop_back();
    }

    unsigned char outBuf[64] = {0};
    int outSize = sizeof(outBuf);
    UkOutputType outType;

    int processed = engine.processBackspace(out_backs, outBuf, outSize, outType);
    if (processed) {
        if (outSize > 0) {
            out_str = std::string(reinterpret_cast<char*>(outBuf), outSize);
        }
        return true;
    }
    return false;
}

void EngineWrapper::reset() {
    engine.reset();
    current_raw_word.clear();
}

} // namespace senkey
