/*
 * Simple program using xcb to listen for _NET_ACTIVE_WINDOW changes and remembers the last two focused windows.
 *
 * When the program is already running and it called again it will switch the focus to the previous window (switching desktops if necessary).
 *
 * Copyright 2022 Jordan Callicoat <jordan.callicoat@gmail.com>
 *
 * gcc focus_last.c -o focus_last -lxcb -lxcb-ewmh
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xproto.h>

#define SCREEN_NUM 0

#define LOCK_FILE "focus_last.lock"
#define STATE_FILE "focus_last.state"

static char *lock_path;
static char *state_path;

typedef struct {
    uint32_t desktop;
    xcb_window_t window;
} seen_window_t;

seen_window_t seen_windows[2] = {0};


xcb_ewmh_connection_t get_ewmh_connection() {
    xcb_connection_t *connection = xcb_connect(NULL, NULL);
    int err = xcb_connection_has_error(connection);
    if (err) {
        fprintf(stderr, "Error connecting to X11 (error code %d)\n", err);
        exit(1);
    }

    xcb_ewmh_connection_t ewmh;
    if (!xcb_ewmh_init_atoms_replies(&ewmh,
                                     xcb_ewmh_init_atoms(connection, &ewmh),
                                     NULL)) {
        fputs("EWMH init failed\n", stderr);
        exit(1);
    }
    
    return ewmh;
}

void cleanup_connection(xcb_ewmh_connection_t *ewmh) {
    xcb_ewmh_connection_wipe(ewmh);
    xcb_disconnect(ewmh->connection);
}

uint32_t get_current_desktop(xcb_ewmh_connection_t *ewmh) {
    uint32_t current_desktop;
    xcb_ewmh_get_current_desktop_reply(ewmh,
                                       xcb_ewmh_get_current_desktop(ewmh, 0),
                                       &current_desktop, 0);
    return current_desktop;
}

xcb_window_t get_active_window(xcb_ewmh_connection_t *ewmh) {
    xcb_window_t active_window;
    xcb_ewmh_get_active_window_reply(ewmh,
                                     xcb_ewmh_get_active_window(ewmh, 0),
                                     &active_window, 0);
    return active_window;
}

void set_current_desktop(xcb_ewmh_connection_t *ewmh, uint32_t desktop) {
    uint32_t data[5] = { desktop, 0, XCB_CURRENT_TIME, 0, 0 }; // new_index, timestamp, padding
    xcb_ewmh_send_client_message(ewmh->connection,
                                 ewmh->screens[SCREEN_NUM]->root,
                                 ewmh->screens[SCREEN_NUM]->root,
                                 ewmh->_NET_CURRENT_DESKTOP,
                                 sizeof(data), data);
    xcb_flush(ewmh->connection);
}

void set_active_window(xcb_ewmh_connection_t *ewmh, xcb_window_t window) {
    uint32_t data[5] = { 2, XCB_CURRENT_TIME, 0, 0, 0 }; // source indication (1=app, 2=pager), timestamp, currently active window, padding
    xcb_ewmh_send_client_message(ewmh->connection,
                                 window,
                                 ewmh->screens[SCREEN_NUM]->root,
                                 ewmh->_NET_ACTIVE_WINDOW,
                                 sizeof(data), data);
    xcb_flush(ewmh->connection);
}

void activate_last_seen_window(xcb_ewmh_connection_t *ewmh) {
    /* printf("[0] Desktop %d, Window %d\n", seen_windows[0].desktop, seen_windows[0].window); */
    /* printf("[1] Desktop %d, Window %d\n", seen_windows[1].desktop, seen_windows[1].window); */
    set_current_desktop(ewmh, seen_windows[0].desktop);
    set_active_window(ewmh, seen_windows[0].window);
}

void write_state_file();
void on_active_window_changed(xcb_ewmh_connection_t *ewmh) {
    xcb_window_t window = get_active_window(ewmh);
    if (window > 0) {
        uint32_t desktop = get_current_desktop(ewmh);
        if (seen_windows[0].window == 0) {
            seen_windows[0].desktop = desktop;
            seen_windows[0].window = window;
        } else if (seen_windows[1].window != window) {
            if (seen_windows[1].window != 0) {
                seen_windows[0].desktop = seen_windows[1].desktop;
                seen_windows[0].window = seen_windows[1].window;
            }
            seen_windows[1].desktop = desktop;
            seen_windows[1].window = window;

        }
        printf("[0] Desktop %d, Window %d\n", seen_windows[0].desktop, seen_windows[0].window);
        printf("[1] Desktop %d, Window %d\n", seen_windows[1].desktop, seen_windows[1].window);
        write_state_file();
    }
}

void receive_events(xcb_ewmh_connection_t *ewmh) {
    const uint32_t mask[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
    xcb_change_window_attributes(ewmh->connection,
                                 ewmh->screens[SCREEN_NUM]->root,
                                 XCB_CW_EVENT_MASK,
                                 &mask);
    xcb_flush(ewmh->connection);

    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(ewmh->connection)))
    {
        if (event->response_type == XCB_PROPERTY_NOTIFY)
        {
            xcb_property_notify_event_t *e = (void *) event;
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
    memcpy(lock_path + dirlen, "/" LOCK_FILE,
        sizeof("/" LOCK_FILE));  /* copies '\0' */

    state_path = malloc(dirlen + sizeof("/" STATE_FILE));
    if (state_path == NULL) {
        fputs("Couldn't allocate memory.", stderr);
        exit(1);
    }
    memcpy(state_path, dir, dirlen);
    memcpy(state_path + dirlen, "/" STATE_FILE,
        sizeof("/" STATE_FILE));  /* copies '\0' */
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
     
    struct flock fl = {
        .l_start = 0,
        .l_len = 0,
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET
    };
    if (fcntl(fd, F_SETLK, &fl) < 0)
        is_running = 1;

    return is_running;
}

void read_state_file() {
    FILE *fp = fopen(state_path, "rb");

    if (fp == NULL) {
        fprintf(stderr, "Could not open state file for reading %s\n", state_path);
        return; // don't care if we don't have a state file yet
    }

    seen_window_t seen_window = {0};
    for (int i = 0 ; i < 2 ; i++) {
        if (fread(&seen_window, sizeof(seen_window_t), 1, fp) != 1) {
            fprintf(stderr, "Parsing state file %s failed\n", state_path);
            break;
       }
       seen_windows[i] = seen_window;
    }
    fclose(fp);
}

void write_state_file() {
    FILE *fp = fopen(state_path, "wb");
    
    if (fp == NULL) 
        fprintf(stderr, "Could open state file for writing %s - this is necessary for this program to work.\n", state_path);
    
    // we don't care about if writing fails, we only read valid records above
    for (int i = 0 ; i < 2 ; i++)
        fwrite(&seen_windows[i], sizeof(seen_window_t), 1, fp);

    fflush(fp);
    fclose(fp);
}

void main() {
    set_paths();
    read_state_file();

    int is_running = check_or_acquire_instance_lock();
    
    xcb_ewmh_connection_t ewmh = get_ewmh_connection();

    /* print_info(&ewmh); */
    if (is_running) {
        activate_last_seen_window(&ewmh);
        usleep(150); // we need to let the wm have time to process the events
        /* print_info(&ewmh); */
        cleanup_connection(&ewmh);
        free_paths();
        exit(0);
    }

    atexit(unlink_lock_file);

    receive_events(&ewmh);

    cleanup_connection(&ewmh);
}
