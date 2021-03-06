/*THIS CODE IS MY OWN WORK. IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - XIANG CHENG*/

/* This code is a mimic of UNIX command "ar". 
 * Written by XIANG CHENG
 * EMAIL: CHENGXIANG.CN@GMAIL.COM; XIANG.CHENG@EMORY.EDU
 */
#include <ar.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <utime.h>
#include <dirent.h>

#define MAX_FILE_NAME 16
#define HDR_LEN 60
#define BLOCK_SIZE 4096

int append_files (int argc, char **argv);// 'q': quickly append named files to archive
int extract_files (int argc, char **argv, int xo);// 'x' : extract named files; 'xo': extract named files restoring mtime
int print_table (int argc, char **argv, int tv);// 't' : print a concise table of contents of the archive; 'tv': print a verbose table of contents of the archive
int delete_files (int argc, char **argv);// 'd' : delete named files from archive
int append_dir (int argc, char **argv);// 'A' : quickly append all 'regular' files in the current directory
void usage();
void wrong_key(char *arg);
void arc_access_error(char *arc);
void arc_read_error(char *arc);
int fname2str(char *fname);
void print_mode(int mode);
int fnamecpy(char *fname, char *fnew);
void d_printc(char *s, int l);//del
void check_mag_str(char *arMagStr, int len_read);
int add_file2ar(int fd_ar, int fdnew, char *fname, struct stat statbuf);



int main(int argc, char **argv)
{
    int key_len = 0;
    if ( argc < 3){
        printf("  Error: Only %d argument entered! 3 or more are needed!\n", argc);
        usage(); exit(EXIT_FAILURE);
    }
    key_len = strlen(argv[1]);
    //printf("    key_len = %d\n", key_len); //del
    if (key_len > 2)
    {
        printf("  Error: key \"%s\" not available in command myar!\n", argv[1]);
        usage();
        exit(EXIT_FAILURE);
    }
    
    switch (argv[1][0]){
        case 'q':
            if(key_len==1){
                append_files(argc, argv);
                //printf("  key 'q' passed\n");
            }
            else
                wrong_key(argv[1]);
            break;
        case 'x':
            if ( key_len == 1){
                extract_files(argc, argv, 0);
            }
            else if (argv[1][1] == 'o'){
                extract_files(argc, argv, 1);
                //printf("  key \"xo\" passed\n");
            }
            else
                wrong_key(argv[1]);
            break;
        case 't':
            if ( key_len == 1){
                print_table(argc, argv, 0);
            }
            else if (argv[1][1] == 'v'){
                print_table(argc, argv, 1);
            }
            else
                wrong_key(argv[1]);
            break;
        case 'd':
            if(key_len == 1){
                delete_files(argc, argv);
                //printf("  key 'd' passed\n");
            }
            else
                wrong_key(argv[1]);
            break;
        case 'A':
            if(key_len == 1) {
                append_dir(argc, argv);
                //printf("  key 'A' passed\n");
            }
            else
                wrong_key(argv[1]);
            break;
        default:
            wrong_key(argv[1]);
    }
    
    return(1);
}

// 'q': quickly append named files to archive
int append_files (int argc, char **argv)
{
    char arMagStr[SARMAG];
    int len_read = 0;
    int fd = open(argv[2], O_RDWR | O_CREAT | O_APPEND, 0666);
    int fd2;
    short i; 
    struct stat statbuf;
    if (fd == -1)
        arc_read_error(argv[2]);
    umask(0);
    lseek(fd, 0, SEEK_SET);
    len_read = read(fd, arMagStr, SARMAG);
    lseek(fd, 0, SEEK_SET);
    if (len_read < 0) {
        printf("myar: %s: Error while reading the file.\n", argv[2]);
        exit(EXIT_FAILURE);
    } 
    else if (len_read > 0)
        check_mag_str(arMagStr, len_read);
    else
        write(fd, ARMAG, SARMAG);
    lseek(fd, SARMAG, SEEK_SET);
    if(argc==3){
        printf("  Info: no files specified to append.\n");
        printf("  myar: creating arc.\n");

        close(fd);
        exit(EXIT_SUCCESS);
    }
    i = 3;
    while( i < argc){
        char *fname = argv[i];
        fd2 = open(fname, O_RDONLY);
        if (fd2 < 0) {
            printf("  myar: %s : No such file or directory\n", fname);
            printf("  For safety, no files after it are archived!\n");
            exit(EXIT_FAILURE);
            close(fd);
            close(fd2);
        }
        if( stat(fname, &statbuf)==-1 ) {
            printf("  Error: append(): stat() failed!\n");
            exit(EXIT_FAILURE);
        }
        if((statbuf.st_mode & S_IFMT)==S_IFREG){
            /* Handle regular file */
            lseek(fd2, 0, SEEK_SET);
        }
        else if((statbuf.st_mode & S_IFMT)==S_IFLNK){
            /* Handle symbolic link */
            lseek(fd2, 0, SEEK_SET);
        }
        else{ /* Error */
            printf( "  myar: %s: Not a regular or symbolic file.\n", fname);
            exit(EXIT_FAILURE);
            close(fd);
            close(fd2);
        }
        add_file2ar(fd, fd2, fname, statbuf);
        close(fd2);
        i++;
    }
    close(fd);
    return(0);
}

int add_file2ar(int fd, int fd2, char *fname, struct stat statbuf){
    struct ar_hdr myhdr;
    int len_read=0, lw=0, lr=0;
    char buf[BLOCK_SIZE];

    sprintf(myhdr.ar_name, "%-16.16s", fname);
	sprintf(myhdr.ar_date, "%-12u", (unsigned int)statbuf.st_mtime);
	sprintf(myhdr.ar_uid,  "%-6u",  statbuf.st_uid);
	sprintf(myhdr.ar_gid,  "%-6u",  statbuf.st_gid);
	sprintf(myhdr.ar_mode, "%-8o",  statbuf.st_mode);
    sprintf(myhdr.ar_size, "%-10u", (unsigned int)statbuf.st_size);
	sprintf(myhdr.ar_fmag, "%s", ARFMAG);

    myhdr.ar_name[strlen(fname)] = '/';
    
        
    if( (write(fd, (char *)&myhdr, HDR_LEN))==-1 ){
        perror(" Error: append(): add_file2ar(): write()-1: unable to write to ar file.\n");
        close(fd);
        close(fd2);
        exit(EXIT_FAILURE);
    }
    // if it is a symbolic link, only for append_dir
    if(fd2==-2){
        //printf("  There is a link %s:", fname);
        if ((lr = readlink(fname, buf, sizeof(buf)-1)) == -1)
        {
            printf(" Error: append_dir(): readlink(): read link failed!\n");
            exit(EXIT_FAILURE);
        }
        lw = write(fd, buf, lr);
        if(lw % 2 == 1){
            if(write(fd, "\n", 1)==-1){
                perror("  Error: append(): add_file2ar()-3: writing file");
                close(fd);
                exit(EXIT_FAILURE);
            }
        }
        return(0);
    }
    
    while (len_read < statbuf.st_size) {
        lr = read(fd2, buf, BLOCK_SIZE);
        lw = write(fd, buf, lr);
        if (lr != lw) {
            perror("  Error: append(): add_file2ar()-2: writing file");
            close(fd);
            close(fd2);
            exit(EXIT_FAILURE);
        }
        len_read += lr;
    }
    if(len_read % 2 == 1){
        if(write(fd, "\n", 1)==-1){
            perror("  Error: append(): add_file2ar()-3: writing file");
            close(fd);
            close(fd2);
            exit(EXIT_FAILURE);
        }
    }
    
    return(0);
}

// 'x' : extract named files; 'xo': extract named files restoring mtime
int extract_files (int argc, char **argv, int xo)
{
    struct ar_hdr myheader;
    char arMagStr[SARMAG];
    int len_read = 0;
    int fd = open(argv[2], O_RDONLY);
    int fdnew, mode, fsize, read_len = 0, offset=0;
    int fnum_left = argc - 3;
    short ex_flag = 0, j, file_x_flag[fnum_left]; 
    struct stat statbuf;
    struct utimbuf newt;
    for(j=0; j<fnum_left; j++) file_x_flag[j]=0; 
    
    if (fd == -1)
        arc_read_error(argv[2]);
    lseek(fd, 0, SEEK_SET);
    len_read = read(fd, arMagStr, SARMAG);
    check_mag_str(arMagStr, len_read);
    lseek(fd, SARMAG, SEEK_SET);

    while ( read(fd, (char *) &myheader, HDR_LEN )==HDR_LEN 
            && (fnum_left > 0 || argc==3) ){
        if(strncmp(myheader.ar_fmag, ARFMAG, 2) != 0){
            printf("  Error: extract(): wrong header!\n");
            exit(EXIT_FAILURE);
        }
        fname2str(myheader.ar_name);
        ex_flag = 0;
        if(argc==3)
            ex_flag = 1;
        else{
            for (j = 0; j < argc-3; j++ )
            {
                if(strcmp(myheader.ar_name, argv[3+j])==0)
                {
                    if( file_x_flag[j]==0 ){
                        ex_flag = 1;
                        file_x_flag[j]=1;
                        break;
                    }
                }
            }
        }
        fsize = atoi(myheader.ar_size);
        offset = fsize % 2;
        if(ex_flag==1){ // 'x': extract named file
            sscanf(myheader.ar_mode, "%o", &mode);
            fdnew = creat(myheader.ar_name, mode);
            if (fdnew == -1) {
                printf("  Error: extract(): cannot create new files!\n");
                exit(EXIT_FAILURE);
            } 
            while(fsize > 0){
                char buf[fsize];
                read_len = read(fd, buf, fsize);
                write(fdnew, buf, read_len);
                fsize -= read_len;
            }
            close(fdnew);
            //printf("  file: %s extracted\n", myheader.ar_name);
            lseek(fd, offset, SEEK_CUR);
            fnum_left--;
        }
        else if(ex_flag==0){
            lseek(fd, fsize+offset, SEEK_CUR);
        }
        if(xo==1 && ex_flag==1){
            if( stat(myheader.ar_name, &statbuf)==-1 ){
                perror("  Error: extract(): stat()\n");
                exit(EXIT_FAILURE);
            }
            time_t modtime = atoi(myheader.ar_date); 
            newt.actime = modtime;
            newt.modtime= modtime;
            if (utime(myheader.ar_name, &newt) == -1) {
                perror("  Error: extract(): utime()");
                exit(1);
            }
        }
    }
    for(j=0; j<argc-3; j++){
        if(file_x_flag[j]==0){
            printf("  no entry \"%s\" in archive!\n", argv[j+3]);
        }
    }
    
    return(0);
}

// 't'/'tv' : print a concise/verbose table of contents of the archive
int print_table (int argc, char **argv, int tv)
{ // add ./myar t arc one two nofile
    struct ar_hdr myheader;
    char arMagStr[SARMAG];
    int len_read = 0;
    int fd = open(argv[2], O_RDONLY);
    time_t fdate;
    int fuid, fgid, fmode, fsize;     
    struct tm * ftime;
    char fdatetime[30];
    if (fd == -1)
        arc_read_error(argv[2]);
    lseek(fd, 0, SEEK_SET);
    len_read = read(fd, arMagStr, SARMAG);
    check_mag_str(arMagStr, len_read);
    lseek(fd, SARMAG, SEEK_SET);
    while ( read(fd, (char *) &myheader, HDR_LEN ) == HDR_LEN ){
        if(strncmp(myheader.ar_fmag, ARFMAG, 2) != 0){
            printf("  Error: print_table(): wrong header!\n");
            exit(EXIT_FAILURE);
        }
        else{ //    int fdate,fuid, fgid, fmode, fsize;   
            fname2str(myheader.ar_name);  // convert name to a str; replace '\' with '\0'
            if(tv == 0){
                printf("  %s\n", myheader.ar_name);
                sscanf(myheader.ar_size, "%d", &fsize);

            }
            else{
                fdate = atoi(myheader.ar_date);
                sscanf(myheader.ar_uid,  "%d", &fuid);
                sscanf(myheader.ar_gid,  "%d", &fgid);
                sscanf(myheader.ar_mode, "%o", &fmode);
                sscanf(myheader.ar_size, "%d", &fsize);
                ftime = localtime(&fdate);
                strftime(fdatetime, 30, "%b %d %R %Y", ftime);
                printf("  ");
                print_mode(fmode);
                printf(" %-d/%-d %6d %s %s\n", fuid, fgid, fsize, fdatetime, myheader.ar_name);
            }
        }
        //printf("    ar_size: %s\n", myheader.ar_size); //del
        //fsize = atoi(myheader.ar_size);
        fsize += (fsize%2);
        //printf("    fsize : %d\n", fsize); //del
        lseek(fd, fsize, SEEK_CUR);
    }

    
    return(0);
}

// 'd' : delete named files from archive
int delete_files (int argc, char **argv)
{
    if(argc<4){
        printf("  No files specified to delete.\n");
        exit(EXIT_SUCCESS);
    }
    struct ar_hdr myheader;
    char arMagStr[SARMAG], fname[MAX_FILE_NAME];
    int len_read = 0;
    int fd = open(argv[2], O_RDONLY);
    int fdnew, fsize, read_len = 0, offset=0;
    int fnum_left = argc - 3;
    short ex_flag = 0, j, file_x_flag[fnum_left]; 
    struct stat statbuf;
    //struct utimbuf newt;
    for(j=0; j<fnum_left; j++) file_x_flag[j]=0; 
    
    if (fd == -1)
        arc_read_error(argv[2]);
    lseek(fd, 0, SEEK_SET);
    len_read = read(fd, arMagStr, SARMAG);
    check_mag_str(arMagStr, len_read);
    lseek(fd, SARMAG, SEEK_SET);
    if( stat(argv[2], &statbuf)==-1 ){
        perror("  Error: delete(): stat()\n");
        exit(EXIT_FAILURE);
    }
    unlink(argv[2]);
    fdnew = creat(argv[2], statbuf.st_mode);
    if (fdnew == -1) {
        printf("  Error: delete(): cannot create new files!\n");
        exit(EXIT_FAILURE);
    }
    write(fdnew, arMagStr, SARMAG);
    
    while ( read(fd, (char *) &myheader, HDR_LEN )==HDR_LEN ) {
        if(strncmp(myheader.ar_fmag, ARFMAG, 2) != 0){
            printf("  Error: delete(): wrong header!\n");
            exit(EXIT_FAILURE);
        }
        fnamecpy(myheader.ar_name, fname);
        ex_flag = 1;
        for (j = 0; j < argc-3; j++ ) {
            if(strcmp(fname, argv[3+j])==0){
                if( file_x_flag[j]==0 ){
                    ex_flag = 0;
                    file_x_flag[j]=1;
                    fnum_left--;
                    break;
                }
            }
        }
        fsize = atoi(myheader.ar_size);
        offset = fsize % 2;
        fsize += offset; 
        if( ex_flag==1 ){ 
            write(fdnew, (char *) &myheader, HDR_LEN);
            while(fsize > 0){
                char buf[fsize];
                read_len = read(fd, buf, fsize);
                write(fdnew, buf, read_len);
                fsize -= read_len;
            }
            //printf("  file: %s transfered\n", fname); //del
        }
        else{
            lseek(fd, fsize, SEEK_CUR);
        }
    }
    close(fdnew);
    close(fd);
    for(j=0; j<argc-3; j++){
        if(file_x_flag[j]==0){
            printf("  no entry \"%s\" in archive!\n", argv[j+3]);
        }
    }
    
    return(0);
}

// 'A' : quickly append all 'regular' files in the current directory
int append_dir (int argc, char **argv)
{
    DIR *dirp;
    struct dirent *dp;
    if ((dirp = opendir(".")) == NULL) {
        perror(" Error: append_dir(): couldn't open current directory '.'\n");
        exit(EXIT_FAILURE);
    }
    
    char arMagStr[SARMAG];
    int len_read = 0;
    int fd = open(argv[2], O_RDWR | O_CREAT | O_APPEND, 0666);
    int fd2;
    struct stat statbuf;
    if (fd == -1)
        arc_read_error(argv[2]);
    umask(S_IWGRP | S_IWOTH);
    lseek(fd, 0, SEEK_SET);
    len_read = read(fd, arMagStr, SARMAG);
    lseek(fd, 0, SEEK_SET);
    if (len_read < 0) {
        printf("  myar: %s: Error while reading the file.\n", argv[2]);
        exit(EXIT_FAILURE);
    } 
    else if (len_read > 0)
        check_mag_str(arMagStr, len_read);
    else
        write(fd, ARMAG, SARMAG);
    lseek(fd, SARMAG, SEEK_SET);
    if(argc>3){
        printf("  Info: key 'A' will append all files in current directory\n"
               "        to archive. Files after '%s' are omitted!\n", argv[2]);
        close(fd);
        exit(EXIT_SUCCESS);
    }
    do {
        if ((dp = readdir(dirp)) != NULL) {
            if( strcmp(dp->d_name, ".") != 0 && 
                strcmp(dp->d_name, "..")!=0  &&
                strcmp(dp->d_name, argv[2]) != 0){
                
                char *fname = dp->d_name;
                fd2 = open(fname, O_RDONLY);
                if (fd2 < 0) {
                    printf("  myar: %s : No such file or directory\n", fname);
                    printf("  For safety, no files after it are archived!\n");
                    exit(EXIT_FAILURE);
                    close(fd);
                    close(fd2);
                }
                if( stat(fname, &statbuf)==-1 ) {
                    printf("  Error: append_dir(): stat() failed!\n");
                    exit(EXIT_FAILURE);
                }
                if((statbuf.st_mode & S_IFMT)==S_IFLNK || S_ISLNK(statbuf.st_mode)){
                    /* Handle symbolic link */
                    close(fd2);
                    fd2 = -2; // -2 is my self-defined flag for symmbolic link 
                    lstat(fname, &statbuf);
                    printf(" Link file: %s\n", fname);
                    add_file2ar(fd, fd2, fname, statbuf);
                }
                else if((statbuf.st_mode & S_IFMT)==S_IFREG){
                    /* Handle regular file */
                    lseek(fd2, 0, SEEK_SET);
                    add_file2ar(fd, fd2, fname, statbuf);
                }
                else{ /* Error */
                    printf( "  myar: '%s' omitted : Not a regular or symbolic file.\n", fname);
                }
                close(fd2);
            }
        }
    } while (dp != NULL);
    
    (void) closedir(dirp);
    return(0);
}


void usage()
{
    printf("  Usage: ./myar key archive-file file...\n");
    printf("  keys/commands\n");
    printf("    q : quickly append named files to archive\n");
    printf("    x : extract named files\n");
    printf("    xo: extract named files restoring mtime\n");
    printf("    t : print a concise table of contents of the archive\n");
    printf("    tv: print a verbose table of contents of the archive\n");
    printf("    d : delete named files from archive\n");
    printf("    A : quickly append all \"regular\" files in the current directory\n");
}

void wrong_key(char *arg)
{
    printf("  Error: key \"%s\" not available in command myar!\n", arg);
    usage();
    exit(EXIT_FAILURE);  
}

void arc_read_error(char *arc)
{
    printf("  Error: \"%s\": No such file or directory\n", arc);
    exit(EXIT_FAILURE);
}

int fname2str(char *fname)
{
    int i = 0;
    while( i < MAX_FILE_NAME){
        if(fname[i] == '/'){
            fname[i] = '\0';
            return(0);
        }
        i++;
    }
    return(1);
}


int fnamecpy(char *fname, char *fnew)
{
    int i = 0;
    while( i < MAX_FILE_NAME){
        if(fname[i] == '/'){
            fnew[i] = '\0';
            return(0);
        }
        else{
            fnew[i] = fname[i];
        }
        i++;
    }
    return(1);
}

void print_mode(int fmode)
{
    printf( ( fmode & S_IRUSR ) ? "r" : "-");
    printf( ( fmode & S_IWUSR ) ? "w" : "-");
    printf( ( fmode & S_IXUSR ) ? "x" : "-");
    printf( ( fmode & S_IRGRP ) ? "r" : "-");
    printf( ( fmode & S_IWGRP ) ? "w" : "-");
    printf( ( fmode & S_IXGRP ) ? "x" : "-");
    printf( ( fmode & S_IROTH ) ? "r" : "-");
    printf( ( fmode & S_IWOTH ) ? "w" : "-");
    printf( ( fmode & S_IXOTH ) ? "x" : "-");
}
  
void check_mag_str(char *arMagStr, int len_read){
    if (len_read < SARMAG){
        printf("  Error: Magic string: read failure OR not an archive file!\n");
    }
    if(strncmp(arMagStr, ARMAG, SARMAG) != 0)
    {
        printf("  More Info: not an archive file\n  File 'magic' string: ");
        d_printc(arMagStr, SARMAG);
        exit(EXIT_FAILURE);
    }
}

void d_printc(char *s, int l)
{
    int i=0;
    for (i = 0; i < l; i++)
    {
        printf("%c", s[i]);
    }
    printf("\n");
    
}
