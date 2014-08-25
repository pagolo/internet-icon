#include "findtask.h"

int
find_task (char *inputName)
{
  const char *directory = "/proc";
  size_t taskNameSize = 1024;
  char *taskName = calloc (1, taskNameSize);
  size_t uidSize = 1024;
  char *uidBuffer = calloc (1, uidSize);
  int found = 0;

  DIR *dir = opendir (directory);

  if (inputName == NULL || *inputName == '\0')
    inputName = "internet_icon";
  else
    inputName = basename (inputName);

  if (dir) {
    struct dirent *de = 0;

    while ((de = readdir (dir)) != 0) {
      if (strcmp (de->d_name, ".") == 0 || strcmp (de->d_name, "..") == 0)
        continue;

      int pid = -1;
      int res = sscanf (de->d_name, "%d", &pid);

      if (res == 1) {
        // we have a valid pid

        // open the cmdline file to determine what's the name of the process running
        char cmdline_file[1024] = { 0 };
        sprintf (cmdline_file, "%s/%d/cmdline", directory, pid);

        FILE *cmdline = fopen (cmdline_file, "r");
        if (cmdline == NULL)
          return 0;

        if (getline (&taskName, &taskNameSize, cmdline) > 0) {
          // is it the process we care about?
          if (strstr (taskName, inputName) != 0) {
            char uid_file[1024] = { 0 };

            sprintf (uid_file, "%s/%d/loginuid", directory, pid);

            FILE *uid = fopen (uid_file, "r");
            if (uid == NULL) {
              fclose (cmdline);
              return 0;
            }
            if (getline (&uidBuffer, &uidSize, uid) > 0) {
              if (atoi (uidBuffer) == geteuid ()) {
                printf ("Processo %s, userid %s\n", taskName, uidBuffer);
                ++found;
              }
            }
            fclose (uid);
          }
        }

        fclose (cmdline);
      }
    }

    closedir (dir);
  }

  // just let the OS free this process' memory!
  //free(taskName);

  return (found > 1);
}
