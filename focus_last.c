/*
 * Simple program using xcb to listen for _NET_ACTIVE_WINDOW changes and
 * remembers the last two focused windows.
 *
 * When the program is already running and it's called again it will switch the
 * focus to the previous window (switching desktops if necessary).
 *
 * Copyright 2022 Jordan Callicoat <jordan.callicoat@gmail.com>
 *
 */

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xproto.h>

#define SCREEN_NUM 0
#define SLEEP_TIME 250
#define FILTER_NORMAL_WINDOWS true

#define LOCK_FILE "focus_last.lock"
#define STATE_FILE "focus_last.state"

const uint32_t STATE_VERSION = 0;

static char *lock_path;
static char *state_path;

xcb_window_t seen_windows[2] = {0};

uint32_t get_current_desktop(xcb_ewmh_connection_t *ewmh) {
    uint32_t current_desktop = XCB_NONE;
    xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop(ewmh, SCREEN_NUM),
                                       &current_desktop, NULL);
    return current_desktop;
}

xcb_window_t get_active_window(xcb_ewmh_connection_t *ewmh) {
    xcb_window_t active_window = XCB_NONE;
    xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, SCREEN_NUM),
                                     &active_window, NULL);
    return active_window;
}

void set_current_desktop(xcb_ewmh_connection_t *ewmh, uint32_t desktop) {
    xcb_ewmh_request_change_current_desktop(ewmh, SCREEN_NUM, desktop, XCB_CURRENT_TIME);
}

void set_active_window(xcb_ewmh_connection_t *ewmh, xcb_window_t window) {
    xcb_ewmh_request_change_active_window(ewmh, SCREEN_NUM, window,
                                          XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER, // pager
                                          XCB_CURRENT_TIME, 0); // don't set current window
}

uint32_t get_desktop_for_window(xcb_ewmh_connection_t *ewmh, xcb_window_t window) {
    uint32_t desktop = XCB_NONE;
    xcb_ewmh_get_wm_desktop_reply(ewmh, xcb_ewmh_get_wm_desktop(ewmh, window), &desktop, NULL);
    return desktop;
}

void activate_last_seen_window(xcb_ewmh_connection_t *ewmh) {
    uint32_t current_desktop = get_current_desktop(ewmh);
    uint32_t window_desktop = get_desktop_for_window(ewmh, seen_windows[0]);
    if (window_desktop != current_desktop)
        set_current_desktop(ewmh, window_desktop);
    set_active_window(ewmh, seen_windows[0]);
    xcb_flush(ewmh->connection);
}

bool is_normal_window(xcb_ewmh_connection_t *ewmh, xcb_window_t window) {
    if (!FILTER_NORMAL_WINDOWS)
        return true; // just pretend all windows are normal

    bool is_normal = false;
    xcb_ewmh_get_atoms_reply_t window_type;
    if (xcb_ewmh_get_wm_window_type_reply(ewmh, xcb_ewmh_get_wm_window_type(ewmh, window),
                                          &window_type, NULL)) {
        for (unsigned int i = 0; i < window_type.atoms_len; i++) {
            if (window_type.atoms[i] == ewmh->_NET_WM_WINDOW_TYPE_NORMAL) {
                is_normal = true;
                break;
            }
        }
        xcb_ewmh_get_atoms_reply_wipe(&window_type);
    }
    return is_normal;
}

void write_state_file();
void on_active_window_changed(xcb_ewmh_connection_t *ewmh) {
    xcb_window_t window = get_active_window(ewmh);
    if (window > 0 && is_normal_window(ewmh, window)) {
        if (seen_windows[0] == 0)
            seen_windows[0] = window;
        else if (seen_windows[1] != window) {
            if (seen_windows[1] != 0)
                seen_windows[0] = seen_windows[1];
            seen_windows[1] = window;
        }
        printf("[0] Window %d\n", seen_windows[0]);
        printf("[1] Window %d\n", seen_windows[1]);
        write_state_file();
    }
}

void receive_events(xcb_ewmh_connection_t *ewmh) {
    const uint32_t mask[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
    xcb_change_window_attributes(ewmh->connection, ewmh->screens[SCREEN_NUM]->root,
                                 XCB_CW_EVENT_MASK, &mask);
    xcb_flush(ewmh->connection);

    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(ewmh->connection))) {
        if (event->response_type == XCB_PROPERTY_NOTIFY) {
            xcb_property_notify_event_t *e = (void *)event;
            if (e->atom == ewmh->_NET_ACTIVE_WINDOW) {
                on_active_window_changed(ewmh);
            }
        }
        free(event);
    }
}

void print_info(xcb_ewmh_connection_t *ewmh) {
    printf("Current Desktop is: %d\n", get_current_desktop(ewmh));
    printf("Active Window is: %d\n", get_active_window(ewmh));
}

void set_paths() {
    char *dir = getenv("XDG_RUNTIME_DIR");
    if (dir == NULL || dir[0] != '/')
        dir = "/tmp";

    size_t dirlen = strlen(dir);
    lock_path = malloc(dirlen + sizeof("/" LOCK_FILE));
    if (lock_path == NULL) {
        fputs("Couldn't allocate memory.", stderr);
        exit(1);
    }
    memcpy(lock_path, dir, dirlen);
    memcpy(lock_path + dirlen, "/" LOCK_FILE, sizeof("/" LOCK_FILE));

    state_path = malloc(dirlen + sizeof("/" STATE_FILE));
    if (state_path == NULL) {
        fputs("Couldn't allocate memory.", stderr);
        exit(1);
    }
    memcpy(state_path, dir, dirlen);
    memcpy(state_path + dirlen, "/" STATE_FILE, sizeof("/" STATE_FILE));
}

void free_paths() {
    free(lock_path);
    free(state_path);
}

void unlink_lock_file() {
    unlink(lock_path);
    free_paths();
}

int check_or_acquire_instance_lock() {
    int is_running = 0;

    int fd = open(lock_path, O_RDWR | O_CREAT, 0600);
    if (fd < 0) {
        fprintf(stderr, "Couldn't create lock file: %s\n", lock_path);
        exit(1);
    }

    struct flock fl = {.l_start = 0, .l_len = 0, .l_type = F_WRLCK, .l_whence = SEEK_SET};
    if (fcntl(fd, F_SETLK, &fl) < 0)
        is_running = 1;

    return is_running;
}

void read_state_file() {
    FILE *fp = fopen(state_path, "rb");

    if (fp == NULL) {
        fprintf(stderr, "Could not open state file for reading %s\n", state_path);
        return;  // don't care if we don't have a state file yet
    }

    int state_version;
    if (fread(&state_version, sizeof(int), 1, fp) != 1 || state_version != STATE_VERSION) {
        puts("State file version mismatch, removing existing state file");
        fclose(fp);
        unlink(state_path);
        return;
    }

    xcb_window_t seen_window = XCB_NONE;
    for (unsigned int i = 0; i < 2; i++) {
        if (fread(&seen_window, sizeof(xcb_window_t), 1, fp) != 1) {
            fprintf(stderr, "Parsing state file %s failed\n", state_path);
            break;
        }
        seen_windows[i] = seen_window;
    }
    fclose(fp);
}

void write_state_file() {
    FILE *fp = fopen(state_path, "wb");

    if (fp == NULL) {
        fprintf(stderr,
                "Could open state file for writing %s "
                "- the state file is necessary for this program to work.\n",
                state_path);
        return;
    }

    fwrite(&STATE_VERSION, sizeof(STATE_VERSION), 1, fp);
    for (unsigned int i = 0; i < 2; i++)
        fwrite(&seen_windows[i], sizeof(xcb_window_t), 1, fp);

    fflush(fp);
    fclose(fp);
}

xcb_ewmh_connection_t get_ewmh_connection() {
    xcb_connection_t *connection = xcb_connect(NULL, NULL);
    int err = xcb_connection_has_error(connection);
    if (err) {
        fprintf(stderr, "Error connecting to X11 (error code %d)\n", err);
        exit(1);
    }

    xcb_ewmh_connection_t ewmh;
    if (!xcb_ewmh_init_atoms_replies(&ewmh, xcb_ewmh_init_atoms(connection, &ewmh), NULL)) {
        fputs("EWMH init failed\n", stderr);
        exit(1);
    }

    return ewmh;
}

void cleanup_connection(xcb_ewmh_connection_t *ewmh) {
    xcb_ewmh_connection_wipe(ewmh);
    xcb_disconnect(ewmh->connection);
}

int main() {
    set_paths();
    read_state_file();

    int is_running = check_or_acquire_instance_lock();

    xcb_ewmh_connection_t ewmh = get_ewmh_connection();

    /* print_info(&ewmh); */
    if (is_running) {
        activate_last_seen_window(&ewmh);
        // we need to let the wm have time to process the events
        // before closing our connection
        usleep(SLEEP_TIME);
        /* print_info(&ewmh); */
        cleanup_connection(&ewmh);
        free_paths();
        exit(0);
    }

    atexit(unlink_lock_file);

    receive_events(&ewmh);

    cleanup_connection(&ewmh);

    return 0;
}
