/*
 * libdpkg - Debian packaging suite library routines
 * command.c - command execution support
 *
 * Copyright © 2010-2012 Guillem Jover <guillem@debian.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <compat.h>

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include <dpkg/dpkg.h>
#include <dpkg/i18n.h>
#include <dpkg/string.h>
#include <dpkg/path.h>
#include <dpkg/command.h>

//posix spawn
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
extern char **environ;

/**
 * Initialize a command structure.
 *
 * If name is NULL, then the last component of the filename path will be
 * used to initialize the name member.
 *
 * @param cmd The command structure to initialize.
 * @param filename The filename of the command to execute.
 * @param name The description of the command to execute.
 */
void
command_init(struct command *cmd, const char *filename, const char *name)
{
	cmd->filename = filename;
	if (name == NULL)
		cmd->name = path_basename(filename);
	else
		cmd->name = name;
	cmd->argc = 0;
	cmd->argv_size = 10;
	cmd->argv = m_malloc(cmd->argv_size * sizeof(const char *));
	cmd->argv[0] = NULL;
}

/**
 * Destroy a command structure.
 *
 * Free the members managed by the command functions (i.e. the argv pointer
 * array), and zero all members of a command structure.
 *
 * @param cmd The command structure to free.
 */
void
command_destroy(struct command *cmd)
{
	cmd->filename = NULL;
	cmd->name = NULL;
	cmd->argc = 0;
	cmd->argv_size = 0;
	free(cmd->argv);
	cmd->argv = NULL;
}

static void
command_grow_argv(struct command *cmd, int need)
{
	/* We need a ghost byte for the NUL character. */
	need++;

	/* Check if we already have enough room. */
	if ((cmd->argv_size - cmd->argc) >= need)
		return;

	cmd->argv_size = (cmd->argv_size + need) * 2;
	cmd->argv = m_realloc(cmd->argv, cmd->argv_size * sizeof(const char *));
}

/**
 * Append an argument to the command's argv.
 *
 * @param cmd The command structure to act on.
 * @param arg The argument to append to argv.
 */
void
command_add_arg(struct command *cmd, const char *arg)
{
	command_grow_argv(cmd, 1);

	cmd->argv[cmd->argc++] = arg;
	cmd->argv[cmd->argc] = NULL;
}

/**
 * Append an argument array to the command's argv.
 *
 * @param cmd The command structure to act on.
 * @param argv The NULL terminated argument array to append to argv.
 */
void
command_add_argl(struct command *cmd, const char **argv)
{
	int i, add_argc = 0;

	while (argv[add_argc] != NULL)
		add_argc++;

	command_grow_argv(cmd, add_argc);

	for (i = 0; i < add_argc; i++)
		cmd->argv[cmd->argc++] = argv[i];

	cmd->argv[cmd->argc] = NULL;
}

/**
 * Append a va_list of argument to the command's argv.
 *
 * @param cmd The command structure to act on.
 * @param args The NULL terminated va_list of argument array to append to argv.
 */
void
command_add_argv(struct command *cmd, va_list args)
{
	va_list args_copy;
	int i, add_argc = 0;

	va_copy(args_copy, args);
	while (va_arg(args_copy, const char *) != NULL)
		add_argc++;
	va_end(args_copy);

	command_grow_argv(cmd, add_argc);

	for (i = 0; i < add_argc; i++)
		cmd->argv[cmd->argc++] = va_arg(args, const char *);

	cmd->argv[cmd->argc] = NULL;
}

/**
 * Append a variable list of argument to the command's argv.
 *
 * @param cmd The command structure to act on.
 * @param ... The NULL terminated variable list of argument to append to argv.
 */
void
command_add_args(struct command *cmd, ...)
{
	va_list args;

	va_start(args, cmd);
	command_add_argv(cmd, args);
	va_end(args);
}

/**
 * Execute the command specified.
 *
 * The command is executed searching the PATH if the filename does not
 * contain any slashes, or using the full path if it's either a relative or
 * absolute pathname. This functions does not return.
 *
 * @param cmd The command structure to act on.
 */
void
command_exec(struct command *cmd)
{
	execvp(cmd->filename, (char * const *)cmd->argv);
	ohshite(_("unable to execute %s (%s)"), cmd->name, cmd->filename);
}

/**
 * Execute a shell with a possible command.
 *
 * @param cmd The command string to execute, if it's NULL an interactive
 *            shell will be executed instead.
 * @param name The description of the command to execute.
 */
void
command_shell(const char *cmd, const char *name)
{
	const char *shell;
	const char *mode;

	if (cmd == NULL) {
		mode = "-i";
		shell = getenv("SHELL");
	} else {
		mode = "-c";
		shell = NULL;
	}

	if (str_is_unset(shell))
		shell = DEFAULTSHELL;

	execlp(shell, shell, mode, cmd, NULL);
	ohshite(_("unable to execute %s (%s)"), name, cmd);
}

#define PROC_PIDPATHINFO_MAXSIZE  (1024)
static int file_exist(const char *filename) {
    struct stat buffer;
    int r = stat(filename, &buffer);
    return (r == 0);
}

static char *searchpath(const char *binaryname){
    if (strstr(binaryname, "/") != NULL){
        if (file_exist(binaryname)){
            char *foundpath = (char *)malloc((strlen(binaryname) + 1) * (sizeof(char)));
            strcpy(foundpath, binaryname);
            return foundpath;
        } else {
       return NULL;
   }
    }
    
    char *pathvar = getenv("PATH");
    
    char *dir = strtok(pathvar,":");
    while (dir != NULL){
        char searchpth[PROC_PIDPATHINFO_MAXSIZE];
        strcpy(searchpth, dir);
        strcat(searchpth, "/");
        strcat(searchpth, binaryname);
        
        if (file_exist(searchpth)){
            char *foundpath = (char *)malloc((strlen(searchpth) + 1) * (sizeof(char)));
            strcpy(foundpath, searchpth);
            return foundpath;
        }
        
        dir = strtok(NULL, ":");
    }
    return NULL;
}

static bool isShellScript(const char *path){
    FILE *file = fopen(path, "r");
    uint8_t header[2];
    if (fread(header, sizeof(uint8_t), 2, file) == 2){
        if (header[0] == '#' && header[1] == '!'){
            fclose(file);
            return true;
        }
    }
    fclose(file);
    return false;
}

static char *getInterpreter(char *path){
    FILE *file = fopen(path, "r");
    char *interpreterLine = NULL;
    unsigned long lineSize = 0;
    getline(&interpreterLine, &lineSize, file);
    
    char *rawInterpreter = (interpreterLine+2);
    rawInterpreter = strtok(rawInterpreter, " ");
    rawInterpreter = strtok(rawInterpreter, "\n");
    
    char *interpreter = (char *)malloc((strlen(rawInterpreter)+1) * sizeof(char));
    strcpy(interpreter, rawInterpreter);
    
    free(interpreterLine);
    fclose(file);
    return interpreter;
}

static char *fixedCmd(const char *cmdStr){
    char *cmdCpy = (char *)malloc((strlen(cmdStr)+1) * sizeof(char));
    strcpy(cmdCpy, cmdStr);
    
    char *cmd = strtok(cmdCpy, " ");
    
    uint8_t size = strlen(cmd) + 1;
    
    char *args = cmdCpy + size;
    if ((strlen(cmdStr) - strlen(cmd)) == 0)
        args = NULL;
    
    char *abs_path = searchpath(cmd);
    if (abs_path){
        bool isScript = isShellScript(abs_path);
        if (isScript){
            char *interpreter = getInterpreter(abs_path);
            
            uint8_t commandSize = strlen(interpreter) + 1 + strlen(abs_path);
            
            if (args){
                commandSize += 1 + strlen(args);
            }
            
            char *rawCommand = (char *)malloc(sizeof(char) * (commandSize + 1));
            strcpy(rawCommand, interpreter);
            strcat(rawCommand, " ");
            strcat(rawCommand, abs_path);
            
            if (args){
                strcat(rawCommand, " ");
                strcat(rawCommand, args);
            }
       rawCommand[(commandSize)+1] = '\0';
            
            free(interpreter);
            free(abs_path);
            free(cmdCpy);
            
            return rawCommand;
        } else {
            uint8_t commandSize = strlen(abs_path);
            
            if (args){
                commandSize += 1 + strlen(args);
            }
            
            char *rawCommand = (char *)malloc(sizeof(char) * (commandSize + 1));
            strcat(rawCommand, abs_path);
            
            if (args){
                strcat(rawCommand, " ");
                strcat(rawCommand, args);
            }
       rawCommand[(commandSize)+1] = '\0';
            
            free(abs_path);
            free(cmdCpy);
            
            return rawCommand;
        }
    }
    return cmdCpy;
}

void
runcmd(struct command *cmd)
{
	int i = 0;
	char cmdstring[200];
	char *ptr = cmdstring; // set ptr to the start of the destination buffer
	for (i=0; i<cmd->argc; i++) {
		const char *current_arg = cmd->argv[i];
		char c;
		while ( (c = *current_arg++) ) {
			// copy each character to the destination buffer until the end of the current string
			*ptr++ = c;
		}
		*ptr++ = ' '; // or whatever joining character you want
	}
	*ptr = '\0'; // null terminate
	char *fullcmd = fixedCmd(cmdstring);
	command_shell(fullcmd, cmd->name);
	ohshite(_("unable to execute %s (%s)"), cmd->name, cmd->filename);
}