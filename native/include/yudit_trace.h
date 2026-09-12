/**
 * yudit_trace.h — Per-lookup trace callback for Yudit's OT engine.
 *
 * Provides a global callback that getOTFFeature() and getPositions()
 * can call for each lookup they process. This gives per-lookup
 * granularity equivalent to HarfBuzz's buffer message system.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef YUDIT_TRACE_H
#define YUDIT_TRACE_H

#include "stoolkit/STypes.h"

/**
 * Trace event types.
 */
enum YuditTraceEvent {
    YUDIT_TRACE_LOOKUP_START = 0,   /* about to try this lookup */
    YUDIT_TRACE_LOOKUP_END   = 1,   /* lookup processing done */
};

/**
 * Trace callback signature.
 *
 * @param event     start or end
 * @param table     "GSUB" or "GPOS"
 * @param lookup_idx   lookup list index (0-based)
 * @param feature   4-char feature tag (e.g. "half")
 * @param matched   1 if this lookup produced a result, 0 if skipped
 * @param user_data opaque pointer passed through from set function
 */
typedef void (*YuditTraceCallback)(
    YuditTraceEvent event,
    const char* table,
    unsigned int lookup_idx,
    const char* feature,
    int matched,
    void* user_data
);

/**
 * Set the global trace callback. Pass NULL to disable tracing.
 */
void yudit_set_trace_callback(YuditTraceCallback cb, void* user_data);

/**
 * Set the current feature tag for trace events.
 * Call this before each gsub()/gpos() call so the callback can report it.
 */
void yudit_set_current_feature(const char* feature);

/**
 * Get the current callback and user data.
 */
YuditTraceCallback yudit_get_trace_callback(void** user_data);

/**
 * Emit a trace event (inline helper — no-op when callback is NULL).
 */
static inline void yudit_trace_emit(
    YuditTraceEvent event,
    const char* table,
    unsigned int lookup_idx,
    const char* feature,
    int matched)
{
    void* ud = 0;
    YuditTraceCallback cb = yudit_get_trace_callback(&ud);
    if (cb) cb(event, table, lookup_idx, feature, matched, ud);
}

#endif /* YUDIT_TRACE_H */
