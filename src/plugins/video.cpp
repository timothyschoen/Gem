////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.kug.ac.at
//
// Implementation file
//
//    Copyright (c) 1997-1998 Mark Danks.
//    Copyright (c) G�nther Geiger.
//    Copyright (c) 2001-2003 IOhannes m zmoelnig. forum::f�r::uml�ute. IEM
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////
  
#include "plugins/video.h"
#include "Gem/RTE.h"
using namespace gem;

#if 0
# define debugPost post
#else
# define debugPost
#endif

#include <pthread.h>

#ifdef _WIN32
# include <winsock2.h>
#endif

/**
 * video capturing states
 *
 *  state                user-pov            system-pov
 * ----------------------------------------------------
 * is device open?       m_haveVideo         m_haveVideo
 * is device streaming?  m_pimpl->shouldrun  m_capturing
 * is thread running     (opaque)            m_pimpl->running
 *
 */

class video :: PIMPL {
  friend class video;
public:
  /* interfaces */
  // the list of provided device-classes
  std::vector<std::string>m_providers;

  /* threading */
  bool threading;
  pthread_t thread;
  pthread_mutex_t**locks;
  unsigned int numlocks;

  unsigned int timeout;

  bool cont;
  bool running;

  bool shouldrun; /* we should be capturing */

  const std::string name;

  PIMPL(const std::string name_, unsigned int locks_, unsigned int timeout_) :
    threading(locks_>0),
    locks(NULL),
    numlocks(0),
    timeout(timeout_),
    cont(true),
    running(false),
    shouldrun(false),
    name(name_)
  {
    if(locks_>0) {
      numlocks=locks_;
      locks=new pthread_mutex_t*[numlocks];
      unsigned int i=0;
      for(i=0; i<locks_; i++)
        locks[i]=NULL;
    }
  }
  ~PIMPL(void) {
    cont=false;
    lock_delete();
    delete[]locks; 
    locks=NULL;
  }

  void lock(unsigned int i) {
    //    post("lock %d?\t%d", i, numlocks);

    if(i<numlocks && locks[i]) {
      pthread_mutex_lock(locks[i]);
    }
  }
  void unlock(unsigned int i) {
    //      post("unlock %d? %d", i,numlocks);

    if(i<numlocks && locks[i]) {
      pthread_mutex_unlock(locks[i]);
    }
  }
  bool lock_new(void) {
    if(locks) {
      unsigned int i=0;
      for(i=0; i<numlocks; i++) {
        locks[i]=new pthread_mutex_t;
        if ( pthread_mutex_init(locks[i], NULL) < 0 ) {
          lock_delete();
          return false;
        }
      }
      return true;
    }
    return true;
  }
  void lock_delete(void) {
    if(locks) {
      unsigned int i=0;
      for(i=0; i<numlocks; i++) {
        if(locks[i]) {
          pthread_mutex_destroy(locks[i]); 
          delete locks[i];
          locks[i]=NULL;
        }
      }
    }
  }

  static void*threadfun(void*you) {
    video*me=(video*)you;
    pixBlock*pix=NULL;
    post("starting capture thread");
    me->m_pimpl->cont=true;
    me->m_pimpl->running=true;
    
    while(me->m_pimpl->cont) {
      if(!me->grabFrame()) {
        break;
      }
    }
    me->m_pimpl->running=false;
    post("exiting capture thread");
    return NULL;
  }
};


/////////////////////////////////////////////////////////
//
// pix_videoLinux
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
video :: video(const std::string name, unsigned int locks, unsigned int timeout) :
  m_capturing(false), m_haveVideo(false), 
  m_width(64), m_height(64),
  m_channel(0), m_norm(0),
  m_reqFormat(GL_RGBA),
  m_devicename(std::string("")), m_devicenum(0), m_quality(0),
  m_pimpl(new PIMPL(name.empty()?std::string("<unknown>"):name, locks, timeout))
{
  if(!name.empty()) {
    provide(name);
  }
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
video :: ~video()
{
  if(m_pimpl)delete m_pimpl; m_pimpl=NULL;

  if(m_haveVideo)closeDevice();
}
/////////////////////////////////////////////////////////
// open/close
//
/////////////////////////////////////////////////////////
bool video :: open()
{
  debugPost("open: %d -> %d", m_haveVideo, m_capturing);
  if(m_haveVideo)close();
  m_haveVideo=openDevice();
  return m_haveVideo;
}
void video :: close()
{
  debugPost("close: %d -> %d", m_capturing, m_haveVideo);
  if(m_capturing)stop();
  if(m_haveVideo)closeDevice();
  m_pimpl->shouldrun=false;
  m_haveVideo=false;
}
/////////////////////////////////////////////////////////
// start/stop
//
/////////////////////////////////////////////////////////
bool video :: start()
{
  debugPost("start: %d -> %d", m_haveVideo, m_capturing);
  if(!m_haveVideo)return false;
  if(m_capturing)stop();
  m_capturing=startTransfer();
  if(m_capturing)
    startThread();

  m_pimpl->shouldrun=true;
  return m_capturing;
}
bool video :: stop()
{
  debugPost("stop(%d): %d -> %d", m_pimpl->shouldrun, m_capturing, m_haveVideo);
  bool running=m_pimpl->shouldrun;
  m_pimpl->shouldrun=false;
  if(!m_haveVideo)return false;
  if(m_capturing) {
    stopThread();
    stopTransfer();
  }

  m_capturing=false;
  return running;
}



/////////////////////////////////////////////////////////
// openDevice
//
/////////////////////////////////////////////////////////
bool video :: openDevice()
{
  return false;
}

/////////////////////////////////////////////////////////
// closeDevice
//
/////////////////////////////////////////////////////////
void video :: closeDevice()
{}

/////////////////////////////////////////////////////////
// resetDevice
//
/////////////////////////////////////////////////////////
bool video :: reset()
{
  return(false);
}
/////////////////////////////////////////////////////////
// startTransfer
//
/////////////////////////////////////////////////////////
bool video :: startTransfer()
{
  return false;
}

/////////////////////////////////////////////////////////
// stopTransfer
//
/////////////////////////////////////////////////////////
bool video :: stopTransfer()
{
  return false;
}

/////////////////////////////////////////////////////////
// startTransfer
//
/////////////////////////////////////////////////////////
bool video :: restartTransfer()
{
  debugPost("restartTransfer");
  bool running=stop();
  if(running)return start();

  return false;
}


bool video::startThread() {
  debugPost("startThread %x", m_pimpl);
  if(m_pimpl->running) {
    stopThread();
  }

  if(m_pimpl->threading) {
    if(!m_pimpl->lock_new())return false;

    pthread_create(&m_pimpl->thread, 
                   0,
                   m_pimpl->threadfun, 
                   this);
    return true;
  }
  return false;
}
bool video::stopThread(int timeout) {
  int i=0;
  if(!m_pimpl->threading)return true;

  debugPost("stopThread: %d", timeout);

  m_pimpl->cont=false;
  if(timeout<0)timeout=m_pimpl->timeout;
  if(timeout>0) {
    while(m_pimpl->running) {
      usleep(10);
      i+=10;
      if(i>timeout) {
        return false;
      }
    }
  } else {
    while(m_pimpl->running) {
      usleep(10);
      i+=10;
      if(i>1000000) {
        post("waiting for video grabbing thread to terminate...");
        i=0;
      }
    }
    //pthread_join(m_pimpl->thread, NULL);
  }

  m_pimpl->lock_delete();

  return true;
}
void video::lock(unsigned int id) {
  m_pimpl->lock(id);
}
void video::unlock(unsigned int id) {
  m_pimpl->unlock(id);
}
void video::usleep(unsigned long usec) {
  struct timeval sleep;
  long usec_ = usec%1000000;
  long sec_=0;
  //  long  sec_ = usec\1000000;
  sleep.tv_sec=sec_;
  sleep.tv_usec=usec_; 
  select(0,0,0,0,&sleep);
}

pixBlock* video::getFrame(void) {
  pixBlock*pix=&m_image;
  if(!(m_haveVideo && m_capturing))return NULL;
  if(m_pimpl->threading) {
     // get from thread
    if(!m_pimpl->running){
      pix=NULL;
    }
  } else {
    // no thread, grab it directly
    if(!grabFrame()) {
      m_capturing=false;
      pix=NULL;
    }
  }
  lock();
  return pix;
}


void video::releaseFrame(void) {
  m_image.newimage=false;
  unlock();
}

/////////////////////////////////////////////////////////
// set dimension
bool video :: setDimen(int x, int y, int leftmargin, int rightmargin, int topmargin, int bottommargin){
  //  post("setting the dimension for video is not supported by this OS/device");

  m_width=x;
  m_height=y;
  return false;
}

/////////////////////////////////////////////////////////
// set the tv-norm
bool video :: setNorm(const std::string norm){
  post("setting the video-norm is not supported by this OS/device");
  return false;
}
/////////////////////////////////////////////////////////
// set the channel of the current device
bool video :: setChannel(int chan, t_float freq){
  m_channel=chan;
  m_frequency=freq;
  return false;
}
/////////////////////////////////////////////////////////
// set the color-space
bool video :: setColor(int d){
  post("setting the color-space is not supported by this OS/device");
  return false;
}
/////////////////////////////////////////////////////////
// set the quality for DV decoding
bool video :: setQuality(int d){
  post("setting the quality is not supported by this OS/device");
  return false;
}
/////////////////////////////////////////////////////////
// open a dialog for the settings
bool video :: dialog(std::vector<std::string>dialognames){
  return false;
}

std::vector<std::string>video::enumerate(void) {
  std::vector<std::string>result;
  return result;
}

////////////////////
// set the video device
bool video :: setDevice(int d)
{
  m_devicename.clear();
  if (d==m_devicenum)return true;
  m_devicenum=d;
  return true;
}
bool video :: setDevice(const std::string name)
{
  m_devicenum=-1;
  m_devicename=name;
  return true;
}

const std::string video :: getName() {
  return m_pimpl->name;
}




/////////////////////////////////////////////////////////
// query whether this backend provides a certain type of video decoding, e.g. "dv"
bool video :: provides(const std::string name) {
  if(!m_pimpl)return false;
  unsigned int i;
  for(i=0; i<m_pimpl->m_providers.size(); i++)
    if(name == m_pimpl->m_providers[i])return true;

  return false;
}
std::vector<std::string>video :: provides() {
  std::vector<std::string>result;
  if(m_pimpl) {
    unsigned int i;
    for(i=0; i<m_pimpl->m_providers.size(); i++)
      result.push_back(m_pimpl->m_providers[i]);
  }
  return result;
}


/////////////////////////////////////////////////////////
// remember that this backend provides a certain type of video decoding, e.g. "dv"
void video :: provide(const std::string name) {
  if(!m_pimpl)return;
  if(!provides(name)) {
    m_pimpl->m_providers.push_back(name);
  }
}

bool video :: enumProperties(std::vector<std::string>&readable,
			     std::vector<std::string>&writeable) {
  readable.clear();
  writeable.clear();
  return false;
}

void video :: setProperties(gem::Properties&props) {
  // nada

  std::vector<std::string> keys=props.keys();
  int i=0;
  for(i=0; i<keys.size(); i++) {
    enum gem::Properties::PropertyType typ=props.type(keys[i]);
    std::cerr  << "key["<<keys[i]<<"]: "<<typ<<" :: ";
    switch(typ) {
    case (gem::Properties::NONE):
      props.erase(keys[i]);
      break;
    case (gem::Properties::DOUBLE):
      std::cerr << gem::any_cast<double>(props.get(keys[i]));
      break;
    case (gem::Properties::STRING):
      std::cerr << "'" << gem::any_cast<std::string>(props.get(keys[i])) << "'";
      break;
    default:
      std::cerr << "<unkown:" << props.get(keys[i]).get_type().name() << ">";
      break;
    }
  }
  std::cerr << std::endl;
}

void video :: getProperties(gem::Properties&props) {
  // nada
  std::vector<std::string>keys=props.keys();
  int i=0;
  for(i=0; i<keys.size(); i++) {
    gem::any unset;
    props.set(keys[i], unset);
  }
}



INIT_VIDEOFACTORY();
