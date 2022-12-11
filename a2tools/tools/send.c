/* No more readability now */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>

#define DELAY_MS 3
#define LONG_DELAY_MS 50

/* TODO stty -F /dev/ttyUSB0 9600 cs8 -onlcr */

static void setup_tty(char *ttypath) {
  struct termios tty;
  int port = open(ttypath, O_RDWR);

  if(tcgetattr(port, &tty) != 0) {
    printf("tcgetattr error: %s\n", strerror(errno));
    close(port);
    exit(1);
  }
  cfsetispeed(&tty, B9600);
  cfsetospeed(&tty, B9600);

  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CREAD | CLOCAL;

  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO;
  tty.c_lflag &= ~ISIG;

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

  tty.c_oflag &= ~OPOST;
  tty.c_oflag &= ~ONLCR;

  if (tcsetattr(port, TCSANOW, &tty) != 0) {
    printf("tcgetattr error: %s\n", strerror(errno));
    close(port);
    exit(1);
  }
  close(port);
}

int main(int argc, char **argv) {
  FILE *fp, *outfp;
  char *filename;
  char *filetype;
  char c;
  int count = 0;
  char buf[128];
  struct stat statbuf;

  if (argc < 3) {
    printf("Usage: %s [file] [output tty]\n", argv[0]);
    exit(1);
  }

  if (stat(argv[1], &statbuf) < 0) {
    printf("Can't stat %s\n", argv[1]);
    exit(1);
  }
  
  fp = fopen(argv[1],"r");
  if (fp == NULL) {
    printf("Can't open %s\n", argv[1]);
    exit(1);
  }
  
  filename = basename(argv[1]);
  if (strchr(filename, '.') != NULL) {
    filetype = strchr(filename, '.') + 1;
    *(strchr(filename, '.')) = '\0';
  } else {
    filetype = "TXT";
  }

  setup_tty(argv[2]);

  outfp = fopen(argv[2], "r+b");
  if (outfp == NULL) {
    printf("Can't open %s\n", argv[2]);
    exit(1);
  }

  /* Send filename */
  fprintf(outfp, "%s\n", filename);
  printf("Filename sent:    %s\n", filename);
  usleep(LONG_DELAY_MS*1000);

  /* Send filetype */
  fprintf(outfp, "%s\n", filetype);
  printf("Filetype sent:    %s\n", filetype);
  usleep(LONG_DELAY_MS*1000);

  if (!strcasecmp(filetype, "BIN")) {
    char buf[58];
    fread(buf, 1, 58, fp);
    if (buf[0] == 0x00 && buf[1] == 0x05
     && buf[2] == 0x16 && buf[3] == 0x00) {
      printf("AppleSingle file detected, skipping header\n");
    } else {
      rewind(fp);
    }
  }

  /* Send data length */
  fprintf(outfp, "%ld\n", statbuf.st_size - ftell(fp));
  printf("Data length sent: %ld\n", statbuf.st_size - ftell(fp));
  usleep(LONG_DELAY_MS*1000);

  while(fread(&c, 1, 1, fp) > 0) {
    fwrite(&c, 1, 1, outfp);
    fflush(outfp);
    count++;

    if (count % 512 == 0) {
      printf("Wrote %d bytes...\n", count);
    }
    usleep(DELAY_MS*1000);
  }

  fclose(fp);
  fclose(outfp);
  printf("Wrote %d bytes.\n", count);
  exit(0);
}
