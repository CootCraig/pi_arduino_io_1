/* vim: set ft=cpp: */
/*
 * Test Serial IO between Raspberry Pi and Arduino
 *
 * This sketch writes to the serial port on a 5 second timer
 * prefix 99
 * The 99 is actually a count of how many writes have been done since the
 * prefix was changed.
 *
 * Sending "different prefix<MESSAGE_TERMINATOR_CHAR>" to the Arduio will change the timer output to
 * different prefix 1
 *
 */
#include <stdio.h>
#include <string.h>

#include <SimpleTimer.h>

// the timer object
SimpleTimer timer;

#define MESSAGE_START_CHAR '\f'
#define MESSAGE_TERMINATOR_CHAR '\v'

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
      cr_ptr = (char *)memchr(cmd_buf,MESSAGE_TERMINATOR_CHAR,IO_BUF_SIZE);
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
// A command is available if a MESSAGE_TERMINATOR_CHAR is in the buf
bool fifoCmdBufCommandAvailable(char * buf, int size)
{
  bool available = false;
  char * cr_ptr = NULL;
  cr_ptr = (char *)memchr(buf,MESSAGE_TERMINATOR_CHAR,(size-1));
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
  int cmd_prefix_len = 0;
  bool start_char_seen = 0;
  char * term_ptr = NULL;
  char xfer_char = '\0';

  memset(dst_buf,0,dst_size);
  term_ptr = (char *)memchr(src_buf,MESSAGE_TERMINATOR_CHAR,(src_size-1));
  if (term_ptr)
  {
    cmd_prefix_len = (term_ptr-src_buf) + 1;
    for (int ii=0; ii<cmd_prefix_len; ++ii)
    {
      xfer_char = fifoCmdBufDequeueChar(src_buf,src_size);
      if (xfer_char == MESSAGE_START_CHAR)
      {
        start_char_seen = 1;
        // handle multiple MESSAGE_START_CHAR
        // by clearing any non-NULL characters at start
        for (int clear_i=0; dst_buf[clear_i] && (clear_i<dst_size); ++clear_i)
        {
          dst_buf[clear_i] = '\0';
        }
        continue;
      }
      else if (start_char_seen)
      {
        fifoCmdBufEnqueueChar(dst_buf, dst_size, xfer_char);
      }
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
  cr_ptr = (char *)memchr(src_buf,MESSAGE_TERMINATOR_CHAR,(src_size-1));
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
  int timer_write_len = 0;
  char prefix_write_count_buf[80]; // ' ' + prefix_write_count
  int prefix_write_count_buf_len = 0;

  ++prefix_write_count;

  // Buffer up the prefix_write_count
  memset(prefix_write_count_buf,0,sizeof(prefix_write_count_buf));
  sprintf(prefix_write_count_buf," %d",prefix_write_count);

  // Clear the message buffer
  memset(timer_write_buf,0,sizeof(timer_write_buf));

  // Start with MESSAGE_START_CHAR
  fifoCmdBufEnqueueChar(timer_write_buf, sizeof(timer_write_buf), MESSAGE_START_CHAR); 

  // Add the prefix_string
  prefix_len = strlen(prefix_string);
  fifoCmdBufEnqueueString(prefix_string,prefix_len, timer_write_buf, sizeof(timer_write_buf));

  // Add the prefix_write_count
  prefix_write_count_buf_len = strlen(prefix_write_count_buf);
  fifoCmdBufEnqueueString(prefix_write_count_buf,sizeof(prefix_write_count_buf), timer_write_buf, sizeof(timer_write_buf));

  // End with MESSAGE_TERMINATOR_CHAR
  fifoCmdBufEnqueueChar(timer_write_buf, sizeof(timer_write_buf), MESSAGE_TERMINATOR_CHAR); 

  // And queue up the message to be sent to the host
  timer_write_len = strlen(timer_write_buf);
  fifoCmdBufEnqueueString(timer_write_buf,timer_write_len,output_buf,IO_BUF_SIZE);
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

