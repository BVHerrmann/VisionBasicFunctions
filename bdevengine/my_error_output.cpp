// HDevEngine/C++ example implementations for showing (error) messages
// in a HALCON window
//
//   This file contains a simple implementation just passing the message
//   and a more complex implementation passing full error information
//
// (C) 2008-2021 MVTec Software GmbH
//

#include "my_error_output.h"
#ifdef __APPLE__
#  ifndef HC_LARGE_IMAGES
#    include <HALCONCpp/HalconCpp.h>
#  else
#    include <HALCONCppxl/HalconCpp.h>
#  endif
#else
#  include "halconcpp/HalconCpp.h"
#endif

// The POSIX name for strdup has been declared deprecated by MS.
// Instead, use the ISO C++ conformant name _strdup on Windows
#if defined(_WIN32) || defined(_WIN64)
#  define strdup _strdup
#endif

#define ERR_WIN_WIDTH_SIMPLE 1000
#define ERR_WIN_HEIGHT_SIMPLE 100

#define ERR_WIN_WIDTH_COMPLEX 1000
#define ERR_WIN_HEIGHT_COMPLEX 200


#define WRITE_TO_CONSOLE

using namespace HalconCpp;


// writes a message into the passed HALCON window adding a newline at the end
// and setting the start column to the same column where the text output
// started
void WriteMessageNL(HWindow& win, const char* message)
{
  char* buf  = strdup(message);
  char* line = buf;

  Hlong row, column, max_descent, max_width, max_height;
  win.GetTposition(&row, &column);
  win.GetFontExtents(&max_descent, &max_width, &max_height);

  while (*line)
  {
    char* nl = strchr(line, '\n');
    if (nl)
      *nl = '\0';
    win.WriteString(line);
    win.SetTposition(row += max_height + 2, column);
    if (nl)
      line = nl + 1;
    else
      break;
  }
  free(buf);
#ifdef WRITE_TO_CONSOLE
  printf("%s\n", message);
#endif
}


// simply writes the error message into a HALCON window
void DispMessage(const char* message)
{
  HWindow win(100, 100, ERR_WIN_WIDTH_SIMPLE, ERR_WIN_HEIGHT_SIMPLE, NULL,
              "visible", "");
  win.SetPart(0, 0, ERR_WIN_HEIGHT_SIMPLE - 1, ERR_WIN_WIDTH_SIMPLE - 1);
  win.SetColor("yellow");
  win.SetTposition(10, 10);
  WriteMessageNL(win, message);

  // wait for mouse click to continue
  win.SetColor("red");
  win.SetTposition(ERR_WIN_HEIGHT_SIMPLE / 2 + 10, ERR_WIN_WIDTH_SIMPLE / 2);
  win.WriteString("...click into window to continue");
  win.Click();
}


// writes all information that is stored in the exception into a HALCON window
void DispErrorMessage(const HDevEngineCpp::HDevEngineException& exception,
                      const char* context_msg /*=NULL*/)
{
  char text[2000];

  HWindow win(100, 100, ERR_WIN_WIDTH_COMPLEX, ERR_WIN_HEIGHT_COMPLEX, NULL,
              "visible", "");
  win.SetPart(0, 0, ERR_WIN_HEIGHT_COMPLEX - 1, ERR_WIN_WIDTH_COMPLEX - 1);
  win.SetTposition(10, 10);

  if (context_msg)
  {
    win.SetColor("orange");
    WriteMessageNL(win, context_msg);
    WriteMessageNL(win, "\n");
  }

  win.SetColor("yellow");
  WriteMessageNL(win, exception.Message());
  WriteMessageNL(win, "");

  sprintf(text, "    Error category: <%d : %s>", exception.Category(),
          exception.CategoryText());
  WriteMessageNL(win, text);

  sprintf(text, "    Error code:   <%d>", exception.HalconErrorCode());
  WriteMessageNL(win, text);

  sprintf(text, "    Procedure:      <%s>", exception.ExecProcedureName());
  WriteMessageNL(win, text);

  sprintf(text, "        Line:         <%d : %s>", exception.ProgLineNum(),
          exception.ProgLineName());
  WriteMessageNL(win, text);

  // wait for mouse click to continue
  win.SetColor("red");
  win.SetTposition(ERR_WIN_HEIGHT_COMPLEX - 30, ERR_WIN_WIDTH_COMPLEX / 2);
  win.WriteString("...click into window to continue");
  win.Click();
}
