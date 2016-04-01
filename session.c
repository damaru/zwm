#include "zwm.h"

void
zwm_session_save(void){
	char path[256];
	if(!head)return;
	sprintf(path, "%s/.zwm_session",getenv("HOME"));
	FILE *sf = fopen(path, "w+");
	Client *c;
	if(sf){	
		zwm_client_foreach(c) {
			fprintf(sf, "%s\n", c->cmd);
//			printf("%d %s\n",c->view, c->cmd);
		}

		fclose(sf);
	}
	session_dirty = 0;
}

void
zwm_session_restore(void){
	char path[256];
	sprintf(path, "%s/.zwm_session",getenv("HOME"));
	FILE *sf = fopen(path, "r");
	if(sf) {
		while( fgets(path, 256, sf) ) {
			printf("RESTORE: %s\n",path);
			zwm_util_spawn(path);
		}
		fclose(sf);
	}
}

