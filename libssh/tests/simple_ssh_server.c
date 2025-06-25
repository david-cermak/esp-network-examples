/*
 * Simple SSH Server Example
 * 
 * This is a minimal SSH server that:
 * - Listens on 0.0.0.0 (all interfaces)
 * - Accepts password authentication
 * - Provides a simple shell
 * - Uses the built libssh library
 */

#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define DEFAULT_PORT "2222"
#define DEFAULT_USERNAME "testuser"
#define DEFAULT_PASSWORD "testpass"

static int authenticated = 0;
static int tries = 0;
static ssh_channel channel = NULL;

// Forward declarations
static int pty_request(ssh_session session, ssh_channel channel, 
                      const char *term, int cols, int rows, 
                      int py, int px, void *userdata);
static int shell_request(ssh_session session, ssh_channel channel, void *userdata);
static int data_function(ssh_session session, ssh_channel channel, 
                        void *data, uint32_t len, int is_stderr, void *userdata);

// Signal handler for clean shutdown
static void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        printf("\nReceived signal %d, shutting down...\n", signo);
        exit(0);
    }
}

// Authentication callback - deny none authentication
static int auth_none(ssh_session session, const char *user, void *userdata) {
    (void)user;
    (void)userdata;
    
    printf("[DEBUG] Auth none requested for user: %s\n", user);
    ssh_set_auth_methods(session, SSH_AUTH_METHOD_PASSWORD);
    return SSH_AUTH_DENIED;
}

// Password authentication callback
static int auth_password(ssh_session session, const char *user, 
                        const char *password, void *userdata) {
    (void)userdata;
    
    printf("[DEBUG] Password auth attempt for user: %s\n", user);
    
    if (strcmp(user, DEFAULT_USERNAME) == 0 && 
        strcmp(password, DEFAULT_PASSWORD) == 0) {
        authenticated = 1;
        printf("[DEBUG] Authentication successful for user: %s\n", user);
        return SSH_AUTH_SUCCESS;
    }
    
    tries++;
    if (tries >= 3) {
        printf("[DEBUG] Too many authentication attempts\n");
        ssh_disconnect(session);
        return SSH_AUTH_DENIED;
    }
    
    printf("[DEBUG] Authentication failed (attempt %d/3)\n", tries);
    return SSH_AUTH_DENIED;
}

    struct ssh_channel_callbacks_struct channel_cb = {
        .userdata = NULL,
        .channel_pty_request_function = pty_request,
        .channel_shell_request_function = shell_request,
        .channel_data_function = data_function
    };


// Channel open callback
static ssh_channel channel_open(ssh_session session, void *userdata) {
    (void)userdata;
    
    if (channel != NULL) {
        printf("[DEBUG] Channel already exists\n");
        return NULL;
    }
    
    printf("[DEBUG] Opening new channel\n");
    channel = ssh_channel_new(session);
    
    // Set up channel callbacks
    
    ssh_callbacks_init(&channel_cb);
    ssh_set_channel_callbacks(channel, &channel_cb);
    
    printf("[DEBUG] Channel created and callbacks set\n");
    return channel;
}

// PTY request callback
static int pty_request(ssh_session session, ssh_channel channel, 
                      const char *term, int cols, int rows, 
                      int py, int px, void *userdata) {
    (void)session;
    (void)channel;
    (void)term;
    (void)cols;
    (void)rows;
    (void)py;
    (void)px;
    (void)userdata;
    
    printf("[DEBUG] PTY requested: %s (%dx%d)\n", term, cols, rows);
    return SSH_OK;
}

// Shell request callback
static int shell_request(ssh_session session, ssh_channel channel, void *userdata) {
    (void)session;
    (void)userdata;
    
    printf("[DEBUG] Shell requested\n");
    
    if (channel == NULL) {
        printf("[DEBUG] Shell requested but channel is NULL\n");
        return SSH_ERROR;
    }
    
    // // Send a welcome message
    // int rc = ssh_channel_write(channel, "Welcome to Simple SSH Server!\n", 30);
    // if (rc != SSH_OK) {
    //     printf("[DEBUG] Failed to write welcome message: %s\n", ssh_get_error(session));
    //     return SSH_ERROR;
    // }
    
    // rc = ssh_channel_write(channel, "Type 'exit' to disconnect.\n\n", 26);
    // if (rc != SSH_OK) {
    //     printf("[DEBUG] Failed to write exit message: %s\n", ssh_get_error(session));
    //     return SSH_ERROR;
    // }
    
    // // Send initial prompt
    // rc = ssh_channel_write(channel, "$ ", 2);
    // if (rc != SSH_OK) {
    //     printf("[DEBUG] Failed to write prompt: %s\n", ssh_get_error(session));
    //     return SSH_ERROR;
    // }
    
    printf("[DEBUG] Shell setup completed successfully\n");
    return SSH_OK;
}

// Data callback for handling incoming data
static int data_function(ssh_session session, ssh_channel channel, 
                        void *data, uint32_t len, int is_stderr, void *userdata) {
    (void)session;
    (void)channel;
    (void)data;
    (void)is_stderr;
    (void)userdata;
    
    printf("[DEBUG] Received %u bytes of data\n", len);
    
    if (channel == NULL || data == NULL) {
        printf("[DEBUG] Data function called with NULL channel or data\n");
        return SSH_ERROR;
    }
    
    // Echo back what was received
    int rc = ssh_channel_write(channel, "Received: ", 10);
    if (rc != SSH_OK) {
        printf("[DEBUG] Failed to write 'Received:' prefix\n");
        return SSH_ERROR;
    }
    
    rc = ssh_channel_write(channel, data, len);
    if (rc != SSH_OK) {
        printf("[DEBUG] Failed to echo data back\n");
        return SSH_ERROR;
    }
    
    // Send new prompt
    rc = ssh_channel_write(channel, "\n$ ", 3);
    if (rc != SSH_OK) {
        printf("[DEBUG] Failed to write prompt\n");
        return SSH_ERROR;
    }
    
    return SSH_OK;
}

int main(int argc, char **argv) {
    ssh_bind sshbind;
    ssh_session session;
    ssh_event event;
    int rc;
    const char *port = DEFAULT_PORT;
    const char *hostkey_path = NULL;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = argv[++i];
        } else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            hostkey_path = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [-p PORT] [-k HOSTKEY]\n", argv[0]);
            printf("  -p PORT     Port to listen on (default: %s)\n", DEFAULT_PORT);
            printf("  -k HOSTKEY  Path to host key file\n");
            printf("  -h          Show this help\n");
            printf("\nDefault credentials: %s/%s\n", DEFAULT_USERNAME, DEFAULT_PASSWORD);
            return 0;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize libssh
    ssh_init();
    
    // Create SSH bind object
    sshbind = ssh_bind_new();
    if (sshbind == NULL) {
        fprintf(stderr, "Failed to create SSH bind object\n");
        return 1;
    }
    
    // Set bind options
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDADDR, "0.0.0.0");
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, port);
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_LOG_VERBOSITY_STR, "1");
    
    // Set host key
    if (hostkey_path != NULL) {
        ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY, hostkey_path);
    } else {
        // Try to use a default key
        ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY, "/etc/ssh/ssh_host_rsa_key");
    }
    
    // Listen for connections
    rc = ssh_bind_listen(sshbind);
    if (rc != SSH_OK) {
        fprintf(stderr, "Failed to listen on 0.0.0.0:%s: %s\n", 
                port, ssh_get_error(sshbind));
        ssh_bind_free(sshbind);
        return 1;
    }
    
    printf("Simple SSH Server listening on 0.0.0.0:%s\n", port);
    printf("Default credentials: %s/%s\n", DEFAULT_USERNAME, DEFAULT_PASSWORD);
    printf("Press Ctrl+C to stop\n\n");
    
    // Accept connections
    while (1) {
        session = ssh_new();
        if (session == NULL) {
            fprintf(stderr, "Failed to create session\n");
            continue;
        }
        
        rc = ssh_bind_accept(sshbind, session);
        if (rc != SSH_OK) {
            fprintf(stderr, "Failed to accept connection: %s\n", 
                    ssh_get_error(sshbind));
            ssh_free(session);
            continue;
        }
        
        printf("[DEBUG] New connection accepted\n");
        
        // Set up server callbacks
        struct ssh_server_callbacks_struct server_cb = {
            .userdata = NULL,
            .auth_none_function = auth_none,
            .auth_password_function = auth_password,
            .channel_open_request_session_function = channel_open
        };
        
        ssh_callbacks_init(&server_cb);
        ssh_set_server_callbacks(session, &server_cb);
        printf("[DEBUG] Server callbacks set\n");
        
        // Handle key exchange
        rc = ssh_handle_key_exchange(session);
        if (rc != SSH_OK) {
            fprintf(stderr, "[DEBUG] Key exchange failed: %s\n", 
                    ssh_get_error(session));
            ssh_disconnect(session);
            ssh_free(session);
            continue;
        }
        
        printf("[DEBUG] Key exchange completed\n");
        
        // Set up authentication methods
        ssh_set_auth_methods(session, SSH_AUTH_METHOD_PASSWORD);
        printf("[DEBUG] Authentication methods set\n");
        
        // Create event for session handling
        event = ssh_event_new();
        if (event == NULL) {
            fprintf(stderr, "[DEBUG] Failed to create event\n");
            ssh_disconnect(session);
            ssh_free(session);
            continue;
        }
        
        // Add session to event
        if (ssh_event_add_session(event, session) != SSH_OK) {
            fprintf(stderr, "[DEBUG] Failed to add session to event\n");
            ssh_event_free(event);
            ssh_disconnect(session);
            ssh_free(session);
            continue;
        }
        
        printf("[DEBUG] Session added to event, starting main loop\n");
        
        // Check initial channel state
        printf("[DEBUG] Initial channel state: channel=%p, is_open=%s\n", 
               channel, channel ? (ssh_channel_is_open(channel) ? "yes" : "no") : "NULL");
        
        // Wait for authentication and channel creation (like the official example)
        int n = 0;
        while (authenticated == 0 || channel == NULL) {
            printf("[DEBUG] Waiting for auth/channel: auth=%s, channel=%p (attempt %d)\n", 
                   authenticated ? "yes" : "no", channel, n);
            
            // If the user has used up all attempts, or if he hasn't been able to
            // authenticate in 10 seconds (n * 100ms), disconnect.
            if (tries >= 3 || n >= 100) {
                printf("[DEBUG] Timeout waiting for authentication/channel\n");
                break;
            }
            
            if (ssh_event_dopoll(event, 100) == SSH_ERROR) {
                printf("[DEBUG] Error polling events: %s\n", ssh_get_error(session));
                break;
            }
            n++;
        }
        
        // If we have a channel, set up callbacks and continue
        if (channel != NULL) {
            printf("[DEBUG] Channel created, setting up callbacks\n");
            
            // Set up channel callbacks
            // struct ssh_channel_callbacks_struct channel_cb = {
            //     .userdata = NULL,
            //     .channel_pty_request_function = pty_request,
            //     .channel_shell_request_function = shell_request,
            //     .channel_data_function = data_function
            // };
            
            // ssh_callbacks_init(&channel_cb);
            // ssh_set_channel_callbacks(channel, &channel_cb);
            
            // Continue polling until channel closes
            while (ssh_channel_is_open(channel)) {
                if (ssh_event_dopoll(event, 1000) == SSH_ERROR) {
                    printf("[DEBUG] Error polling events: %s\n", ssh_get_error(session));
                    break;
                }
            }
        }
        
        printf("[DEBUG] Connection closed\n");
        
        // Clean up
        if (channel != NULL) {
            ssh_channel_free(channel);
            channel = NULL;
        }
        authenticated = 0;
        tries = 0;
        ssh_event_free(event);
        ssh_disconnect(session);
        ssh_free(session);
    }
    
    // Clean up
    ssh_bind_free(sshbind);
    ssh_finalize();
    
    return 0;
}
