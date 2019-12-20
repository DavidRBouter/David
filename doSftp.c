#include "doSftp.h"
/*
 * This program does currently NOT make a safe connection. Passwords are send in plain tekst and 
 * can be intercepted. Only use for non-sensitive data.
 */

#define MAX_XFER_BUF_SIZE 16384

int sftp_helloworld2(ssh_session session, sftp_session sftp)
{
  int access_type = O_WRONLY | O_CREAT | O_TRUNC;
  sftp_file file;
  const char *helloworld = "Hello, World!\n";
  int length = strlen(helloworld);
  int rc, nwritten;
  file = sftp_open(sftp, "/home/pi/Documents/Python_programs/helloworld.txt",
                   access_type, S_IRWXU);
  if (file == NULL)
  {
    fprintf(stderr, "Can't open file for writing: %s\n",
            ssh_get_error(session));
    return SSH_ERROR;
  }
  nwritten = sftp_write(file, helloworld, length);
  if (nwritten != length)
  {
    fprintf(stderr, "Can't write data to file: %s\n",
            ssh_get_error(session));
    sftp_close(file);
    return SSH_ERROR;
  }
  rc = sftp_close(file);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Can't close the written file: %s\n",
            ssh_get_error(session));
    return rc;
  }
  return SSH_OK;
}

int sftp_read_sync(ssh_session session, sftp_session sftp)
{
  int access_type;
  sftp_file file;
  char buffer[MAX_XFER_BUF_SIZE];
  int nbytes, nwritten, rc;
  int fd;
  access_type = O_RDONLY;
  file = sftp_open(sftp, "/home/pi/Documents/Python_programs/dst/img1.jpg",
                   access_type, 0);
  if (file == NULL) {
      fprintf(stderr, "Can't open file for reading: %s\n",
              ssh_get_error(session));
      return SSH_ERROR;
  }
  fd = open("./Ckc_3D_Fototest.jpg", O_CREAT | O_WRONLY, 0777);
  if (fd < 0) {
      fprintf(stderr, "Can't open file for writing: %s\n",
              strerror(errno));
      return SSH_ERROR;
  }
  for (;;) {
      nbytes = sftp_read(file, buffer, sizeof(buffer));
      fprintf(stderr, "test %i\n",nbytes);
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

int sftp_helloworld(ssh_session session, sftp_session sftp)
{
  sftp_dir dir;
  sftp_attributes attributes;
  int rc;
  dir = sftp_opendir(sftp, "/home/pi/Documents/Python_programs");
  if (!dir)
  {
    fprintf(stderr, "Directory not opened: %s\n",
            ssh_get_error(session));
    return SSH_ERROR;
  }
  printf("Name                       Size Perms    Owner\tGroup\n");
  while ((attributes = sftp_readdir(sftp, dir)) != NULL)
  {
    printf("%-20s %10llu %.8o %s(%d)\t%s(%d)\n",
     attributes->name,
     (long long unsigned int) attributes->size,
     attributes->permissions,
     attributes->owner,
     attributes->uid,
     attributes->group,
     attributes->gid);
     sftp_attributes_free(attributes);
  }
  if (!sftp_dir_eof(dir))
  {
    fprintf(stderr, "Can't list directory: %s\n",
            ssh_get_error(session));
    sftp_closedir(dir);
    return SSH_ERROR;
  }
  rc = sftp_closedir(dir);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Can't close directory: %s\n",
            ssh_get_error(session));
    return rc;
  }
}


int sftp_start_session(ssh_session session)
{
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
  if(sftp_helloworld2(session, sftp) < 0){
    fprintf(stderr, "SFTP session failed: %s\n",
              ssh_get_error(session));
  }
  if(sftp_helloworld(session, sftp) < 0){
    fprintf(stderr, "SFTP session failed: %s\n",
              ssh_get_error(session));
  }
  if(sftp_read_sync(session, sftp) < 0){
    fprintf(stderr, "SFTP session failed: %s\n",
              ssh_get_error(session));
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
  ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, "pi@192.168.178.62");
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
