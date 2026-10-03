// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "ClipboardBridge.hpp"
#include "Logger.hpp"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <cstdlib>

namespace senkey {

ClipboardBridge::ClipboardBridge() {
    init_x11();
}

ClipboardBridge::~ClipboardBridge() {
    cleanup_x11();
}

void ClipboardBridge::init_x11() {
    std::lock_guard<std::mutex> lock(clip_mutex);

    const char* disp_env = std::getenv("DISPLAY");
    if (disp_env && disp_env[0] != '\0') {
        dpy = XOpenDisplay(disp_env);
    }
    if (!dpy) {
        dpy = XOpenDisplay(":0");
    }
    if (!dpy) {
        dpy = XOpenDisplay(":1");
    }

    if (!dpy) {
        Logger::warn("ClipboardBridge: Unable to open X11 display. Clipboard injection will fallback to unicode hex.");
        return;
    }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    win = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);

    clipboard_atom = XInternAtom(dpy, "CLIPBOARD", False);
    utf8_atom = XInternAtom(dpy, "UTF8_STRING", False);
    targets_atom = XInternAtom(dpy, "TARGETS", False);
    prop_atom = XInternAtom(dpy, "SENKEY_CLIP_PROP", False);

    Logger::info("ClipboardBridge: Connected to X11 display for instant atomic clipboard swap.");
}

void ClipboardBridge::cleanup_x11() {
    std::lock_guard<std::mutex> lock(clip_mutex);
    if (dpy) {
        if (win) {
            XDestroyWindow(dpy, win);
            win = 0;
        }
        XCloseDisplay(dpy);
        dpy = nullptr;
    }
}

bool ClipboardBridge::is_valid() const {
    return (dpy != nullptr && win != 0);
}

std::string ClipboardBridge::get_current_text(int timeout_ms) {
    if (!is_valid()) return "";
    std::lock_guard<std::mutex> lock(clip_mutex);

    // Request clipboard contents converted to UTF8_STRING onto our property
    XConvertSelection(dpy, clipboard_atom, utf8_atom, prop_atom, win, CurrentTime);
    XFlush(dpy);

    auto start = std::chrono::steady_clock::now();
    XEvent ev;
    while (true) {
        if (XCheckTypedWindowEvent(dpy, win, SelectionNotify, &ev)) {
            if (ev.xselection.property == None) {
                return ""; // Target application/owner could not convert to UTF8
            }

            Atom actual_type;
            int actual_format;
            unsigned long nitems = 0, bytes_after = 0;
            unsigned char* prop_data = nullptr;

            XGetWindowProperty(dpy, win, prop_atom, 0, 65536, True,
                               AnyPropertyType, &actual_type, &actual_format,
                               &nitems, &bytes_after, &prop_data);

            std::string res;
            if (prop_data && nitems > 0) {
                res = std::string(reinterpret_cast<char*>(prop_data), nitems);
                XFree(prop_data);
            }
            return res;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeout_ms) break;
        std::this_thread::sleep_for(std::chrono::microseconds(300));
    }
    return "";
}

void ClipboardBridge::stage_text(const std::string& text) {
    std::lock_guard<std::mutex> lock(clip_mutex);
    staged_text = text;
    serving_staged = true;
    request_served = false;

    if (is_valid()) {
        XSetSelectionOwner(dpy, clipboard_atom, win, CurrentTime);
        XFlush(dpy);
    }
}

bool ClipboardBridge::process_events_until_pasted(int max_wait_ms) {
    if (!is_valid()) return false;

    auto start = std::chrono::steady_clock::now();
    while (true) {
        {
            std::lock_guard<std::mutex> lock(clip_mutex);
            XEvent ev;
            while (XCheckTypedWindowEvent(dpy, win, SelectionRequest, &ev)) {
                XSelectionRequestEvent* req = &ev.xselectionrequest;
                XSelectionEvent reply;
                std::memset(&reply, 0, sizeof(reply));
                reply.type = SelectionNotify;
                reply.display = req->display;
                reply.requestor = req->requestor;
                reply.selection = req->selection;
                reply.target = req->target;
                reply.time = req->time;
                reply.property = req->property;

                const std::string& payload = serving_staged ? staged_text : saved_text;

                if (req->target == targets_atom) {
                    Atom supported[2] = { targets_atom, utf8_atom };
                    XChangeProperty(dpy, req->requestor, req->property, XA_ATOM, 32,
                                    PropModeReplace, reinterpret_cast<unsigned char*>(supported), 2);
                } else if (req->target == utf8_atom || req->target == XA_STRING) {
                    XChangeProperty(dpy, req->requestor, req->property, req->target, 8,
                                    PropModeReplace,
                                    reinterpret_cast<const unsigned char*>(payload.data()),
                                    payload.size());
                    if (serving_staged) {
                        request_served = true;
                    }
                } else {
                    reply.property = None;
                }

                XSendEvent(dpy, req->requestor, False, 0, reinterpret_cast<XEvent*>(&reply));
                XFlush(dpy);
            }
        }

        if (request_served.load()) {
            return true;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= max_wait_ms) break;
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

    return request_served.load();
}

void ClipboardBridge::restore_saved() {
    std::lock_guard<std::mutex> lock(clip_mutex);
    serving_staged = false;
    staged_text.clear();

    if (is_valid()) {
        if (!saved_text.empty()) {
            XSetSelectionOwner(dpy, clipboard_atom, win, CurrentTime);
            XFlush(dpy);
        } else {
            // If user had no clipboard, release selection ownership
            if (XGetSelectionOwner(dpy, clipboard_atom) == win) {
                XSetSelectionOwner(dpy, clipboard_atom, None, CurrentTime);
                XFlush(dpy);
            }
        }
    }
}

} // namespace senkey
