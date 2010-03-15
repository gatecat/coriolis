
// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC/LIP6 2008-2009, All Rights Reserved
//
// ===================================================================
//
// $Id$
//
// x-----------------------------------------------------------------x 
// |                                                                 |
// |                   C O R I O L I S                               |
// |          Alliance / Hurricane  Interface                        |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Header  :       "./Utilities.h"                            |
// | *************************************************************** |
// |  U p d a t e s                                                  |
// |                                                                 |
// x-----------------------------------------------------------------x


#ifndef __CRL_UTILITIES_H__
#define __CRL_UTILITIES_H__

#include <cstdarg>
#include <cstdio>

#include <ostream>
#include <iostream>
#include <string>

#include  "hurricane/Commons.h"
#include  "hurricane/Error.h"
#include  "hurricane/Slot.h"


namespace CRL {


  using Hurricane::_TName;
  using Hurricane::Record;
  using Hurricane::Error;
  using std::string;
  using std::ostream;


// -------------------------------------------------------------------
// Class  :  "CRL::System".


  class  System {
    public:
    // Constructor & Destructor.
      static System* create       ();
    // Methods.
      static System* getSystem    ();
      static void    trapSig      ( int sig );
      inline bool    getCatchCore ();
      inline bool    setCatchCore ( bool catchCore );

    private:
    // Internal: Attributes.
      static System* _singleton;
             bool    _catchCore;

    // Constructors & Destructors.
      inline         System       ();
                     System       ( const System &other );
             System& operator=    ( const System &other );
  };


// -------------------------------------------------------------------
// Class  :  "CRL::IoFile ()".
//
// Class wrapper for the C FILE* stream.


  class IoFile {
    public:
    // Constructors.
      inline         IoFile        ( string path="<unbound>" );
    // Methods
      inline bool    isOpen        () const;
      inline bool    eof           () const;
             char*   readLine      ( char* buffer, size_t length );
      inline FILE*   getFile       ();
      inline size_t  getLineNumber () const;
             bool    open          ( const string& mode );
             void    close         ();
      inline void    rewind        ();
    // Hurricane management.
      inline string  _getTypeName  () const;
             string  _getString    () const;
             Record* _getRecord    () const;

    private:
    // Internal - Attributes.
             FILE*   _file;
             string  _path;
             string  _mode;
             size_t  _lineNumber;
             bool    _eof;

    // Internal - Constructor.
                     IoFile       ( const IoFile& );
  };




// -------------------------------------------------------------------
// Function  :  "CRL::_PName()".
//
// Generate strings with prefixes.

  inline string  _PName ( const string& s ) { return ( "CRL::" + s ); }


// Inline Methods.
  inline         System::System        (): _catchCore(true) { }
  inline bool    System::getCatchCore  () { return _catchCore; }
  inline bool    System::setCatchCore  ( bool catchCore ) { return _catchCore = catchCore; }

  inline         IoFile::IoFile        ( string path ): _file(NULL)
                                                      , _path(path)
                                                      , _mode("")
                                                      , _lineNumber(0)
                                                      , _eof(false) {}
  inline bool    IoFile::isOpen        () const { return _file!=NULL; }
  inline bool    IoFile::eof           () const { return _eof; }
  inline FILE*   IoFile::getFile       () { return _file; }
  inline size_t  IoFile::getLineNumber () const { return _lineNumber; }
  inline void    IoFile::rewind        () { if (_file) std::rewind(_file); _lineNumber=0; }
  inline string  IoFile::_getTypeName  () const { return _TName("IoFile"); }


// -------------------------------------------------------------------
// Error Messages.

  extern const char* BadAllocProperty;
  extern const char* BadCreate;
  extern const char* NullDataBase;
  extern const char* NullTechnology;
  extern const char* NullLibrary;
  extern const char* NullCell;
  extern const char* BadFopen;
  extern const char* BadColorValue;


}  // End of CRL namespace.




// -------------------------------------------------------------------
// Class  :  "::tty()".


class tty {
  public:
    enum Flags { Black     = 0
               , Red       = 1
               , Green     = 2
               , Yellow    = 3
               , Blue      = 4
               , Magenta   = 5
               , Cyan      = 6
               , White     = 7
               , Reset     = 9
               , Normal    = 0
               , Bright    = (1<<4)
               , ColorMask = 0x0F
               , TypeMask  = 0xF0
               };
  public:
    inline static void          enable       ();
    inline static void          disable      ();
    inline static bool          enabled      ();
    inline static std::ostream& cr           ( std::ostream& );
    inline static std::ostream& reset        ( std::ostream& );
    inline static std::ostream& bold         ( std::ostream& );
    inline static std::ostream& faint        ( std::ostream& );
    inline static std::ostream& italic       ( std::ostream& );
    inline static std::ostream& underline    ( std::ostream& );
    inline static std::ostream& slowBlink    ( std::ostream& );
    inline static std::ostream& rapidBlink   ( std::ostream& );
    inline static std::ostream& negative     ( std::ostream& );
    inline static std::ostream& conceal      ( std::ostream& );
    inline static std::ostream& underline2   ( std::ostream& );
    inline static std::ostream& normal       ( std::ostream& );
    inline static std::ostream& underlineOff ( std::ostream& );
    inline static std::ostream& blinkOff     ( std::ostream& );
    inline static std::ostream& positive     ( std::ostream& );
    inline static std::ostream& reveal       ( std::ostream& );
    inline static std::string   fgcolor      ( unsigned int );
    inline static std::string   bgcolor      ( unsigned int );
  private:
    static bool        _enabled;
    static const char* _intensity[4];
};


inline void          tty::enable       () { _enabled=true; }
inline void          tty::disable      () { _enabled=false; }
inline bool          tty::enabled      () { return _enabled; }
inline std::ostream& tty::cr           ( std::ostream& o ) { if (_enabled) o<<"\r"      ; return o; }
inline std::ostream& tty::reset        ( std::ostream& o ) { if (_enabled) o<<"\x1b[0m" ; return o; }
inline std::ostream& tty::bold         ( std::ostream& o ) { if (_enabled) o<<"\x1b[1m" ; return o; }
inline std::ostream& tty::faint        ( std::ostream& o ) { if (_enabled) o<<"\x1b[2m" ; return o; }
inline std::ostream& tty::italic       ( std::ostream& o ) { if (_enabled) o<<"\x1b[3m" ; return o; }
inline std::ostream& tty::underline    ( std::ostream& o ) { if (_enabled) o<<"\x1b[4m" ; return o; }
inline std::ostream& tty::slowBlink    ( std::ostream& o ) { if (_enabled) o<<"\x1b[5m" ; return o; }
inline std::ostream& tty::rapidBlink   ( std::ostream& o ) { if (_enabled) o<<"\x1b[6m" ; return o; }
inline std::ostream& tty::negative     ( std::ostream& o ) { if (_enabled) o<<"\x1b[7m" ; return o; }
inline std::ostream& tty::conceal      ( std::ostream& o ) { if (_enabled) o<<"\x1b[8m" ; return o; }
inline std::ostream& tty::underline2   ( std::ostream& o ) { if (_enabled) o<<"\x1b[21m"; return o; }
inline std::ostream& tty::normal       ( std::ostream& o ) { if (_enabled) o<<"\x1b[22m"; return o; }
inline std::ostream& tty::underlineOff ( std::ostream& o ) { if (_enabled) o<<"\x1b[24m"; return o; }
inline std::ostream& tty::blinkOff     ( std::ostream& o ) { if (_enabled) o<<"\x1b[25m"; return o; }
inline std::ostream& tty::positive     ( std::ostream& o ) { if (_enabled) o<<"\x1b[27m"; return o; }
inline std::ostream& tty::reveal       ( std::ostream& o ) { if (_enabled) o<<"\x1b[28m"; return o; }

inline std::string  tty::fgcolor ( unsigned int mask )
{
  if ( not _enabled ) return "";
  std::string sequence ("\x1b[");
  sequence += ((mask&Bright)?"4":"3");
  sequence += ('0'+(mask&ColorMask));
  sequence += "m";
  return sequence;
}

inline std::string  tty::bgcolor ( unsigned int mask )
{
  if ( not _enabled ) return "";
  std::string sequence ("\x1b[");
  sequence += ((mask&Bright)?"10":"9");
  sequence += ('0'+(mask&ColorMask));
  sequence += "m";
  return sequence;
}


// -------------------------------------------------------------------
// Class  :  "::mstream()".
//
// Wrapper around the STL ostream which uses a verbose level to choose
// wether to print or not.


  class mstream : public std::ostream {
    public:
      enum StreamMasks { Verbose0      = (1<<0)
                       , Verbose1      = (1<<1)
                       , Verbose2      = (1<<2)
                       , Info          = (1<<3)
                       , VerboseLevel0 = Verbose0
                       , VerboseLevel1 = Verbose0|Verbose1
                       , VerboseLevel2 = Verbose0|Verbose1|Verbose2
                       };
    public:
      static void          enable       ( unsigned int mask );
      static void          disable      ( unsigned int mask );
      inline               mstream      ( unsigned int mask, std::ostream &s );
      inline unsigned int  getStreamMask() const;
      inline bool          enabled      () const;
    // Overload for formatted outputs.
      template<typename T> inline mstream& operator<< ( T& t );
      template<typename T> inline mstream& operator<< ( T* t );
      template<typename T> inline mstream& operator<< ( const T& t );
      template<typename T> inline mstream& operator<< ( const T* t );
                           inline mstream& flush      ();
    // Overload for manipulators.
                           inline mstream &operator<< ( std::ostream &(*pf)(std::ostream &) );

    // Internal: Attributes.
    private:
      static unsigned int  _activeMask;
             unsigned int  _streamMask;
  };


  inline               mstream::mstream      ( unsigned int mask, std::ostream& s ): std::ostream(s.rdbuf()) , _streamMask(mask) {}  
  inline unsigned int  mstream::getStreamMask() const { return  _streamMask; }
  inline bool          mstream::enabled      () const { return (_streamMask & _activeMask); }
  inline mstream&      mstream::flush        () { if (enabled()) static_cast<std::ostream*>(this)->flush(); return *this; }  
  inline mstream&      mstream::operator<<   ( std::ostream& (*pf)(std::ostream&) ) { if (enabled()) (*pf)(*this); return *this; }

  template<typename T>
  inline mstream& mstream::operator<< ( T& t )
    { if (enabled()) { *(static_cast<std::ostream*>(this)) << t; } return *this; };

  template<typename T>
  inline mstream& mstream::operator<< ( T* t )
    { if (enabled()) { *(static_cast<std::ostream*>(this)) << t; } return *this; };

  template<typename T>
  inline mstream& mstream::operator<< ( const T& t )
    { if (enabled()) { *(static_cast<std::ostream*>(this)) << t; } return *this; };

  template<typename T>
  inline mstream& mstream::operator<< ( const T* t )
    { if (enabled()) { *(static_cast<std::ostream*>(this)) << t; } return *this; };

// Specific non-member operator overload. Must be one for each type.
#define  MSTREAM_V_SUPPORT(Type)                           \
  inline mstream& operator<< ( mstream& o, const Type s )  \
    { if (o.enabled()) { static_cast<std::ostream&>(o) << s; } return o; };

#define  MSTREAM_R_SUPPORT(Type)                           \
  inline mstream& operator<< ( mstream& o, const Type& s ) \
    { if (o.enabled()) { static_cast<std::ostream&>(o) << s; } return o; };

#define  MSTREAM_P_SUPPORT(Type)                           \
  inline mstream& operator<< ( mstream& o, const Type* s ) \
    { if (o.enabled()) { static_cast<std::ostream&>(o) << s; } return o; };

#define  MSTREAM_PR_SUPPORT(Type) \
         MSTREAM_P_SUPPORT(Type)  \
         MSTREAM_R_SUPPORT(Type)

MSTREAM_PR_SUPPORT(std::string);


  // ---------------------------------------------------------------
  // Shared objects.

  extern mstream  cmess0;
  extern mstream  cmess1;
  extern mstream  cmess2;
  extern mstream  cinfo;


# endif