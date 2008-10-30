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

#include <iostream>
#include <fstream>
using namespace std;

namespace apvlv
{

  string helppdf;

  char *
    absolutepath (const char *path)
      {
        static char abpath[512];

        if (g_path_is_absolute (path))
          {
            return (char *) path;
          }

        if (*path == '~')
          {
            snprintf (abpath, sizeof abpath, "%s%s", 
                      getenv ("HOME"), 
                      ++ path);
          }
        else
          {
            realpath (path, abpath);
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
          }
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
