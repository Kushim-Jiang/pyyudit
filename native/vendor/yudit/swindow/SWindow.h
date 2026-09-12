/** 
 *  Yudit Unicode Editor Source File
 *
 *  GNU Copyright (C) 1997-2023  Gaspar Sinai <gaspar@yudit.org>  
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2,
 *  dated June 1991. See file COPYYING for details.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#ifndef SWindow_h
#define SWindow_h

#include "swindow/SCanvas.h"
#include "swindow/SAccelerator.h"
#include "swindow/SImage.h"

#include "stoolkit/SExcept.h"
#include "stoolkit/SString.h"
#include "stoolkit/SProperties.h"
#include "stoolkit/SRectangle.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract window toolkit
 */

#define SD_WIN_X 0
#define SD_WIN_Y 0
#define SD_WIN_W 10
#define SD_WIN_H 10

// Introduced for Macintsh preedit style.
class SPreEditor {

public: 
  enum SStyle { Style_Default, Style_Selected };

  virtual void preEditClearMarkedText ()=0;
  /* returns the physical rectangle at insertion point of first clear */
  virtual void preEditInsertMarkedText(const SString& utf8, SStyle style=Style_Default)=0;
  virtual SRectangle preEditGlyphRectangleUTF16(unsigned int charPos)=0;
};

//
// Important: if you use doubleBuffer, dont draw with 
// the drawing methods provided here UNLESS SWindow
// came from SWindowListener
//  virtual void redraw (SWindow* w, int x, int y, 
//      unsigned int width, unsigned int height);
//
class SWindow : public SCanvas
{
public:
  virtual ~SWindow() {}
  virtual bool isShown ()=0;
  virtual bool isModal ()=0; 

/**
 * Clear a reagion (set it to the background)
 * @param x is the upper left corner
 * @param y is the upper top corner
 * @param width is the width of the region to clear
 * @param height is the height of the region to clear
 */
  virtual void clear (int x, int y, unsigned int width, unsigned int height)=0;

/**
 * Copy an area on the window to another area.
 * overlap is ok.
 * @param x is the upper left corner
 * @param y is the upper top corner
 * @param width is the width of the region to copy
 * @param height is the height of the region to copy
 * @param tox is the destination left corner
 * @param toy is the destination top corner
 */
  virtual void copy (int x, int y, unsigned int width, 
       unsigned int height, int tox, int toy)=0;

  virtual void setParent (SWindow* w, int x, int y)=0;
  /* set minimum size of this window */
  virtual void setMinimumSize (unsigned int width, unsigned int height)=0;
  virtual void show ()=0;
  virtual void hide ()=0;
  virtual void resize (unsigned int width, unsigned int height)=0;
 /* move window to relative position */
  virtual void move (int x, int y) = 0;
  virtual void redraw (bool clear, int x, int y, 
       unsigned int width, unsigned int height)=0;
/**
 * Officially this is the application icon and not image.
 */
  virtual void setApplicationImage (const SImage& im)=0;

  // return true if already had it.
  virtual void getKeyboardFocus()=0;
/**
 * Assign a rectangualr clip area. Everything outside this area will be clipped.
 */
  virtual void setClippingArea (int x, int y, 
          unsigned int width, unsigned int height)=0;
/**
 *  remove the clipping area.
 */
  virtual void removeClippingArea ()=0;

/**
 * Start a native input method.
 * @param name is the name of the input method:
 *  like "kinput2"
 * @param properties provide some attributes to the input method.
 * It contains:
 * InputStyle: root over-the-spot off-the-spot
 * @return true if it could be started.
 */
  virtual bool startInputMethod (const SString& name, const SProperties& prop, SPreEditor* preEditor)=0;
  virtual void stopInputMethod ()=0;
  virtual void setInputMethodProperties (const SProperties& prop)=0;
/**
 * Get the current input method.
 * it returns a zero sized string if input method is not started.
 */
  virtual SString getInputMethod ()=0;
  /* return true if input method has a status area. */
  virtual bool hasStatusArea ()=0;
  /* return true if window is visible */
  virtual bool isVisible ()=0;
/**
 * Get an utf8 encoded text from clipboard.
 */
  virtual SString  getClipUTF8(bool isPrimary=true)=0;
/**
 * put and utf8-encoded text to clipboard.
 */
  virtual void putClipUTF8(const SString& utf8, bool isPrimary=true)=0;

  /* set the window title */
  virtual void setTitle (const SString& title)=0;

/**
 * add keyboard accelerator
 */
  virtual void addAccelerator (const SAccelerator& a, SAcceleratorListener* l)=0;
/**
 * remove keyboard accelerator
 */
  virtual void removeAccelerator (const SAccelerator& a, SAcceleratorListener* l)=0;
  /*------ drag and drop --------*/
  virtual void setDroppable (const SStringVector& targets)=0;
  virtual void setModal (SWindow* parent, bool decorated)=0;

/**
 * put this window in the middle of target window
 */
  virtual void center (SWindow* window)=0;

/**
 * wait till window has become invisible
 */
  virtual void wait ()=0;


  // Moved to lower void setSize (unsigned int width, unsigned int height);
  // Moved to lower void setPosition (int x, int y);

  virtual unsigned int getWidth() const=0;
  virtual unsigned int getHeight() const=0;
  virtual int getPositionX() const=0;
  virtual int getPositionY() const=0;

  /*--- get an integer id for the underlying window ---*/
  virtual unsigned long getWindowID() const=0;
  // In double buffer mode only setClippingArea and removeClippingArea
  // can be called outside awt callback SWindowListener::redraw (). 
  virtual void setDoubleBuffer (bool isOn)=0; 
  virtual bool isDoubleBufferEnabled () const=0;

  /* Commit text in input method - OSX input method */
  virtual bool commitInputMethod()=0;

};

class SWindowListener
{
public:
  enum SKey { Key_Undefined, 
     Key_Control_R, Key_Control_L, Key_Alt_L, Key_Alt_R,
     Key_Meta_L, Key_Meta_R, Key_Shift_L, Key_Shift_R, 
     Key_Tab, Key_Space, Key_Left, Key_Right, Key_Up, Key_Down,
     Key_Prior, Key_End, Key_Next, Key_Return, Key_Enter, Key_Home,
     Key_Delete, Key_BackSpace, Key_Clear, Key_Escape, Key_Send,
     Key_F1, Key_F2, Key_F3, Key_F4, Key_F5, Key_F6,
     Key_F7, Key_F8, Key_F9, Key_F10, Key_F11, Key_F12,
     Key_a, Key_A, Key_b, Key_B, Key_c, Key_C, Key_d, Key_D,
     Key_e, Key_E, Key_f, Key_F, Key_g, Key_G, Key_h, Key_H,
     Key_i, Key_I, Key_j, Key_J, Key_k, Key_K, Key_l, Key_L,
     Key_m, Key_M, Key_n, Key_N, Key_o, Key_O, Key_p, Key_P,
     Key_q, Key_Q, Key_r, Key_R, Key_s, Key_S, Key_t, Key_T,
     Key_u, Key_U, Key_x, Key_X, Key_y, Key_Y, Key_v, Key_V,
     Key_w, Key_W, Key_z, Key_Z, Key_slash, Key_period, Key_comma, Key_semicolon, Key_colon, Key_underscore, Key_hash, Key_questionmark,
     Key_1,Key_2,Key_3,Key_4,Key_5,Key_6,Key_7,Key_8,Key_9,Key_0
  };

  virtual ~SWindowListener() {}
  virtual bool windowClose (SWindow* w)=0;
  virtual void redraw (SCanvas* w, int x, int y, 
      unsigned int width, unsigned int height)=0;
  virtual void resized (SWindow* w, int x, int y, 
      unsigned int width, unsigned int height)=0;
  virtual void keyPressed (SWindow * w, SKey key, const SString& s,
          bool ctrl, bool shift, bool meta)=0;
  virtual void keyReleased (SWindow * w, SKey key, const SString& s,
          bool ctrl, bool shift, bool meta)=0;
  virtual void buttonPressed (SWindow * w, int button, int x, int y)=0;
  virtual void buttonReleased (SWindow * w, int button, int x, int y)=0;
  virtual void buttonDragged (SWindow * w, int button, int x, int y)=0;
  virtual void lostKeyboardFocus (SWindow* w)=0;
  // return true if accepts that focus.
  virtual bool gainedKeyboardFocus (SWindow* w)=0;
  virtual void lostClipSelection (SWindow* w)=0;
  virtual void enterWindow (SWindow* w)=0;
  virtual void leaveWindow (SWindow* w)=0;
  virtual bool drop (SWindow* w, const SString& mimetype, const SString& data)=0;
};

int getButtonID (const SString& string);


#endif /* SWindow_h */
