#include "fileSystem.h"

int init_fs()
{
	if (!init_space())
	{
		return 0;
	}
	if (!create_root())
	{
		return 0;
	}
	else
	{
		open_root();
	}
	return 1;
}

void fs_start()
{ //启动文件系统
	if (!init_fs())
	{
		printf("FileSystem init error.\n");
		exit(0);
	}
	else
	{
		printf("FileSystem started...\n");
		printf("================================\n");
	}
}

void interface()
{
	char s[10240] = {0};
	while (1)
	{
		printCurrentDirectory();
		gets(s);
		//按照空格拆分命令
		if (strlen(s) < 1)
		{
			continue;
		}
		char *delim = " ";
		char *p = strtok(s, delim);
		if (currentFile < 0)
		{
			if (strcmp(p, "ls") == 0)
			{
				list_directory();
			}
			else if (strcmp(p, "mkdir") == 0)
			{
				p = strtok(NULL, delim);
				if (p != NULL && strlen(p) > 0)
				{
					if (strcmp(p, ".") != 0 && strcmp(p, "..") != 0)
					{ //不允许以.，..作为文件夹名
						if (!make_directory(p))
						{
							printf("An error occured when creating this directory.\n");
						}
					}
				}
			}
			else if (strcmp(p, "cd") == 0)
			{
				p = strtok(NULL, delim);
				if (p != NULL && strlen(p) > 0)
				{
					if (strcmp(p, "..") == 0)
					{
						if (!back_parentDirectory())
						{
							printf("Error.\n");
						}
					}
					else if (strcmp(p, ".") == 0)
					{
						continue;
					}
					else if (!enter_directory(p))
					{
						printf("Directory not found.\n");
					}
				}
			}
			else if (strcmp(p, "rmdir") == 0)
			{
				p = strtok(NULL, delim);
				if (p != NULL && strlen(p) > 0)
				{
					if (!delete_directory(p))
					{
						printf("Error.\n");
					}
				}
			}
			else if (strcmp(p, "create") == 0)
			{
				p = strtok(NULL, delim);
				if (p != NULL && strlen(p) > 0)
				{
					if (!create_file(p))
					{
						printf("Error.\n");
					}
				}
			}
			else if (strcmp(p, "rm") == 0)
			{
				p = strtok(NULL, delim);
				if (p != NULL && strlen(p) > 0)
				{
					if (!remove_file(p))
					{
						printf("Error.\n");
					}
				}
			}
			else if (strcmp(p, "open") == 0)
			{
				p = strtok(NULL, delim);
				if (p != NULL && strlen(p) > 0)
				{
					if (!open_file(p))
					{
						printf("Error.\n");
					}
				}
			}
			else if (strcmp(p, "cat") == 0)
			{
				p = strtok(NULL, delim);
				if (p != NULL && strlen(p) > 0)
				{
					cat_file(p);
				}
			}
		}
		else
		{
			if (strcmp(p, "close") == 0)
			{
				close_file();
			}
			else if (strcmp(p, "read") == 0)
			{
				p = strtok(NULL, delim);
				if (p != NULL && strlen(p) > 0)
				{
					int l = atoi(p);
					read_file(currentFileIndex, l);
				} else {
					read_file(currentFileIndex, 0);
				}
			}
			else if (strcmp(p, "write") == 0)
			{
				write_file(currentFile, s + 6);
			}
		}
	}
}

int main()
{
	fs_start();
	interface();
}
