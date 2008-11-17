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

#include "ApvlvView.hpp"
#include "ApvlvCmds.hpp"
#include "ApvlvParams.hpp"
#include "ApvlvUtil.hpp"

#include <iostream>

#include <locale.h>
#include <gtk/gtk.h>

using namespace apvlv;

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  ApvlvParams sParams;
  gParams = &sParams;

  ApvlvCmds sCmds;
  gCmds = &sCmds;

  gchar *ini = absolutepath (inifile.c_str ());
  if (ini != NULL)
  {
    gboolean exist = g_file_test (ini, G_FILE_TEST_IS_REGULAR);
    if (!exist)
      {
#ifdef WIN32
	    filecpy (inifile.c_str (), "~\\apvlvrc.example");
#else
        string file = PREFIX;
        file += "/share/doc/apvlv/apvlvrc.example";
        filecpy (inifile.c_str (), file.c_str ());
#endif
      }
    else
      {
        gParams->loadfile (inifile.c_str ());
      }
    free (ini);
  }
  //  param.debug ();

  ApvlvView sView (argc, argv);
  gView = &sView;

  // build the helppdf string
#ifndef WIN32
  helppdf = string (PREFIX);
  helppdf += "/share/doc/apvlv/Startup.pdf";
#endif

  char *path = absolutepath (helppdf.c_str ());
  if (argc > 1)
    {
      free (path);
      path = absolutepath (argv[1]);
    }
  
  if (path != NULL)
    {
      gView->loadfile (path);
      free (path);
    }

  gView->show ();

  return 0;
}
