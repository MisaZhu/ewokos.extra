/* vim_syntax.h — syntax highlighting for .json and .rd (init script) files.
 *
 * The highlighter is hooked into refresh() (vim_term.c): whenever a screen
 * line changes, the visible slice is written through syntax_write_slice(),
 * which emits SGR color sequences around the tokens of the line.
 * Both supported formats are strictly line-oriented (JSON strings may not
 * contain raw newlines, .rd files are executed line by line), so each line
 * can be colored independently.
 */
#pragma once

enum {
    SYNTAX_NONE = 0,
    SYNTAX_JSON,
    SYNTAX_RD
};

// select the highlight mode from the file name extension (.json / .rd)
void syntax_set_file(const char* fn);

// the currently active highlight mode (one of SYNTAX_*)
int syntax_mode(void);

/* Write vline[from..to] (inclusive) to stdout, adding SGR color sequences
 * when a syntax mode is active.  vline is one formatted screen line as
 * produced by format_line(): exactly 'columns' chars, space padded,
 * tabs/control chars already expanded. */
void syntax_write_slice(const char* vline, int from, int to);
