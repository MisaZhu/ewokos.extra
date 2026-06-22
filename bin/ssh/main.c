#include "ssh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#include <ewoksys/keydef.h>

#ifdef HAVE_SELECT
#include <sys/select.h>
#include <sys/time.h>
#endif

#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>

static ssh_session_t *g_session = NULL;

static int write_terminal_output(const char *buf, int len);

static void do_cleanup(void) {
    if (g_session) {
        ssh_disconnect(g_session);
        ssh_session_free(g_session);
        g_session = NULL;
    }
}

static void print_usage(const char *program) {
    printf("Usage: %s [options] [user@]hostname [command]\n", program);
    printf("\nOptions:\n");
    printf("  -p port     Port to connect to (default: 22)\n");
    printf("  -l user     Login name\n");
    printf("  -i identity Identity file (private key)\n");
    printf("  -o option   Set option\n");
    printf("  -v          Verbose mode\n");
    printf("  -q          Quiet mode\n");
    printf("  -t          Force pseudo-terminal allocation\n");
    printf("  -T          Disable pseudo-terminal allocation\n");
    printf("  -N          Do not execute remote command\n");
    printf("  -f          Fork into background\n");
    printf("  -h          Show this help\n");
    printf("\nExamples:\n");
    printf("  %s user@example.com\n", program);
    printf("  %s -p 2222 user@example.com\n", program);
    printf("  %s user@example.com ls -la\n", program);
}

static int parse_host(const char *hostspec, char **user, char **host) {
    char *at = strchr(hostspec, '@');
    if (at) {
        *user = malloc(at - hostspec + 1);
        if (!*user) return -1;
        strncpy(*user, hostspec, at - hostspec);
        (*user)[at - hostspec] = '\0';
        *host = strdup(at + 1);
    } else {
        *user = NULL;
        *host = strdup(hostspec);
    }
    return 0;
}

static int do_interactive_session(ssh_session_t *session) {
    int running = 1;
    char buf[4096];
    struct pollfd fds[2];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = session->socket;
    fds[1].events = POLLIN;

    while (running && ssh_is_connected(session)) {
        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        if (fds[0].revents & POLLIN) {
            int n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n > 0) {
                if (n >= 3 && buf[0] == '~' && buf[1] == '~' && buf[2] == '.') {
                    printf("\r\nDisconnecting...\r\n");
                    break;
                }
                if (ssh_channel_send_data(session, buf, n) < 0) {
                    break;
                }
            } else if (n == 0) {
                /*
                 * Ewok console stdin can transiently report readable before
                 * bytes are actually available. Do not treat that as EOF.
                 */
                usleep(1000);
            } else if (errno != EAGAIN && errno != EINTR && errno != 0) {
                break;
            }
        }

        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            ssh_channel_close(session);
            running = 0;
        }

        if (running && (fds[1].revents & POLLIN)) {
            int n = ssh_channel_receive_data(session, buf, sizeof(buf));
            if (n > 0) {
                if (write_terminal_output(buf, n) < 0) {
                    break;
                }
            } else if (n == 0) {
                running = 0;
            } else {
                break;
            }
        }

        if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            running = 0;
        }
    }

    return 0;
}

static int write_terminal_output(const char *buf, int len) {
    char out[4096];
    int out_len = 0;

    if (!buf || len <= 0) {
        return 0;
    }

    for (int i = 0; i < len; i++) {
        if (buf[i] == '\r') {
            continue;
        }
        out[out_len++] = buf[i];
        if (out_len == (int)sizeof(out)) {
            if (write(STDOUT_FILENO, out, out_len) < 0) {
                return -1;
            }
            out_len = 0;
        }
    }

    if (out_len > 0) {
        if (write(STDOUT_FILENO, out, out_len) < 0) {
            return -1;
        }
    }
    return 0;
}

static int do_exec_command(ssh_session_t *session, int argc, char **argv) {
    char command[4096];
    int cmd_len = 0;
    int n;
    char buf[4096];
    
    /* Build command string */
    for (int i = 0; i < argc; i++) {
        if (i > 0) {
            if (cmd_len < sizeof(command) - 1) {
                command[cmd_len++] = ' ';
            }
        }
        int arg_len = strlen(argv[i]);
        if (cmd_len + arg_len < sizeof(command) - 1) {
            memcpy(command + cmd_len, argv[i], arg_len);
            cmd_len += arg_len;
        }
    }
    command[cmd_len] = '\0';
    
    /* Request exec */
    if (ssh_channel_request_exec(session, command) < 0) {
        fprintf(stderr, "Failed to execute command: %s\n", ssh_get_error(session));
        return -1;
    }
    
    /* Read output */
    while ((n = ssh_channel_receive_data(session, buf, sizeof(buf))) > 0) {
        if (write_terminal_output(buf, n) < 0) {
            return -1;
        }
    }
    
    return (n == 0) ? 0 : -1;
}

static int set_nonblock_mode(int fd, int *old_flags) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    if (old_flags) {
        *old_flags = flags;
    }
    if (flags & O_NONBLOCK) {
        return 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void restore_fd_flags(int fd, int old_flags) {
    if (old_flags >= 0) {
        (void)fcntl(fd, F_SETFL, old_flags);
    }
}

static int ssh_read_password(char *password, size_t size) {
    struct pollfd pfd;
    int old_flags = -1;
    size_t len = 0;

    if (!password || size == 0) {
        return -1;
    }

    if (set_nonblock_mode(STDIN_FILENO, &old_flags) < 0) {
        return -1;
    }

    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    pfd.revents = 0;

    for (;;) {
        char buf[64];
        ssize_t n;

        pfd.revents = 0;
        if (poll(&pfd, 1, -1) < 0) {
            if (errno == EINTR) {
                continue;
            }
            restore_fd_flags(STDIN_FILENO, old_flags);
            return -1;
        }

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            restore_fd_flags(STDIN_FILENO, old_flags);
            return -1;
        }

        if (!(pfd.revents & POLLIN)) {
            continue;
        }

        n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n == 0) {
            /*
             * Ewok console stdin can transiently surface readable events
             * before input bytes are actually available.
             */
            usleep(1000);
            continue;
        }
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == 0) {
                usleep(1000);
                continue;
            }
            restore_fd_flags(STDIN_FILENO, old_flags);
            return -1;
        }

        for (ssize_t i = 0; i < n; i++) {
            unsigned char c = (unsigned char)buf[i];

            if (c == '\r' || c == '\n' || c == KEY_ENTER) {
                password[len] = '\0';
                write(STDOUT_FILENO, "\n", 1);
                restore_fd_flags(STDIN_FILENO, old_flags);
                return 0;
            }

            if (c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
                if (len > 0) {
                    len--;
                    write(STDOUT_FILENO, "\b \b", 3);
                }
                continue;
            }

            if (c < KEY_SPACE) {
                continue;
            }

            if (len + 1 < size) {
                password[len++] = (char)c;
                write(STDOUT_FILENO, "*", 1);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int port = SSH_DEFAULT_PORT;
    char *user = NULL;
    char *host = NULL;
    char *identity = NULL;
    int verbose = 0;
    int allocate_pty = 1;
    int no_command = 0;
    int opt;
    int command_start = -1;

    /* Initialize OpenSSL and random seed */
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS | OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, NULL);
    srand(42);  // Initialize random seed for padding generation

    /* Parse options */
    while ((opt = getopt(argc, argv, "p:l:i:o:vqtTNfh")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                if (port <= 0 || port > 65535) {
                    fprintf(stderr, "Invalid port: %s\n", optarg);
                    return 1;
                }
                break;
            case 'l':
                user = strdup(optarg);
                break;
            case 'i':
                identity = strdup(optarg);
                break;
            case 'o':
                /* Options parsing - simplified */
                break;
            case 'v':
                verbose++;
                break;
            case 'q':
                /* Quiet mode */
                break;
            case 't':
                allocate_pty = 1;
                break;
            case 'T':
                allocate_pty = 0;
                break;
            case 'N':
                no_command = 1;
                break;
            case 'f':
                /* Fork into background - not implemented */
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    /* Get hostname */
    if (optind >= argc) {
        fprintf(stderr, "Missing hostname\n");
        print_usage(argv[0]);
        return 1;
    }
    
    /* Parse [user@]hostname */
    char *parsed_user = NULL;
    if (parse_host(argv[optind], &parsed_user, &host) < 0) {
        fprintf(stderr, "Failed to parse hostname\n");
        return 1;
    }
    
    if (parsed_user) {
        if (user) free(user);
        user = parsed_user;
    }
    
    optind++;
    
    /* Check for command */
    if (optind < argc) {
        command_start = optind;
    }
    
    /* Get username if not specified */
    if (!user) {
        user = getenv("USER");
        if (!user) {
            user = "root";
        }
    }
    
    if (verbose) {
        printf("Connecting to %s@%s:%d...\n", user, host, port);
    }
    
    /* Create SSH session */
    g_session = ssh_session_new();
    if (!g_session) {
        fprintf(stderr, "Failed to create SSH session\n");
        return 1;
    }
    
    /* Connect to server */
    if (ssh_connect(g_session, host, port) < 0) {
        fprintf(stderr, "Failed to connect: %s\n", ssh_get_error(g_session));
        ssh_session_free(g_session);
        return 1;
    }
    
    if (verbose) {
        printf("Connected. Starting handshake...\n");
    }
    
    /* Protocol handshake */
    if (ssh_send_banner(g_session) < 0) {
        fprintf(stderr, "Failed to send banner: %s\n", ssh_get_error(g_session));
        goto cleanup;
    }
    
    if (ssh_receive_banner(g_session) < 0) {
        fprintf(stderr, "Failed to receive banner: %s\n", ssh_get_error(g_session));
        goto cleanup;
    }
    
    if (verbose) {
        printf("Server version: %s\n", g_session->server_version);
    }
    
    /* Key exchange - send KEXINIT first, then receive (for kex-strict mode) */
    if (ssh_send_kexinit(g_session) < 0) {
        fprintf(stderr, "Failed to send KEXINIT: %s\n", ssh_get_error(g_session));
        goto cleanup;
    }

    usleep(50000);  /* 50ms delay to ensure server processes KEXINIT */

    if (ssh_receive_kexinit(g_session) < 0) {
        fprintf(stderr, "Failed to receive KEXINIT: %s\n", ssh_get_error(g_session));
        goto cleanup;
    }
    
    if (ssh_handle_kex(g_session) < 0) {
        fprintf(stderr, "Key exchange failed: %s\n", ssh_get_error(g_session));
        goto cleanup;
    }
    
    if (verbose) {
        printf("Key exchange complete\n");
    }
    
    /* Request service */
    {
        ssh_packet_t packet;
        memset(&packet, 0, sizeof(packet));
        packet.type = SSH_MSG_SERVICE_REQUEST;
        
        const char *service = "ssh-userauth";
        ssh_write_string(packet.payload, (const uint8_t *)service, strlen(service));
        packet.payload_len = 4 + strlen(service);
        
        if (ssh_packet_send(g_session, &packet) < 0) {
            fprintf(stderr, "Failed to request service\n");
            goto cleanup;
        }
        
        if (ssh_packet_receive(g_session, &packet) < 0) {
            fprintf(stderr, "Failed to receive service response: %s\n", ssh_get_error(g_session));
            goto cleanup;
        }

        if (packet.type == SSH_MSG_EXT_INFO) {
            if (ssh_packet_receive(g_session, &packet) < 0) {
                fprintf(stderr, "Failed to receive service response after EXT_INFO: %s\n", ssh_get_error(g_session));
                goto cleanup;
            }
        }
        
        if (packet.type != SSH_MSG_SERVICE_ACCEPT) {
            fprintf(stderr, "Service request failed\n");
            goto cleanup;
        }
    }
    
    if (verbose) {
        printf("Starting authentication...\n");
    }
    
    /* Authentication */
    /* Try "none" first to see available methods */
    if (ssh_userauth_list(g_session, user) < 0) {
        /* Need to authenticate */

        /* For now, try password authentication */
        /* In a real implementation, we would support key-based auth too */
        char password[128];
        printf("Password: ");
        fflush(stdout);

        if (ssh_read_password(password, sizeof(password)) < 0) {
            fprintf(stderr, "Failed to read password\n");
            goto cleanup;
        }

        if (ssh_userauth_password(g_session, user, password) < 0) {
            fprintf(stderr, "Authentication failed: %s\n", ssh_get_error(g_session));
            goto cleanup;
        }
    }
    
    if (verbose) {
        printf("Authentication successful\n");
    }
    
    /* Open channel */
    if (ssh_channel_open_session(g_session) < 0) {
        fprintf(stderr, "Failed to open channel: %s\n", ssh_get_error(g_session));
        goto cleanup;
    }
    
    if (verbose) {
        printf("Channel opened\n");
    }
    
    /* Execute command or start shell */
    if (no_command) {
        /* Just keep connection open */
        while (ssh_is_connected(g_session)) {
            sleep(1);
        }
    } else if (command_start >= 0) {
        /* Execute specific command */
        if (do_exec_command(g_session, argc - command_start, argv + command_start) < 0) {
            goto cleanup;
        }
    } else {
        /* Interactive shell */
        if (allocate_pty) {
            if (ssh_channel_request_pty(g_session, getenv("TERM")) < 0) {
                fprintf(stderr, "Failed to request PTY: %s\n", ssh_get_error(g_session));
                /* Continue without PTY */
            }
        }
        
        if (ssh_channel_request_shell(g_session) < 0) {
            fprintf(stderr, "Failed to request shell: %s\n", ssh_get_error(g_session));
            goto cleanup;
        }
        
        if (verbose) {
            printf("Starting interactive session...\n");
        }
        
        do_interactive_session(g_session);
    }
    
cleanup:
    do_cleanup();

    if (host) free(host);
    if (identity) free(identity);

    return 0;
}
