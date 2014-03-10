/* vim: set ft=cpp: */
/*
 *
 *
 */
#include <stdio.h>
#include <string.h>
#include <SimpleTimer.h>

// the timer object
SimpleTimer timer;

#define IO_BUF_SIZE 200
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
  //readInput();
  //doCommand();
}

// Look for a <cr> terminated string at the beginning of input_buf
// If found remove the command string from InputStr
// and execute the command
void doCommand()
{
  int copy_size = 0;
  int cr_pos = -1;
  int null_pos = -1;

  cr_pos = findCharInBuf(input_buf,IO_BUF_SIZE,'\r');
  if (cr_pos >= 0)
  {
    // Only command for now is to use the command string
    // as a timer report prefix
    if (cr_pos >= PREFIX_BUF_SIZE)
    {
      copy_size = PREFIX_BUF_SIZE - 1;
    }
    else
    {
      copy_size = cr_pos;
    }
    memset(prefix_string,0,PREFIX_BUF_SIZE);
    memcpy(prefix_string,(input_buf),copy_size);
    leftShiftBuf(input_buf,IO_BUF_SIZE,(cr_pos+1));
  }
  else
  {
    null_pos = findCharInBuf(input_buf,IO_BUF_SIZE,'\0');
    if ((null_pos+1) >= IO_BUF_SIZE)
    {
      // input_buf is full and no <cr> found
      // fill input_buf with null so more characters can be received
      memset(input_buf,0,IO_BUF_SIZE);
    }
  }
}
// Return the position of a character in a buffer or -1 if not found
// Assume the buffer must always end with a null
// so do not check the last position
int findCharInBuf(char *buf, int buf_size, char char_to_find)
{
  char *foundCharPtr = NULL;
  int foundPos = -1;

  foundCharPtr = (char *) memchr(buf,buf_size,char_to_find);
  if (foundCharPtr)
  {
    foundPos = foundCharPtr - buf;
    if ((foundPos+1) >= buf_size)
    {
      foundPos = -1;
    }
  }
  return foundPos;
}
// Read a character from serial if available and add to input_buf
// if there is room
// input_buf must always end with a null
void readInput()
{
  char input_char;
  int insert_pos = -1;
  if (Serial.available())
  {
    insert_pos = findCharInBuf(input_buf,IO_BUF_SIZE,'\0');
    if (insert_pos >= 0)
    {
      input_buf[insert_pos] = input_char;
    }
  }
}
// Left shift the buf with zero fill
void leftShiftBuf(char *buf,int buf_size,int remove_size)
{
  int clear_pos = 0;
  int move_size = 0;

  if (buf_size > 0)
  {
    if (remove_size >= buf_size)
    {
      memset(buf,0,buf_size);
    }
    else
    {
      move_size = buf_size - remove_size;
      memcpy(buf,(buf+remove_size),move_size);
      clear_pos = buf_size - remove_size;
      memset((buf+clear_pos),0,remove_size);
    }
  }
}

// Write to Serial, called on a timer
// write timer_write_buf and a write count
void timerWrite()
{
  int append_pos_in_output_buf;
  int write_len = 0;
  int space_left_in_output_buf;

  memset(timer_write_buf,0,TIMER_WRITE_BUF_SIZE);
  ++prefix_write_count;
  sprintf(timer_write_buf,"%d\n",prefix_write_count);
  write_len = strlen(timer_write_buf);

  append_pos_in_output_buf = findCharInBuf(output_buf,IO_BUF_SIZE,'\0');

  if (append_pos_in_output_buf >= 0)
  {
    space_left_in_output_buf = IO_BUF_SIZE - append_pos_in_output_buf - 1;
    if (write_len < space_left_in_output_buf) {
      memcpy((output_buf+append_pos_in_output_buf),timer_write_buf,write_len);
    }
  }
}

// Remove the first character in output_buf and write to serial
void writeOutput()
{
  char first_char = output_buf[0];
  if (first_char != '\0')
  {
    Serial.write(output_buf,1);
    leftShiftBuf(output_buf,IO_BUF_SIZE,1);
  }
}

