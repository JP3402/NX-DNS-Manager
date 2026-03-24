// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>
#include <stdbool.h>
#include <ctype.h>

// A struct to hold a single host file entry
typedef struct {
    char ip_address[16];
    char hostname[256];
    bool enabled;
} HostEntry;

// Function to trim leading/trailing whitespace from a string
char *trim(char *str) {
    char *end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}

// Function to read and parse the hosts file
int read_hosts_file(const char *path, HostEntry entries[], int max_entries) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return -1; // File not found or couldn't be opened
    }

    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), file) && count < max_entries) {
        char *trimmed_line = trim(line);
        if (strlen(trimmed_line) == 0) {
            continue; // Skip empty lines
        }

        if (trimmed_line[0] == '#') {
            entries[count].enabled = false;
            // Skip the '#' character for parsing
            trimmed_line++;
        } else {
            entries[count].enabled = true;
        }
        
        trimmed_line = trim(trimmed_line);

        // Parse the line for IP and hostname
        char *token = strtok(trimmed_line, " \t");
        if (token != NULL) {
            strncpy(entries[count].ip_address, token, sizeof(entries[count].ip_address) - 1);
            entries[count].ip_address[sizeof(entries[count].ip_address) - 1] = '\0';

            token = strtok(NULL, " \t");
            if (token != NULL) {
                strncpy(entries[count].hostname, token, sizeof(entries[count].hostname) - 1);
                entries[count].hostname[sizeof(entries[count].hostname) - 1] = '\0';
                count++;
            }
        }
    }

    fclose(file);
    return count;
}

// Function to write the hosts file
bool write_hosts_file(const char *path, HostEntry entries[], int num_entries) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return false; // Could not open the file for writing
    }

    for (int i = 0; i < num_entries; i++) {
        if (!entries[i].enabled) {
            fprintf(file, "#");
        }
        fprintf(file, "%s %s\n", entries[i].ip_address, entries[i].hostname);
    }

    fclose(file);
    return true;
}

// Function to check if a file exists
bool file_exists(const char *path) {
    FILE *file = fopen(path, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    while(appletMainLoop()) {
        const char *potential_files[] = {
            "/atmosphere/hosts/default.txt",
            "/atmosphere/hosts/emummc.txt",
            "/atmosphere/hosts/sysmmc.txt"
        };
        char *found_files[3];
        int num_host_files = 0;
        for (int i = 0; i < 3; i++) {
            if (file_exists(potential_files[i])) {
                found_files[num_host_files] = (char*)potential_files[i];
                num_host_files++;
            }
        }

        int selected_file = 0;
        char chosen_file[256] = "";

        if (num_host_files == 0) {
            printf("\x1b[2J");
            printf("No hosts files found in /atmosphere/hosts/.\n");
            printf("Please create default.txt, emummc.txt, or sysmmc.txt.\n\n");
            printf("Press + to exit.");
            consoleUpdate(NULL);
            while(appletMainLoop()) {
                padUpdate(&pad);
                u64 kDown = padGetButtonsDown(&pad);
                if (kDown & HidNpadButton_Plus) {
                    break;
                }
            }
            break;
        }

        // File selection loop
        while(appletMainLoop()) {
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);

            if (kDown & HidNpadButton_Plus) {
                break;
            }

            if (kDown & HidNpadButton_Up) {
                selected_file--;
                if (selected_file < 0) {
                    selected_file = num_host_files - 1;
                }
            }

            if (kDown & HidNpadButton_Down) {
                selected_file++;
                if (selected_file >= num_host_files) {
                    selected_file = 0;
                }
            }

            if (kDown & HidNpadButton_A) {
                strncpy(chosen_file, found_files[selected_file], sizeof(chosen_file) - 1);
                chosen_file[sizeof(chosen_file) - 1] = '\0';
                break;
            }

            printf("\x1b[2J");
            printf("Select a hosts file to edit:\n\n");

            for (int i = 0; i < num_host_files; i++) {
                if (i == selected_file) {
                    printf("> ");
                } else {
                    printf("  ");
                }
                printf("%s\n", found_files[i]);
            }
            
            printf("\nPress A to select, + to exit.");

            consoleUpdate(NULL);
        }

        if (strlen(chosen_file) == 0) {
            break;
        }

        HostEntry entries[100]; // Array to hold host entries
        int num_entries = read_hosts_file(chosen_file, entries, 100);

        int selected_entry = 0;
        char saved_message[20] = "";

        // Editor loop
        while (appletMainLoop())
        {
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);

            if (kDown & HidNpadButton_Plus) {
                goto exit_app;
            }

            if (kDown & HidNpadButton_B) {
                break; // Return to file selection
            }

            if (num_entries > 0) {
                if (kDown & HidNpadButton_Up) {
                    selected_entry--;
                    if (selected_entry < 0) {
                        selected_entry = num_entries - 1;
                    }
                }

                if (kDown & HidNpadButton_Down) {
                    selected_entry++;
                    if (selected_entry >= num_entries) {
                        selected_entry = 0;
                    }
                }

                if (kDown & HidNpadButton_A) {
                    entries[selected_entry].enabled = !entries[selected_entry].enabled;
                }
                
                if (kDown & HidNpadButton_Y) {
                    if (write_hosts_file(chosen_file, entries, num_entries)) {
                        strcpy(saved_message, "Saved!");
                    } else {
                        strcpy(saved_message, "Error saving!");
                    }
                    num_entries = read_hosts_file(chosen_file, entries, 100);
                } else if (kDown) {
                    strcpy(saved_message, "");
                }
            }

            printf("\x1b[2J");
            
            printf("NX DNS Manager - %s\n", chosen_file);
            printf("Use D-Pad Up/Down to select an entry.\n");
            printf("Press A to toggle an entry on or off.\n");
            printf("Press Y to save changes.\n");
            printf("Press B to go back. Press + to exit.\n\n");

            if (strlen(saved_message) > 0) {
                printf("%s\n", saved_message);
            }

            if (num_entries < 0) {
                printf("Error reading hosts file.\n");
            } else {
                printf("Found %d host entries:\n", num_entries);
            }
            
            for (int i = 0; i < num_entries; i++) {
                if (i == selected_entry) {
                    printf("> ");
                } else {
                    printf("  ");
                }
                printf("[%c] %s %s\n",
                       entries[i].enabled ? 'X' : ' ',
                       entries[i].ip_address,
                       entries[i].hostname);
            }

            consoleUpdate(NULL);
        }
    }

exit_app:

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
