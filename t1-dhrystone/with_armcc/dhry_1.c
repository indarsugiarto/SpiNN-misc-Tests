/*
 ****************************************************************************
 *
 *                   "DHRYSTONE" Benchmark Program
 *                   -----------------------------
 *                                                                            
 *  Version:    C, Version 2.1
 *                                                                            
 *  File:       dhry_1.c (part 2 of 3)
 *
 *  Date:       May 25, 1988
 *
 *  Author:     Reinhold P. Weicker
 *
 ****************************************************************************
 */
// #include <string.h>
#include "dhry.h"
#include <spin1_api.h>

// Indar: let's see in stdfix format
#include "stdfix.h"
#define REAL		accum
#define REAL_CONST(x)	x##k

// Indar: coba dengan REG
#define REG

/* Global Variables: */

Rec_Pointer     Ptr_Glob,
                Next_Ptr_Glob;
int             Int_Glob;
Boolean         Bool_Glob;
char            Ch_1_Glob,
                Ch_2_Glob;
int             Arr_1_Glob [50];
int             Arr_2_Glob [50] [50];

//extern char     *malloc ();
// Enumeration     Func_1 ();
  /* forward declaration necessary since Enumeration may not simply be int */
extern void Proc_6 (Enumeration Enum_Val_Par, Enumeration *Enum_Ref_Par);
extern void Proc_7 (One_Fifty Int_1_Par_Val, One_Fifty Int_2_Par_Val, One_Fifty *Int_Par_Ref);
extern void Proc_8 (Arr_1_Dim Arr_1_Par_Ref, Arr_2_Dim Arr_2_Par_Ref, int Int_1_Par_Val, int Int_2_Par_Val);
extern Enumeration Func_1 (Capital_Letter Ch_1_Par_Val, Capital_Letter Ch_2_Par_Val);
extern Boolean Func_2 (Str_30 Str_1_Par_Ref, Str_30 Str_2_Par_Ref);
extern Boolean Func_3 (Enumeration Enum_Par_Val);

#ifndef REG
        Boolean Reg = false;
#define REG
        /* REG becomes defined as empty */
        /* i.e. no register variables   */
#else
        Boolean Reg = true;
#endif

/* variables for time measurement: */

#ifdef TIMES
struct tms      time_info;
//extern  int     times ();
                /* see library function "times" */
#define Too_Small_Time (2*HZ)
                /* Measurements should last at least about 2 seconds */
#endif
#ifdef TIME
// Indar: since we don't use external time.h
//extern long     time();           /* see library function "time"  */
long time(uint t)
{
    return (long)sv->unix_time;
}

#define Too_Small_Time 2
                /* Measurements should last at least 2 seconds */
#endif
#ifdef MSC_CLOCK
// Indar, clock value from sark
//extern clock_t	clock();
long clock()
{
    return (long)sv->clock_ms;
#define Too_Small_Time (2*HZ)
#endif

long            Begin_Time,
                End_Time,
                User_Time;
float           Microseconds,
                Dhrystones_Per_Second;

/* end of variables for time measurement */

// Indar: forward declaration
void Proc_1 (REG Rec_Pointer Ptr_Val_Par);
void Proc_2 (One_Fifty *Int_Par_Ref);
void Proc_3 (Rec_Pointer *Ptr_Ref_Par);
void Proc_4 (void);
void Proc_5 (void);

void c_main ()
/*****/

  /* main program, corresponds to procedures        */
  /* Main and Proc_0 in the Ada version             */
{
        One_Fifty       Int_1_Loc;
  REG   One_Fifty       Int_2_Loc;
        One_Fifty       Int_3_Loc;
  REG   char            Ch_Index;
        Enumeration     Enum_Loc;
        Str_30          Str_1_Loc;
        Str_30          Str_2_Loc;
  REG   int             Run_Index;
  REG   int             Number_Of_Runs;

  /* Initializations */

  // Indar: ganti malloc dengan sark_alloc
  // Next_Ptr_Glob = (Rec_Pointer) malloc (sizeof (Rec_Type));
  // Ptr_Glob = (Rec_Pointer) malloc (sizeof (Rec_Type));
  Next_Ptr_Glob = (Rec_Pointer) sark_alloc (1, sizeof (Rec_Type));
  Ptr_Glob = (Rec_Pointer) sark_alloc (1, sizeof (Rec_Type));

  Ptr_Glob->Ptr_Comp                    = Next_Ptr_Glob;
  Ptr_Glob->Discr                       = Ident_1;
  Ptr_Glob->variant.var_1.Enum_Comp     = Ident_3;
  Ptr_Glob->variant.var_1.Int_Comp      = 40;
  sark_str_cpy (Ptr_Glob->variant.var_1.Str_Comp,
          "DHRYSTONE PROGRAM, SOME STRING");
  sark_str_cpy (Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING");

  Arr_2_Glob [8][7] = 10;
        /* Was missing in published program. Without this statement,    */
        /* Arr_2_Glob [8][7] would have an undefined value.             */
        /* Warning: With 16-Bit processors and Number_Of_Runs > 32000,  */
        /* overflow may occur for this array element.                   */

  io_printf (IO_STD, "\n");
  io_printf (IO_STD, "Dhrystone Benchmark, Version 2.1 (Language: C)\n");
  io_printf (IO_STD, "\n");
  if (Reg)
  {
    io_printf (IO_STD, "Program compiled with 'register' attribute\n");
    io_printf (IO_STD, "\n");
  }
  else
  {
    io_printf (IO_STD, "Program compiled without 'register' attribute\n");
    io_printf (IO_STD, "\n");
  }
  io_printf (IO_STD, "Please give the number of runs through the benchmark: ");
  {
    int n;
    // scanf ("%d", &n);    // Indar: no scanf
    n = number_of_runs_through_the_benchmark;
    Number_Of_Runs = n;
  }
  io_printf (IO_STD, "\n");

  io_printf (IO_STD, "Execution starts, %d runs through Dhrystone\n", Number_Of_Runs);

  /***************/
  /* Start timer */
  /***************/
 
#ifdef TIMES
  times (&time_info);
  Begin_Time = (long) time_info.tms_utime;
#endif
#ifdef TIME
  // Indar: modify this
  // Begin_Time = time ( (long *) 0); // this is the original line
  Begin_Time = time (0);
#endif
#ifdef MSC_CLOCK
  Begin_Time = clock();
#endif

  for (Run_Index = 1; Run_Index <= Number_Of_Runs; ++Run_Index)
  {

    Proc_5();
    Proc_4();
      /* Ch_1_Glob == 'A', Ch_2_Glob == 'B', Bool_Glob == true */
    Int_1_Loc = 2;
    Int_2_Loc = 3;
    sark_str_cpy (Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING");
    Enum_Loc = Ident_2;
    Bool_Glob = ! Func_2 (Str_1_Loc, Str_2_Loc);
      /* Bool_Glob == 1 */
    while (Int_1_Loc < Int_2_Loc)  /* loop body executed once */
    {
      Int_3_Loc = 5 * Int_1_Loc - Int_2_Loc;
        /* Int_3_Loc == 7 */
      Proc_7 (Int_1_Loc, Int_2_Loc, &Int_3_Loc);
        /* Int_3_Loc == 7 */
      Int_1_Loc += 1;
    } /* while */
      /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
    Proc_8 (Arr_1_Glob, Arr_2_Glob, Int_1_Loc, Int_3_Loc);
      /* Int_Glob == 5 */
    Proc_1 (Ptr_Glob);
    for (Ch_Index = 'A'; Ch_Index <= Ch_2_Glob; ++Ch_Index)
                             /* loop body executed twice */
    {
      if (Enum_Loc == Func_1 (Ch_Index, 'C'))
          /* then, not executed */
        {
        Proc_6 (Ident_1, &Enum_Loc);
        sark_str_cpy (Str_2_Loc, "DHRYSTONE PROGRAM, 3'RD STRING");
        Int_2_Loc = Run_Index;
        Int_Glob = Run_Index;
        }
    }
      /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
    Int_2_Loc = Int_2_Loc * Int_1_Loc;
    Int_1_Loc = Int_2_Loc / Int_3_Loc;
    Int_2_Loc = 7 * (Int_2_Loc - Int_3_Loc) - Int_1_Loc;
      /* Int_1_Loc == 1, Int_2_Loc == 13, Int_3_Loc == 7 */
    Proc_2 (&Int_1_Loc);
      /* Int_1_Loc == 5 */

  } /* loop "for Run_Index" */

  /**************/
  /* Stop timer */
  /**************/
  
#ifdef TIMES
  times (&time_info);
  End_Time = (long) time_info.tms_utime;
#endif
#ifdef TIME
  // Indar: modify this:
  // End_Time = time ( (long *) 0); // this is the original line
  End_Time = time ( 0);
#endif
#ifdef MSC_CLOCK
  End_Time = clock();
#endif

  io_printf (IO_STD, "Execution ends\n");
  io_printf (IO_STD, "\n");
  io_printf (IO_STD, "Final values of the variables used in the benchmark:\n");
  io_printf (IO_STD, "\n");
  io_printf (IO_STD, "Int_Glob:            %d\n", Int_Glob);
  io_printf (IO_STD, "        should be:   %d\n", 5);
  io_printf (IO_STD, "Bool_Glob:           %d\n", Bool_Glob);
  io_printf (IO_STD, "        should be:   %d\n", 1);
  io_printf (IO_STD, "Ch_1_Glob:           %c\n", Ch_1_Glob);
  io_printf (IO_STD, "        should be:   %c\n", 'A');
  io_printf (IO_STD, "Ch_2_Glob:           %c\n", Ch_2_Glob);
  io_printf (IO_STD, "        should be:   %c\n", 'B');
  io_printf (IO_STD, "Arr_1_Glob[8]:       %d\n", Arr_1_Glob[8]);
  io_printf (IO_STD, "        should be:   %d\n", 7);
  io_printf (IO_STD, "Arr_2_Glob[8][7]:    %d\n", Arr_2_Glob[8][7]);
  io_printf (IO_STD, "        should be:   Number_Of_Runs + 10\n");
  io_printf (IO_STD, "Ptr_Glob->\n");
  io_printf (IO_STD, "  Ptr_Comp:          %d\n", (int) Ptr_Glob->Ptr_Comp);
  io_printf (IO_STD, "        should be:   (implementation-dependent)\n");
  io_printf (IO_STD, "  Discr:             %d\n", Ptr_Glob->Discr);
  io_printf (IO_STD, "        should be:   %d\n", 0);
  io_printf (IO_STD, "  Enum_Comp:         %d\n", Ptr_Glob->variant.var_1.Enum_Comp);
  io_printf (IO_STD, "        should be:   %d\n", 2);
  io_printf (IO_STD, "  Int_Comp:          %d\n", Ptr_Glob->variant.var_1.Int_Comp);
  io_printf (IO_STD, "        should be:   %d\n", 17);
  io_printf (IO_STD, "  Str_Comp:          %s\n", Ptr_Glob->variant.var_1.Str_Comp);
  io_printf (IO_STD, "        should be:   DHRYSTONE PROGRAM, SOME STRING\n");
  io_printf (IO_STD, "Next_Ptr_Glob->\n");
  io_printf (IO_STD, "  Ptr_Comp:          %d\n", (int) Next_Ptr_Glob->Ptr_Comp);
  io_printf (IO_STD, "        should be:   (implementation-dependent), same as above\n");
  io_printf (IO_STD, "  Discr:             %d\n", Next_Ptr_Glob->Discr);
  io_printf (IO_STD, "        should be:   %d\n", 0);
  io_printf (IO_STD, "  Enum_Comp:         %d\n", Next_Ptr_Glob->variant.var_1.Enum_Comp);
  io_printf (IO_STD, "        should be:   %d\n", 1);
  io_printf (IO_STD, "  Int_Comp:          %d\n", Next_Ptr_Glob->variant.var_1.Int_Comp);
  io_printf (IO_STD, "        should be:   %d\n", 18);
  io_printf (IO_STD, "  Str_Comp:          %s\n",
                                Next_Ptr_Glob->variant.var_1.Str_Comp);
  io_printf (IO_STD, "        should be:   DHRYSTONE PROGRAM, SOME STRING\n");
  io_printf (IO_STD, "Int_1_Loc:           %d\n", Int_1_Loc);
  io_printf (IO_STD, "        should be:   %d\n", 5);
  io_printf (IO_STD, "Int_2_Loc:           %d\n", Int_2_Loc);
  io_printf (IO_STD, "        should be:   %d\n", 13);
  io_printf (IO_STD, "Int_3_Loc:           %d\n", Int_3_Loc);
  io_printf (IO_STD, "        should be:   %d\n", 7);
  io_printf (IO_STD, "Enum_Loc:            %d\n", Enum_Loc);
  io_printf (IO_STD, "        should be:   %d\n", 1);
  io_printf (IO_STD, "Str_1_Loc:           %s\n", Str_1_Loc);
  io_printf (IO_STD, "        should be:   DHRYSTONE PROGRAM, 1'ST STRING\n");
  io_printf (IO_STD, "Str_2_Loc:           %s\n", Str_2_Loc);
  io_printf (IO_STD, "        should be:   DHRYSTONE PROGRAM, 2'ND STRING\n");
  io_printf (IO_STD, "\n");

  User_Time = End_Time - Begin_Time;

  if (User_Time < Too_Small_Time)
  {
    io_printf (IO_STD, "Measured time too small to obtain meaningful results\n");
    io_printf (IO_STD, "Please increase number of runs\n");
    io_printf (IO_STD, "\n");
  }
  else
  {
#ifdef TIME
    Microseconds = (float) User_Time * Mic_secs_Per_Second 
                        / (float) Number_Of_Runs;
    Dhrystones_Per_Second = (float) Number_Of_Runs / (float) User_Time;
#else
    Microseconds = (float) User_Time * Mic_secs_Per_Second 
                        / ((float) HZ * ((float) Number_Of_Runs));
    Dhrystones_Per_Second = ((float) HZ * (float) Number_Of_Runs)
                        / (float) User_Time;
#endif
    /*
    io_printf (IO_STD, "Microseconds for one run through Dhrystone: ");
    io_printf (IO_STD, "%6.1f \n", Microseconds);
    io_printf (IO_STD, "Dhrystones per Second:                      ");
    io_printf (IO_STD, "%6.1f \n", Dhrystones_Per_Second);
    io_printf (IO_STD, "\n");
    */
    // Indar: armcc doesn't support stdfix.h: it will produce internal failure error
    //        when compiled by armcc
    // also, DMIPS (Dhrystone MIPS) obtained when the Dhrystone score is divided by 1757 
    // (the number of Dhrystones per second obtained on the VAX 11/780, nominally a 1 MIPS machine).


    float dmips = Dhrystones_Per_Second / 1757.0;
    int mcs = (int)Microseconds;
    int umips = (int)dmips;
    io_printf (IO_STD, "Microseconds for one run through Dhrystone: ");
    io_printf (IO_STD, "%d \n", mcs);
    io_printf (IO_STD, "Dhrystones per Second                     : ");
    io_printf (IO_STD, "%u \n", (uint)Dhrystones_Per_Second);
    io_printf (IO_STD, "DMIPS                                     : ");
    io_printf (IO_STD, "%d \n", umips);
    io_printf (IO_STD, "\n");

  }
  
}


void Proc_1 (REG Rec_Pointer Ptr_Val_Par)
/* executed once */
{
  REG Rec_Pointer Next_Record = Ptr_Val_Par->Ptr_Comp;  
                                        /* == Ptr_Glob_Next */
  /* Local variable, initialized with Ptr_Val_Par->Ptr_Comp,    */
  /* corresponds to "rename" in Ada, "with" in Pascal           */
  
  structassign (*Ptr_Val_Par->Ptr_Comp, *Ptr_Glob); 
  Ptr_Val_Par->variant.var_1.Int_Comp = 5;
  Next_Record->variant.var_1.Int_Comp 
        = Ptr_Val_Par->variant.var_1.Int_Comp;
  Next_Record->Ptr_Comp = Ptr_Val_Par->Ptr_Comp;
  Proc_3 (&Next_Record->Ptr_Comp);
    /* Ptr_Val_Par->Ptr_Comp->Ptr_Comp 
                        == Ptr_Glob->Ptr_Comp */
  if (Next_Record->Discr == Ident_1)
    /* then, executed */
  {
    Next_Record->variant.var_1.Int_Comp = 6;
    Proc_6 (Ptr_Val_Par->variant.var_1.Enum_Comp, 
           &Next_Record->variant.var_1.Enum_Comp);
    Next_Record->Ptr_Comp = Ptr_Glob->Ptr_Comp;
    Proc_7 (Next_Record->variant.var_1.Int_Comp, 10, 
           &Next_Record->variant.var_1.Int_Comp);
  }
  else /* not executed */
    structassign (*Ptr_Val_Par, *Ptr_Val_Par->Ptr_Comp);
} /* Proc_1 */


void Proc_2 (One_Fifty   *Int_Par_Ref)
/* executed once */
/* *Int_Par_Ref == 1, becomes 4 */
{
  One_Fifty  Int_Loc;  
  Enumeration   Enum_Loc;

  Int_Loc = *Int_Par_Ref + 10;
  do /* executed once */
    if (Ch_1_Glob == 'A')
      /* then, executed */
    {
      Int_Loc -= 1;
      *Int_Par_Ref = Int_Loc - Int_Glob;
      Enum_Loc = Ident_1;
    } /* if */
  while (Enum_Loc != Ident_1); /* true */
} /* Proc_2 */


void Proc_3 (Rec_Pointer *Ptr_Ref_Par)
/* executed once */
/* Ptr_Ref_Par becomes Ptr_Glob */
{
  if (Ptr_Glob != Null)
    /* then, executed */
    *Ptr_Ref_Par = Ptr_Glob->Ptr_Comp;
  Proc_7 (10, Int_Glob, &Ptr_Glob->variant.var_1.Int_Comp);
} /* Proc_3 */


void Proc_4 (void) /* without parameters */
/* executed once */
{
  Boolean Bool_Loc;

  Bool_Loc = Ch_1_Glob == 'A';
  Bool_Glob = Bool_Loc | Bool_Glob;
  Ch_2_Glob = 'B';
} /* Proc_4 */


void Proc_5 (void) /* without parameters */
/* executed once */
{
  Ch_1_Glob = 'A';
  Bool_Glob = false;
} /* Proc_5 */


        /* Procedure for the assignment of structures,          */
        /* if the C compiler doesn't support this feature       */
#ifdef  NOSTRUCTASSIGN
memcpy (d, s, l)
register char   *d;
register char   *s;
register int    l;
{
        while (l--) *d++ = *s++;
}
#endif


