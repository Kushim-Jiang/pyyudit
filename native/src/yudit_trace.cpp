/**
 * yudit_trace.cpp — Global trace callback implementation.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "yudit_trace.h"

static YuditTraceCallback g_trace_cb = 0;
static void*              g_trace_ud  = 0;
static char               g_current_feature[5] = "";

void yudit_set_trace_callback(YuditTraceCallback cb, void* user_data) {
    g_trace_cb = cb;
    g_trace_ud = user_data;
}

void yudit_set_current_feature(const char* feature) {
    if (feature) {
        g_current_feature[0] = feature[0];
        g_current_feature[1] = feature[1];
        g_current_feature[2] = feature[2];
        g_current_feature[3] = feature[3];
        g_current_feature[4] = '\0';
    } else {
        g_current_feature[0] = '\0';
    }
}

YuditTraceCallback yudit_get_trace_callback(void** user_data) {
    if (user_data) *user_data = g_trace_ud;
    return g_trace_cb;
}
