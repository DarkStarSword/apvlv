/*
* This file is part of the apvlv package
*
* Copyright (C) 2008 Alf.
*
* Contact: Alf <naihe2010@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation; either version 2.1 of
* the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
*/
/* @CPPFILE ApvlvCmds.hpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_CMDS_H_
#define _APVLV_CMDS_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include <gtk/gtk.h>

#include <iostream>
#include <vector>
#include <map>
using namespace std;

namespace apvlv
{
  const guint CmdSize = 6;
  const guint CmdStrLen = 16;

  typedef enum 
    {
      CT_CMD,
      CT_STRING,
      CT_STRING_RETURN
    } cmdType;

  typedef map <guint, const char *> KeyStringMap;
  typedef map <const char *, guint> StringKeyMap;

  class ApvlvCmd;
  typedef vector <guint> ApvlvCmdKeyv;
  typedef map <ApvlvCmdKeyv, ApvlvCmd *> ApvlvCmdMap;

  class ApvlvCmd
    {
  public:
    ApvlvCmd ();

    ApvlvCmd (ApvlvCmd &cmd);

    ~ApvlvCmd ();

    void push (const char *s, cmdType type = CT_CMD);

    void append (GdkEventKey *key);

    const char *append (const char *s);

    bool cmp (ApvlvCmd &cmd);

    void type (cmdType type);

    cmdType type ();

    void bemap (bool bemap);

    bool bemap ();

    void canmap (bool canmap);

    bool canmap ();

    void hascount (bool hascount);

    bool hascount ();

    const char *c_str ();

    ApvlvCmdKeyv *keyvalv_p ();

    ApvlvCmdKeyv keyvalv ();

    void precount (gint precount);

    gint precount ();

    guint keyval (guint id);

    ApvlvCmd *next ();

    void origin (ApvlvCmd *cmd);

    ApvlvCmd *origin ();

  private:
    
    // command type
    cmdType mType;

    // if cmd is be mapped
    bool mBeMap;

    // if cmd can be mapped
    bool mCanMap;

    // if has count
    bool mHasPreCount;

    // how to descripe this command in .apvlvrc
    // like <C-d><C-w>, <S-b>s, or :run, :vsp, ...
    string mStrCommand;

    // key's value list
    ApvlvCmdKeyv mKeyVals;

    // cmd's pre count
    gint mPreCount;

    // next command
    ApvlvCmd *mNext;

    // when a key is map to other, this is the origin key.
    // after a maped key was processed, return to this cmds
    ApvlvCmd *mOrigin;
    };

  class ApvlvCmds
    {
      // New module
  public:
    bool buildmap (const char *os, const char *ms);

  private:
    ApvlvCmd *process (ApvlvCmd *cmd);

    returnType ismap (ApvlvCmdKeyv *ack);

    ApvlvCmd *getmap (const char *os);

    ApvlvCmd *getmap (ApvlvCmd *cmd);

    ApvlvCmdMap mMaps;

    ApvlvCmd *mCmdHead;
    // End New

  public:
    ApvlvCmds ();

    ~ApvlvCmds ();

    void append (GdkEventKey *gev);

  private:

    void translate (GdkEventKey *gev, string *s);

    bool translate (string &s, GdkEventKey *gev);

    bool run ();

    bool getcount ();

    bool getcmd ();

    returnType getmap ();

    void sendmapkey (const char *s);

    enum cmdState
      {
        GETTING_COUNT,
        GETTING_CMD,
        CMD_OK,
      } state;

    static gboolean apvlv_cmds_timeout_cb (gpointer);
    gint timeouttimer;

    bool hasop;
    string count;
    KeyStringMap KS;

    bool buildevent (const char *p);

    void destroyevent ();

    void sendevent ();

    int sendtimer;

    static gboolean ae_send_next (void *);

    vector <GdkEventKey> m_keys;

    };

  extern ApvlvCmds *gCmds;
}

#endif
