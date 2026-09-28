// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <iostream>
#include <cassert>
#include <vector>
#include <string>

#include "EngineWrapper.hpp"
#include "Types.hpp"

using namespace senkey;

static void pop_back_utf8(std::string& s, int count) {
    for (int i = 0; i < count && !s.empty(); ++i) {
        while (!s.empty()) {
            unsigned char c = static_cast<unsigned char>(s.back());
            s.pop_back();
            if ((c & 0xC0) != 0x80) {
                break;
            }
        }
    }
}

static void test_telex_basic() {
    EngineWrapper engine;
    SenKeyConfig cfg;
    cfg.input_method = InputMethod::TELEX;
    cfg.modern_spelling = true;
    cfg.spell_check = true;
    engine.apply_config(cfg);

    struct TestCase {
        std::string input;
        std::string expected_final;
    };

    std::vector<TestCase> cases = {
        {"vieetj", "việt"},
        {"tieengs", "tiếng"},
        {"ddoongf", "đồng"},
        {"dduwowngf", "đường"},
        {"nguowif", "người"},
        {"quaanf", "quần"},
        {"chieecs", "chiếc"},
        {"toor", "tổ"},
        {"chaof", "chào"},
        {"cacs", "các"},
        {"bajn", "bạn"}
    };

    for (const auto& tc : cases) {
        engine.reset();
        std::string current;
        for (char c : tc.input) {
            int backs = 0;
            std::string rep;
            if (engine.process_key(c, backs, rep)) {
                pop_back_utf8(current, backs);
                current += rep;
            } else {
                current += c;
            }
        }
        if (current != tc.expected_final) {
            std::cerr << "FAIL Telex: input='" << tc.input << "', expected='"
                      << tc.expected_final << "', got='" << current << "'\n";
            std::exit(1);
        }
    }
    std::cout << "[PASS] Telex basic typing tests passed.\n";
}

static void test_vni_basic() {
    EngineWrapper engine;
    SenKeyConfig cfg;
    cfg.input_method = InputMethod::VNI;
    cfg.modern_spelling = true;
    engine.apply_config(cfg);

    struct TestCase {
        std::string input;
        std::string expected_final;
    };

    std::vector<TestCase> cases = {
        {"viet65", "việt"},
        {"tie6ng1", "tiếng"},
        {"d9o6ng2", "đồng"},
        {"d9u7o7ng2", "đường"}
    };

    for (const auto& tc : cases) {
        engine.reset();
        std::string current;
        for (char c : tc.input) {
            int backs = 0;
            std::string rep;
            if (engine.process_key(c, backs, rep)) {
                pop_back_utf8(current, backs);
                current += rep;
            } else {
                current += c;
            }
        }
        if (current != tc.expected_final) {
            std::cerr << "FAIL VNI: input='" << tc.input << "', expected='"
                      << tc.expected_final << "', got='" << current << "'\n";
            std::exit(1);
        }
    }
    std::cout << "[PASS] VNI basic typing tests passed.\n";
}

static void test_backspace_handling() {
    EngineWrapper engine;
    SenKeyConfig cfg;
    cfg.input_method = InputMethod::TELEX;
    engine.apply_config(cfg);

    // Gõ chuỗi "vieetj" -> kết quả mong đợi là "việt"
    std::string current;
    for (char c : std::string("vieetj")) {
        int backs = 0;
        std::string rep;
        if (engine.process_key(c, backs, rep)) {
            pop_back_utf8(current, backs);
            current += rep;
        } else {
            current += c;
        }
    }
    assert(current == "việt");

    // Nhấn phím xóa lùi (Backspace)
    int backs = 0;
    std::string rep;
    if (engine.process_backspace(backs, rep)) {
        pop_back_utf8(current, backs);
        current += rep;
    } else {
        pop_back_utf8(current, 1);
    }
    // Sau 1 lần xóa lùi, bộ gõ phục hồi trạng thái ký tự trước đó
    assert(!current.empty());
    std::cout << "[PASS] Backspace handling test passed.\n";
}

int main() {
    test_telex_basic();
    test_vni_basic();
    test_backspace_handling();
    std::cout << "All Engine tests completed successfully.\n";
    return 0;
}
