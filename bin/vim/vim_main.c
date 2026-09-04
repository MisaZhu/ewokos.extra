/* vim_main.c — entry point and per-file editing loop (split from vi.c) */
#include "vim.h"

void run_cmds(char* p) {
    while (p) {
        char* q = p;
        p = strchr(q, '\n');
        if (p)
            while (*p == '\n')
                *p++ = '\0';
        if (strlen(q) < MAX_INPUT_LEN)
            colon(q);
    }
}

void edit_file(char* fn) {
    int c;

    editing = 1;               // 0 = exit, 1 = one file, 2 = multiple files
    new_screen(rows, columns); // get memory for virtual screen
    init_text_buffer(fn);

    YDreg = 26;                 // default Yank/Delete reg
                                //    Ureg = 27; - const        // hold orig line for "U" cmd
    mark[26] = mark[27] = text; // init "previous context"

    crow = 0;
    ccol = 0;

    cmd_mode = 0; // 0=command  1=insert  2='R'eplace
    cmdcnt = 0;
    offset = 0; // no horizontal offset
    c = '\0';
    if (ioq_start)
        free(ioq_start);
    ioq_start = NULL;
    adding2q = 0;

    redraw(false); // dont force every col re-draw
    //------This is the main Vi cmd handling loop -----------------------
    while (editing > 0) {
        c = get_one_char(); // get a cmd from user
        // save a copy of the current line- for the 'U" command
        if (begin_line(dot) != edit_file_cur_line) {
            edit_file_cur_line = begin_line(dot);
            text_yank(begin_line(dot), end_line(dot), Ureg, PARTIAL);
        }
        // If c is a command that changes text[],
        // (re)start remembering the input for the "." command.
        if (!adding2q && ioq_start == NULL && cmd_mode == 0 // command mode
            && c > '\0'                                     // exclude NUL and non-ASCII chars
            && c < 0x7f                                     // (Unicode and such)
            && strchr(modifying_cmds, c)) {
            start_new_cmd_q(c);
        }
        do_cmd(c); // execute the user command

        // poll to see if there is input already waiting. if we are
        // not able to display output fast enough to keep up, skip
        // the display update until we catch up with input.
        if (!readbuffer[0]) {
            // no input pending - so update output
            refresh(false);
            show_status_line();
        }
    }
    //-------------------------------------------------------------------

    go_bottom_and_clear_to_eol();
}

void* xmalloc_open_read_close(const char* filename) {
    int fd;

    fd = open(filename, O_RDONLY);
    if(fd < 0)
        return NULL;

    struct stat st;
    if(fstat(fd, &st) != 0) {
        close(fd);
        return NULL;
    }

    int l = st.st_size;
    char* buf = malloc(l + 1);
    if (buf == NULL) {
        close(fd);
        return NULL;
    }
    read(fd, buf, l);
    close(fd);
    buf[l] = 0;
    return buf;
}

int main(int argc, char** argv) {
    last_modified_count = -1;
    get_screen_xy(&columns, &rows);
    /* "" but has space for 2 chars: */
    last_search_pattern = zalloc(2);
    tabstop = 8;

    // undo_stack_tail = NULL; - already is
    undo_queue_state = UNDO_EMPTY;
    // undo_q = 0; - already is

    if (setjmp(die_jmp))
        goto done;

    char* cmds = NULL;
    char* exrc = "/.exrc";
    struct stat st;

    if (stat(exrc, &st) >= 0)
        cmds = xmalloc_open_read_close(exrc);

    if (cmds) {
        init_text_buffer(NULL);
        run_cmds(cmds);
        free(cmds);
    }

    // "Save cursor, use alternate screen buffer, clear screen"
    //puts_no_eol(ESC "[?1049h");
    puts_no_eol(ESC "[H");
    puts_no_eol(ESC "[2J");
    fflush(stdout);
    // This is the main file handling loop
    if (argc == 0)
        argc++;
    vi_optind = 0;
    if(argc < 2)
        edit_file(NULL); // might be NULL on 1st iteration
    else {
        for (vi_optind = 1; vi_optind < argc; vi_optind++)
            if (argv[vi_optind])
                edit_file(argv[vi_optind]);
            else
                edit_file(NULL); // might be NULL on 1st iteration
    }
done:
    flush_undo_data();
    if (text)
        free(text);
    if (screen)
        free(screen);
    if (last_search_pattern)
        free(last_search_pattern);
    // "Use normal screen buffer, restore cursor"
    //puts_no_eol(ESC "[?1049l");
    puts_no_eol(ESC "[H");
    puts_no_eol(ESC "[2J");
    fflush(stdout);
    return 0;
}
