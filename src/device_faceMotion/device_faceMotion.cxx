/** \file device_faceMotion.cxx
 *
 *  MotionBuilder plugin library entry. Registers the device class and its
 *  layout with the FBSDK loader. Adapted from OpenMoBu's device_facecap.cxx
 *  (BSD 3-clause).
 */

#include <fbsdk/fbsdk.h>

#ifdef KARCH_ENV_WIN
    #include <windows.h>
#endif

#include "device_faceMotion_device.h"
#include "device_faceMotion_layout.h"

FBLibraryDeclare(device_faceMotion)
{
    FBLibraryRegister(CDevice_FaceMotion);
    FBLibraryRegister(CDevice_FaceMotion_Layout);
}
FBLibraryDeclareEnd;

bool FBLibrary::LibInit()    { return true; }
bool FBLibrary::LibOpen()    { return true; }
bool FBLibrary::LibReady()   { return true; }
bool FBLibrary::LibClose()   { return true; }
bool FBLibrary::LibRelease() { return true; }
