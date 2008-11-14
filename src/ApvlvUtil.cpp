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
#include "ApvlvUtil.hpp"

#include <stdlib.h>
#include <gtk/gtk.h>

#ifdef WIN32
# include <windows.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
using namespace std;

namespace apvlv
{

#ifdef WIN32
	string helppdf = "~\\Startup.pdf";
	string inifile = "~\\_apvlvrc";
	string sessionfile = "~\\_apvlvinfo";
#else
  string helppdf;
  string inifile = "~/.apvlvrc";
  string sessionfile = "~/.apvlvinfo";
#endif

  char *
    absolutepath (const char *path)
      {
        static char abpath[512];

		if (path == NULL)
			return NULL;

        if (g_path_is_absolute (path))
          {
            return (char *) path;
          }

        if (*path == '~')
          {
#ifdef WIN32
            char *home = g_win32_get_package_installation_directory_of_module (NULL);
#else
            char *home = getenv ("HOME");
#endif
            snprintf (abpath, sizeof abpath, "%s%s", 
                      home, 
                      ++ path);
          }
        else
          {
#ifdef WIN32
			GetCurrentDirectoryA (sizeof abpath, abpath);
			strcat (abpath, "\\");
			strcat (abpath, path);
#else
            realpath (path, abpath);
#endif
          }

        return abpath;
      }

  bool 
    filecpy (const char *dst, const char *src)
      {
        ifstream ifs (absolutepath (src));
        ofstream ofs (absolutepath (dst));
        if (ifs.is_open () && ofs.is_open ())
          {
            while (ifs.eof () == false)
              {
                string s;
                getline (ifs, s);
                ofs << s << endl;
              }

            ifs.close ();
            ofs.close ();
			return true;
          }
		return false;
      }

  void
    gtk_insert_widget_inbox (GtkWidget *prev, bool after, GtkWidget *n)
      {
        GtkWidget *parent = gtk_widget_get_parent (prev);
        gtk_box_pack_start (GTK_BOX (parent), n, TRUE, TRUE, 0);

        gint id = after? 1: 0;
        GList *children = gtk_container_get_children (GTK_CONTAINER (parent));
        for (GList *child = children; child != NULL; child = child->next)
          {
            if (child->data == prev) 
              {
                break;
              }
            else
              {
                id ++;
              }
          }
        g_list_free (children);

        gtk_box_reorder_child (GTK_BOX (parent), n, id);

        gtk_widget_show_all (parent);
      }

  void
    logv (const char *level, const char *file, int line, const char *func, const char *ms, ...)
      {
        char p[256], temp[256];
        va_list vap;

        va_start (vap, ms);
        vsprintf (p, ms, vap);
        snprintf (temp, sizeof temp, "[%s] %s: %d: %s(): %s",
                  level, file, line, func,
                  p);
        va_end (vap);

        cerr << temp << endl;
      }
}
