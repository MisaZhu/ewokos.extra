/* vim_syntax.c — line-oriented syntax highlighting for .json and .rd files.
 *
 * Color model: every column of the formatted screen line gets a color id,
 * then the changed slice is emitted with SGR escape sequences inserted at
 * color boundaries.  Colors are a pure function of the line text, so the
 * char-only virtual screen diff in refresh() stays valid.
 */
#include "vim.h"
#include "vim_syntax.h"

// color ids (index into sgr_seq)
enum {
    C_DEFAULT = 0,
    C_STRING,   // green:          quoted strings
    C_KEY,      // bright cyan:    JSON object keys
    C_NUMBER,   // magenta:        numbers
    C_LITERAL,  // yellow:         JSON true/false/null, .rd $VARS
    C_BRACE,    // bright white:   JSON {} []
    C_COMMENT,  // bright black:   .rd '#' comment lines
    C_CMD,      // bright green:   .rd command (first token)
    C_PATH,     // cyan:           .rd /absolute and ./relative paths
    C_OPER,     // red:            .rd '@' prefix and | > < & operators
    C_COUNT
};

// SGR sequences for the color ids above (gterminal supports 30-37 and SGR 1)
static const char* const sgr_seq[C_COUNT] = {
    ESC "[m",      // C_DEFAULT
    ESC "[32m",    // C_STRING
    ESC "[1;36m",  // C_KEY
    ESC "[35m",    // C_NUMBER
    ESC "[33m",    // C_LITERAL
    ESC "[1;37m",  // C_BRACE
    ESC "[1;30m",  // C_COMMENT
    ESC "[1;32m",  // C_CMD
    ESC "[36m",    // C_PATH
    ESC "[31m",    // C_OPER
};

static int cur_syntax = SYNTAX_NONE;

// per-column color ids of the line currently being written
static uint8_t col_buf[MAX_SCR_COLS + MAX_TABSTOP * 2] UDATA;

void syntax_set_file(const char* fn) {
    cur_syntax = SYNTAX_NONE;
    if (fn == NULL)
        return;
    const char* ext = strrchr(fn, '.');
    if (ext == NULL)
        return;
    if (strcmp(ext, ".json") == 0)
        cur_syntax = SYNTAX_JSON;
    else if (strcmp(ext, ".rd") == 0)
        cur_syntax = SYNTAX_RD;
}

int syntax_mode(void) { return cur_syntax; }

static void paint(uint8_t* col, int from, int to, uint8_t cl) {
    while (from <= to)
        col[from++] = cl;
}

//----- JSON -----------------------------------------------------------
static void color_json(const char* s, int n, uint8_t* col) {
    int i = 0;
    while (i < n) {
        char c = s[i];
        if (c == '"') {
            // find the closing quote (strings cannot span lines)
            int j = i + 1;
            while (j < n && s[j] != '"') {
                if (s[j] == '\\' && j + 1 < n)
                    j++; // skip escaped char
                j++;
            }
            int e = (j < n) ? j : n - 1; // closing quote or end of line
            // an object key is a string followed by ':'
            int k = (j < n) ? j + 1 : n;
            while (k < n && (s[k] == ' ' || s[k] == '\t'))
                k++;
            paint(col, i, e, (k < n && s[k] == ':') ? C_KEY : C_STRING);
            i = e + 1;
        } else if (c == '{' || c == '}' || c == '[' || c == ']') {
            col[i++] = C_BRACE;
        } else if (isdigit((uint8_t)c) ||
                   (c == '-' && i + 1 < n && isdigit((uint8_t)s[i + 1]))) {
            int j = i + 1;
            while (j < n && (isdigit((uint8_t)s[j]) || s[j] == '.' || s[j] == 'e' ||
                             s[j] == 'E' || s[j] == '+' || s[j] == '-'))
                j++;
            paint(col, i, j - 1, C_NUMBER);
            i = j;
        } else if (isalpha((uint8_t)c)) {
            int j = i;
            while (j < n && isalpha((uint8_t)s[j]))
                j++;
            int len = j - i;
            if ((len == 4 && strncmp(s + i, "true", 4) == 0) ||
                (len == 5 && strncmp(s + i, "false", 5) == 0) ||
                (len == 4 && strncmp(s + i, "null", 4) == 0))
                paint(col, i, j - 1, C_LITERAL);
            i = j;
        } else {
            i++;
        }
    }
}

//----- .rd init scripts (executed line by line by /bin/shell) ---------
static void color_rd(const char* s, int n, uint8_t* col) {
    int i = 0;

    // a '#' in column 0 comments out the whole line (see /bin/shell)
    if (n > 0 && s[0] == '#') {
        paint(col, 0, n - 1, C_COMMENT);
        return;
    }
    // '@' prefix: run without echoing the command
    if (n > 0 && s[0] == '@')
        col[i++] = C_OPER;

    // leading whitespace
    while (i < n && (s[i] == ' ' || s[i] == '\t'))
        i++;

    // first token is the command (shell builtin or executable path)
    int j = i;
    while (j < n && s[j] != ' ' && s[j] != '\t')
        j++;
    paint(col, i, j - 1, C_CMD);
    i = j;

    // arguments
    while (i < n) {
        char c = s[i];
        if (c == ' ' || c == '\t') {
            i++;
            continue;
        }
        if (c == '|' || c == '>' || c == '<' || c == '&') {
            col[i++] = C_OPER;
            continue;
        }
        if (c == '"') {
            int q = i + 1;
            while (q < n && s[q] != '"')
                q++;
            int e = (q < n) ? q : n - 1; // closing quote or end of line
            paint(col, i, e, C_STRING);
            i = e + 1;
            continue;
        }
        // plain token, up to the next whitespace or operator
        int t = i;
        while (t < n && s[t] != ' ' && s[t] != '\t' && s[t] != '|' && s[t] != '>' &&
               s[t] != '<' && s[t] != '&')
            t++;
        uint8_t cl = C_DEFAULT;
        if (s[i] == '/' || (s[i] == '.' && i + 1 < n && s[i + 1] == '/'))
            cl = C_PATH;
        else if (s[i] == '$')
            cl = C_LITERAL;
        else if (isdigit((uint8_t)s[i]))
            cl = C_NUMBER;
        paint(col, i, t - 1, cl);
        i = t;
    }
}

// a line past the end of the file shows as '~' followed by spaces
static bool is_filler_line(const char* s, int n) {
    if (n < 1 || s[0] != '~')
        return false;
    for (int i = 1; i < n; i++)
        if (s[i] != ' ')
            return false;
    return true;
}

void syntax_write_slice(const char* vline, int from, int to) {
    if (cur_syntax == SYNTAX_NONE || to < from)
        goto plain;

    int n = (int)columns; // vline is exactly this wide (space padded)
    if (to >= n)
        to = n - 1;
    if (to < from || is_filler_line(vline, n))
        goto plain;

    memset(col_buf, C_DEFAULT, n);
    if (cur_syntax == SYNTAX_JSON)
        color_json(vline, n, col_buf);
    else
        color_rd(vline, n, col_buf);

    int cur = -1; // SGR state currently active on the terminal
    for (int i = from; i <= to; i++) {
        int cl = col_buf[i];
        if (cl != cur) {
            puts_no_eol(sgr_seq[cl]);
            cur = cl;
        }
        putchar(vline[i]);
    }
    if (cur != C_DEFAULT)
        puts_no_eol(ESC_NORM_TEXT);
    return;

plain:
    for (int i = from; i <= to; i++)
        putchar(vline[i]);
}
