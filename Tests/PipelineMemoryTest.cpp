// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>

#include "EngineWrapper.hpp"
#include "Config.hpp"
#include "Macro.hpp"
#include "Ipc.hpp"
#include "Types.hpp"

using namespace senkey;

static void test_high_volume_pipeline() {
    EngineWrapper engine;
    SenKeyConfig cfg;
    cfg.input_method = InputMethod::TELEX;
    cfg.macro_enabled = true;
    engine.apply_config(cfg);

    const std::vector<std::string> sample_words = {
        "coong", "hoa2", "xax", "hooi5", "chu3", "nghi4a",
        "vieetj", "nam", "ddoocj", "laappj", "tuwj", "do",
        "hajn", "phucs", "tieengs", "vietj", "thaan", "yeu"
    };

    // Kiểm thử áp lực: 500 vòng lặp gõ các câu hoàn chỉnh
    for (int round = 0; round < 500; ++round) {
        std::string buffer;
        for (const auto& w : sample_words) {
            for (char c : w) {
                int backs = 0;
                std::string rep;
                if (engine.process_key(c, backs, rep)) {
                    if (backs > 0 && backs <= static_cast<int>(buffer.size())) {
                        buffer.erase(buffer.size() - backs);
                    }
                    buffer += rep;
                } else {
                    buffer += c;
                }
            }
            // Phím cách kết thúc từ và đẩy ký tự ra luồng
            int backs = 0;
            std::string rep;
            if (engine.process_key(' ', backs, rep)) {
                if (backs > 0 && backs <= static_cast<int>(buffer.size())) {
                    buffer.erase(buffer.size() - backs);
                }
                buffer += rep;
            } else {
                buffer += ' ';
            }
        }
        // Định kỳ đặt lại bộ đệm và trạng thái bộ gõ
        engine.reset();
    }

    std::cout << "[PASS] High volume pipeline test (10,000+ keystrokes) passed.\n";
}

static void test_macro_stability() {
    std::string test_macro_file = "/tmp/senkey_test_macro.txt";
    unlink(test_macro_file.c_str());

    {
        MacroManager mm(test_macro_file);
        for (int i = 0; i < 500; ++i) {
            mm.add("key" + std::to_string(i), "value" + std::to_string(i));
        }
        assert(mm.size() == 500);
        assert(mm.lookup("key123") == "value123");
        mm.save();
    }

    {
        MacroManager mm2(test_macro_file);
        mm2.load();
        assert(mm2.size() == 500);
        assert(mm2.lookup("key456") == "value456");
        for (int i = 0; i < 250; ++i) {
            mm2.remove("key" + std::to_string(i));
        }
        assert(mm2.size() == 250);
        mm2.save();
    }

    unlink(test_macro_file.c_str());
    std::cout << "[PASS] MacroManager lifecycle and memory test passed.\n";
}

static void test_config_lifecycle() {
    std::string test_cfg_file = "/tmp/senkey_test.conf";
    unlink(test_cfg_file.c_str());

    {
        ConfigManager cm(test_cfg_file);
        SenKeyConfig& cfg = cm.get_mutable();
        cfg.input_method = InputMethod::VNI;
        cfg.hotkey = HotkeyToggle::ALT_Z;
        cfg.micro_delay_us = 1800;
        cm.save();
    }

    {
        ConfigManager cm2(test_cfg_file);
        cm2.load();
        const SenKeyConfig& cfg = cm2.get();
        assert(cfg.input_method == InputMethod::VNI);
        assert(cfg.hotkey == HotkeyToggle::ALT_Z);
        assert(cfg.micro_delay_us == 1800);
        (void)cfg;
    }

    unlink(test_cfg_file.c_str());
    std::cout << "[PASS] ConfigManager lifecycle test passed.\n";
}

static void test_ipc_lifecycle() {
    IpcServer server;
    bool handler_called = false;
    bool started = server.start([&](const std::string& cmd) -> std::string {
        if (cmd == "PING") {
            handler_called = true;
            return "PONG";
        }
        return "UNKNOWN";
    });
    assert(started);
    (void)started;

    std::string resp;
    bool sent = IpcServer::send_command("PING", resp);
    assert(sent);
    assert(resp == "PONG");
    assert(handler_called);
    (void)sent;

    server.stop();
    std::cout << "[PASS] IpcServer lifecycle and connection test passed.\n";
}

int main() {
    test_high_volume_pipeline();
    test_macro_stability();
    test_config_lifecycle();
    test_ipc_lifecycle();
    std::cout << "All Pipeline and Memory Stability tests passed successfully.\n";
    return 0;
}
