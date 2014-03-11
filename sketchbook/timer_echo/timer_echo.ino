/* vim: set ft=cpp: */
/*
 * Test Serial IO between Raspberry Pi and Arduino
 *
 * This sketch writes to the serial port on a 5 second timer
 * prefix 99
 * The 99 is actually a count of how many writes have been done since the
 * prefix was changed.
 *
 * Send "different prefix\r" to the Arduio will change the timer output to
 * different prefix 1
 *
 */
#include <stdio.h>
#include <string.h>
#include <SimpleTimer.h>

// the timer object
SimpleTimer timer;

#define IO_BUF_SIZE 200
char cmd_buf[IO_BUF_SIZE];
char input_buf[IO_BUF_SIZE];
char output_buf[IO_BUF_SIZE];

#define PREFIX_BUF_SIZE 80
char prefix_string[PREFIX_BUF_SIZE];
int prefix_write_count = 0;

#define TIMER_WRITE_BUF_SIZE (PREFIX_BUF_SIZE + 16)
char timer_write_buf[TIMER_WRITE_BUF_SIZE];

void setup()
{
  memset(input_buf,0,IO_BUF_SIZE);
  memset(output_buf,0,IO_BUF_SIZE);

  memset(prefix_string,0,PREFIX_BUF_SIZE);
  strcpy(prefix_string,"Not Set");
  prefix_write_count = 0;

  Serial.begin(9600);

  timer.setInterval((5*1000), timerWrite);
}

void loop()
{
  timer.run();
  writeOutput();
  readSerialInput();
  doCommand();
}

// Look for a <cr> terminated string at the beginning of input_buf
// If found remove the command string from InputStr
// and execute the command
void doCommand()
{
  int cmd_len = 0;
  char * cr_ptr = NULL;
  int xfer_len = 0;

  fifoCmdBufDequeueCommand(input_buf,IO_BUF_SIZE,cmd_buf,IO_BUF_SIZE);
  cmd_len = strlen(cmd_buf);
  if (cmd_len > 0)
  {
    if (cmd_len > 1)
    {
      // The only command for now is to change the prefix_string
      cr_ptr = (char *)memchr(cmd_buf,'\r',IO_BUF_SIZE);
      if (cr_ptr)
      {
        *cr_ptr = '\0'; // Do not want <cr> in prefix_string
        xfer_len = strlen(cmd_buf);
        memset(prefix_string,0,PREFIX_BUF_SIZE);
        fifoCmdBufEnqueueString(cmd_buf,xfer_len,prefix_string,PREFIX_BUF_SIZE);
        prefix_write_count = 0;
      }
    }
  }
}

// fifoCmdBuf* functions
// ensure CmdBuf is always null terminated
// so strlen(CmdBuf) will always work
// each cmd is terminated with <cr>
//
// void fifoCmdBufEnqueueChar(char * buf, int size, char add_char)
// void fifoCmdBufClear(char * buf, int size)
// bool fifoCmdBufCommandAvailable(char * buf, int size)
// char fifoCmdBufDequeueChar(char * src_buf, int src_size)
// void fifoCmdBufDequeueCommand(char * src_buf, int src_size, char * dst_buf, int dst_size)
// void fifoCmdBufEnqueueString(char * src_buf, int src_size, char * dst_buf, char * dst_size)
// void fifoCmdBufPeekCommand(char * src_buf, int src_size, char * dst_buf, int dst_size)

void fifoCmdBufClear(char * buf, int size)
{
  memset(buf,0,size);
}
// A command is available if a <cr> is in the buf
bool fifoCmdBufCommandAvailable(char * buf, int size)
{
  bool available = false;
  char * cr_ptr = NULL;
  cr_ptr = (char *)memchr(buf,'\r',(size-1));
  if (cr_ptr)
  {
    available = true;
  }
  return available;
}
char fifoCmdBufDequeueChar(char * src_buf, int src_size)
{
  char first_char = '\0';
  char * zero_fill_ptr = NULL;

  first_char = src_buf[0];
  memcpy(src_buf,(src_buf+1),(src_size-1));
  zero_fill_ptr = src_buf + (src_size-2);
  memset(zero_fill_ptr,0,2);

  return first_char;
}
// dequeue a command from src_buf and put into dst_buf
// destructive of dst_buf
void fifoCmdBufDequeueCommand(char * src_buf, int src_size, char * dst_buf, int dst_size)
{
  int cmd_len = 0;
  char * cr_ptr = NULL;
  char xfer_char = '\0';

  memset(dst_buf,0,dst_size);
  cr_ptr = (char *)memchr(src_buf,'\r',(src_size-1));
  if (cr_ptr)
  {
    cmd_len = (cr_ptr-src_buf) + 1;
    for (int ii=0; ii<cmd_len; ++ii)
    {
      xfer_char = fifoCmdBufDequeueChar(src_buf,src_size);
      fifoCmdBufEnqueueChar(dst_buf, dst_size, xfer_char);
    }
  }
}
// Add a character, make room with a left shift if needed
void fifoCmdBufEnqueueChar(char * buf, int size, char add_char)
{
  char * add_ptr = NULL;
  char * null_ptr = NULL;
  int space_used = 0;

  null_ptr = (char *)memchr(buf,'\0',size);
  if (null_ptr)
  {
    space_used = null_ptr - buf;
  }
  else
  {
    space_used = size;
  }
  if ((space_used+1) >= size)
  {
    // left shift to make room to add
    memcpy(buf,(buf+1),(size-1));
    buf[size-1] = '\0'; // should not be needed, but making sure last char is NULL
    add_ptr = buf + (size-2);
  }
  else
  {
    add_ptr = null_ptr; // already have room to add
  }
  *add_ptr = add_char;
}
void fifoCmdBufEnqueueString(char * src_buf, int src_size, char * dst_buf, int dst_size)
{
  int src_strlen = 0;

  src_strlen = strlen(src_buf);
  for (int ii=0; ii<src_strlen; ++ii)
  {
    fifoCmdBufEnqueueChar(dst_buf, dst_size, src_buf[ii]);
  }
}
void fifoCmdBufPeekCommand(char * src_buf, int src_size, char * dst_buf, int dst_size)
{
  int cmd_len = 0;
  char * cr_ptr = NULL;
  char xfer_char = '\0';

  memset(dst_buf,0,dst_size);
  cr_ptr = (char *)memchr(src_buf,'\r',(src_size-1));
  if (cr_ptr)
  {
    cmd_len = (cr_ptr-src_buf) + 1;
    for (int ii=0; ii<cmd_len; ++ii)
    {
      xfer_char = src_buf[ii];
      fifoCmdBufEnqueueChar(dst_buf, dst_size, xfer_char);
    }
  }
}
void readSerialInput(void)
{
  while (Serial.available()) {
    char inChar = (char)Serial.read(); 

    fifoCmdBufEnqueueChar(input_buf, IO_BUF_SIZE, inChar);
  }
}
// Periodic write to Serial to test RPi <-> Arduino
void timerWrite()
{
  int prefix_len = 0;
  char write_buf[80];
  int write_buf_len = 0;

  ++prefix_write_count;
  memset(write_buf,0,sizeof(write_buf));
  sprintf(write_buf," %d\r",prefix_write_count);

  prefix_len = strlen(prefix_string);
  fifoCmdBufEnqueueString(prefix_string,prefix_len,output_buf,IO_BUF_SIZE);
  write_buf_len = strlen(write_buf);
  fifoCmdBufEnqueueString(write_buf,write_buf_len,output_buf,IO_BUF_SIZE);
}

// Remove the first character in output_buf and write to serial
void writeOutput()
{
  char first_char = '\0';
  first_char = fifoCmdBufDequeueChar(output_buf, IO_BUF_SIZE);
  if (first_char != '\0')
  {
    Serial.write(first_char);
  }
}

