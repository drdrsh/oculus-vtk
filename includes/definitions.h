#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include "Extras/OVR_Math.h"

#define VTK_NEW(var, type)  vtkSmartPointer<type> var = vtkSmartPointer<type>::New();

#ifndef VALIDATE
	#define VALIDATE(x, msg) if (!(x)) { std::cerr << msg; exit(-1); }
#endif

typedef OVR::Size<int>  Sizei;

#endif