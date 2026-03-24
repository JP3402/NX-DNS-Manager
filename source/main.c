// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_ENTRIES 1000

// Differentiate between host entries and pure comments/empty lines
typedef enum {
    ENTRY_TYPE_HOST,
    ENTRY_TYPE_COMMENT,
    ENTRY_TYPE_EMPTY
} EntryType;

// A struct to hold a single host file line
typedef struct {
    EntryType type;
    char raw_line[512]; 
    char ip_address[64];
    char hostname[256];
    bool enabled;
} HostEntry;

// Function to trim leading/trailing whitespace
char *trim(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Function to read and parse the hosts file safely
int read_hosts_file(const char *path, HostEntry *entries, int max_entries) {
    FILE *file = fopen(path, "r");
    if (file == NULL) return -1;

    char line[512];
    char original_line[512];
    int count = 0;

    while (fgets(line, sizeof(line), file) && count < max_entries) {
        strcpy(original_line, line);
        char *trimmed_line = trim(line);
        
        if (strlen(trimmed_line) == 0) {
            entries[count].type = ENTRY_TYPE_EMPTY;
            strcpy(entries[count].raw_line, original_line);
            count++;
            continue;
        }

        bool is_commented = false;
        char *parse_ptr = trimmed_line;

        if (parse_ptr[0] == '#') {
            is_commented = true;
            parse_ptr++; 
            parse_ptr = trim(parse_ptr);
        }

        char temp_line[512];
        strncpy(temp_line, parse_ptr, sizeof(temp_line) - 1);
        temp_line[sizeof(temp_line) - 1] = '\0';

        char *token1 = strtok(temp_line, " \t");
        char *token2 = strtok(NULL, " \t");
        char *token3 = strtok(NULL, " \t");

        if (token1 != NULL && token2 != NULL && token3 == NULL) {
            entries[count].type = ENTRY_TYPE_HOST;
            entries[count].enabled = !is_commented;
            strncpy(entries[count].ip_address, token1, sizeof(entries[count].ip_address) - 1);
            strncpy(entries[count].hostname, token2, sizeof(entries[count].hostname) - 1);
        } else {
            entries[count].type = ENTRY_TYPE_COMMENT;
            strcpy(entries[count].raw_line, original_line);
        }
        count++;
    }
    fclose(file);
    return count;
}

// Function to write the hosts file
bool write_hosts_file(const char *path, HostEntry *entries, int num_entries) {
    FILE *file = fopen(path, "w");
    if (file == NULL) return false;

    for (int i = 0; i < num_entries; i++) {
        if (entries[i].type == ENTRY_TYPE_HOST) {
            if (!entries[i].enabled) fprintf(file, "#");
            fprintf(file, "%s %s\n", entries[i].ip_address, entries[i].hostname);
        } else {
            fprintf(file, "%s", entries[i].raw_line);
            if (strchr(entries[i].raw_line, '\n') == NULL) fprintf(file, "\n");
        }
    }
    fclose(file);
    return true;
}

void print_header(const char *subtitle) {
    printf("\x1b[2J\x1b[H"); // Clear screen and home cursor
    printf("\x1b[33mNX DNS Manager - v1.1.0 \x1b[0m\n");
    if (subtitle != NULL && strlen(subtitle) > 0) {
        printf("%s\n", subtitle);
    }
    printf("\n");
}

bool file_exists(const char *path) {
    FILE *file = fopen(path, "r");
    if (file) { fclose(file); return true; }
    return false;
}

int main(int argc, char* argv[]) {
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    const char *potential_files[] = {
        "/atmosphere/hosts/default.txt",
        "/atmosphere/hosts/emummc.txt",
        "/atmosphere/hosts/sysmmc.txt"
    };

    while(appletMainLoop()) {
        char *found_files[3];
        int num_host_files = 0;

        for (int i = 0; i < 3; i++) {
            if (file_exists(potential_files[i])) {
                found_files[num_host_files] = (char*)potential_files[i];
                num_host_files++;
            }
        }

        if (num_host_files == 0) {
            print_header(NULL);
            printf("No hosts files found in /atmosphere/hosts/.\n");
            printf("Press + to exit.");
            consoleUpdate(NULL);
            while(appletMainLoop()) {
                padUpdate(&pad);
                if (padGetButtonsDown(&pad) & HidNpadButton_Plus) return 0;
            }
        }

        int selected_file = 0;
        char chosen_file[256] = "";

        // --- FILE SELECTION LOOP ---
        while(appletMainLoop()) {
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);

            if (kDown & HidNpadButton_Plus) return 0;

            if (kDown & HidNpadButton_Up) {
                selected_file--;
                if (selected_file < 0) selected_file = num_host_files - 1;
            }
            if (kDown & HidNpadButton_Down) {
                selected_file++;
                if (selected_file >= num_host_files) selected_file = 0;
            }
            if (kDown & HidNpadButton_A) {
                strncpy(chosen_file, found_files[selected_file], sizeof(chosen_file) - 1);
                break;
            }

            print_header("Select a hosts file to edit:");
            for (int i = 0; i < num_host_files; i++) {
                if (i == selected_file) printf("\x1b[32m> %s\x1b[0m\n", found_files[i]);
                else printf("  %s\n", found_files[i]);
            }
            printf("\nPress A to select, + to exit.");
            consoleUpdate(NULL);
        }

        if (strlen(chosen_file) == 0) break;

        // --- EDITOR SECTION ---
        HostEntry *entries = (HostEntry*)malloc(MAX_ENTRIES * sizeof(HostEntry));
        if (!entries) break;

        int num_entries = read_hosts_file(chosen_file, entries, MAX_ENTRIES);
        int selected_entry = 0;
        char saved_message[64] = "";

        // Set initial cursor to first host entry
        for (int i = 0; i < num_entries; i++) {
            if (entries[i].type == ENTRY_TYPE_HOST) {
                selected_entry = i;
                break;
            }
        }

        while (appletMainLoop()) {
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);

            if (kDown & HidNpadButton_Plus) {
                free(entries);
                consoleExit(NULL);
                return 0;
            }
            
            // GO BACK TO MENU
            if (kDown & HidNpadButton_B) {
                break; 
            }

            if (num_entries > 0) {
                if (kDown & HidNpadButton_Up) {
                    int prev = selected_entry;
                    do {
                        selected_entry--;
                        if (selected_entry < 0) selected_entry = num_entries - 1;
                    } while (entries[selected_entry].type != ENTRY_TYPE_HOST && selected_entry != prev);
                }
                if (kDown & HidNpadButton_Down) {
                    int prev = selected_entry;
                    do {
                        selected_entry++;
                        if (selected_entry >= num_entries) selected_entry = 0;
                    } while (entries[selected_entry].type != ENTRY_TYPE_HOST && selected_entry != prev);
                }
                if (kDown & HidNpadButton_A && entries[selected_entry].type == ENTRY_TYPE_HOST) {
                    entries[selected_entry].enabled = !entries[selected_entry].enabled;
                    saved_message[0] = '\0';
                }
                if (kDown & HidNpadButton_Y) {
                    if (write_hosts_file(chosen_file, entries, num_entries)) 
                        strcpy(saved_message, "\x1b[32mSaved successfully!\x1b[0m");
                    else 
                        strcpy(saved_message, "\x1b[31mError saving!\x1b[0m");
                }
            }

            print_header(chosen_file);
            printf("D-Pad: Select | A: Toggle | Y: Save | B: Back | +: Exit\n\n");
            if (saved_message[0] != '\0') printf("%s\n", saved_message);

            int max_display = 25;
            int start_idx = selected_entry - (max_display / 2);
            if (start_idx < 0) start_idx = 0;
            if (start_idx + max_display > num_entries) start_idx = num_entries - max_display;
            if (start_idx < 0) start_idx = 0;

            for (int i = start_idx; i < num_entries && i < start_idx + max_display; i++) {
                if (entries[i].type == ENTRY_TYPE_HOST) {
                    if (i == selected_entry) printf("> [%c] %-15s %s\n", entries[i].enabled ? 'X' : ' ', entries[i].ip_address, entries[i].hostname);
                    else printf("  [%c] %-15s %s\n", entries[i].enabled ? 'X' : ' ', entries[i].ip_address, entries[i].hostname);
                } else if (entries[i].type == ENTRY_TYPE_COMMENT) {
                    printf("     \x1b[36m%.40s...\x1b[0m\n", entries[i].raw_line);
                }
            }
            consoleUpdate(NULL);
        }
        free(entries); // Clean up before going back to the file selection loop
    }

    consoleExit(NULL);
    return 0;
}
