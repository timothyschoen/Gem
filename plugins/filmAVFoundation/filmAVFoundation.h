/*-----------------------------------------------------------------

GEM - Graphics Environment for Multimedia

Load a digital video (.mov, .mp4, etc) into a pix block on macOS 10.7+

Copyright (c) 2016 Dan Wilcox. danomatika@gmail.com

For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/
#ifndef _INCLUDE_GEMPLUGIN__FILMAVF_FILMAVF_H_
#define _INCLUDE_GEMPLUGIN__FILMAVF_FILMAVF_H_

#include "plugins/film.h"
#include "Gem/Image.h"

// forward declare PIMPL types, depending on the compiler
#ifdef __OBJC__
  @class AVFMoviePlayer;
#else
  typedef struct objc_object AVFMoviePlayer;
#endif

/*-----------------------------------------------------------------
  -------------------------------------------------------------------
  CLASS
  filmAVFoundation

  Loads in a film on macOS using the AVFoundation framework

  KEYWORDS
  pix

  DESCRIPTION

  -----------------------------------------------------------------*/
namespace gem { namespace plugins {
class GEM_EXPORT filmAVFoundation : public film {

public:

  //////////
  // Constructor
  filmAVFoundation(void);
  
  //////////
  // Destructor
  virtual ~filmAVFoundation(void);

  //////////
  // open a movie up
  virtual bool open(const std::string &filename, const gem::Properties &props);
  
  //////////
  // get the next frame
  virtual pixBlock* getFrame(void);

  //////////
  // set the next frame to read
  virtual errCode changeImage(int imgNum, int trackNum = -1);

  //////////
  // returns true if instance can be used in thread
  virtual bool isThreadable(void) {return true;}

  //////////
  // close the movie file
  virtual void close(void);

  //////////
  // list all properties the currently opened film supports
  virtual bool enumProperties(gem::Properties &readable,
                              gem::Properties &writeable);

  //////////
  // set properties: "auto", "colorspace"
  virtual void setProperties(gem::Properties &props);

  //////////
  // get properties: "width", "height", "frames", "fps"
  virtual void getProperties(gem::Properties &props);

protected:

  //////////
  // set the pixel format: GL_YCBCR_422_APPLE, GL_RGBA_GEM, or GL_LUMINANCE
  // (re)allocates when necessary
  void changeFormat(GLenum format);

  // PIMPL pointer to the Obj-C object behind this class
  AVFMoviePlayer* m_moviePlayer;

  pixBlock m_image;             //< frame storage
  GLenum m_wantedFormat;        //< requested color space
  int m_numFrames, m_numTracks; //< num frames & tracks in the stream
  int m_curFrame, m_curTrack;   //< current frame and track
  double m_fps;                 //< frame rate
  bool m_readNext;              //< is the video frame new & should be read?
  //double m_auto;                //< auto increment

};};};

#endif // header guard
