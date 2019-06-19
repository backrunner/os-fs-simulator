#ifndef __file_system__
#define __file_system__

#define BLOCKSIZE 512 //单块大小512B
#define SIZE 4096000  //分配一共4MB的文件存储空间
//一共8000块
#define MAXOPEN_USER 50   //用户最多能同时打开的文件数量
#define MAXOPEN_SYSTEM 50 //系统最多能同时打开的文件数量
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

//sizeof(FCB)=40, sizeof(INode)=40, total_size = 80, 80*256=20480=40*BLOCKSIZE
//BLOCK 1-40: FCB*224+INode*284
typedef struct FCB
{
	char filename[32];
	unsigned short inodeIndex; //inode号（文件号）
} FCB;

typedef struct INode
{
	unsigned char attribute; //0: directory 1:file
	unsigned char status;	//状态 0：未占用 1：已占用
	unsigned int length;	 //文件长度
	int directPointer[8];	//直接指针8个
	int indirectPointer;	 //1级间接指针1个
	int subIndirectPointer;  //2级间接指针1个
} INode;

//sizeof(Directory) = BLOCKSIZE
typedef struct Directory
{
	unsigned int parentINodeIndex; //父级目录的inode号
	int files[127];				   //其他文件的inode号
} Directory;

//sizeof(INodeBlock) = BLOCKSIZE
typedef struct INodeBlock
{
	int pointers[128]; //间接索引
} INodeBlock;

typedef struct USEROPEN
{								   //用户打开表
	unsigned char openMode;		   //打开方式	0:r 1:w 2:rw
	unsigned char systemOpenEntry; //系统文件打开表入口
	unsigned char status;		   //状态
	unsigned short parentIndex;	//父项在用户打开表的位置
} USEROPEN;

typedef struct SYSTEMOPEN
{ //系统打开表
	char filename[32];
	unsigned char attribute;   //0: directory 1:file
	unsigned int length;	   //文件长度
	unsigned short inodeIndex; //inode号
	unsigned char status;	  //FCB的状态
	unsigned int shareCount;   //共享打开数目

	unsigned short parentIndex; //父文件目录在打开表的位置
	unsigned short parentINode; //父文件目录的INode ID
} SYSTEMOPEN;

typedef struct blockBit
{
	int val : 1;
} blockBit;

//==== 指针变量 ====
void *FILEBLOCK_START; //文件空间起始位置

//==== 常数变量 ====
int totalFCB = 256;
int usedFCB = 0;
int freeFCB = 256;
int totalBlock = 8000;
int usedBlock = 0;
int freeBlock = 8000;

int currentDirectory = -1;		//当前目录的文件号
int currentDirectoryIndex = -1; //当前目录在用户文件打开表的序号
int currentFile = -1;			//当前打开的文件
int currentFileIndex = -1;		//当前打开的文件在目录打开表的序号
int currentFileUsedBlock = -1;

struct FCB fcb[256];
struct INode inode[256];
struct USEROPEN useropen[MAXOPEN_USER];
struct SYSTEMOPEN systemopen[MAXOPEN_SYSTEM];
struct blockBit blockMap[8000];

//==== 方法 ====
int init_space()
{ //初始化文件空间
	FILEBLOCK_START = malloc(SIZE);
	memset(FILEBLOCK_START, 0, SIZE);
	if (FILEBLOCK_START != NULL)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//更新位示图
void update_blockMap(int blockNum, int newValue)
{
	blockMap[blockNum].val = newValue;
}

int get_blockMapValue(int blockNum)
{
	return blockMap[blockNum].val;
}

int block_free(int blockNum)
{
	if (get_blockMapValue(blockNum))
	{
		//块写入指针偏移 = FILEBLOCK_START + 偏移量 (块大小*(块数))
		memset(FILEBLOCK_START + blockNum * BLOCKSIZE, 0, BLOCKSIZE);
		//更新位示图
		update_blockMap(blockNum, 0);
		return 1;
	}
	else
	{
		return 0;
	}
}

int block_write(int blockNum, const void *data_start, int data_length)
{
	if (data_length > 512)
	{
		return 0;
	}
	if (!get_blockMapValue(blockNum))
	{
		//块写入指针偏移 = FILEBLOCK_START + 偏移量 (块大小*(块数))
		memcpy(FILEBLOCK_START + BLOCKSIZE * blockNum, data_start, data_length);
		update_blockMap(blockNum, 1);
		return 1;
	}
	else
	{
		return 0;
	}
}

void block_read(int blockNum)
{
	if (get_blockMapValue(blockNum))
	{
		//块读取指针偏移 = FILEBLOCK_START + 偏移量 (块大小*(块数))
		char *_start = (char *)(FILEBLOCK_START + blockNum * BLOCKSIZE);
		for (int i = 0; i < 256; i++)
		{
			if (*_start != 0){
				printf("%c", *_start);
			}
			_start++;
		}
	}
	else
	{
		printf("Block read error.");
	}
}

int getEmptyBlock()
{
	for (int i = 1; i <= 8000; i++)
	{
		if (!get_blockMapValue(i))
		{
			return i;
		}
	}
	return -1;
}

int getEmptyINode()
{
	for (int i = 0; i < 256; i++)
	{
		if (!inode[i].status)
		{
			return i;
		}
	}
	return -1;
}

int getEmptyUseropen()
{
	for (int i = 0; i < 50; i++)
	{
		if (!useropen[i].status)
		{
			return i;
		}
	}
	return -1;
}

int getEmptySystemopen()
{
	for (int i = 0; i < 50; i++)
	{
		if (!systemopen[i].status)
		{
			return i;
		}
	}
	return -1;
}

struct Directory *getCurrentDirectory()
{
	unsigned int b = inode[currentDirectory].directPointer[0]; //获取到当前目录所在的块号
	struct Directory *cd = (struct Directory *)(FILEBLOCK_START + BLOCKSIZE * b);
	return cd;
}

//根目录
int create_root()
{
	//根目录放置于FCB[0]、INode[0]
	inode[0].directPointer[0] = 0; //块1
	inode[0].indirectPointer = -1;
	inode[0].subIndirectPointer = -1;
	inode[0].attribute = 0;		  //文件夹
	inode[0].status = 1;		  //标记占用
	strcpy(fcb[0].filename, "/"); //文件名
	fcb[0].inodeIndex = 0;		  //文件号
	//写入根目录文件到第一块
	struct Directory *directory = (struct Directory *)malloc(sizeof(Directory));
	directory->parentINodeIndex = 0;
	memset(directory->files, -1, sizeof(directory->files));
	return block_write(0, directory, sizeof(Directory));
}

//默认启动后打开根目录
void open_root()
{
	systemopen[0].attribute = 0;
	strcpy(systemopen[0].filename, fcb[0].filename);
	systemopen[0].inodeIndex = 0;
	systemopen[0].shareCount = 1;
	systemopen[0].status = 1;
	useropen[0].status = 1;
	useropen[0].openMode = 2;
	useropen[0].systemOpenEntry = 0;
	currentDirectory = 0;
	currentDirectoryIndex = 0;
}

//打印当前目录
void printCurrentDirectory()
{
	char buffer[512] = {0};
	char buffer_final[512] = {0};
	char s[512] = {0};
	int dIndex = useropen[currentDirectoryIndex].systemOpenEntry;
	int dInode = systemopen[dIndex].inodeIndex;
	while (dIndex != 0)
	{
		snprintf(buffer, sizeof(buffer), "%s%s", systemopen[dIndex].filename, "/");
		int len = strlen(buffer);
		int len_final = strlen(buffer_final);
		memcpy(s, buffer, len * sizeof(char));
		memcpy(s + len * sizeof(char), buffer_final, len_final * sizeof(char));
		memcpy(buffer_final, s, sizeof(s));
		memset(s, 0, sizeof(s));
		dIndex = systemopen[dIndex].parentIndex;
		dInode = systemopen[dIndex].inodeIndex;
	}
	printf("/%s", buffer_final);
	printf("%s", " # ");
	if (currentFile >= 0)
	{
		printf("(%s) ", fcb[currentFile].filename);
	}
}

//向当前目录添加文件
int addfile_directory(int fileId)
{
	struct Directory *cd = getCurrentDirectory();
	int emptyIndex = -1;
	for (int i = 0; i < 127; i++)
	{
		if (cd->files[i] == -1)
		{
			emptyIndex = i;
			break;
		}
	}
	if (emptyIndex < 0)
	{
		return 0;
	}
	cd->files[emptyIndex] = fileId;
	return 1;
}

//从当前目录移除文件
int removefile_directory(int fileId)
{
	struct Directory *cd = getCurrentDirectory();
	for (int i = 0; i < 127; i++)
	{
		if (cd->files[i] == fileId)
		{
			cd->files[i] = -1;
			return 1;
		}
	}
	return 0;
}

//列出当前目录的内容
void list_directory()
{
	struct Directory *directory = getCurrentDirectory();
	int count = 0;
	for (int i = 0; i < 127; i++)
	{
		if (directory->files[i] != -1)
		{
			if (inode[directory->files[i]].attribute == 0)
			{ //是文件夹
				printf("[D]%s\t", fcb[directory->files[i]].filename);
			}
			else
			{
				printf("%s\t", fcb[directory->files[i]].filename); //是文件
			}
			count++;
			if (count % 6 == 0)
			{
				printf("\n");
			}
		}
	}
	if (count == 0)
	{
		printf("There are no files under this folder.\n");
	}
	else
	{
		printf("\n");
	}
}

//建立新的文件夹
int make_directory(char *name)
{
	//检测重名
	struct Directory *cd = getCurrentDirectory();
	for (int i = 0; i < 127; i++)
	{
		if (cd->files[i] != -1)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0 && inode[cd->files[i]].attribute == 0)
			{
				//存在重名
				return 0;
			}
		}
	}
	int inodeId = getEmptyINode(); //获取空的INode节点
	if (inodeId >= 0)
	{
		int blockNum = getEmptyBlock();
		if (blockNum >= 0)
		{
			//写入目录文件
			strcpy(fcb[inodeId].filename, name);
			fcb[inodeId].inodeIndex = inodeId;
			inode[inodeId].attribute = 0;
			inode[inodeId].status = 1;
			inode[inodeId].directPointer[0] = blockNum;
			inode[inodeId].indirectPointer = -1;
			inode[inodeId].subIndirectPointer = -1;
			//更新当前目录
			if (!addfile_directory(inodeId))
			{
				//创建失败时释放分配的fcb
				memset(&inode[inodeId], 0, sizeof(inode[inodeId]));
				memset(&fcb[inodeId], 0, sizeof(fcb[inodeId]));
				return 0;
			}
			//写入目录到文件空间
			struct Directory *directory = (struct Directory *)malloc(sizeof(Directory));
			directory->parentINodeIndex = currentDirectory;
			memset(directory->files, -1, sizeof(directory->files));
			return block_write(blockNum, directory, sizeof(Directory));
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

//进入新目录
int enter_directory(char *name)
{
	struct Directory *cd = getCurrentDirectory();
	int file = -1;
	for (int i = 0; i < 127; i++)
	{
		if (inode[cd->files[i]].attribute == 0)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0)
			{
				file = cd->files[i];
				break;
			}
		}
	}
	if (file < 0)
	{
		return 0;
	}

	for (int i = 0; i < 50; i++)
	{ //检查文件是不是已经打开了
		if (systemopen[i].inodeIndex == file)
		{
			systemopen[i].shareCount++;
			int emptyUseropen = getEmptyUseropen();
			if (emptyUseropen < 0)
			{
				return 0;
			}
			useropen[emptyUseropen].status = 1;
			useropen[emptyUseropen].systemOpenEntry = i;
			useropen[emptyUseropen].openMode = 2;
			return 1;
		}
	}

	//没有打开
	int emptySystemopen = getEmptySystemopen();
	int emptyUseropen = getEmptyUseropen();
	if (emptySystemopen < 0 || emptyUseropen < 0)
	{
		return 0;
	}

	//加入到文件打开表
	systemopen[emptySystemopen].attribute = 0;
	strcpy(systemopen[emptySystemopen].filename, fcb[file].filename);
	systemopen[emptySystemopen].inodeIndex = fcb[file].inodeIndex;
	systemopen[emptySystemopen].status = 1;
	systemopen[emptySystemopen].shareCount = 1;
	systemopen[emptySystemopen].parentIndex = useropen[currentDirectoryIndex].systemOpenEntry;
	systemopen[emptySystemopen].parentINode = currentDirectory;
	useropen[emptyUseropen].status = 1;
	useropen[emptyUseropen].systemOpenEntry = emptySystemopen;
	useropen[emptyUseropen].openMode = 2;
	useropen[emptyUseropen].parentIndex = currentDirectoryIndex;
	//更新全局
	currentDirectory = file;
	currentDirectoryIndex = emptyUseropen;
}

//回到上级目录
int back_parentDirectory()
{
	//获取父目录相关信息
	unsigned short parentInode = systemopen[useropen[currentDirectoryIndex].systemOpenEntry].parentINode;
	unsigned short parentIndex = useropen[currentDirectoryIndex].parentIndex;
	//关闭当前目录文件
	useropen[currentDirectoryIndex].status = 0;
	systemopen[useropen[currentDirectoryIndex].systemOpenEntry].shareCount--;
	if (systemopen[useropen[currentDirectoryIndex].systemOpenEntry].shareCount <= 0)
	{
		memset(&systemopen[useropen[currentDirectoryIndex].systemOpenEntry], 0, sizeof(SYSTEMOPEN));
	}
	//清零
	memset(&useropen[currentDirectoryIndex], 0, sizeof(USEROPEN));
	//更新全局
	currentDirectory = parentInode;
	currentDirectoryIndex = parentIndex;
	return 1;
}

//删除目录
int delete_directory(char *name)
{
	struct Directory *cd = getCurrentDirectory();
	int file = -1;
	int fileIndex = -1;
	for (int i = 0; i < 127; i++)
	{
		if (inode[cd->files[i]].attribute == 0)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0)
			{
				file = cd->files[i];
				fileIndex = i;
				break;
			}
		}
	}
	if (file < 0)
	{
		printf("Directory not found.\n");
		return 1;
	}
	//获取目标目录文件
	struct Directory *target = (struct Directory *)(FILEBLOCK_START + BLOCKSIZE * inode[file].directPointer[0]);
	for (int i = 0; i < 127; i++)
	{
		if (target->files[i] != -1)
		{
			//目录下存在文件
			int id = target->files[i];
			printf("Can not delete the directory because some files are still under it.\n");
			return 1;
		}
	}
	//删除目录文件块
	if (!block_free(inode[file].directPointer[0]))
	{
		return 0;
	}
	//释放fcb
	memset(&fcb[file], 0, sizeof(fcb[file]));
	memset(&inode[file], 0, sizeof(inode[file]));
	//更新当前目录
	cd->files[fileIndex] = -1;
	return 1;
}

//创建文件
int create_file(char *name)
{
	//检测重名
	struct Directory *cd = getCurrentDirectory();
	for (int i = 0; i < 127; i++)
	{
		if (cd->files[i] != -1)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0 && inode[cd->files[i]].attribute == 1)
			{
				//存在重名
				return 0;
			}
		}
	}
	//分配inode
	int inodeId = getEmptyINode(); //获取空的INode节点
	if (inodeId >= 0)
	{
		//写入文件时分配磁盘空间，创建不分配
		inode[inodeId].attribute = 1;
		inode[inodeId].status = 1;
		strcpy(fcb[inodeId].filename, name);
		fcb[inodeId].inodeIndex = inodeId;
		//指针初始化
		for (int i = 0; i < 8; i++)
		{
			inode[inodeId].directPointer[i] = -1;
		}
		inode[inodeId].indirectPointer = -1;
		inode[inodeId].subIndirectPointer = -1;
		//更新当前文件夹
		if (!addfile_directory(inodeId))
		{
			//创建失败时释放分配的fcb
			memset(&inode[inodeId], 0, sizeof(inode[inodeId]));
			memset(&fcb[inodeId], 0, sizeof(fcb[inodeId]));
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

//删除文件
int remove_file(char *name)
{
	struct Directory *cd = getCurrentDirectory();
	int file = -1;
	int fileIndex = -1;
	for (int i = 0; i < 127; i++)
	{
		if (inode[cd->files[i]].attribute == 1)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0)
			{
				file = cd->files[i];
				fileIndex = i;
				break;
			}
		}
	}
	if (file < 0)
	{
		printf("File not found.\n");
		return 1;
	}
	//循环释放目标文件的文件块
	for (int i = 0; i < 8; i++)
	{
		if (inode[file].directPointer[i] >= 0)
		{
			block_free(inode[file].directPointer[i]);
		}
	}
	//一级指针检查
	if (inode[file].indirectPointer >= 0)
	{
		struct INodeBlock *block = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[file].indirectPointer);
		for (int i = 0; i < 128; i++)
		{
			//指针被占用，释放对应块
			if (block->pointers[i] >= 0)
			{
				block_free(block->pointers[i]);
			}
		}
		//释放该指针块
		block_free(inode[file].indirectPointer);
	}
	//二级指针检查
	if (inode[file].subIndirectPointer >= 0)
	{
		struct INodeBlock *block = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[file].subIndirectPointer);
		for (int i = 0; i < 128; i++)
		{
			if (block->pointers[i] >= 0)
			{
				struct INodeBlock *subBlock = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * block->pointers[i]);
				for (int j = 0; j < 128; j++)
				{
					if (subBlock->pointers[j] >= 0)
					{
						block_free(subBlock->pointers[j]);
					}
				}
				//释放当前子指针块
				block_free(block->pointers[i]);
			}
		}
		//释放指针块
		block_free(inode[file].subIndirectPointer);
	}
	//处理fcb
	memset(&fcb[file], 0, sizeof(fcb[file]));
	memset(&inode[file], 0, sizeof(inode[file]));
	//更新当前目录
	cd->files[fileIndex] = -1;
	return 1;
}

//打开文件
int open_file(char *name)
{
	struct Directory *cd = getCurrentDirectory();
	int file = -1;
	for (int i = 0; i < 127; i++)
	{
		if (inode[cd->files[i]].attribute == 1)
		{
			if (strcmp(fcb[cd->files[i]].filename, name) == 0)
			{
				file = cd->files[i];
				break;
			}
		}
	}
	if (file < 0)
	{
		printf("File not found.\n");
		return 1;
	}

	for (int i = 0; i < 50; i++)
	{ //检查文件是不是已经打开了
		if (systemopen[i].inodeIndex == file)
		{
			systemopen[i].shareCount++;
			int emptyUseropen = getEmptyUseropen();
			if (emptyUseropen < 0)
			{
				return 0;
			}
			useropen[emptyUseropen].status = 1;
			useropen[emptyUseropen].systemOpenEntry = i;
			useropen[emptyUseropen].openMode = 2;
			return 1;
		}
	}

	//没有打开
	int emptySystemopen = getEmptySystemopen();
	int emptyUseropen = getEmptyUseropen();
	if (emptySystemopen < 0 || emptyUseropen < 0)
	{
		return 0;
	}

	//加入到文件打开表
	systemopen[emptySystemopen].attribute = 0;
	strcpy(systemopen[emptySystemopen].filename, fcb[file].filename);
	systemopen[emptySystemopen].inodeIndex = fcb[file].inodeIndex;
	systemopen[emptySystemopen].status = 1;
	systemopen[emptySystemopen].shareCount = 1;
	systemopen[emptySystemopen].parentIndex = useropen[currentDirectoryIndex].systemOpenEntry;
	systemopen[emptySystemopen].parentINode = currentDirectory;
	useropen[emptyUseropen].status = 1;
	useropen[emptyUseropen].systemOpenEntry = emptySystemopen;
	useropen[emptyUseropen].openMode = 2;
	useropen[emptyUseropen].parentIndex = currentDirectoryIndex;
	//更新全局
	currentFile = file;
	currentFileIndex = emptyUseropen;
	currentFileUsedBlock = (int)ceil((double)inode[file].length / BLOCKSIZE);
	return 1;
}

//关闭文件
int close_file()
{
	//关闭当前文件
	useropen[currentFileIndex].status = 0;
	systemopen[useropen[currentFileIndex].systemOpenEntry].shareCount--;
	if (systemopen[useropen[currentFileIndex].systemOpenEntry].shareCount <= 0)
	{
		memset(&systemopen[useropen[currentFileIndex].systemOpenEntry], 0, sizeof(SYSTEMOPEN));
	}
	//清零
	memset(&useropen[currentFileIndex], 0, sizeof(USEROPEN));
	//更新全局
	currentFile = -1;
	currentFileIndex = -1;
	currentFileUsedBlock = -1;
}

//写文件
int write_file(char *content)
{
	if (currentFile >= 0)
	{
		//文件内容不存在
		int len = strlen(content) * sizeof(char); //获取写入内容的长度
		int offset = 0;
		void *p = (void *)content;
		while (len - offset > 0)
		{
			int bs = BLOCKSIZE;
			if (len - offset < BLOCKSIZE)
			{
				//长度不足一个BLOCK，则只取其长度
				bs = len;
			}
			//取需要写入的数据放到缓冲区
			void *buffer = malloc(bs);
			memcpy(buffer, p + offset, bs);
			//缓冲区写入到文件空间
			if (currentFileUsedBlock >= 136)
			{
				//已经使用了超过136块内容，应该写入到二级间接指针下的block
				if (currentFileUsedBlock == 136)
				{
					//分配二级间接指针
					int b_subinode = getEmptyBlock();
					if (b_subinode >= 0)
					{
						struct INodeBlock *ib = (struct INodeBlock *)malloc(sizeof(INode));
						block_write(b_subinode, ib, sizeof(INode));
						inode[currentFile].subIndirectPointer = b_subinode;
					}
					else
					{
						printf("Failed, no more empty block.");
						return 1;
					}
				}
				int subUsedBlock = currentFileUsedBlock - 136;
				int i_sub = (int)ceil((double)subUsedBlock / 128);																	   //子索引块块的位置
				int i_subib = subUsedBlock % 128;																					   //内容在子索引块块内的位置
				struct INodeBlock *subib = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[currentFile].subIndirectPointer); //二级间接指针的索引块
				//每128块需要分配一个新的子块
				if (subUsedBlock % 8 == 0)
				{
					//分配子间接指针的索引块
					int b_inode = getEmptyBlock();
					if (b_inode >= 0)
					{
						struct INodeBlock *ib = (struct INodeBlock *)malloc(sizeof(INode));
						block_write(b_inode, ib, sizeof(INode));
						subib->pointers[i_sub] = b_inode;
					}
					else
					{
						printf("Failed, no more empty block.");
						return 1;
					}
				}
				//写入buffer到块内
				int b = getEmptyBlock();
				if (b >= 0)
				{
					block_write(b, buffer, bs); //将buffer写入到文件
					struct INodeBlock *c_ib = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * subib->pointers[i_sub]);
					c_ib->pointers[i_subib] = b;
					inode[currentFile].length += bs;
					currentFileUsedBlock++;
				}
				else
				{
					printf("Failed, no more empty block.");
					return 1;
				}
			}
			else if (currentFileUsedBlock >= 8)
			{
				//已经使用了超过8块内容，应该写入到一级间接指针下的block
				if (currentFileUsedBlock == 8)
				{
					//分配一级间接指针
					int b_inode = getEmptyBlock();
					if (b_inode >= 0)
					{
						struct INodeBlock *ib = (struct INodeBlock *)malloc(sizeof(INode));
						block_write(b_inode, ib, sizeof(INode));
						inode[currentFile].indirectPointer = b_inode;
					}
					else
					{
						printf("Failed, no more empty block.");
						return 1;
					}
				}
				int b = getEmptyBlock();
				if (b >= 0)
				{
					block_write(b, buffer, bs); //将buffer写入到文件
					struct INodeBlock *indexBlock = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[currentFile].indirectPointer);
					indexBlock->pointers[currentFileUsedBlock - 8] = b;
					inode[currentFile].length += bs;
					currentFileUsedBlock++;
				}
				else
				{
					printf("Failed, no more empty block.");
					return 1;
				}
			}
			else
			{
				//未超过8块，写入到直接指针
				int b = getEmptyBlock();
				if (b >= 0)
				{
					block_write(b, buffer, bs); //将buffer写入到文件
					inode[currentFile].directPointer[currentFileUsedBlock] = b;
					inode[currentFile].length += bs;
					currentFileUsedBlock++;
				}
				else
				{
					printf("Failed, no more empty block.");
					return 1;
				}
			}
			//释放缓冲区
			free(buffer);
			//更新变量
			offset += bs;
		}
	}
	else
	{
		return 0;
	}
	return 1;
}

//读文件
int read_file()
{
	if (currentFile >= 0)
	{
		if (currentFileUsedBlock == 0)
		{
			printf("File is empty.\n");
			return 1;
		}
		for (int i = 0; i < 8; i++)
		{
			if (inode[currentFile].directPointer[i] >= 0)
			{
				block_read(inode[currentFile].directPointer[i]);
			}
			else
			{
				break;
			}
		}
		if (inode[currentFile].indirectPointer >= 0)
		{
			struct INodeBlock *ib = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[currentFile].indirectPointer);
			for (int i = 0; i < 128; i++)
			{
				if (ib->pointers[i] >= 0)
				{
					block_read(ib->pointers[i]);
				}
				else
				{
					break;
				}
			}
		}
		if (inode[currentFile].subIndirectPointer >= 0)
		{
			struct INodeBlock *subib = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * inode[currentFile].subIndirectPointer);
			for (int i = 0; i < 128; i++)
			{
				if (subib->pointers[i] >= 0)
				{
					struct INodeBlock *ib = (struct INodeBlock *)(FILEBLOCK_START + BLOCKSIZE * subib->pointers[i]);
					for (int j = 0; j < 128; j++)
					{
						if (ib->pointers[j] >= 0)
						{
							block_read(ib->pointers[j]);
						}
						else
						{
							break;
						}
					}
				}
				else
				{
					break;
				}
			}
		}
	}
	else
	{
		return 0;
	}
	printf("\n");
	return 1;
}

void cat_file(char *name){
	if (open_file(name)){
		read_file();
		close_file();
	}
}