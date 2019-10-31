// Simple File System in User Space.
/*
Name: Abdalraheem Alsmadi.
*/

#ifndef FUSE_USE_VERSION
	#define FUSE_USE_VERSION 30
#endif
#include <fuse.h> 		
#include <stdio.h>				
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>	// ENOMEM
#include <unistd.h> 
#include <pwd.h>	// Due to getUsername()
// https://github.com/osxfuse/fuse
// (find /usr/include | xargs grep ENOMEM | grep '#define') 2>/dev/null

char *_fileName = "alertMe";
char *fileName = "/alertMe";
char *pathOfBashrc;

char *getUsername() {
	register struct passwd *pw;
  	register uid_t uid;
  	uid = geteuid();
  	pw = getpwuid(uid);
	return pw->pw_name;
}

char* getTheAlertText() {

			FILE *stream;
			char *line = NULL;	// The Data Of That line, null terminated
			size_t len = 0;	
			ssize_t nread; 		// The Length of the line 
			int x = 0;
			stream = fopen(pathOfBashrc, "r");
			if (stream == NULL) 
				return NULL;

			while ((nread = getline(&line, &len, stream)) != -1) {
				//fwrite(line, nread, 1, stdout);
				if (strncmp("cowsay", line, strlen("cowsay")) == 0) {
					x = 1;
					break;
				}
			}

			fclose(stream);
			if (x == 0) {
				return NULL;
			} else {
				int repace = strlen("cowsay -e \"o0\" ");
				char *x = malloc(nread - repace + 1);
				memcpy(x, line + repace, nread - repace);
				memset(x + nread - repace, 0, 1);
				free(line);
				return x;
			}
			return line;
}
			

int getLengthOfTheAlertText() { 
	char *j = getTheAlertText();
	if (j == NULL) 
		return 0;
	int c = strlen(j);
	free(j);
	return c;
}

void setTheAlertText(const char *x, size_t size) {	
	/*
	// Prepare Memory Stream ...
	*/
	FILE *stream;
	char *buf /* Buffer Data */ ; size_t lenOfChar; /* Len Of Data */
	stream = open_memstream(&buf, &lenOfChar);

	/*
	// Read .bashrc File And Process Out The Content ...
	*/
	FILE *file;
	char *line = NULL;	
	size_t len = 0;
	ssize_t nread; 	
	file = fopen(pathOfBashrc, "r");
	if (file == NULL) 
		 return;

	size_t y = strlen("cowsay");
	while (nread = getline(&line, &len, file) != -1) 
		if (strncmp("cowsay", line, y) != 0) 
			fprintf(stream, "%s", line);


	/*
	// Close The Data, Prepae The Files ...
	*/
	fclose(file);
	free(line);
	if (size >= 3)
		fprintf(stream, "cowsay -e \"o0\" %s", x);
	fclose(stream);


	/*
	// Write The New Text 
	*/
	file = fopen(pathOfBashrc, "w");
	if (file == NULL) 
		return;
	fprintf(file, "%s", buf);
	fclose(file);
	free(buf);
}

// will be executed when the system asks for attributes of a file that was stored in the mount point
int fun_getattr( const char *path, struct stat *stateBuff ) {
		
	stateBuff->st_uid = getuid();
	stateBuff->st_gid = getgid();
	stateBuff->st_atime = stateBuff->st_mtime = stateBuff->st_ctime = time(NULL);

	if ( strcmp( path, "/" ) == 0 ) { // Info For The mount point
		stateBuff->st_mode = S_IFDIR | 0755;
		stateBuff->st_nlink = 2;
	}
	else if (strcmp( path, fileName ) == 0){ // Info For The req file
		stateBuff->st_mode = S_IFREG | 0644;
		stateBuff->st_nlink = 1;
		stateBuff->st_size = getLengthOfTheAlertText();
	} else 
		return -ENOENT;

	return 0;
}



//  will be executed when the system asks for a list of files that were stored in the mount point
int fun_readdir( const char *path, void *buff, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	filler( buff, ".", NULL, 0 ); 	// Current Directory
	filler( buff, "..", NULL, 0 ); 	// Parent Directory
	
	// Get the files/directories which are exist in the mount point
	if ( strcmp( path, "/" ) == 0 )
		filler( buff, _fileName , NULL, 0 );
	else 
		return -ENOENT;

	return 0;
}


// will be executed when the system is trying to read one of the files that were stored in the mount point
int fun_read( const char *path, char *buff, size_t size, off_t offset, struct fuse_file_info *fi ) {

	char *selectedText = NULL;
	int len = getLengthOfTheAlertText();
	if (offset >= len)
		return 0;

	if (len == 0)
		return len;
	else if ( strcmp( path, fileName ) == 0 )
		{		
		selectedText = getTheAlertText();
		printf("Read Done successfully\n");
		}
	else
		return -ENOENT;

	if (size > len - offset)
		size = len - offset;

	memcpy(buff, selectedText + offset, size);
	return size;
}


int fun_write(const char *path, const char *buff, size_t size, off_t offset, struct fuse_file_info *fi) {
	if (strcmp( path, fileName ) == 0 )
		{		
		printf("Writing Done successfully\n");
		setTheAlertText(buff, size);
		}
	else
		return -ENOENT;

	return size;
}

int fun_release(const char *path, struct fuse_file_info *fi) {
	printf("Release Function\n");
	return 0;
}



int fun_flush(const char *path, struct fuse_file_info *fi) { 
	printf("Flush Function\n");
	return 0;
}



int fun_open(const char *path, struct fuse_file_info *fi) {
	
	if ( strcmp(path, fileName) == 0 ) 
		printf("has Been opened successfully\n");
	else
		return -ENOENT;

	return 0;
}

int fun_truncate(const char *path, off_t newsize)
{
	return 0;
}

int fun_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
	return 0;
}


static struct fuse_operations operations = {
    .getattr	= fun_getattr,
    .readdir	= fun_readdir,
    .read		= fun_read,
	.write		= fun_write,
	.open		= fun_open,
	.release	= fun_release,  
	.flush		= fun_flush,  
	.truncate 	= fun_truncate,
	.ftruncate 	= fun_ftruncate,
};

// gcc FUSE.c -o myFUSE `pkg-config fuse --cflags --libs`
int main( int argc, char *argv[] ) {
	// Get .bashrc File Path
	size_t s = strlen("/home/") + strlen(getUsername()) + strlen("/.bashrc") + 1;
	pathOfBashrc = malloc(s);
	memset(pathOfBashrc, 0, s);

	sprintf(pathOfBashrc + strlen(pathOfBashrc), "/home/");
	sprintf(pathOfBashrc + strlen(pathOfBashrc), "%s", getUsername());
	sprintf(pathOfBashrc + strlen(pathOfBashrc), "/.bashrc");
	return fuse_main( argc, argv, &operations, NULL );
}
