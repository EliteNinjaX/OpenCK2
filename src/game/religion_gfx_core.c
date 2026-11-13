#include "religion_gfx_core.h"
#include "ck2_lexer_core.h"
#include <string.h>

static bool is_block_start(struct ck2_span name)
{
    return name.length && name.start[0] != '#' &&
        name.start[0] != '{' && name.start[0] != '}' &&
        name.start[0] != '=';
}

/* A single religion or group block. Only direct assignments should supply
 * graphical culture; nested scripted triggers are deliberately skipped. */
static void parse_child(struct ck2_lexer *lex, char *style, size_t capacity)
{
    for (struct ck2_span key; (key=ck2_lex(lex)).length;) {
        if (ck2_span_eq(key, "}")) return;
        if (!ck2_span_eq(ck2_lex(lex), "=")) return;
        const struct ck2_span value=ck2_lex(lex);
        if (ck2_span_eq(value, "{")) ck2_skip_scope(lex);
        else if (ck2_span_eq(key,"graphical_culture"))
            ck2_span_copy(style,capacity,value);
    }
}

bool religion_gfx_from_text(const char *source, const char *religion,
                            char *style, size_t size)
{
    if (!source || !religion || !*religion || !style || !size) return false;
    style[0]=0;
    struct ck2_lexer root={source};
    for (struct ck2_span group; (group=ck2_lex(&root)).length;) {
        if (!is_block_start(group) || !ck2_span_eq(ck2_lex(&root),"=")) break;
        const struct ck2_span open=ck2_lex(&root);
        if (!ck2_span_eq(open,"{")) continue;
        char group_style[64]="", own_style[64]="";
        bool matched=false;
        for (struct ck2_span key; (key=ck2_lex(&root)).length;) {
            if (ck2_span_eq(key,"}")) break;
            if (!ck2_span_eq(ck2_lex(&root),"=")) break;
            const struct ck2_span value=ck2_lex(&root);
            if (ck2_span_eq(value,"{")) {
                if (ck2_span_eq(key,religion)) {
                    matched=true;
                    parse_child(&root,own_style,sizeof own_style);
                } else ck2_skip_scope(&root);
            } else if (ck2_span_eq(key,"graphical_culture"))
                ck2_span_copy(group_style,sizeof group_style,value);
        }
        if (matched) {
            const char *chosen=own_style[0] ? own_style : group_style;
            if (*chosen) {
                const size_t len=strlen(chosen);
                const size_t n=len<size-1?len:size-1;
                memcpy(style,chosen,n); style[n]=0;
                return true;
            }
            return false;
        }
    }
    return false;
}
