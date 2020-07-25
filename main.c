/*-------------------------------------------------------------------------
  Prints instruction size based on the instruction size calculator
   of SDCC's peep hole optimizer

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  In other words, you are welcome to use, share and improve this program.
  You are forbidden to forbid anyone else to use, share and improve
  what you give them.   Help stamp out software-hoarding!
-------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

// simplified of sdcc's struct from SDCCicode.h
typedef struct iCode{
  int lineno;                   /* file & lineno for debug information */
  char *filename;
}iCode;

// simplified of sdcc's struct from SDCCgen.h
typedef struct lineNode_s{
  char *line;
  iCode *ic;
  unsigned int isInline:1;
  unsigned int isComment:1;
  unsigned int isDebug:1;
  unsigned int isLabel:1;
  unsigned int visited:1;
}lineNode;

int z80instructionSize(lineNode *pl);
const char * StrStr (const char * str1, const char * str2);

int main(int argc, char *argv[]){
  if(argc != 2){
    printf("Give .asm file as first parameter\n");
    return 1;
  }
  char ch, file_name[25];
  FILE *fp;
  char * line = NULL;
  iCode ic = {-1, argv[1]};
  lineNode ln = {NULL, &ic, false, false, false, false, false};

  int bufferLength = 255;
  char buffer[bufferLength];

  fp = fopen(argv[1], "r"); // read mode

  if (fp == NULL){
    perror("Error while opening the file.\n");
    exit(EXIT_FAILURE);
  }

  printf("%s:\n", argv[1]);

  while(fgets(buffer, bufferLength, fp)) {
    // ends with a newline, remove that
    for(line = buffer; *line; ++line){
      if(*line == '\n' || *line == '\r'){
        *line = '\0';
      }
      // simplify padding
      if(*line == '\t'){
        *line = ' ';
      }
    }

    printf("%-48s", buffer);

    ++ic.lineno;
    ln.isComment = (buffer[0] == ';');
    ln.isLabel = (!isspace(buffer[0]));
    ln.line = buffer;
    // skip spaces
    while (isspace (*ln.line))
      ++ln.line;
    //ln.isDebug = (*ln.line == '.');
    if(*ln.line == '\0' || *ln.line == ';')
      ln.isComment = true;
    if (ln.line &&
        !ln.isComment &&
        !ln.isLabel &&
        !ln.isDebug){
      printf(" ; %db", z80instructionSize(&ln));
    }
    printf("\n");
  }

  fclose(fp);
  return 0;
}

#define IS_GB true
#define TARGET_IS_TLCS90 false
#define IS_RAB false
#define IS_EZ80_Z80 false
#define IS_Z80N false
#define IS_TLCS90 false
#define IS_Z180 false
#define IS_R3KA false

#define STRCASECMP   strcasecmp
#define STRNCASECMP  strncasecmp
#define FALSE false
#define wassert(x)
#define werrorfl(a, b, c, d, e, f)

// following code is taken from sdcc src/SDCCpeeph.c
/*-----------------------------------------------------------------*/
/* StrStr - case-insensitive strstr implementation                 */
/*-----------------------------------------------------------------*/
const char * StrStr (const char * str1, const char * str2)
{
  const char * cp = str1;
  const char * s1;
  const char * s2;

  if ( !*str2 )
    return str1;

  while (*cp)
  {
    s1 = cp;
    s2 = str2;

    while ( *s1 && *s2 && !(tolower(*s1)-tolower(*s2)) )
      s1++, s2++;

    if (!*s2)
      return( cp );

    cp++;
  }

  return (NULL) ;
}

// following code is taken from sdcc src/z80/peep.c

#define ISINST(l, i) (!STRNCASECMP((l), (i), sizeof(i) - 1) && (!(l)[sizeof(i) - 1] || isspace((unsigned char)((l)[sizeof(i) - 1]))))

/* Check if reading arg implies reading what. */
static bool argCont(const char *arg, const char *what)
{
  wassert (arg);

  while(isspace (*arg) || *arg == ',')
    arg++;

  if (arg[0] == '#')
    return false;

  if(arg[0] == '(' && arg[1] && arg[2] && (arg[2] != ')' && arg[3] != ')')
     && !(IS_GB && (arg[3] == '-' || arg[3] == '+') && arg[4] == ')'))
    return FALSE;

  if(*arg == '(')
    arg++;

  // Get suitable end to avoid reading into further arguments.
  const char *end = strchr(arg, ',');
  if (!end)
    end = arg + strlen(arg);

  const char *found = StrStr(arg, what);

  return(found && found < end);
}

int z80instructionSize(lineNode *pl)
{
  const char *op1start, *op2start;

  /* move to the first operand:
   * leading spaces are already removed, skip the mnemonic */
  for (op1start = pl->line; *op1start && !isspace (*op1start); ++op1start);

  /* skip the spaces between mnemonic and the operand */
  while (isspace (*op1start))
    ++op1start;
  if (!(*op1start))
    op1start = NULL;

  if (op1start)
    {
      /* move to the second operand:
       * find the comma and skip the following spaces */
      op2start = strchr(op1start, ',');
      if (op2start)
        {
          do
            ++op2start;
          while (isspace (*op2start));

          if ('\0' == *op2start)
            op2start = NULL;
        }
    }
  else
    op2start = NULL;

  if(TARGET_IS_TLCS90) // Todo: More accurate estimate.
    return(6);

  /* All ld instructions */
  if(ISINST(pl->line, "ld"))
    {
      /* These 4 are the only cases of 4 byte long ld instructions. */
      if(!STRNCASECMP(op1start, "ix", 2) || !STRNCASECMP(op1start, "iy", 2))
        return(4);
      if((argCont(op1start, "(ix)") || argCont(op1start, "(iy)")) && op2start[0] == '#')
        return(4);

      if(op1start[0] == '('               && STRNCASECMP(op1start, "(bc)", 4) &&
         STRNCASECMP(op1start, "(de)", 4) && STRNCASECMP(op1start, "(hl" , 3) &&
         STRNCASECMP(op2start, "hl", 2)   && STRNCASECMP(op2start, "a", 1)   &&
         (!IS_GB || STRNCASECMP(op2start, "sp", 2)) ||
         op2start[0] == '('               && STRNCASECMP(op2start, "(bc)", 4) &&
         STRNCASECMP(op1start, "(de)", 4) && STRNCASECMP(op2start, "(hl" , 3) &&
         STRNCASECMP(op1start, "hl", 2)   && STRNCASECMP(op1start, "a", 1))
        return(4);

      /* Rabbit 16-bit pointer load */
      if(IS_RAB && !STRNCASECMP(op1start, "hl", 2) && (argCont(op2start, "(hl)") || argCont(op2start, "(iy)")))
        return(4);
      if(IS_RAB && !STRNCASECMP(op1start, "hl", 2) && (argCont(op2start, "(sp)") || argCont(op2start, "(ix)")))
        return(3);

      if(IS_EZ80_Z80 && /* eZ80 16-bit pointer load */
        (!STRNCASECMP(op1start, "bc", 2) || !STRNCASECMP(op1start, "de", 2) || !STRNCASECMP(op1start, "hl", 2) || !STRNCASECMP(op1start, "ix", 2) || !STRNCASECMP(op1start, "iy", 2)))
        {
          if (!STRNCASECMP(op2start, "(hl)", 4))
            return(2);
          if (argCont(op2start, "(ix)") || argCont(op2start, "(iy)"))
            return(3);
        }

      /* These 4 are the only remaining cases of 3 byte long ld instructions. */
      if(argCont(op2start, "(ix)") || argCont(op2start, "(iy)"))
        return(3);
      if(argCont(op1start, "(ix)") || argCont(op1start, "(iy)"))
        return(3);
      if((op1start[0] == '(' && STRNCASECMP(op1start, "(bc)", 4) && STRNCASECMP(op1start, "(de)", 4) && STRNCASECMP(op1start, "(hl", 3)) ||
         (op2start[0] == '(' && STRNCASECMP(op2start, "(bc)", 4) && STRNCASECMP(op2start, "(de)", 4) && STRNCASECMP(op2start, "(hl", 3)))
        return(3);
      if(op2start[0] == '#' &&
         (!STRNCASECMP(op1start, "bc", 2) || !STRNCASECMP(op1start, "de", 2) || !STRNCASECMP(op1start, "hl", 2) || !STRNCASECMP(op1start, "sp", 2)))
        return(3);

      /* These 3 are the only remaining cases of 2 byte long ld instructions. */
      if(op2start[0] == '#')
        return(2);
      if(!STRNCASECMP(op1start, "i", 1) || !STRNCASECMP(op1start, "r", 1) ||
         !STRNCASECMP(op2start, "i", 1) || !STRNCASECMP(op2start, "r", 1))
        return(2);
      if(!STRNCASECMP(op2start, "ix", 2) || !STRNCASECMP(op2start, "iy", 2))
        return(2);

      /* All other ld instructions */
      return(1);
    }

  // load from sp with offset
  if(IS_GB && (ISINST(pl->line, "lda") || ISINST(pl->line, "ldhl")))
    {
      return(2);
    }
  // load from/to 0xffXX addresses
  if(IS_GB && (ISINST(pl->line, "ldh")))
    {
      if(STRNCASECMP(pl->line, "(c)", 3))
        return(1);
      return(2);
    }

  /* Exchange */
  if(ISINST(pl->line, "exx"))
    return(1);
  if(ISINST(pl->line, "ex"))
    {
      if(!op2start)
        {
          werrorfl(pl->ic->filename, pl->ic->lineno, W_UNRECOGNIZED_ASM, __FUNCTION__, 4, pl->line);
          return(4);
        }
      if(argCont(op1start, "(sp)") && (IS_RAB || !STRNCASECMP(op2start, "ix", 2) || !STRNCASECMP(op2start, "iy", 2)))
        return(2);
      return(1);
    }

  /* Push / pop */
  if(ISINST(pl->line, "push")  && IS_Z80N && op1start[0] == '#')
    return(4);
  if(ISINST(pl->line, "push") || ISINST(pl->line, "pop"))
    {
      if(!STRNCASECMP(op1start, "ix", 2) || !STRNCASECMP(op1start, "iy", 2))
        return(2);
      return(1);
    }

  /* 16 bit add / subtract / and */
  if(IS_Z80N && ISINST(pl->line, "add") && (!STRNCASECMP(op1start, "bc", 2) || !STRNCASECMP(op1start, "de", 2) || !STRNCASECMP(op1start, "hl", 2)))
    return(4);
  if((ISINST(pl->line, "add") || ISINST(pl->line, "adc") || ISINST(pl->line, "sbc") || IS_RAB && ISINST(pl->line, "and")) &&
     !STRNCASECMP(op1start, "hl", 2))
    {
      if(ISINST(pl->line, "add") || ISINST(pl->line, "and"))
        return(1);
      return(2);
    }
  if(ISINST(pl->line, "add") && (!STRNCASECMP(op1start, "ix", 2) || !STRNCASECMP(op1start, "iy", 2)))
    return(2);

  /* signed 8 bit adjustment to stack pointer */
  if((IS_RAB || IS_GB) && ISINST(pl->line, "add") && !STRNCASECMP(op1start, "sp", 2))
    return(2);

  /* 16 bit adjustment to stack pointer */
  if(IS_TLCS90 && ISINST(pl->line, "add") && !STRNCASECMP(op1start, "sp", 2))
    return(3);

  /* 8 bit arithmetic, two operands */
  if(op2start &&  op1start[0] == 'a' &&
     (ISINST(pl->line, "add") || ISINST(pl->line, "adc") || ISINST(pl->line, "sub") || ISINST(pl->line, "sbc") ||
      ISINST(pl->line, "cp")  || ISINST(pl->line, "and") || ISINST(pl->line, "or")  || ISINST(pl->line, "xor")))
    {
      if(argCont(op2start, "(ix)") || argCont(op2start, "(iy)"))
        return(3);
      if(op2start[0] == '#')
        return(2);
      return(1);
    }

  if(ISINST(pl->line, "rlca") || ISINST(pl->line, "rla") || ISINST(pl->line, "rrca") || ISINST(pl->line, "rra"))
    return(1);

  /* Increment / decrement */
  if(ISINST(pl->line, "inc") || ISINST(pl->line, "dec"))
    {
      if(!STRNCASECMP(op1start, "ix", 2) || !STRNCASECMP(op1start, "iy", 2))
        return(2);
      if(argCont(op1start, "(ix)") || argCont(op1start, "(iy)"))
        return(3);
      return(1);
    }

  if(ISINST(pl->line, "rlc") || ISINST(pl->line, "rl")  || ISINST(pl->line, "rrc") || ISINST(pl->line, "rr") ||
     ISINST(pl->line, "sla") || ISINST(pl->line, "sra") || ISINST(pl->line, "srl"))
    {
      if(argCont(op1start, "(ix)") || argCont(op1start, "(iy)"))
        return(4);
      return(2);
    }

  if(ISINST(pl->line, "rld") || ISINST(pl->line, "rrd"))
    return(2);

  /* Bit */
  if(ISINST(pl->line, "bit") || ISINST(pl->line, "set") || ISINST(pl->line, "res"))
    {
      if(argCont(op2start, "(ix)") || argCont(op2start, "(iy)"))
        return(4);
      return(2);
    }

  if(ISINST(pl->line, "jr") || ISINST(pl->line, "djnz"))
    return(2);

  if(ISINST(pl->line, "jp"))
    {
      if(!STRNCASECMP(op1start, "(hl)", 4))
        return(1);
      if(!STRNCASECMP(op1start, "(ix)", 4) || !STRNCASECMP(op1start, "(iy)", 4))
        return(2);
      return(3);
    }

  if (IS_RAB &&
      (ISINST(pl->line, "ipset3") || ISINST(pl->line, "ipset2") ||
       ISINST(pl->line, "ipset1") || ISINST(pl->line, "ipset0") ||
       ISINST(pl->line, "ipres")))
    return(2);

  if(!IS_GB && (ISINST(pl->line, "reti") || ISINST(pl->line, "retn")))
    return(2);

  if(ISINST(pl->line, "ret") || ISINST(pl->line, "reti") || ISINST(pl->line, "rst"))
    return(1);

  if(ISINST(pl->line, "call"))
    return(3);

  if(ISINST(pl->line, "ldi") || ISINST(pl->line, "ldd") || ISINST(pl->line, "cpi") || ISINST(pl->line, "cpd"))
    return(2);

  if(ISINST(pl->line, "neg"))
    return(2);

  if(ISINST(pl->line, "daa") || ISINST(pl->line, "cpl")  || ISINST(pl->line, "ccf") || ISINST(pl->line, "scf") ||
     ISINST(pl->line, "nop") || ISINST(pl->line, "halt") || ISINST(pl->line,  "ei") || ISINST(pl->line, "di")  ||
     (IS_GB && ISINST(pl->line, "stop")))
    return(1);

  if(ISINST(pl->line, "im"))
    return(2);

  if(ISINST(pl->line, "in") || ISINST(pl->line, "out") || ISINST(pl->line, "ot"))
    return(2);

  if((IS_Z180 || IS_EZ80_Z80) && (ISINST(pl->line, "in0") || ISINST(pl->line, "out0")))
    return(3);

  if((IS_Z180 || IS_EZ80_Z80 || IS_Z80N) && ISINST(pl->line, "mlt"))
    return(2);

  if((IS_Z180 || IS_EZ80_Z80 || IS_Z80N) && ISINST(pl->line, "tst"))
    return((op1start[0] == '#' || op2start && op1start[0] == '#') ? 3 : 2);

  if(IS_RAB && ISINST(pl->line, "mul"))
    return(1);

  if(ISINST(pl->line, "lddr") || ISINST(pl->line, "ldir"))
    return(2);

  if(IS_R3KA &&
    (ISINST(pl->line, "lddsr") || ISINST(pl->line, "ldisr") ||
     ISINST(pl->line, "lsdr")  || ISINST(pl->line, "lsir")  ||
     ISINST(pl->line, "lsddr") || ISINST(pl->line, "lsidr")))
    return(2);

  if(IS_R3KA && (ISINST(pl->line, "uma") || ISINST(pl->line, "ums")))
    return(2);

  if(IS_RAB && ISINST(pl->line, "bool"))
    return(!STRNCASECMP(op1start, "hl", 2) ? 1 : 2);

  if(IS_EZ80_Z80 && (ISINST(pl->line, "lea") || ISINST(pl->line, "pea")))
    return(3);

  if(IS_GB || IS_Z80N && ISINST(pl->line, "swap"))
    return(2);

  if(ISINST(pl->line, ".db") || ISINST(pl->line, ".byte"))
    {
      int i, j;
      for(i = 1, j = 0; pl->line[j]; i += pl->line[j] == ',', j++);
      return(i);
    }

  if(ISINST(pl->line, ".dw") || ISINST(pl->line, ".word"))
    {
      int i, j;
      for(i = 1, j = 0; pl->line[j]; i += pl->line[j] == ',', j++);
      return(i * 2);
    }

  /* If the instruction is unrecognized, we shouldn't try to optimize.  */
  /* For all we know it might be some .ds or similar possibly long line */
  /* Return a large value to discourage optimization.                   */
  if (pl->ic)
    werrorfl(pl->ic->filename, pl->ic->lineno, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
  else
    werrorfl("unknown", 0, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
  return(999);
}