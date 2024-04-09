// HDevEngine/C++ example implementations for showing (error) messages
// in a HALCON window
//
//   This file contains a simple implementation just passing the message
//   and a more complex implementation passing full error information
//
// (C) 2008-2021 MVTec Software GmbH
//

#ifndef MY_ERROR_OUTPUT_H
#define MY_ERROR_OUTPUT_H

#ifdef __APPLE__
#  ifndef HC_LARGE_IMAGES
#    include <HALCONCpp/HalconCpp.h>
#    include <HDevEngineCpp/HDevEngineCpp.h>
#  else
#    include <HALCONCppxl/HalconCpp.h>
#    include <HDevEngineCppxl/HDevEngineCpp.h>
#  endif
#else
#include "halconcpp/HalconCpp.h"
#include "hdevengine/HDevEngineCpp.h"
#endif

// writes a message intot he passed HALCON window adding a newline at the end
// and setting the start column to the same column where the text output
// started
void WriteMessageNL(HalconCpp::HWindow& win, const char* message);

// simply writes a message into a HALCON window and on the console
void DispMessage(const char* message);

// writes all information that is stored in the exception into a HALCON window
// and on the console
void DispErrorMessage(const HDevEngineCpp::HDevEngineException& exception,
                      const char* context_msg=NULL);


#endif // #ifndef MY_ERROR_OUTPUT_H

