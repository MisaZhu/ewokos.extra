/* vim_term.c — terminal drawing, screen refresh and status line (split from vi.c) */
#include "vim.h"
#include "vim_syntax.h"

//----- Terminal Drawing ---------------------------------------
// The terminal is made up of 'rows' line of 'columns' columns.
// classically this would be 24 x 80.
//  screen coordinates
//  0,0     ...     0,79
//  1,0     ...     1,79
//  .       ...     .
//  .       ...     .
//  22,0    ...     22,79
//  23,0    ...     23,79   <- status line

//----- Move the cursor to row x col (count from 0, not 1) -------
void place_cursor(int row, int col) {
    char cm1[sizeof(ESC_SET_CURSOR_POS) + sizeof(int) * 3 * 2];

    if (row < 0)
        row = 0;
    if (row >= rows)
        row = rows - 1;
    if (col < 0)
        col = 0;
    if (col >= columns)
        col = columns - 1;

    sprintf(cm1, ESC_SET_CURSOR_POS, row + 1, col + 1);
    puts_no_eol(cm1);
}

//----- Erase from cursor to end of line -----------------------
void clear_to_eol(void) { puts_no_eol(ESC_CLEAR2EOL); }

void go_bottom_and_clear_to_eol(void) {
    place_cursor(rows - 1, 0);
    clear_to_eol();
}

//----- Start standout mode ------------------------------------
void standout_start(void) { puts_no_eol(ESC_BOLD_TEXT); }

//----- End standout mode --------------------------------------
void standout_end(void) { puts_no_eol(ESC_NORM_TEXT); }

//----- Erase the Screen[] memory ------------------------------
void screen_erase(void) {
    memset(screen, ' ', screensize); // clear new screen
}

void new_screen(int ro, int co) {
    char* s;

    if (screen)
        free(screen);
    screensize = ro * co + 8;
    s = screen = malloc(screensize);
    // initialize the new screen. assume this will be a empty file.
    screen_erase();
    // non-existent text[] lines start with a tilde (~).
    // screen[(1 * co) + 0] = '~';
    // screen[(2 * co) + 0] = '~';
    //..
    // screen[((ro-2) * co) + 0] = '~';
    ro -= 2;
    while (--ro >= 0) {
        s += co;
        *s = '~';
    }
}

//----- Synchronize the cursor to Dot --------------------------
void sync_cursor(char* d, int* row, int* col) {
    char* beg_cur; // begin and end of "d" line
    char* tp;
    int cnt, ro, co;

    beg_cur = begin_line(d); // first char of cur line

    if (beg_cur < screenbegin) {
        // "d" is before top line on screen
        // how many lines do we have to move
        cnt = count_lines(beg_cur, screenbegin);
    sc1:
        screenbegin = beg_cur;
        if (cnt > (rows - 1) / 2) {
            // we moved too many lines. put "dot" in middle of screen
            for (cnt = 0; cnt < (rows - 1) / 2; cnt++) {
                screenbegin = prev_line(screenbegin);
            }
        }
    } else {
        char* end_scr;          // begin and end of screen
        end_scr = end_screen(); // last char of screen
        if (beg_cur > end_scr) {
            // "d" is after bottom line on screen
            // how many lines do we have to move
            cnt = count_lines(end_scr, beg_cur);
            if (cnt > (rows - 1) / 2)
                goto sc1; // too many lines
            for (ro = 0; ro < cnt - 1; ro++) {
                // move screen begin the same amount
                screenbegin = next_line(screenbegin);
                // now, move the end of screen
                end_scr = next_line(end_scr);
                end_scr = end_line(end_scr);
            }
        }
    }
    // "d" is on screen- find out which row
    tp = screenbegin;
    for (ro = 0; ro < rows - 1; ro++) { // drive "ro" to correct row
        if (tp == beg_cur)
            break;
        tp = next_line(tp);
    }

    // find out what col "d" is on
    co = 0;
    do {                 // drive "co" to correct column
        if (*tp == '\n') // vda || *tp == '\0')
            break;
        co = next_column(*tp, co) - 1;
        // inserting text before a tab, don't include its position
        if (cmd_mode && tp == d - 1 && *d == '\t') {
            co++;
            break;
        }
    } while (tp++ < d && ++co);

    // "co" is the column where "dot" is.
    // The screen has "columns" columns.
    // The currently displayed columns are  0+offset -- columns+ofset
    // |-------------------------------------------------------------|
    //               ^ ^                                ^
    //        offset | |------- columns ----------------|
    //
    // If "co" is already in this range then we do not have to adjust offset
    //      but, we do have to subtract the "offset" bias from "co".
    // If "co" is outside this range then we have to change "offset".
    // If the first char of a line is a tab the cursor will try to stay
    //  in column 7, but we have to set offset to 0.

    if (co < 0 + offset) {
        offset = co;
    }
    if (co >= columns + offset) {
        offset = co - columns + 1;
    }
    // if the first char of the line is a tab, and "dot" is sitting on it
    //  force offset to 0.
    if (d == beg_cur && *d == '\t') {
        offset = 0;
    }
    co -= offset;

    *row = ro;
    *col = co;
}

//----- Format a text[] line into a buffer ---------------------
char* format_line(char* src /*, int li*/) {
    uint8_t c;
    int co;
    int ofs = offset;
    char* dest = scr_out_buf; // [MAX_SCR_COLS + MAX_TABSTOP * 2]

    c = '~'; // char in col 0 in non-existent lines is '~'
    co = 0;
    while (co < columns + tabstop) {
        // have we gone past the end?
        if (src < end) {
            c = *src++;
            if (c == '\n')
                break;
            if ((c & 0x80) && !is_asciionly(c)) {
                c = '.';
            }
            if (c < ' ' || c == 0x7f) {
                if (c == '\t') {
                    c = ' ';
                    //      co %    8     !=     7
                    while ((co % tabstop) != (tabstop - 1)) {
                        dest[co++] = c;
                    }
                    // Skip the extra space since we already filled the tab
                    continue;
                } else {
                    dest[co++] = '^';
                    if (c == 0x7f)
                        c = '?';
                    else
                        c += '@'; // Ctrl-X -> 'X'
                }
            }
        }
        dest[co++] = c;
        // discard scrolled-off-to-the-left portion,
        // in tabstop-sized pieces
        if (ofs >= tabstop && co >= tabstop) {
            memmove(dest, dest + tabstop, co);
            co -= tabstop;
            ofs -= tabstop;
        }
        if (src >= end)
            break;
    }
    // check "short line, gigantic offset" case
    if (co < ofs)
        ofs = co;
    // discard last scrolled off part
    co -= ofs;
    dest += ofs;
    // fill the rest with spaces
    if (co < columns)
        memset(&dest[co], ' ', columns - co);
    return dest;
}

//----- Refresh the changed screen lines -----------------------
// Copy the source line from text[] into the buffer and note
// if the current screenline is different from the new buffer.
// If they differ then that line needs redrawing on the terminal.
//
void refresh(int full_screen) {

    int li, changed;
    char *tp, *sp; // pointer into text[] and screen[]

    sync_cursor(dot, &crow, &ccol); // where cursor will be (on "dot")
    tp = screenbegin;               // index into text[] of top line

    // compare text[] to screen[] and mark screen[] lines that need updating
    for (li = 0; li < rows - 1; li++) {
        int cs, ce; // column start & end
        char* out_buf;
        // format current text line
        out_buf = format_line(tp /*, li*/);

        // skip to the end of the current text[] line
        if (tp < end) {
            char* t = memchr(tp, '\n', end - tp);
            if (!t)
                t = end - 1;
            tp = t + 1;
        }

        // see if there are any changes between virtual screen and out_buf
        changed = false; // assume no change
        cs = 0;
        ce = columns - 1;
        sp = &screen[li * columns]; // start of screen line
        if (full_screen) {
            // force re-draw of every single column from 0 - columns-1
            goto re0;
        }
        // compare newly formatted buffer with virtual screen
        // look forward for first difference between buf and screen
        for (; cs <= ce; cs++) {
            if (out_buf[cs] != sp[cs]) {
                changed = true; // mark for redraw
                break;
            }
        }

        // look backward for last difference between out_buf and screen
        for (; ce >= cs; ce--) {
            if (out_buf[ce] != sp[ce]) {
                changed = true; // mark for redraw
                break;
            }
        }
        // now, cs is index of first diff, and ce is index of last diff

        // if horz offset has changed, force a redraw
        if (offset != refresh_old_offset) {
        re0:
            changed = true;
        }

        // make a sanity check of columns indexes
        if (cs < 0)
            cs = 0;
        if (ce > columns - 1)
            ce = columns - 1;
        if (cs > ce) {
            cs = 0;
            ce = columns - 1;
        }
        // is there a change between virtual screen and out_buf
        if (changed) {
            // copy changed part of buffer to virtual screen
            memcpy(sp + cs, out_buf + cs, ce - cs + 1);
            place_cursor(li, cs);
            // write line out to terminal (with syntax colors when enabled)
            syntax_write_slice(out_buf, cs, ce);
            fflush(stdout);
        }
    }

    place_cursor(crow, ccol);

    if (!keep_index)
        cindex = ccol + offset;

    refresh_old_offset = offset;
}

// show file status on status line
int format_edit_status(void) {
    static const char cmd_mode_indicator[] = "-IR-";

    int cur, percent, ret, trunc_at;

    // modified_count is now a counter rather than a flag.  this
    // helps reduce the amount of line counting we need to do.
    // (this will cause a mis-reporting of modified status
    // once every MAXINT editing operations.)

    // it would be nice to do a similar optimization here -- if
    // we haven't done a motion that could have changed which line
    // we're on, then we shouldn't have to do this count_lines()
    cur = count_lines(text, dot);

    // count_lines() is expensive.
    // Call it only if something was changed since last time
    // we were here:
    if (modified_count != last_modified_count) {
        format_edit_status_tot = cur + count_lines(dot, end - 1) - 1;
        last_modified_count = modified_count;
    }

    //    current line         percent
    //   -------------    ~~ ----------
    //    format_edit_status_total lines            100
    if (format_edit_status_tot > 0) {
        percent = (100 * cur) / format_edit_status_tot;
    } else {
        cur = format_edit_status_tot = 0;
        percent = 100;
    }

    trunc_at = columns < STATUS_BUFFER_LEN - 1 ? columns : STATUS_BUFFER_LEN - 1;

    ret = snprintf(status_buffer, trunc_at + 1, "%c %s%s %d/%d %d%%",
                   cmd_mode_indicator[cmd_mode & 3],
                   (current_filename != NULL ? current_filename : "No file"),
                   (modified_count ? " [Modified]" : ""), cur, format_edit_status_tot, percent);

    if (ret >= 0 && ret < trunc_at)
        return ret; // it all fit

    return trunc_at; // had to truncate
}

int bufsum(char* buf, int count) {
    int sum = 0;
    char* e = buf + count;
    while (buf < e)
        sum += (uint8_t)*buf++;
    return sum;
}

void redraw(int full_screen);

void Hit_Return(void) {
    int c;

    standout_start();
    puts_no_eol("[Hit return to continue]");
    standout_end();
    while ((c = get_one_char()) != '\n' && c != '\r')
        continue;
    redraw(true); // force redraw all
}

void show_status_line(void) {
    int cnt = 0, cksum = 0;

    // either we already have an error or status message, or we
    // create one.
    if (!have_status_msg) {
        cnt = format_edit_status();
        cksum = bufsum(status_buffer, cnt);
    }
    if (have_status_msg || ((cnt > 0 && last_status_cksum != cksum))) {
        last_status_cksum = cksum; // remember if we have seen this line
        go_bottom_and_clear_to_eol();
        puts_no_eol(status_buffer);
        if (have_status_msg) {
            if (((int)strlen(status_buffer) - (have_status_msg - 1)) > (columns - 1)) {
                have_status_msg = 0;
                Hit_Return();
            }
            have_status_msg = 0;
        }
        place_cursor(crow, ccol); // put cursor back in correct place
    }
    fflush(stdout);
}

//----- Force refresh of all Lines -----------------------------
void redraw(int full_screen) {
    // cursor to top,left; clear to the end of screen
    puts_no_eol(ESC_SET_CURSOR_TOPLEFT ESC_CLEAR2EOS);
    screen_erase();        // erase the internal screen buffer
    last_status_cksum = 0; // force status update
    refresh(full_screen);  // this will redraw the entire display
    show_status_line();
}

//----- Draw the status line at bottom of the screen -------------
//----- Flash the screen  --------------------------------------
void flash(int ms) {
    standout_start();
    redraw(true);
    proc_usleep(ms);
    standout_end();
    redraw(true);
}

void indicate_error(void) {
    cmd_error = true;
    if (!err_method) {
        puts_no_eol(ESC_BELL);
    } else {
        flash(100);
    }
}

//----- format the status buffer, the bottom line of screen ------
void status_line(const char* format, ...) {
    va_list args;

    va_start(args, format);
    vsnprintf(status_buffer, STATUS_BUFFER_LEN, format, args);
    va_end(args);

    have_status_msg = 1;
}

void status_line_bold(const char* format, ...) {
    va_list args;

    va_start(args, format);
    strcpy(status_buffer, ESC_BOLD_TEXT);
    vsnprintf(status_buffer + (sizeof(ESC_BOLD_TEXT) - 1),
              STATUS_BUFFER_LEN - sizeof(ESC_BOLD_TEXT) - sizeof(ESC_NORM_TEXT), format, args);
    strcat(status_buffer, ESC_NORM_TEXT);
    va_end(args);

    have_status_msg = 1 + (sizeof(ESC_BOLD_TEXT) - 1) + (sizeof(ESC_NORM_TEXT) - 1);
}

void status_line_bold_errno(const char* fn) {
    status_line_bold("'%s' %s", fn, strerror(errno));
}

// copy s to buf, convert unprintable
void print_literal(char* buf, const char* s) {
    char* d;
    uint8_t c;

    if (!s[0])
        s = "(NULL)";

    d = buf;
    for (; *s; s++) {
        c = *s;
        if ((c & 0x80) && !is_asciionly(c))
            c = '?';
        if (c < ' ' || c == 0x7f) {
            *d++ = '^';
            c |= '@'; // 0x40
            if (c == 0x7f)
                c = '?';
        }
        *d++ = c;
        *d = '\0';
        if (d - buf > MAX_INPUT_LEN - 10) // paranoia
            break;
    }
}
void not_implemented(const char* s) {
    char buf[MAX_INPUT_LEN];
    print_literal(buf, s);
    status_line_bold("'%s' is not implemented", buf);
}

/* How long to wait for a terminal to answer the cursor-position report.
 * A real terminal replies within a few milliseconds even over a slow serial
 * line, so the timeout only bounds the wait for terminals that never answer. */
#define VI_SIZE_PROBE_TIMEOUT_MS 200

// Read the "ESC [ <row> ; <col> R" cursor-position report from stdin.
// Returns true and fills *rows/*cols on success, false on timeout/garbage.
bool read_cursor_pos_reply(uint32_t* rows, uint32_t* cols) {
    enum { WANT_ESC, WANT_BRACKET, WANT_PARAMS } state = WANT_ESC;
    uint32_t row = 0, col = 0;
    bool parsing_col = false;

    for (;;) {
        struct pollfd pfd;
        pfd.fd = 0; // stdin
        pfd.events = POLLIN;
        pfd.revents = 0;
        // The opening ESC may never arrive; the bytes after it come together.
        int wait_ms = (state == WANT_ESC) ? VI_SIZE_PROBE_TIMEOUT_MS : 100;
        if (poll(&pfd, 1, wait_ms) <= 0)
            return false;

        char c;
        if (read(0, &c, 1) != 1)
            return false;

        if (state == WANT_ESC) {
            if (c == 0x1b)
                state = WANT_BRACKET;
            // else: stale input ahead of the reply - skip it
        }
        else if (state == WANT_BRACKET) {
            state = (c == '[') ? WANT_PARAMS : WANT_ESC;
        }
        else { // WANT_PARAMS
            if (c >= '0' && c <= '9') {
                if (parsing_col)
                    col = col * 10 + (uint32_t)(c - '0');
                else
                    row = row * 10 + (uint32_t)(c - '0');
            }
            else if (c == ';') {
                parsing_col = true;
            }
            else if (c == 'R') {
                if (row > 0 && col > 0) {
                    *rows = row;
                    *cols = col;
                    return true;
                }
                return false;
            }
            else {
                // not a cursor-position report; resync on the next ESC
                state = WANT_ESC;
                row = col = 0;
                parsing_col = false;
            }
        }
    }
}

void get_screen_xy(uint32_t* x, uint32_t* y) {
    uint32_t cols = 80, rows = 24; // VT100 fallback for a silent terminal
    bool have_size = false;

    // Preferred path: ask the terminal driver for its window size directly.
    // The GUI consoles (consoled/xterm) publish their live textgrid geometry
    // through TIOCGWINSZ. This is a single synchronous IPC with no round-trip
    // race, so it is both faster and far more reliable than the cursor-position
    // probe below (which needs the terminal to answer on stdin mid-poll).
    if (isatty(0)) {
        struct winsize ws;
        memset(&ws, 0, sizeof(ws));
        if (ioctl(0, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
            rows = ws.ws_row;
            cols = ws.ws_col;
            have_size = true;
        }
    }

    // Fallback: ask the terminal itself over the wire. Park the cursor at an
    // absurd row/col (it clamps to the real bottom-right corner) and request a
    // cursor-position report. This is how the /dev/tty0 serial console reveals
    // the geometry of whatever emulator is attached, and it works the same for
    // ssh/telnet. Only probe a real terminal so we never swallow redirected or
    // piped stdin.
    if (!have_size && isatty(0) && isatty(1)) { // stdin and stdout are the terminal
        fflush(stdout);
        puts_no_eol(ESC "[999;999H");
        puts_no_eol(ESC "[6n");
        fflush(stdout);

        uint32_t r = 0, c = 0;
        if (read_cursor_pos_reply(&r, &c)) {
            rows = r;
            cols = c;
        }
        // we moved the cursor to the corner; put it back home
        puts_no_eol(ESC_SET_CURSOR_TOPLEFT);
        fflush(stdout);
    }

    // keep the geometry inside the buffers the rest of vi assumes
    if (cols < 2) cols = 2;
    if (rows < 2) rows = 2;
    if (cols > MAX_SCR_COLS) cols = MAX_SCR_COLS;
    if (rows > MAX_SCR_ROWS) rows = MAX_SCR_ROWS;

    *x = cols;
    *y = rows;
}

