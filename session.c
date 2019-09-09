#include "zwm.h"

/**
 * Allocate the space and join two strings.
 */
static inline char*
mstrjoin(const char* src1, const char* src2)
{
	char* str = calloc(strlen(src1) + strlen(src2) + 1, sizeof(char));

	strcpy(str, src1);
	strcat(str, src2);

	return str;
}

/**
 * Allocate the space and join two strings;
 */
static inline char*
mstrjoin3(const char* src1, const char* src2, const char* src3)
{
	char* str = calloc(strlen(src1) + strlen(src2)
		+ strlen(src3) + 1,
	    sizeof(char));

	strcpy(str, src1);
	strcat(str, src2);
	strcat(str, src3);

	return str;
}

static FILE*
open_config_file(const char* config_filename, const char* mode, char* cpath, char** ppath)
{
	const static char* config_home_suffix = "/.config";
	const static char* config_system_dir = "/etc/xdg";

	char *dir = NULL, *home = NULL;
	char* path = cpath;
	FILE* f = NULL;

	if (path) {
		f = fopen(path, mode);
		if (f && ppath)
			*ppath = path;
		return f;
	}

	// Check user configuration file in $XDG_CONFIG_HOME firstly
	if (!((dir = getenv("XDG_CONFIG_HOME")) && strlen(dir))) {
		if (!((home = getenv("HOME")) && strlen(home)))
			return NULL;

		path = mstrjoin3(home, config_home_suffix, config_filename);
	} else
		path = mstrjoin(dir, config_filename);

	f = fopen(path, mode);

	if (f && ppath)
		*ppath = path;
	else
		free(path);
	if (f)
		return f;

	// Check system configuration file in $XDG_CONFIG_DIRS at last
	if ((dir = getenv("XDG_CONFIG_DIRS")) && strlen(dir)) {
		char* part = strtok(dir, ":");
		while (part) {
			path = mstrjoin(part, config_filename);
			f = fopen(path, mode);
			if (f && ppath)
				*ppath = path;
			else
				free(path);
			if (f)
				return f;
			part = strtok(NULL, ":");
		}
	} else {
		path = mstrjoin(config_system_dir, config_filename);
		f = fopen(path, mode);
		if (f && ppath)
			*ppath = path;
		else
			free(path);
		if (f)
			return f;
	}

	return NULL;
}

void zwm_session_save(void)
{
	if (session_dirty <= 0)
		return;
	char* path = NULL;
	FILE* sf = open_config_file("/zwm/session.json", "w+", NULL, &path);
	if (path) {
		//printf(":SAVED %s\n", path);
		Client* c;
		if (sf) {
			zwm_client_foreach(c)
			{
				fprintf(sf, "%s\n", c->cmd);
			}
			fclose(sf);
		}
		session_dirty = 0;
		free(path);
	}
}

void zwm_session_restore(void)
{
	char* fpath;
	char cmds[256][256];
	FILE* sf = open_config_file("/zwm/session.json", "r", NULL, &fpath);
	//printf(":LOAD %s\n", fpath);
	int i = 0, j = 0;
	if (sf) {
		while (fgets(cmds[i], 256, sf)) {
			printf("cmd %d: %s", i, cmds[i]);
			i++;
		}
		fclose(sf);
		free(fpath);

		for (j = 0; j < i; j++) {
			zwm_util_spawn(cmds[j]);
		}
	}
}
