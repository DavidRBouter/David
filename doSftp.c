#include "doSftp.h"
/*
 * This program does currently NOT make a safe connection. Passwords are send in plain tekst and 
 * can be intercepted. Only use for non-sensitive data.
 */

#define MAX_XFER_BUF_SIZE 16384
#define PATH_MAX 128

int sftp_read_sync(ssh_session session, sftp_session sftp, char fileNum[3])
{
  char basePathSrc[PATH_MAX] = "/home/pi/Documents/Python_programs/dst/";
  char basePathDst[PATH_MAX] = "./";
  char fileNameSrc [10] = "img";
  char fileExt[5] = ".jpg";
  char fileNameDst[10] = "test";
  char *ptr = basePathSrc;
  strcat(basePathSrc, fileNameSrc);
  strcat(basePathSrc, fileNum);
  strcat(basePathSrc, fileExt);
  strcat(basePathDst, fileNameDst);
  strcat(basePathDst, fileNum);
  strcat(basePathDst, fileExt);
while(*ptr != '\0') {
    printf("%c", *ptr);    
    ptr++;
  }
ptr = basePathDst;
printf("\n");
while(*ptr != '\0') {
    printf("%c", *ptr);    
    ptr++;
  }
printf("\n");
  int access_type;
  sftp_file file;
  char buffer[MAX_XFER_BUF_SIZE];
  int nbytes, nwritten, rc;
  int fd;
  access_type = O_RDONLY;
  file = sftp_open(sftp, basePathSrc,
                   access_type, 0);//TODO make 2nd argument variable (const char*)
  if (file == NULL) {
      fprintf(stderr, "Can't open file for reading: %s\n",
              ssh_get_error(session));
      return SSH_ERROR;
  }
  fd = open(basePathDst, O_CREAT | O_WRONLY, 0777);
  if (fd < 0) {
      fprintf(stderr, "Can't open file for writing: %s\n",
              strerror(errno));
      return SSH_ERROR;
  }
  for (;;) {
      nbytes = sftp_read(file, buffer, sizeof(buffer));
      if (nbytes == 0) {
          break; // EOF
      } else if (nbytes < 0) {
          fprintf(stderr, "Error while reading file: %s\n",
                  ssh_get_error(session));
          sftp_close(file);
          return SSH_ERROR;
      }
      nwritten = write(fd, buffer, nbytes);
      if (nwritten != nbytes) {
          fprintf(stderr, "Error writing: %s %i %i %i\n",
                  strerror(errno), nbytes, nwritten, fd);
          sftp_close(file);
          return SSH_ERROR;
      }
  }
  rc = sftp_close(file);
  if (rc != SSH_OK) {
      fprintf(stderr, "Can't close the read file: %s\n",
              ssh_get_error(session));
      return rc;
  }
  return SSH_OK;
}

char *my_itoa(int num, char *str)
{
        if(str == NULL)
        {
                return NULL;
        }
        sprintf(str, "%d", num);
        return str;
}

int sftp_start_session(ssh_session session)
{
  const int imagesToImport = 10;
  char buf [3];
  sftp_session sftp;
  int rc;
  sftp = sftp_new(session);
  if (sftp == NULL)
  {
    fprintf(stderr, "Error allocating SFTP session: %s\n",
            ssh_get_error(session));
    return SSH_ERROR;
  }
  rc = sftp_init(sftp);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Error initializing SFTP session: code %d.\n",
            sftp_get_error(sftp));
    sftp_free(sftp);
    return rc;
  }
/*
  if(sftp_helloworld2(session, sftp) < 0){
    fprintf(stderr, "SFTP session failed: %s\n",
              ssh_get_error(session));
  }
  if(sftp_helloworld(session, sftp) < 0){
    fprintf(stderr, "SFTP session failed: %s\n",
              ssh_get_error(session));
  }
*/
  for (int i = 1; i < imagesToImport + 1; i++){
    if(i > 999){
      fprintf(stderr, "Max amount of images (999) reached. Ending session... \n");
      break;
    }
    my_itoa(i,buf);
    if(sftp_read_sync(session, sftp, buf) < 0){
      fprintf(stderr, "SFTP session failed: %s\n",
                ssh_get_error(session));
    }
  }
  sftp_free(sftp);
  return SSH_OK;
}

int show_remote_processes(ssh_session session)
{
  ssh_channel channel;
  int rc;
  char buffer[256];
  int nbytes;
  channel = ssh_channel_new(session);
  if (channel == NULL)
    return SSH_ERROR;
  rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK)
  {
    ssh_channel_free(channel);
    return rc;
  }
  rc = ssh_channel_request_exec(channel, "sudo python Documents/Python_programs/Flash_foto.py");//file location + command TODO sudo?
  //rc = ssh_channel_request_exec(channel, "ls ");//TEST
  if (rc != SSH_OK)
  {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return rc;
  }
  nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  while (nbytes > 0)
  {
    if (write(1, buffer, nbytes) != (unsigned int) nbytes)
    {
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      return SSH_ERROR;
    }
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  }
  if (nbytes < 0)
  {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return SSH_ERROR;
  }

  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);
  return SSH_OK;
}

int main(){
  ssh_session my_ssh_session;
  int rc;
  char *password;
  int verbosity = SSH_LOG_PROTOCOL;
  int port = 22;
  // Open session and set options
  my_ssh_session = ssh_new();
  if (my_ssh_session == NULL)
    exit(-1);
  ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, "pi@192.168.90.20");
  //ssh_options_set(my_ssh_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
  ssh_options_set(my_ssh_session, SSH_OPTIONS_PORT, &port);
  // Connect to server
  rc = ssh_connect(my_ssh_session);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Error connecting to localhost: %s\n",
            ssh_get_error(my_ssh_session));
    ssh_free(my_ssh_session);
    exit(-1);

  }

  // Authenticate ourselves
  // FOR CUSTOM PASSWORDS //password = getpass("Password: ");
  password = "Divad4149";
  rc = ssh_userauth_password(my_ssh_session, NULL, password);
  if (rc != SSH_AUTH_SUCCESS)
  {
    fprintf(stderr, "Error authenticating with password: %s\n",
            ssh_get_error(my_ssh_session));
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    exit(-1);
  }
  if(show_remote_processes(my_ssh_session) < 0){
   fprintf(stderr, "Error, connection closed incorrectly: %s\n",
      ssh_get_error(my_ssh_session));
   }
  if(sftp_start_session(my_ssh_session) < 0){
     fprintf(stderr, "Error, connection closed incorrectly: %s\n",
      ssh_get_error(my_ssh_session));
  }  
   ssh_disconnect(my_ssh_session);
   ssh_free(my_ssh_session);

   return 0;
}





/*
while(*ptr != '\0') {
    printf("%c", *ptr);    
    ptr++;
  }
  ptr = str;
  strcpy(str, basePath);
  strcat(str, dirPath);
while(*ptr != '\0') {
    printf("%c", *ptr);    
    ptr++;
  }
*/

/*


  char str[PATH_MAX] = "home/libs/yes/kangebeuren\n";
  char basePath [10] = "hallo";
  char dirPath [10] = "wereld";
  char *ptr = str;

while(*ptr != '\0') {
    printf("%c", *ptr);    
    ptr++;
  }
  ptr = str;
  strcpy(str, basePath);
  strcat(str, dirPath);
while(*ptr != '\0') {
    printf("%c", *ptr);    
    ptr++;
  }
*/
