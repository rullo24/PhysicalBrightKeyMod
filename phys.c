// std C lib includes
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// OS-specific includes
#include <windows.h>
#include <HighLevelMonitorConfigurationAPI.h>

// defining structs + macros
#define BRIGHTNESS_CHANGE_VAL 10
typedef struct {
    HMONITOR h_monitor;
    DWORD num_monitors;
    LPPHYSICAL_MONITOR p_monitors;
    HANDLE h_physical_def_monitor;
} CUSTOM_MONITOR_INFO;

// defining functions above main
int get_monitor_brightness_val_and_limits(CUSTOM_MONITOR_INFO *p_monitor_info, DWORD *p_min_brightness, DWORD *p_curr_brightness, DWORD *p_max_brightness);
void cleanup(CUSTOM_MONITOR_INFO *p_monitor_info);

// argc:1 --> return brightness || argc:2 --> second value "+ve" means increase, "-ve" means decrease
int main(int argc, char *argv[]) {
    // init basic variables
    CUSTOM_MONITOR_INFO monitor_info = {
        .h_monitor = NULL,
        .num_monitors = 0,
        .p_monitors = NULL,
        .h_physical_def_monitor = NULL,
    };

    // gracefully failing if too many arguments are parsed
    if (argc > 2) {
        fprintf(stderr, "ERROR: %s\n", "too many arguments available");
        cleanup(&monitor_info);
        return -1;
    }

    // reacting to return brightness requirement
    monitor_info.h_monitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
    if (monitor_info.h_monitor == NULL) {
        fprintf(stderr, "ERROR: %s\n", "failed to retrieve the default monitor handle");
        cleanup(&monitor_info);
        return -1;
    }

    if (GetNumberOfPhysicalMonitorsFromHMONITOR(monitor_info.h_monitor, &(monitor_info.num_monitors)) && monitor_info.num_monitors > 0) { // checking that a monitor exists (and is grabbed successfully)
        monitor_info.p_monitors = (LPPHYSICAL_MONITOR)malloc(monitor_info.num_monitors * sizeof(PHYSICAL_MONITOR));

        if (GetPhysicalMonitorsFromHMONITOR(monitor_info.h_monitor, monitor_info.num_monitors, monitor_info.p_monitors)) {
            monitor_info.h_physical_def_monitor = monitor_info.p_monitors[0].hPhysicalMonitor; // using the first monitor (default)

            // getting physical monitor brightness values + limits from Win32 API
            DWORD min_brightness, curr_brightness, max_brightness;
            int get_brightness_res = get_monitor_brightness_val_and_limits(&monitor_info, &min_brightness, &curr_brightness, &max_brightness);
            if (get_brightness_res < 0) {
                fprintf(stderr, "ERROR: %s\n", "failed to get brightness info");
                cleanup(&monitor_info);
                return -1;
            }

            if (argc == 1) { // printing current brightness to console
                printf("Min: %lu, Current: %lu, Max: %lu\n", min_brightness, curr_brightness, max_brightness);
            } else if (argc > 1) { // setting the brightness to a defined value
                // process the user input (ensure that it is a valid number)
                char *user_choice = argv[1]; // argv[1] available as through argc check
                size_t size_user_choice = strlen(user_choice); // presumed NULL-byte terminated as given by cmd

                // checking user argument validity
                if (strncmp(user_choice, "+ve", size_user_choice) == 0) { // user input equivalent to "+ve"
                    int diff_between_curr_bright_and_max = max_brightness - curr_brightness;
                    DWORD dw_pos_change = 0; // used for the change to the monitor brightness
                    if (curr_brightness < max_brightness) { // checking brightness can change
                        if (diff_between_curr_bright_and_max < BRIGHTNESS_CHANGE_VAL) { // if can't move up BRIGHTNESS_CHANGE_VAL (not enough room)
                            dw_pos_change = (DWORD)diff_between_curr_bright_and_max;
                        } else { // increasing brightness by default value (enough room)
                            dw_pos_change = (DWORD)BRIGHTNESS_CHANGE_VAL; 
                        }

                        // changing brightness by modifier --> increasing brightness
                        WINBOOL bright_set_flag = SetMonitorBrightness(monitor_info.h_monitor, dw_pos_change); 
                        if (bright_set_flag == 0) {
                            fprintf(stderr, "ERROR: %s\n", "failed to set the monitor brightness. Presumed lack of monitor drivers for this");
                            cleanup(&monitor_info);
                            return -1;
                        }
                    }

                } else if (strncmp(user_choice, "-ve", strlen("-ve")) == 0) { // user input equivalent to "-ve"
                    int diff_between_curr_bright_and_min = curr_brightness - min_brightness;
                    DWORD dw_neg_change = 0; // used for the change to the monitor brightness
                    if (curr_brightness > min_brightness) { // checking brightness can change
                        if (diff_between_curr_bright_and_min < BRIGHTNESS_CHANGE_VAL) { // if can't move up BRIGHTNESS_CHANGE_VAL (not enough room)
                            dw_neg_change = (DWORD)diff_between_curr_bright_and_min;
                        } else { // increasing brightness by default value (enough room)
                            dw_neg_change = (DWORD)BRIGHTNESS_CHANGE_VAL; 
                        }

                        // changing brightness by modifier --> decreasing brightness
                        WINBOOL bright_set_flag = SetMonitorBrightness(monitor_info.h_monitor, dw_neg_change); 
                        if (bright_set_flag == 0) {
                            fprintf(stderr, "ERROR: %s\n", "failed to set the monitor brightness. Presumed lack of monitor drivers for this");
                            cleanup(&monitor_info);
                            return -1;
                        }
                    } else {
                        fprintf(stderr, "ERROR: %s\n", "invalid user argument");
                        cleanup(&monitor_info);
                        return -1;
                    }
                }
            } else { // no physical monitor handles
                fprintf(stderr, "ERROR: %s\n", "failed to get physical monitor handles");
                cleanup(&monitor_info);
                return -1;
            }
        }
    } else {
        fprintf(stderr, "ERROR: %s\n", "no physical monitors found");
        cleanup(&monitor_info);
        return -1;
    }

    return 0;
}

int get_monitor_brightness_val_and_limits(CUSTOM_MONITOR_INFO *p_monitor_info, DWORD *p_min_brightness, DWORD *p_curr_brightness, DWORD *p_max_brightness) {
    // attempting to gather the physical monitors limitations and max + min vals
    if (GetMonitorBrightness(p_monitor_info->h_physical_def_monitor, p_min_brightness, p_curr_brightness, p_max_brightness)) {
        return 0;
    } 
    return -1;
}

void cleanup(CUSTOM_MONITOR_INFO *p_monitor_info) {
    // cleaning self-called malloc
    if (p_monitor_info->p_monitors != NULL) {
        free(p_monitor_info->p_monitors);
        p_monitor_info->p_monitors = NULL;
    }   

    // cleaning physical monitor alloc'd mem
    if (p_monitor_info->h_physical_def_monitor != NULL) {
        DestroyPhysicalMonitors(p_monitor_info->num_monitors, p_monitor_info->p_monitors); // Release the physical monitor handle
        p_monitor_info->p_monitors = NULL;
    }
}