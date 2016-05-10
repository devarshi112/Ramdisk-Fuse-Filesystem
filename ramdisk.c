//Courtesy: libfuse/example/hello.c
//I have modified hello.c from the libfuse pkg to implement Program 4 Ramdisk as below:
#define FUSE_USE_VERSION 30

//#include <config.h>

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

#define MAXLEN 4096
//#define DEBUG

char nFile[256];
long memAvail;
int totalNodes;
typedef struct _node
{
        char nName[256];
        int isDir;
	int isFile;
	struct _node *parent;
        struct _node *child;                
        struct _node *next;
        struct stat *nMeta;
	char *fMeta;        
}Node;
Node *root;

void initRamdisk()
{
        time_t tmpTime;
        time(&tmpTime);
        root = (Node *)malloc(sizeof(Node));
        root->nMeta = (struct stat *)malloc(sizeof(struct stat));
	
	strcpy(root->nName, "/");
	
	root->isDir = 1;
	root->isFile = 0;
	
	root->parent = NULL;
    	root->child = NULL;
    	root->next = NULL;
	root->fMeta = NULL;
	
        root->nMeta->st_mode = S_IFDIR | 0755;
	root->nMeta->st_nlink = 2;
        root->nMeta->st_uid = 0;
        root->nMeta->st_gid = 0;
    	root->nMeta->st_atime = tmpTime;
    	root->nMeta->st_mtime = tmpTime;
    	root->nMeta->st_ctime = tmpTime;    	
	
	long less = sizeof(Node) + sizeof(stat);
	memAvail -= less;
}

int validatePath(const char *path)
{
        char tmpPath[MAXLEN];
        strcpy(tmpPath, path);
        char *token = strtok(tmpPath, "/");
        
        if(token == NULL && (strcmp(path, "/") == 0))
        	return 0;
        else
        {
                int flag = 0;
                Node *tmpNode  = root;
                Node *temp = NULL;
                
                while(token != NULL) {
                	temp = tmpNode->child;
                        while(temp) {
                        	if(strcmp(temp->nName, token) == 0 )
                                {
                                        flag = 1;
                                        break;
                                }
                                temp = temp->next;
                        }
                        
                        token = strtok(NULL, "/");
                        
                        if(flag == 1) {
                        	if( token == NULL )
                                	return 0;
			}
                        else {
                                if (token)
                                	return -1;
                                else
                                	return 1;
                        }            
                       
                        tmpNode = temp;
                        flag = 0;
                }
        }
        //if it came till here then
        return -1;
}


Node *getPath(const char *path)
{
        char tmpPath[MAXLEN];
        strcpy(tmpPath, path);
        char *token = strtok(tmpPath, "/");
        
        if(token == NULL && (strcmp(path, "/") == 0)) {
                return root;
        }
        else
        {
                int flag = 0;
                Node *tmpNode  = root;
                Node *temp = NULL;
                while(token != NULL)
                {
                        flag = 0;
                        temp = tmpNode->child;
                        while(temp) {
                        	if(strcmp(temp->nName, token) == 0 )
                                {
                                        flag = 1;
                                        break;
                                }
                                temp = temp->next;
                        }
                        if(flag == 1) {
                        	token = strtok(NULL, "/");
                        	if( token == NULL )
                        		return temp;
			}
                        else {
                        	strcpy(nFile, token);
                                return tmpNode; 
                        }            
                        
                        tmpNode = temp;
                }
         }
         return NULL;
}



/*getattr
Return file attributes. The "stat" structure is described in detail in the stat(2) manual page. For the given pathname, this should fill in the elements of the "stat" structure. If a field is meaningless or semi-meaningless (e.g., st_ino) then it should be set to 0 or given a "reasonable" value. This call is pretty much required for a usable filesystem.
*/
static int ramdisk_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	if(validatePath(path) == 0)
	{
		Node *tmpNode;
		tmpNode = getPath(path);
		stbuf->st_atime = tmpNode->nMeta->st_atime;
                stbuf->st_mtime = tmpNode->nMeta->st_mtime;  
                stbuf->st_ctime = tmpNode->nMeta->st_ctime;
                
		stbuf->st_nlink = tmpNode->nMeta->st_nlink;
		stbuf->st_mode  = tmpNode->nMeta->st_mode;
		
		stbuf->st_uid   = tmpNode->nMeta->st_uid;
                stbuf->st_gid   = tmpNode->nMeta->st_gid;
                
                stbuf->st_size = tmpNode->nMeta->st_size;
                res = 0;		
	}
	else
		res = -ENOENT;
	
	return res;
}

static int ramdisk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	int isValid = validatePath(path);
	if(isValid == 0 || isValid == 1)
	{
		Node *tmpNode, *iter;
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		tmpNode = getPath(path);
		for(iter = tmpNode->child; iter != NULL; iter = iter->next)                
                        filler(buf, iter->nName, NULL, 0);
                
                time_t tmpTime;
                time(&tmpTime);
                tmpNode->nMeta->st_atime = tmpTime;
                
                return 0;                
	}
	else
		return -ENOENT;
}


/*open
Open a file. If you aren't using file handles, this function should just check for existence and permissions and return either success or an error code. If you use file handles, you should also allocate any necessary structures and set fi->fh. In addition, fi has some other fields that an advanced filesystem might find useful; see the structure definition in fuse_common.h for very brief commentary.
*/
static int ramdisk_open(const char *path, struct fuse_file_info *fi)	//done
{
	if(validatePath(path) == 0)	
		return 0;
	else
		return -ENOENT;
}


/*read
Read size bytes from the given file into the buffer buf, beginning offset bytes into the file. See read(2) for full details. Returns the number of bytes transferred, or 0 if offset was at or beyond the end of the file. Required for any sensible filesystem.
*/
static int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	Node *tmpNode;
	size_t len;
	tmpNode = getPath(path);
	if(tmpNode->isDir == 1)	
		return -EISDIR;	
	
	len = tmpNode->nMeta->st_size;
	if(offset < len) {
		if(offset + size > len)		
			size = len - offset;		
		memcpy(buf, tmpNode->fMeta + offset, size);
	} else	
		size = 0;
		
	if(size > 0) {
		time_t tmpTime;
		time(&tmpTime);
		tmpNode->nMeta->st_atime = tmpTime;
	}
	
	return size;	
}


void setParamsForWrite(Node **tmpNode, off_t offset, size_t size)
{
	(*tmpNode)->nMeta->st_size = offset + size;
	time_t tmpTime;
	time(&tmpTime);
	(*tmpNode)->nMeta->st_ctime = tmpTime;
	(*tmpNode)->nMeta->st_mtime = tmpTime;	
}


/*write
As for read above, except that it can't return 0.
*/
static int ramdisk_write(const char *path, const char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	if(size > memAvail)
		return -ENOSPC;
	
	Node *tmpNode;
	size_t len;
	tmpNode = getPath(path);
	if(tmpNode->isDir == 1)	
		return -EISDIR;	
	
	len = tmpNode->nMeta->st_size;
	
	if( size > 0) {
		if(len == 0) {
			offset = 0;
			tmpNode->fMeta = (char *)malloc(sizeof(char) * size);
			memcpy(tmpNode->fMeta + offset, buf, size);
			setParamsForWrite(&tmpNode, offset, size);
			memAvail -= size;
		}		
		else {
			if(offset > len)
				offset = len;
			char *fo = (char *)realloc(tmpNode->fMeta, sizeof(char) * (offset+size));
			if(fo == NULL)
				return -ENOSPC;
			else
			{
				tmpNode->fMeta = fo;
				memcpy(tmpNode->fMeta + offset, buf, size);
				
				setParamsForWrite(&tmpNode, offset, size);
					
				long less = offset + size;
				long less2 = len - less; 
				memAvail += less2;
			}
		}
		
	}
	return size;
}


void setParamsForMkDir(Node **tmpDir, Node *tmpNode)
{
	time_t tmpTime;
        time(&tmpTime);
        strcpy((*tmpDir)->nName, nFile);
        
        (*tmpDir)->isDir = 1;
	(*tmpDir)->isFile = 0;
                
        (*tmpDir)->nMeta->st_atime = tmpTime;
        (*tmpDir)->nMeta->st_mtime = tmpTime;
        (*tmpDir)->nMeta->st_ctime = tmpTime;
        
        (*tmpDir)->parent = tmpNode;
        (*tmpDir)->child = NULL;
        (*tmpDir)->next = NULL;
        (*tmpDir)->nMeta->st_size = MAXLEN;
	
	(*tmpDir)->nMeta->st_nlink = 2;
        (*tmpDir)->nMeta->st_mode = S_IFDIR | 0755;
        
        (*tmpDir)->nMeta->st_uid = getuid();
        (*tmpDir)->nMeta->st_gid = getgid();
}

void setParamsForCreate(Node **newNode, Node *tmpNode, mode_t mode)
{
	time_t tmpTime;
	time(&tmpTime);
	strcpy((*newNode)->nName, nFile);
	
	(*newNode)->isDir = 0;
	(*newNode)->isFile = 1;
	
	(*newNode)->nMeta->st_atime = tmpTime;
	(*newNode)->nMeta->st_mtime = tmpTime;
	(*newNode)->nMeta->st_ctime = tmpTime;
	
	(*newNode)->nMeta->st_size = 0;	
	(*newNode)->parent = tmpNode;
	(*newNode)->child = NULL;
	(*newNode)->next = NULL;
	
	(*newNode)->nMeta->st_mode = S_IFREG | mode;
	(*newNode)->nMeta->st_nlink = 1;
	(*newNode)->nMeta->st_uid = getuid();
	(*newNode)->nMeta->st_gid = getgid();  	
	(*newNode)->fMeta = NULL;
}


int chkMemAvailable()
{
	if(memAvail < 0)
		return -1;
	return 0;
}

int chkNewMemAvailable(Node **newNode)
{
	(*newNode) = (Node *)malloc(sizeof(Node));
	(*newNode)->nMeta = (struct stat *)malloc(sizeof(struct stat));
	if((*newNode) == NULL)
		return -1;
	return 0;
}


void insertNode(Node **tmpNode, Node **tmpDir)
{
	if((*tmpNode)->child) {
		Node *iter;
                for(iter = (*tmpNode)->child; iter->next != NULL; iter = iter->next)
                	;
                iter->next = (*tmpDir);			
	}
	else
		(*tmpNode)->child = (*tmpDir);
}


/*mkdir
Create a directory with the given name. The directory permissions are encoded in mode. See mkdir(2) for details. This function is needed for any reasonable read/write filesystem.
*/
static int ramdisk_mkdir(const char *path, mode_t mode)
{
	Node *tmpNode = getPath(path);
	Node *tmpDir;	
	if(chkNewMemAvailable(&tmpDir) == -1)
		return -ENOSPC;
	
	memAvail -= (sizeof(Node) - sizeof(struct stat));
	if (chkMemAvailable() == -1)
		return -ENOSPC;
		
	setParamsForMkDir(&tmpDir, tmpNode);
	
	insertNode(&tmpNode, &tmpDir);
			
        tmpNode->nMeta->st_nlink = tmpNode->nMeta->st_nlink + 1;
        return 0;
}



void rmUtil(Node **supNode, Node **tmpNode)
{
        if((*supNode)->child == (*tmpNode))
        {
        	if((*tmpNode)->next == NULL)
        	{
        		//delete the lone leaf directory		
                	(*supNode)->child = NULL;
                }
                else
                {
                	//make super node point to 2nd node in line
                	(*supNode)->child = (*tmpNode)->next;
                }
        }		  
        else
        {
                Node *iter;
                for(iter = (*supNode)->child; iter != NULL; iter = iter->next)
                {
                        if(iter->next == (*tmpNode))
                        {
                                iter->next = (*tmpNode)->next;
                                break;
                        }
                }
        }
}


/*rmdir
Remove the given directory. This should succeed only if the directory is empty (except for "." and ".."). See rmdir(2) for details.
*/
static int ramdisk_rmdir(const char *path)
{
	int isValid = validatePath(path);
	if(isValid == 0)
	{
		Node *tmpNode;
		tmpNode = getPath(path);
		if(tmpNode->child)
		{
			#ifdef DEBUG
				printf("Directory is not empty... cannot do rmdir\n");				
			#endif
			return -ENOTEMPTY;
		}
		else
		{
			Node *supNode;
			supNode = tmpNode->parent;
			
			rmUtil(&supNode, &tmpNode);
			
			free(tmpNode->nMeta);
			free(tmpNode);
		        memAvail += (sizeof(Node) + sizeof(struct stat));
		        
		        supNode->nMeta->st_nlink = supNode->nMeta->st_nlink-1;
		        return 0;
                }
	}
	else
		return -ENOENT;
		
}


/*opendir
Open a directory for reading.
*/
static int ramdisk_opendir(const char *path, struct fuse_file_info *fi)
{
	return 0;
}



/*unlink
Remove (delete) the given file, symbolic link, hard link, or special node.
*/
static int ramdisk_unlink(const char *path)
{
	int isValid = validatePath(path);
	if(isValid != 0) {
		return -ENOENT;	
	}
	Node *tmpNode, *supNode;
	tmpNode = getPath(path);
	supNode = tmpNode->parent;
		
	rmUtil(&supNode, &tmpNode);
	
	if(tmpNode->nMeta->st_size == 0)
	{
		free(tmpNode->nMeta);
		free(tmpNode);
	}
	else
	{	
		memAvail = memAvail + tmpNode->nMeta->st_size;
		free(tmpNode->fMeta);
		free(tmpNode->nMeta);
		free(tmpNode);
	}	
	
	long more = (sizeof(Node) + sizeof(struct stat));
	memAvail += more;
	return 0;
}


//create
static int ramdisk_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	if(chkMemAvailable() == -1)
		return -ENOSPC;
	
	Node *tmpNode = getPath(path);	
	Node *newNode;
	
	if(chkNewMemAvailable(&newNode) == -1)
		return -ENOSPC;
	
	long less = sizeof(Node) + sizeof(stat);	
	memAvail -= less;
	
	setParamsForCreate(&newNode, tmpNode, mode);
	
	insertNode(&tmpNode, &newNode);
	return 0;
}

static int ramdisk_utime(const char *path, struct utimbuf *ubuf)
{
	return 0;
}

int setSizeForRename(Node **new_node, Node *ptr)
{
	(*new_node)->nMeta->st_size = ptr->nMeta->st_size; 
	if(ptr->nMeta->st_size > 0) {
		(*new_node)->fMeta = (char *)malloc(sizeof(char)* ptr->nMeta->st_size);
		if((*new_node)->fMeta) {
			strcpy((*new_node)->fMeta, ptr->fMeta);
			memAvail -= ptr->nMeta->st_size;		
		}
		else
			return -1;
	}
	return 0;
}

int setSizeForRename2(Node **temp, Node *ptr, int fromSize) 
{
	if(fromSize != 0) {
		char *new_fMeta = (char *)realloc((*temp)->fMeta, sizeof(char)*(fromSize));
		if(new_fMeta)
		{
			(*temp)->fMeta = new_fMeta;
			strcpy((*temp)->fMeta, ptr->fMeta);
			memAvail -= fromSize;
		}
		else
			return -1;
	}
	(*temp)->nMeta->st_size = fromSize;
	return 0;
}



void setParamsForRename(Node **new_node, Node *ptr)
{
	(*new_node)->nMeta->st_atime = ptr->nMeta->st_atime;
	(*new_node)->nMeta->st_mtime = ptr->nMeta->st_mtime;
	(*new_node)->nMeta->st_ctime = ptr->nMeta->st_ctime;
	
	(*new_node)->nMeta->st_mode = ptr->nMeta->st_mode;
	(*new_node)->nMeta->st_nlink = ptr->nMeta->st_nlink;
	
	(*new_node)->nMeta->st_uid = ptr->nMeta->st_uid;
	(*new_node)->nMeta->st_gid = ptr->nMeta->st_gid;
}

/*rename
Rename the file, directory, or other object "from" to the target "to". Note that the source and target don't have to be in the same directory, so it may be necessary to move the source to an entirely new directory. See rename(2) for full details.
*/
static int ramdisk_rename(const char *from, const char *to)
{
	int isValid, isValid2;
	isValid = validatePath(from);
	isValid2 = validatePath(to);	
		
	if(isValid == 0) {
		Node *ptr = getPath(from);
		Node *temp = getPath(to);
		char new_file_name[256];
				
		strcpy(new_file_name, nFile);
		if(isValid2 == 1) {	//to path not already present. Then create one
			if(temp->isFile == 1 ) {
				memset(ptr->nName, 0, 255);
				strcpy(ptr->nName, new_file_name);
				return 0;
			}
			if(temp->isDir == 1) {
				ramdisk_create(to, ptr->nMeta->st_mode, NULL);
				Node *new_node  = getPath(to);
				
			       	setParamsForRename(&new_node, ptr);
				if(setSizeForRename(&new_node, ptr) == -1)
					return -ENOSPC;
				    		
				ramdisk_unlink(from);
				return 0;
			}			
			else
				return -ENOENT;
		}
		else if(isValid2 == 0) {	//to path already present.
			if(temp->isFile == 1) {
				int fromSize, toSize;
				setParamsForRename(&temp, ptr);
				fromSize = ptr->nMeta->st_size;
				toSize = temp->nMeta->st_size;
				memset(temp->fMeta, 0, toSize);
				
				if(setSizeForRename2(&temp, ptr, fromSize) == -1)
					return -ENOSPC;
				
				memset(temp->fMeta, 0, toSize);
				
				ramdisk_unlink(from);
				return 0;
		        }
		}
		else
			return -ENOENT;	
	}
	else
		return -ENOENT;
        return 0;
}


static int ramdisk_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	return 0;
}

static struct fuse_operations ramdisk_oper = {
	.getattr	= ramdisk_getattr,
	.readdir	= ramdisk_readdir,
	.open		= ramdisk_open,
	.opendir	= ramdisk_opendir,
	.read		= ramdisk_read,
	.write		= ramdisk_write,
	.mkdir		= ramdisk_mkdir,
	.rmdir		= ramdisk_rmdir,
	.create		= ramdisk_create,
	.unlink		= ramdisk_unlink, 
	.rename		= ramdisk_rename, 
	.utime		= ramdisk_utime,
	.fsync		= ramdisk_fsync
};

void writeEC(char *fileName)
{
	#ifdef DEBUG
        	printf("6.1\n");
        #endif
	FILE *fw = fopen(fileName, "wb");
	if (fw) {
		fwrite(root, sizeof(Node), totalNodes, fw);
    		fclose(fw);
	}	
}

void readEC(char *fileName)
{
	#ifdef DEBUG
        	printf("4.1\n");
        #endif
	FILE *fr = fopen(fileName, "rb");
	if (fr) {
		fread(root, sizeof(Node), totalNodes, fr);
    		fclose(fr);
	}
}


int main(int argc, char *argv[])
{
	int flagEC = 0;
	char *fileName = NULL;
	if( argc != 3 && argc != 4 )
        {
                printf("Invalid arguments\n");	//todo too many args
                printf("Usage: ramdisk </path/to/dir> <size> [<filename>]\n");
                return -1;
        }
        #ifdef DEBUG
        	printf("1\n");
        #endif
        if(argc == 4)
        	flagEC = 1;
     	argc--;
        memAvail = (long) atoi(argv[2]);
        memAvail = memAvail * 1024 * 1024;
        #ifdef DEBUG
        	printf("2\n");
        #endif
        initRamdisk();
        if(flagEC == 1) {
        	#ifdef DEBUG
        	printf("3\n");
        	#endif
        	flagEC = 1;
        	totalNodes = memAvail / sizeof(Node);
        	fileName=(char*)malloc(sizeof(char)*strlen(argv[3]));
		strcpy(fileName, argv[3]);
        	argc--;
        	#ifdef DEBUG
        	printf("4\n");
        	#endif
        	readEC(fileName);
        }	
        #ifdef DEBUG
        	printf("5\n");
        #endif
	fuse_main(argc, argv, &ramdisk_oper, NULL);
	#ifdef DEBUG
        	printf("6\n");
        #endif
	if(flagEC == 1) {
		writeEC(fileName);
	}
	#ifdef DEBUG
        	printf("7\n");
        #endif
	return 0;
}
