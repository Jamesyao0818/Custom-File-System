#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mfs.h"

// ========================================================================
// FORWARD DECLARATIONS (internal functions)
// ========================================================================
static void cmd_pwd(void);
static void cmd_cd(const char *path);
static void cmd_ls(const char *path);
static void cmd_ls_long(const char *path);
static void cmd_mkdir(const char *path);
static void cmd_rmdir(const char *path);
static void cmd_rm(const char *filename);
static void cmd_stat(const char *path);
static void cmd_isfile(const char *path);
static void cmd_isdir(const char *path);
static void cmd_tree(const char *path);
static void cmd_tree_recursive(const char *path, int depth);
static void cmd_help(void);

// ========================================================================
// PUBLIC FUNCTIONS (you can call these from main)
// ========================================================================

/**
 * shell_run - Interactive shell
 * Call this from your main() to start the shell
 */
void shell_run(void) {
    char input[512];
    char command[64];
    char arg[448];
    
    printf("\n╔═══════════════════════════════════════════════════╗\n");
    printf("║     Simple File System Shell                  ║\n");
    printf("║     Type 'help' for available commands        ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    
    while (1) {
        // Show prompt with current directory
        char cwd[256];
        if (fs_getcwd(cwd, sizeof(cwd))) {
            printf("%s> ", cwd);
        } else {
            printf("> ");
        }
        
        // Get input
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        
        // Skip empty input
        if (strlen(input) == 0) {
            continue;
        }
        
        // Parse command and argument
        command[0] = '\0';
        arg[0] = '\0';
        sscanf(input, "%63s %447s", command, arg);
        
        // Execute commands
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            printf("Goodbye!\n");
            break;
        }
        else if (strcmp(command, "help") == 0) {
            cmd_help();
        }
        else if (strcmp(command, "pwd") == 0) {
            cmd_pwd();
        }
        else if (strcmp(command, "cd") == 0) {
            cmd_cd(arg[0] ? arg : NULL);
        }
        else if (strcmp(command, "ls") == 0) {
            // Check for -l flag
            if (strcmp(arg, "-l") == 0) {
                cmd_ls_long(".");
            } else {
                cmd_ls(arg[0] ? arg : NULL);
            }
        }
        else if (strcmp(command, "mkdir") == 0) {
            cmd_mkdir(arg[0] ? arg : NULL);
        }
        else if (strcmp(command, "rmdir") == 0) {
            cmd_rmdir(arg[0] ? arg : NULL);
        }
        else if (strcmp(command, "rm") == 0) {
            cmd_rm(arg[0] ? arg : NULL);
        }
        else if (strcmp(command, "stat") == 0) {
            cmd_stat(arg[0] ? arg : NULL);
        }
        else if (strcmp(command, "isfile") == 0) {
            cmd_isfile(arg[0] ? arg : NULL);
        }
        else if (strcmp(command, "isdir") == 0) {
            cmd_isdir(arg[0] ? arg : NULL);
        }
        else if (strcmp(command, "tree") == 0) {
            cmd_tree(arg[0] ? arg : NULL);
        }
        else {
            printf("%s: command not found\n", command);
            printf("Type 'help' for available commands\n");
        }
    }
}

/**
 * execute_command - Execute a single command (for testing)
 * Call this from your code to run commands programmatically
 * Example: execute_command("pwd");
 */
int execute_command(const char *cmdline) {
    char input[512];
    char command[64];
    char arg[448];
    
    if (!cmdline) return -1;
    
    strncpy(input, cmdline, sizeof(input) - 1);
    input[sizeof(input) - 1] = '\0';
    
    // Parse command and argument
    command[0] = '\0';
    arg[0] = '\0';
    sscanf(input, "%63s %447s", command, arg);
    
    // Execute commands
    if (strcmp(command, "pwd") == 0) {
        cmd_pwd();
    }
    else if (strcmp(command, "cd") == 0) {
        cmd_cd(arg[0] ? arg : NULL);
    }
    else if (strcmp(command, "ls") == 0) {
        if (strcmp(arg, "-l") == 0) {
            cmd_ls_long(".");
        } else {
            cmd_ls(arg[0] ? arg : NULL);
        }
    }
    else if (strcmp(command, "mkdir") == 0) {
        cmd_mkdir(arg[0] ? arg : NULL);
    }
    else if (strcmp(command, "rmdir") == 0) {
        cmd_rmdir(arg[0] ? arg : NULL);
    }
    else if (strcmp(command, "rm") == 0) {
        cmd_rm(arg[0] ? arg : NULL);
    }
    else if (strcmp(command, "stat") == 0) {
        cmd_stat(arg[0] ? arg : NULL);
    }
    else if (strcmp(command, "tree") == 0) {
        cmd_tree(arg[0] ? arg : NULL);
    }
    else {
        printf("%s: command not found\n", command);
        return -1;
    }
    
    return 0;
}

// ========================================================================
// COMMAND IMPLEMENTATIONS (all static - internal only)
// ========================================================================

/**
 * pwd - Print working directory
 */
static void cmd_pwd(void) {
    char cwd[256];
    
    if (fs_getcwd(cwd, sizeof(cwd))) {
        printf("%s\n", cwd);
    } else {
        printf("pwd: error getting current directory\n");
    }
}

/**
 * cd - Change directory
 */
static void cmd_cd(const char *path) {
    if (!path || strlen(path) == 0) {
        // No argument - go to root
        path = "/";
    }
    
    if (fs_setcwd((char*)path) == 0) {
        // Success
        char cwd[256];
        if (fs_getcwd(cwd, sizeof(cwd))) {
            printf("Changed to: %s\n", cwd);
        }
    } else {
        printf("cd: %s: No such directory\n", path);
    }
}

/**
 * ls - List directory contents
 */
static void cmd_ls(const char *path) {
    const char *targetPath = path ? path : ".";
    
    fdDir *dir = fs_opendir(targetPath);
    if (!dir) {
        printf("ls: cannot access '%s': No such file or directory\n", targetPath);
        return;
    }
    
    struct fs_diriteminfo *entry;
    while ((entry = fs_readdir(dir)) != NULL) {
        // Skip . and .. for cleaner output (optional)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        printf("%s\n", entry->d_name);
    }
    
    fs_closedir(dir);
}

/**
 * ls -l - List directory contents with details
 */
static void cmd_ls_long(const char *path) {
    const char *targetPath = path ? path : ".";
    
    fdDir *dir = fs_opendir(targetPath);
    if (!dir) {
        printf("ls: cannot access '%s': No such file or directory\n", targetPath);
        return;
    }
    
    printf("%-10s %10s %s\n", "Type", "Size", "Name");
    printf("%-10s %10s %s\n", "----------", "----------", "----");
    
    struct fs_diriteminfo *entry;
    while ((entry = fs_readdir(dir)) != NULL) {
        // Build full path to get stats
        char fullpath[512];
        if (strcmp(targetPath, "/") == 0) {
            snprintf(fullpath, sizeof(fullpath), "/%s", entry->d_name);
        } else if (strcmp(targetPath, ".") == 0) {
            snprintf(fullpath, sizeof(fullpath), "%s", entry->d_name);
        } else {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", targetPath, entry->d_name);
        }
        
        struct fs_stat st;
        if (fs_stat(fullpath, &st) == 0) {
            const char *type = (entry->fileType == FT_DIRECTORY) ? "DIR" : "FILE";
            printf("%-10s %10ld %s\n", type, (long)st.st_size, entry->d_name);
        } else {
            printf("%-10s %10s %s\n", "?", "?", entry->d_name);
        }
    }
    
    fs_closedir(dir);
}

/**
 * mkdir - Create directory
 */
static void cmd_mkdir(const char *path) {
    if (!path || strlen(path) == 0) {
        printf("mkdir: missing operand\n");
        printf("Usage: mkdir <directory>\n");
        return;
    }
    
    if (fs_mkdir(path, 0755) == 0) {
        printf("Directory '%s' created\n", path);
    } else {
        printf("mkdir: cannot create directory '%s'\n", path);
    }
}

/**
 * rmdir - Remove empty directory
 */
static void cmd_rmdir(const char *path) {
    if (!path || strlen(path) == 0) {
        printf("rmdir: missing operand\n");
        printf("Usage: rmdir <directory>\n");
        return;
    }
    
    if (fs_rmdir(path) == 0) {
        printf("Directory '%s' removed\n", path);
    } else {
        printf("rmdir: failed to remove '%s'\n", path);
    }
}

/**
 * rm - Remove file
 */
static void cmd_rm(const char *filename) {
    if (!filename || strlen(filename) == 0) {
        printf("rm: missing operand\n");
        printf("Usage: rm <file>\n");
        return;
    }
    
    if (fs_delete((char*)filename) == 0) {
        printf("File '%s' removed\n", filename);
    } else {
        printf("rm: cannot remove '%s'\n", filename);
    }
}

/**
 * stat - Display file/directory status
 */
static void cmd_stat(const char *path) {
    if (!path || strlen(path) == 0) {
        printf("stat: missing operand\n");
        printf("Usage: stat <path>\n");
        return;
    }
    
    struct fs_stat st;
    if (fs_stat(path, &st) != 0) {
        printf("stat: cannot stat '%s': No such file or directory\n", path);
        return;
    }
    
    printf("\n");
    printf("  File: %s\n", path);
    printf("  ────────────────────────────────────────\n");
    printf("  Size:         %ld bytes\n", (long)st.st_size);
    printf("  Block size:   %ld bytes\n", (long)st.st_blksize);
    printf("  Blocks:       %ld (512B blocks)\n", (long)st.st_blocks);
    printf("  ────────────────────────────────────────\n");
    printf("  Created:      %s", ctime(&st.st_createtime));
    printf("  Modified:     %s", ctime(&st.st_modtime));
    printf("  Accessed:     %s", ctime(&st.st_accesstime));
    printf("\n");
}

/**
 * isfile - Check if path is a file
 */
static void cmd_isfile(const char *path) {
    if (!path || strlen(path) == 0) {
        printf("isfile: missing operand\n");
        printf("Usage: isfile <path>\n");
        return;
    }
    
    int result = fs_isFile((char*)path);
    if (result == 1) {
        printf("'%s' is a file\n", path);
    } else if (result == 0) {
        printf("'%s' is not a file\n", path);
    } else {
        printf("'%s' does not exist\n", path);
    }
}

/**
 * isdir - Check if path is a directory
 */
static void cmd_isdir(const char *path) {
    if (!path || strlen(path) == 0) {
        printf("isdir: missing operand\n");
        printf("Usage: isdir <path>\n");
        return;
    }
    
    int result = fs_isDir((char*)path);
    if (result == 1) {
        printf("'%s' is a directory\n", path);
    } else if (result == 0) {
        printf("'%s' is not a directory\n", path);
    } else {
        printf("'%s' does not exist\n", path);
    }
}

/**
 * tree - Display directory tree (recursive helper)
 */
static void cmd_tree_recursive(const char *path, int depth) {
    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    // Get just the directory name
    const char *dirname = strrchr(path, '/');
    dirname = dirname ? dirname + 1 : path;
    
    printf("├── %s/\n", dirname);
    
    fdDir *dir = fs_opendir(path);
    if (!dir) {
        return;
    }
    
    struct fs_diriteminfo *entry;
    while ((entry = fs_readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Build full path
        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        
        // Print indentation
        for (int i = 0; i <= depth; i++) {
            printf("  ");
        }
        
        if (entry->fileType == FT_DIRECTORY) {
            printf("├── %s/\n", entry->d_name);
            // Recursively show subdirectory (limit depth to prevent infinite recursion)
            if (depth < 5) {
                cmd_tree_recursive(fullpath, depth + 1);
            }
        } else {
            printf("├── %s\n", entry->d_name);
        }
    }
    
    fs_closedir(dir);
}

/**
 * tree - Display directory tree
 */
static void cmd_tree(const char *path) {
    const char *targetPath = path ? path : ".";
    
    printf("\n%s\n", targetPath);
    
    fdDir *dir = fs_opendir(targetPath);
    if (!dir) {
        printf("tree: cannot access '%s'\n", targetPath);
        return;
    }
    
    struct fs_diriteminfo *entry;
    while ((entry = fs_readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Build full path
        char fullpath[512];
        if (strcmp(targetPath, ".") == 0) {
            snprintf(fullpath, sizeof(fullpath), "%s", entry->d_name);
        } else {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", targetPath, entry->d_name);
        }
        
        if (entry->fileType == FT_DIRECTORY) {
            printf("├── %s/\n", entry->d_name);
            if (fs_opendir(fullpath)) {
                cmd_tree_recursive(fullpath, 1);
            }
        } else {
            printf("├── %s\n", entry->d_name);
        }
    }
    
    fs_closedir(dir);
    printf("\n");
}

/**
 * help - Display available commands
 */
static void cmd_help(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║             Available Commands                            ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  pwd              Print working directory                 ║\n");
    printf("║  cd <dir>         Change directory                        ║\n");
    printf("║  ls [dir]         List directory contents                 ║\n");
    printf("║  ls -l [dir]      List with details (long format)         ║\n");
    printf("║  mkdir <dir>      Create directory                        ║\n");
    printf("║  rmdir <dir>      Remove empty directory                  ║\n");
    printf("║  rm <file>        Remove file                             ║\n");
    printf("║  stat <path>      Display file/directory status           ║\n");
    printf("║  isfile <path>    Check if path is a file                 ║\n");
    printf("║  isdir <path>     Check if path is a directory            ║\n");
    printf("║  tree [dir]       Display directory tree                  ║\n");
    printf("║  help             Show this help message                  ║\n");
    printf("║  exit             Exit shell                              ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");
}