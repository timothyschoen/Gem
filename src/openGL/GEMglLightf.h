/* ------------------------------------------------------------------
 * GEM - Graphics Environment for Multimedia
 *
 *  Copyright (c) 2002-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
 *     zmoelnig@iem.kug.ac.at
 *  For information on usage and redistribution, and for a DISCLAIMER
 *  OF ALL WARRANTIES, see the file, "GEM.LICENSE.TERMS"
 *
 *  this file has been generated...
 * ------------------------------------------------------------------
 */

#ifndef _INCLUDE__GEM_OPENGL_GEMGLLIGHTF_H_
#define _INCLUDE__GEM_OPENGL_GEMGLLIGHTF_H_

#include "Base/GemGLBase.h"

/*
 CLASS
        GEMglLightf
 KEYWORDS
        openGL  0
 DESCRIPTION
        wrapper for the openGL-function
        "glLightf( GLenum light, GLenum pname, GLfloat param)"
 */

class GEM_EXTERN GEMglLightf : public GemGLBase
{
  CPPEXTERN_HEADER(GEMglLightf, GemGLBase);

public:
  // Constructor
  GEMglLightf (t_float, t_float, t_float);      // CON

protected:
  // Destructor
  virtual ~GEMglLightf ();
  // Do the rendering
  virtual void  render (GemState *state);

  // variables
  GLenum        light;          // VAR
  virtual void  lightMess(t_float);     // FUN

  GLenum        pname;          // VAR
  virtual void  pnameMess(t_float);     // FUN

  GLfloat       param;          // VAR
  virtual void  paramMess(t_float);     // FUN


private:

  // we need some inlets
  t_inlet *m_inlet[3];

  // static member functions
  static void    lightMessCallback (void*, t_float);
  static void    pnameMessCallback (void*, t_float);
  static void    paramMessCallback (void*, t_float);
};
#endif // for header file
