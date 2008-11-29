/****************************************************************************
 * Copyright (c) 1998-2005,2006 Free Software Foundation, Inc.              
 *                                                                          
 * Permission is hereby granted, free of charge, to any person obtaining a  
 * copy of this software and associated documentation files (the            
 * "Software"), to deal in the Software without restriction, including      
 * without limitation the rights to use, copy, modify, merge, publish,      
 * distribute, distribute with modifications, sublicense, and/or sell       
 * copies of the Software, and to permit persons to whom the Software is    
 * furnished to do so, subject to the following conditions:                 
 *                                                                          
 * The above copyright notice and this permission notice shall be included  
 * in all copies or substantial portions of the Software.                   
 *                                                                          
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               
 *                                                                          
 * Except as contained in this notice, the name(s) of the above copyright   
 * holders shall not be used in advertising or otherwise to promote the     
 * sale, use or other dealings in this Software without prior written       
 * authorization.                                                           
 ****************************************************************************/

/****************************************************************************
 *  Author:    YuPengda
 *  AuthorRef: Alf <naihe2010@gmail.com>
 *  Blog:      http://naihe2010.cublog.cn
 ****************************************************************************/
#include "ApvlvParams.hpp"
#include "ApvlvCmds.hpp"
#include "ApvlvView.hpp"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <iostream>
#include <sstream>

namespace apvlv
{
  ApvlvView *gView = NULL;

  ApvlvView::ApvlvView (int argc, char *argv[])
    {
      gtk_init (&argc, &argv);

      mainwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      g_signal_connect (G_OBJECT (mainwindow), "size-allocate",
                        G_CALLBACK (apvlv_view_resized_cb), this);

      int w = atoi (gParams->settingvalue ("width"));
      int h = atoi (gParams->settingvalue ("height"));

      gtk_widget_set_size_request (mainwindow, w, h);

      g_object_set_data (G_OBJECT (mainwindow), "view", this);
      g_signal_connect (G_OBJECT (mainwindow), "key-press-event",
                        G_CALLBACK (apvlv_view_keypress_cb), this);

      GtkWidget *vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_add (GTK_CONTAINER (mainwindow), vbox);

      m_rootWindow = new ApvlvWindow (NULL);
      m_rootWindow->setcurrentWindow (NULL, m_rootWindow);
      gtk_box_pack_start (GTK_BOX (vbox), m_rootWindow->widget (), FALSE, FALSE, 0);

      statusbar = gtk_entry_new ();
      gtk_box_pack_end (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);
      g_object_set_data (G_OBJECT (statusbar), "view", this);
      g_signal_connect (G_OBJECT (statusbar), "key-press-event", G_CALLBACK (apvlv_view_statusbar_cb), this);

      g_signal_connect (G_OBJECT (mainwindow), "delete-event",
                        G_CALLBACK (apvlv_view_delete_cb), this);
      g_signal_connect (G_OBJECT (mainwindow), "destroy-event",
                        G_CALLBACK (apvlv_view_delete_cb), this);

      gtk_widget_show_all (mainwindow);

      pro_cmd = 0;

      full_has = FALSE;

      const char *fs = gParams->settingvalue ("fullscreen");
      if (strcmp (fs, "yes") == 0)
        {
          fullscreen ();
        }
      else
        {
          gtk_widget_set_usize (mainwindow, width, height);
        }

      cmd_hide ();
    }

  ApvlvView::~ApvlvView ()
    {
      delete m_rootWindow;

      map <string, ApvlvDoc *>::iterator it;
      for (it = mDocs.begin (); it != mDocs.end (); ++ it)
        {
          delete it->second;
        }
      mDocs.clear ();
    }

  void
    ApvlvView::show ()
      {
        gtk_main ();
      }

  void 
    ApvlvView::open ()
      {
        GtkWidget *dia = gtk_file_chooser_dialog_new ("",
                                                      GTK_WINDOW (mainwindow),
                                                      GTK_FILE_CHOOSER_ACTION_SAVE,
                                                      GTK_STOCK_CANCEL,
                                                      GTK_RESPONSE_CANCEL,
                                                      GTK_STOCK_OK,
                                                      GTK_RESPONSE_ACCEPT,
                                                      NULL);
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dia), gParams->settingvalue ("defaultdir"));

        GtkFileFilter *filter = gtk_file_filter_new ();
        gtk_file_filter_add_mime_type (filter, "PDF File");
        gtk_file_filter_add_pattern (filter, "*.pdf");
        gtk_file_filter_add_pattern (filter, "*.PDF");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dia), filter);

        gint ret = gtk_dialog_run (GTK_DIALOG (dia));
        if (ret == GTK_RESPONSE_ACCEPT)
          {
            gchar *filename =
              gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dia));

            loadfile (filename);
            g_free (filename);
          }
        gtk_widget_destroy (dia);
      }

  bool
    ApvlvView::loadfile (const char *filename)
      {
        bool ret = false;
        char *abpath = absolutepath (filename);
        ApvlvDoc *ndoc = hasloaded (abpath);
        if (ndoc != NULL)
          {
            currentWindow ()->setDoc (ndoc);
            ret = true;
          }
        else
          {
            ndoc = currentWindow ()->loadDoc (filename);
            if (ndoc)
              {
                mDocs[abpath] = ndoc;
                ret = true;
              }
          }

        g_free (abpath);
        return ret;
      }

  ApvlvDoc *
    ApvlvView::hasloaded (const char *abpath)
      {
        if (mDocs[abpath] != NULL)
          {
            debug ("has loaded: %p", mDocs[abpath]);
            return mDocs[abpath];
          }
        return NULL;
      }

  void 
    ApvlvView::promptsearch ()
      {
        gtk_entry_set_text (GTK_ENTRY (statusbar), "/");
        cmd_mode = SEARCH;
        cmd_show ();
      }

  void 
    ApvlvView::promptbacksearch ()
      {
        gtk_entry_set_text (GTK_ENTRY (statusbar), "?");
        cmd_mode = BACKSEARCH;
        cmd_show ();
      }

  void 
    ApvlvView::promptcommand ()
      {
        gtk_entry_set_text (GTK_ENTRY (statusbar), ":");
        cmd_mode = COMMANDMODE;
        cmd_show ();
      }

  void
    ApvlvView::cmd_show ()
      {
        if (mainwindow == NULL)
          return;

        debug ("cmd show");
        m_rootWindow->setsize (width, height - 20);
        gtk_widget_set_usize (statusbar, width, 20);

        gtk_widget_grab_focus (statusbar);
        gtk_entry_set_position (GTK_ENTRY (statusbar), 1);
        cmd_has = TRUE;
      }

  void
    ApvlvView::cmd_hide ()
      {
        if (mainwindow == NULL)
          return;

        debug ("cmd hide");
        m_rootWindow->setsize (width, height);
        gtk_widget_set_usize (statusbar, width, 1);

        gtk_widget_grab_focus (mainwindow);
        cmd_has = FALSE;
      }

  void 
    ApvlvView::fullscreen () 
      { 
        if (full_has == false)
          {
            gtk_window_maximize (GTK_WINDOW (mainwindow)); 
            full_has = true; 
          }
        else
          {
            gtk_window_unmaximize (GTK_WINDOW (mainwindow)); 
            full_has = false;
          }
      }

  returnType 
    ApvlvView::subprocess (int ct, guint key)
      {
        guint procmd = pro_cmd;
        pro_cmd = 0;
        switch (procmd)
          {
          case CTRL ('w'):
            if (key == 'q'
                || key == CTRL ('Q')
            )
              {
                ApvlvWindow *nwin = currentWindow ()->getneighbor (1, CTRL ('w'));
                if (nwin == NULL)
                  {
                    quit ();
                  }
                else
                  {
                    delete currentWindow ();
                  }
              }
            else
              {
                return currentWindow ()->process (ct, key);
              }
            break;

          case 'm':
            crtadoc ()->markposition (key);
            break;

          case '\'':
            crtadoc ()->jump (key);
            break;

          case 'z':
            if (key == 'i')
              crtadoc ()->zoomin ();
            else if (key == 'o')
              crtadoc ()->zoomout ();
            break;

          default:
            return NO_MATCH;
            break;
          }

        return MATCH;
      }

  returnType 
    ApvlvView::process (int ct, guint key)
      {
        if (pro_cmd != 0)
          {
            return subprocess (ct, key);
          }

        switch (key)
          {
          case GDK_Page_Down:
          case CTRL ('f'):
            crtadoc ()->nextpage (ct);
            break;
          case GDK_Page_Up:
          case CTRL ('b'):
            crtadoc ()->prepage (ct);
            break;
          case CTRL ('d'):
            crtadoc ()->halfnextpage (ct);
            break;
          case CTRL ('u'):
            crtadoc ()->halfprepage (ct);
            break;
          case CTRL ('w'):
            pro_cmd = CTRL ('w');
            return NEED_MORE;
            break;
          case ':':
            promptcommand ();
            return NEED_MORE;
          case '/':
            promptsearch ();
            return NEED_MORE;
          case '?':
            promptbacksearch ();
            return NEED_MORE;
          case 'H':
            crtadoc ()->scrollto (0.0);
            break;
          case 'M':
            crtadoc ()->scrollto (0.5);
            break;
          case 'L':
            crtadoc ()->scrollto (1.0);
            break;
          case CTRL ('p'):
          case GDK_Up:
          case 'k':
            crtadoc ()->scrollup (ct);
            break;
          case CTRL ('n'):
          case CTRL ('j'):
          case GDK_Down:
          case 'j':
            crtadoc ()->scrolldown (ct);
            break;
          case GDK_BackSpace:
          case GDK_Left:
          case CTRL ('h'):
          case 'h':
            crtadoc ()->scrollleft (ct);
            break;
          case GDK_space:
          case GDK_Right:
          case CTRL ('l'):
          case 'l':
            crtadoc ()->scrollright (ct);
            break;
          case 'R':
            crtadoc ()->reload ();
            break;
          case 'o':
            open ();
            break;
          case 'r':
            crtadoc ()->rotate (ct);
            break;
          case 'g':
            crtadoc ()->markposition ('\'');
            crtadoc ()->showpage (ct - 1);
            break;
          case 'm':
            pro_cmd = 'm';
            return NEED_MORE;
            break;
          case '\'':
            pro_cmd = '\'';
            return NEED_MORE;
            break;
          case 'q':
            quit ();
            break;
          case 'f':
            fullscreen ();
            break;
          case 'z':
            pro_cmd = 'z';
            return NEED_MORE;
            break;
          default:
            return NO_MATCH;
            break;
          }

        return MATCH;
      }

  void
    ApvlvView::run (const char *str)
      {
        switch (cmd_mode)
          {
          case SEARCH:
            crtadoc ()->markposition ('\'');
            crtadoc ()->search (str);
            break;

          case BACKSEARCH:
            crtadoc ()->markposition ('\'');
            crtadoc ()->backsearch (str);
            break;

          case COMMANDMODE:
            debug ("run: [%s]", str);
            runcmd (str);
            break;

          default:
            break;
          }
      }

  void 
    ApvlvView::runcmd (const char *str)
      {
        if (*str == '!')
          {
            system (str + 1);
          }
        else
          {
            stringstream ss (str);
            string cmd, subcmd, argu;
            ss >> cmd >> subcmd >> argu;

            if (cmd == "set")
              {
                gParams->settingpush (subcmd, argu);
              }
            else if (cmd == "map")
              {
                gParams->mappush (subcmd, argu);
              }
            else if (cmd == "TOtext")
              {
                crtadoc ()->totext (subcmd.c_str ());
              }
            else if (cmd == "pr" || cmd == "print")
              {
                crtadoc ()->print (atoi (subcmd.c_str ()));
              }
            else if (cmd == "sp")
              {
                currentWindow ()->separate (false);
              }
            else if (cmd == "vsp")
              {
                currentWindow ()->separate (true);
              }
            else if (cmd == "zoom" || cmd == "z")
              {
                crtadoc ()->setzoom (subcmd.c_str ());
              }
            else if (cmd == "forwardpage" || cmd == "fp")
              {
                if (subcmd == "")
                crtadoc ()->nextpage (1);
                else
                crtadoc ()->nextpage (atoi (subcmd.c_str ()));
              }
            else if (cmd == "prewardpage" || cmd == "bp")
              {
                if (subcmd == "")
                crtadoc ()->prepage (1);
                else
                crtadoc ()->prepage (atoi (subcmd.c_str ()));
              }
            else if (cmd == "goto" || cmd == "g")
              {
                crtadoc ()->markposition ('\'');
                crtadoc ()->showpage (atoi (subcmd.c_str ()) - 1);
              }
            else if ((cmd == "help" || cmd == "h")
                     && subcmd == "info")
              {
                loadfile (helppdf);
                crtadoc ()->showpage (1);
              }
            else if ((cmd == "help" || cmd == "h")
                     && subcmd == "command")
              {
                loadfile (helppdf);
                crtadoc ()->showpage (3);
              }
            else if ((cmd == "help" || cmd == "h")
                     && subcmd == "setting")
              {
                crtadoc ()->loadfile (helppdf);
                crtadoc ()->showpage (7);
              }
            else if ((cmd == "help" || cmd == "h")
                     && subcmd == "prompt")
              {
                crtadoc ()->loadfile (helppdf);
                crtadoc ()->showpage (8);
              }
            else if (cmd == "help" || cmd == "h")
              {
                loadfile (helppdf);
              }
            else if (cmd == "q" || cmd == "quit")
              {
                ApvlvWindow *nwin = currentWindow ()->getneighbor (1, CTRL ('w'));
                if (nwin == NULL)
                  {
                    quit ();
                  }
                else
                  {
                    delete currentWindow ();
                  }
              }
          }
      }

  void
    ApvlvView::apvlv_view_resized_cb (GtkWidget * wid, GtkAllocation * al,
                                      ApvlvView * view)
      {
        int w, h;

        gtk_window_get_size (GTK_WINDOW (wid), &w, &h);
        if (w != view->width 
            || h != view->height)
          {
            if (view->cmd_has)
              {
        debug ("here");
                view->m_rootWindow->setsize (w, h - 20);
                gtk_widget_set_usize (view->statusbar, w, 20);
              }
            else
              {
        debug ("here");
                view->m_rootWindow->setsize (w, h);
                gtk_widget_set_usize (view->statusbar, w, 0);
              }

            view->width = w;
            view->height = h;
          }
      }

  gint
    ApvlvView::apvlv_view_keypress_cb (GtkWidget * wid, GdkEvent * ev)
      {
        ApvlvView *view =
          (ApvlvView *) g_object_get_data (G_OBJECT (wid), "view");

        if (view->cmd_has == FALSE)
          {
            gCmds->append ((GdkEventKey *) ev);
            return TRUE;
          }

        return FALSE;
      }

  gint
    ApvlvView::apvlv_view_statusbar_cb (GtkWidget * wid, GdkEvent * ev)
      {
        ApvlvView *view = (ApvlvView *) g_object_get_data (G_OBJECT (wid), "view");

        if (view->cmd_has == TRUE)
          {
            GdkEventKey *gek = (GdkEventKey *) ev;
            if (gek->keyval == GDK_Return)
              {
                gchar *str =
                  (gchar *) gtk_entry_get_text (GTK_ENTRY (view->statusbar));
                if (str && strlen (str) > 0)
                  {
                    view->run (str + 1);
                  }
                view->cmd_hide ();
                return TRUE;
              }
            else if (gek->keyval == GDK_Escape)
              {
                view->cmd_hide ();
                return TRUE;
              }

            return FALSE;
          }

        return FALSE;
      }

  void
    ApvlvView::apvlv_view_delete_cb (GtkWidget * wid, GtkAllocation * al,
                                     ApvlvView * view)
      {
        view->mainwindow = NULL;
        gtk_main_quit ();
      }
}
