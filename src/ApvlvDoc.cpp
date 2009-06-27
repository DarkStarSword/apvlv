/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvDoc.cpp
 *
 *  Author: Alf <naihe2010@gmail.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvUtil.hpp"
#include "ApvlvView.hpp"
#include "ApvlvDoc.hpp"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkprintoperation.h>
#include <glib/poppler.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
#ifdef HAVE_PTHREAD
  static pthread_mutex_t rendermutex = PTHREAD_MUTEX_INITIALIZER;
#endif
  static GtkPrintSettings *settings = NULL;

  ApvlvDoc::ApvlvDoc (int w, int h, const char *zm, bool cache)
    {
      mCurrentCache = NULL;
#ifdef HAVE_PTHREAD
      if (cache)
        {
          mUseCache = true;
          mLastCache = NULL;
          mNextCache = NULL;
        }
      else
        {
          mUseCache = false;
        }
#endif

      mReady = false;

      mZoominit = false;
      mLines = 50;
      mChars = 80;

      mProCmd = 0;

      mRotatevalue = 0;

      mDoc = NULL;

      mResults = NULL;
      mSearchstr = "";

      mImage = gtk_image_new ();
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (mScrollwin),
                                             mImage);

      mStatus = new ApvlvDocStatus (this);

      gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

      setsize (w, h);

      setzoom (zm);
    }

  ApvlvDoc::~ApvlvDoc ()
    {
      if (mCurrentCache)
        delete mCurrentCache;
#ifdef HAVE_PTHREAD
      if (mUseCache)
        {
          if (mNextCache)
            delete mNextCache;
          if (mLastCache)
            delete mLastCache;
        }
#endif

      if (mFilestr != helppdf)
        {
          savelastposition ();
        }
      mPositions.clear ();

      delete mStatus;
    }

  returnType
    ApvlvDoc::subprocess (int ct, guint key)
      {
        guint procmd = mProCmd;
        mProCmd = 0;
        switch (procmd)
          {
          case 'm':
            markposition (key);
            break;

          case '\'':
            jump (key);
            break;

          case 'z':
            if (key == 'i')
              zoomin ();
            else if (key == 'o')
              zoomout ();
            break;

          default:
            return NO_MATCH;
            break;
          }

        return MATCH;
      }

  returnType
    ApvlvDoc::process (int ct, guint key)
      {
        if (mProCmd != 0)
          {
            return subprocess (ct, key);
          }

        switch (key)
          {
          case GDK_Page_Down:
          case CTRL ('f'):
            nextpage (ct);
            break;
          case GDK_Page_Up:
          case CTRL ('b'):
            prepage (ct);
            break;
          case CTRL ('d'):
            halfnextpage (ct);
            break;
          case CTRL ('u'):
            halfprepage (ct);
            break;
          case ':':
          case '/':
          case '?':
            gView->promptcommand (key);
            return NEED_MORE;
          case 'H':
            scrollto (0.0);
            break;
          case 'M':
            scrollto (0.5);
            break;
          case 'L':
            scrollto (1.0);
            break;
          case CTRL ('p'):
          case GDK_Up:
          case 'k':
            scrollup (ct);
            break;
          case CTRL ('n'):
          case CTRL ('j'):
          case GDK_Down:
          case 'j':
            scrolldown (ct);
            break;
          case GDK_BackSpace:
          case GDK_Left:
          case CTRL ('h'):
          case 'h':
            scrollleft (ct);
            break;
          case GDK_space:
          case GDK_Right:
          case CTRL ('l'):
          case 'l':
            scrollright (ct);
            break;
          case 'R':
            reload ();
            break;
          case 'o':
            gView->open ();
            break;
          case 'O':
            gView->opendir ();
            break;
          case CTRL (']'):
            gotolink (ct);
            break;
          case CTRL ('t'):
            returnlink (ct);
            break;
          case 'r':
            rotate (ct);
            break;
          case 'G':
            markposition ('\'');
            showpage (ct - 1);
            break;
          case 'm':
          case '\'':
          case 'z':
            mProCmd = key;
            return NEED_MORE;
            break;
          default:
            return NO_MATCH;
            break;
          }

        return MATCH;
      }

  ApvlvDoc *
    ApvlvDoc::copy ()
      {
        char rate[16];
        g_snprintf (rate, sizeof rate, "%f", mZoomrate);
        ApvlvDoc *ndoc = new ApvlvDoc (mWidth, mHeight, rate, usecache ());
        ndoc->loadfile (mFilestr, false);
        ndoc->showpage (mPagenum, scrollrate ());
        return ndoc;
      }

  PopplerDocument *
    ApvlvDoc::getdoc () 
      { 
        return mDoc; 
      }

  bool
    ApvlvDoc::savelastposition ()
      {
        gchar *path = absolutepath (sessionfile.c_str ());
        ofstream os (path, ios::app);
        g_free (path);

        if (os.is_open ())
          {
            os << ">";
            os << filename () << "\t";
            os << mPagenum << "\t";
            os << scrollrate ();
            os << "\n";
            os.close ();
            return true;
          }
        return false;
      }

  bool
    ApvlvDoc::loadlastposition ()
      {
        bool ret = false;
        int pagen = 0;
        double scrollr = 0.00;

        char *path = absolutepath (sessionfile.c_str ());
        ifstream os (path, ios::in);
        g_free (path);

        if (os.is_open ())
          {
            string line;

            while ((getline (os, line)) != NULL)
              {
                const char *p = line.c_str ();

                if (*p == '>')
                  {
                    stringstream ss (++ p);

                    string files;
                    ss >> files;

                    if (files == mFilestr)
                      {
                        ss >> pagen >> scrollr;
                        ret = true;
                      }
                  }
              }
            os.close ();
          }

        mScrollvalue = scrollr;
        showpage (pagen);

        // Warning
        // I can't think a better way to scroll correctly when
        // the page is not be displayed correctly
        g_timeout_add (50, apvlv_doc_first_scroll_cb, this);

        return ret;
      }

  bool 
    ApvlvDoc::reload () 
      { 
        savelastposition (); 
        return loadfile (mFilestr, false); 
      }

  bool 
    ApvlvDoc::loadfile (string & filename, bool check) 
      { 
        return loadfile (filename.c_str (), check); 
      }

  bool
    ApvlvDoc::usecache ()
      {
#ifdef HAVE_PTHREAD
        return mUseCache;
#else
        return false;
#endif
      }

  void
    ApvlvDoc::usecache (bool use)
      {
#ifdef HAVE_PTHREAD
        if (use && !mUseCache)
          {
            mUseCache = use;
            mNextCache = new ApvlvDocCache (this);
            mNextCache->set (convertindex (mPagenum + 1), false);
            mLastCache = new ApvlvDocCache (this);
            mLastCache->set (convertindex (mPagenum - 1), true);
          }
        else if (!use && mUseCache)
          {
            mUseCache = use;
            if (mNextCache)
              {
                delete mNextCache;
                mNextCache = NULL;
              }
            if (mLastCache)
              {
                delete mLastCache;
                mLastCache = NULL;
              }
          }
#else
        if (use)
          {
            warnp ("No pthread, can't support cache!!!");
            warnp ("If you really have pthread, please recomplie the apvlv and try again.");
          }
#endif
      }

  bool
    ApvlvDoc::loadfile (const char *filename, bool check)
      {
        if (check)
          {
            if (strcmp (filename, mFilestr.c_str ()) == 0)
              {
                return false;
              }
          }

        mReady = false;

        mDoc = file_to_popplerdoc (filename);
        if (mDoc != NULL)
          {
            mFilestr = filename;

#ifdef HAVE_PTHREAD
            if (mUseCache)
              {
                if (mLastCache != NULL)
                  delete mLastCache;
                if (mNextCache != NULL)
                  delete mNextCache;
                mLastCache = new ApvlvDocCache (this);
                mNextCache = new ApvlvDocCache (this);
              }
#endif
            if (mCurrentCache != NULL)
              delete mCurrentCache;
            mCurrentCache = new ApvlvDocCache (this);

            loadlastposition ();

            mStatus->show ();

            setactive (true);

            mReady = true;
          }

        return mDoc == NULL? false: true;
      }

  gint 
    ApvlvDoc::pagesum () 
      { 
        return mDoc? poppler_document_get_n_pages (mDoc): 0; 
      }

  int
    ApvlvDoc::convertindex (int p)
      {
        if (mDoc != NULL)
          {
            int c = poppler_document_get_n_pages (mDoc);
            if (p >= 0)
              {
                return p % c;
              }
            else if (p < 0)
              {
                return c + p;
              }
          }
        return -1;
      }

  void
    ApvlvDoc::markposition (const char s)
      {
        ApvlvDocPosition adp = { mPagenum, scrollrate () };
        mPositions[s] = adp;
      }

  void
    ApvlvDoc::jump (const char s)
      {
        ApvlvDocPositionMap::iterator it;
        for (it = mPositions.begin (); it != mPositions.end (); ++ it)
          {
            if ((*it).first == s)
              {
                ApvlvDocPosition adp = (*it).second;
                markposition ('\'');
                showpage (adp.pagenum, adp.scrollrate);
                break;
              }
          }
      }

  void
    ApvlvDoc::showpage (int p, double s)
      {
        int rp = convertindex (p);
        if (rp < 0)
          return;

        mPagenum = rp;

#ifdef HAVE_PTHREAD
        if (mUseCache)
          {
            ApvlvDocCache *ac = fetchcache (rp);
            if (ac != NULL)
              {
                GdkPixbuf *buf = ac->getbuf (true);
                asst (buf);
                gtk_image_set_from_pixbuf (GTK_IMAGE (mImage), buf);
                scrollto (s);

                mCurrentCache = ac;
                return;
              }
          }
#endif
        if (mZoominit == false)
          {
            mZoominit = true;

            PopplerPage *mPage = poppler_document_get_page (mDoc, rp);
            getpagesize (mPage, &mPagex, &mPagey);

            switch (mZoommode)
              {
              case NORMAL:
                mZoomrate = 1.2;
                break;
              case FITWIDTH:
                debug ("mWidth: %d, zoom rate: %f", mWidth, mZoomrate);
                mZoomrate = ((double) (mWidth - 26)) / mPagex;
                break;
              case FITHEIGHT:
                mZoomrate = ((double) (mHeight - 26)) / mPagey;
                break;
              case CUSTOM:
                break;
              default:
                break;
              }

            debug ("zoom rate: %f", mZoomrate);
          }

        refresh ();

        scrollto (s);
      }

  void 
    ApvlvDoc::nextpage (int times)
      { 
        showpage (mPagenum + times); 
      }

  void 
    ApvlvDoc::prepage (int times) 
      { 
        showpage (mPagenum - times); 
      }

  void
    ApvlvDoc::refresh ()
      {
        if (mDoc == NULL)
          return;

        mCurrentCache->set (mPagenum, false);
        GdkPixbuf *buf = mCurrentCache->getbuf (true);
        gtk_image_set_from_pixbuf (GTK_IMAGE (mImage), buf);

        debug ("zoom rate: %f", mZoomrate);
        mStatus->show ();

#ifdef HAVE_PTHREAD
        if (mUseCache)
          {
            int rp = convertindex (mPagenum + 1);
            mNextCache->set (rp, true);
            mLastCache->set (mLastCache->getpagenum (), true);
          }
#endif
      }

#ifdef HAVE_PTHREAD
  ApvlvDocCache *
    ApvlvDoc::fetchcache (guint pn)
      {
        ApvlvDocCache *ac = NULL;

        if (mNextCache->getpagenum () == pn)
          {
            ac = mNextCache;
            mNextCache = mLastCache;
            mLastCache = mCurrentCache;
            mNextCache->set (convertindex (pn + 1));
          }

        else if (mLastCache->getpagenum () == pn)
          {
            ac = mLastCache;
            if ((int) mCurrentCache->getpagenum () == convertindex (pn + 1))
              {
                mLastCache = mNextCache;
                mLastCache->set (convertindex (pn - 1));
                mNextCache = mCurrentCache;
              }
            else
              {
                mLastCache = mCurrentCache;
                mLastCache->set (convertindex (pn - 1));
                mNextCache->set (convertindex (pn + 1));
              }
          }

        return ac;
      }
#endif

  void
    ApvlvDoc::halfnextpage (int times)
      {
        double sr = scrollrate ();
        int rtimes = times / 2;

        if (times % 2 != 0)
          {
            if (sr > 0.5)
              {
                sr = 0;
                rtimes += 1;
              }
            else
              {
                sr = 1;
              }
          }

        showpage (mPagenum + rtimes, sr);
      }

  void
    ApvlvDoc::halfprepage (int times)
      {
        double sr = scrollrate ();
        int rtimes = times / 2;

        if (times % 2 != 0)
          {
            if (sr < 0.5)
              {
                sr = 1;
                rtimes += 1;
              }
            else
              {
                sr = 0;
              }
          }

        showpage (mPagenum - rtimes, sr);
      }

  void
    ApvlvDoc::markselection ()
      {
        PopplerRectangle *rect = (PopplerRectangle *) mResults->data;

        gchar *txt = poppler_page_get_text (mCurrentCache->getpage (), POPPLER_SELECTION_GLYPH, rect);
        if (txt == NULL)
          {
            debug ("no search result");
            return;
          }

        // Caculate the correct position
        gint x1 = (gint) ((rect->x1) * mZoomrate);
        gint y1 = (gint) ((mPagey - rect->y2) * mZoomrate);
        gint x2 = (gint) ((rect->x2) * mZoomrate);
        gint y2 = (gint) ((mPagey - rect->y1) * mZoomrate);

        // make the selection at the page center
        gdouble val = ((y1 + y2) - mVaj->page_size) / 2;
        if (val + mVaj->page_size > mVaj->upper)
          {
            gtk_adjustment_set_value (mVaj, mVaj->upper);
          }
        else if (val > 0)
          {
            gtk_adjustment_set_value (mVaj, val);
          }
        else
          {
            gtk_adjustment_set_value (mVaj, mVaj->lower);
          }
        val = ((x1 + x2) - mHaj->page_size) / 2;
        if (val + mHaj->page_size > mHaj->upper)
          {
            gtk_adjustment_set_value (mHaj, mHaj->upper);
          }
        else if (val > 0)
          {
            gtk_adjustment_set_value (mHaj, val);
          }
        else
          {
            gtk_adjustment_set_value (mHaj, mHaj->lower);
          }

        guchar *pagedata = mCurrentCache->getdata (true);
        GdkPixbuf *pixbuf = mCurrentCache->getbuf (true);
        // change the back color of the selection
        for (gint y = y1; y < y2; y ++)
          {
            for (gint x = x1; x < x2; x ++)
              {
                gint p = (gint) (y * 3 * mPagex * mZoomrate + (x * 3));
                pagedata[p + 0] = 0xFF - pagedata[p + 0];
                pagedata[p + 1] = 0xFF - pagedata[p + 0];
                pagedata[p + 2] = 0xFF - pagedata[p + 0];
              }
          }

        gtk_image_set_from_pixbuf (GTK_IMAGE (mImage), pixbuf);

        g_free (rect);
        mResults = g_list_remove (mResults, rect);
      }

  GList *
    ApvlvDoc::searchpage (int num)
      {
        if (mDoc == NULL)
          return NULL;

        PopplerPage *page = poppler_document_get_page (mDoc, num);

        GList *ret = poppler_page_find_text (page, mSearchstr.c_str ());

        return ret;
      }

  bool
    ApvlvDoc::needsearch (const char *str)
      {
        if (mDoc == NULL)
          return false;

        if (strlen (str) > 0)
          {
            g_list_free (mResults);
            mResults = NULL;

            mSearchstr = str;

            return true;
          }
        else
          {
            if (mResults != NULL)
              {
                markselection ();
                return false;
              }
            else
              {
                return true;
              }
          }
      }

  void
    ApvlvDoc::search (const char *str)
      {
        if (needsearch (str))
          {
            int num = poppler_document_get_n_pages (mDoc);
            int i = strlen (str) > 0? mPagenum - 1: mPagenum;
            while (i ++ < num - 1)
              {
                mResults = searchpage (i);
                if (mResults != NULL)
                  {
                    showpage (i);
                    markselection ();
                    break;
                  }
                i ++;
              }
          }
      }

  void
    ApvlvDoc::backsearch (const char *str)
      {
        if (needsearch (str))
          {
            poppler_document_get_n_pages (mDoc);
            int i = strlen (str) > 0? mPagenum + 1: mPagenum;
            while (i -- > 0)
              {
                mResults = g_list_reverse (searchpage (i));
                if (mResults != NULL)
                  {
                    showpage (i);
                    markselection ();
                    break;
                  }
              }
          }
      }

  void
    ApvlvDoc::getpagesize (PopplerPage *p, double *x, double *y)
      {
        if (mRotatevalue == 90
            || mRotatevalue == 270)
          {
            poppler_page_get_size (p, y, x);
          }
        else
          {
            poppler_page_get_size (p, x, y);
          }
      }

  bool
    ApvlvDoc::getpagetext (PopplerPage *page, char **text)
      {
        double x, y;
        getpagesize (page, &x, &y);
        PopplerRectangle rect = { 0, 0, x, y };
        *text = poppler_page_get_text (page, POPPLER_SELECTION_WORD, &rect);
        if (*text != NULL)
          {
            return true;
          }
        return false;
      }

  bool
    ApvlvDoc::totext (const char *file)
      {
        if (mDoc == NULL)
          return false;

        PopplerPage *page = mCurrentCache->getpage ();
        char *txt;
        bool ret = getpagetext (page, &txt);
        if (ret == true)
          {
            g_file_set_contents (file, txt, -1, NULL);
            return true;
          }
        return false;
      }

  void
    ApvlvDoc::setactive (bool act)
      {
        mStatus->active (act);
        mActive = act;
      }

  bool
    ApvlvDoc::rotate (int ct)
      {
        // just hack
        if (ct == 1) ct = 90;

        if (ct % 90 != 0)
          {
            warnp ("Not a 90 times value, ignore.");
            return false;
          }

        mRotatevalue += ct;
        while (mRotatevalue < 0)
          {
            mRotatevalue += 360;
          }
        refresh ();
        return true;
      }

  void
    ApvlvDoc::gotolink (int ct)
      {
        int nn = -1;

        PopplerLinkMapping *map = (PopplerLinkMapping *) g_list_nth_data (mCurrentCache->getlinks (), ct - 1);

        debug ("Ctrl-] %d", ct);
        if (map)
          {
            PopplerAction *act = map->action;
            if (act && * (PopplerActionType *) act == POPPLER_ACTION_GOTO_DEST)
              {
                PopplerDest *pd = ((PopplerActionGotoDest *) act)->dest;
                if (pd->type == POPPLER_DEST_NAMED) 
                  {
                    PopplerDest *destnew = poppler_document_find_dest (mDoc, 
                                                                       pd->named_dest);
                    if (destnew != NULL)
                      {
                        nn = destnew->page_num - 1;
                        debug ("nn: %d", nn);
                        poppler_dest_free (destnew);
                      }
                  }
                else
                  {
                    nn = pd->page_num - 1;
                    debug ("nn: %d", nn);
                  }
              }
          }

        if (nn >= 0)
          {
            markposition ('\'');

            ApvlvDocPosition p = { mPagenum, scrollrate () };
            mLinkPositions.push_back (p);

            showpage (nn);
          }
      }

  void
    ApvlvDoc::returnlink (int ct)
      {
        debug ("Ctrl-t %d", ct);
        if (ct <= (int) mLinkPositions.size () && ct > 0)
          {
            markposition ('\'');
            ApvlvDocPosition p = { 0, 0 };
            while (ct -- > 0)
              {
                p = mLinkPositions[mLinkPositions.size () - 1];
                mLinkPositions.pop_back ();
              }
            showpage (p.pagenum, p.scrollrate);
          }
      }

  bool
    ApvlvDoc::print (int ct)
      {
#ifdef WIN32
        return false;
#else
        bool ret = false;
        GtkPrintOperation *print = gtk_print_operation_new ();

        gtk_print_operation_set_allow_async (print, TRUE);
        gtk_print_operation_set_show_progress (print, TRUE);

        PrintData *data = new PrintData;
        data->doc = mDoc;
        data->frmpn = mPagenum;
        data->endpn = mPagenum;

        g_signal_connect (G_OBJECT (print), "begin-print", G_CALLBACK (begin_print), data);
        g_signal_connect (G_OBJECT (print), "draw-page", G_CALLBACK (draw_page), data);
        g_signal_connect (G_OBJECT (print), "end-print", G_CALLBACK (end_print), data);
        if (settings != NULL)
          {
            gtk_print_operation_set_print_settings (print, settings);
          }
        int r = gtk_print_operation_run (print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW (gView->widget ()), NULL);
        if (r == GTK_PRINT_OPERATION_RESULT_APPLY)
          {
            if (settings != NULL)
              {
                g_object_unref (settings);
              }
            settings = gtk_print_operation_get_print_settings (print);
            ret = true;
          }
        g_object_unref (print);
        return ret;
#endif
      }

  gboolean
    ApvlvDoc::apvlv_doc_first_scroll_cb (gpointer data)
      {
        ApvlvDoc *doc = (ApvlvDoc *) data;
        return doc->scrollto (doc->mScrollvalue) == TRUE? FALSE: TRUE;
      }

  gboolean
    ApvlvDoc::apvlv_doc_first_copy_cb (gpointer data)
      {
        ApvlvDoc *doc = (ApvlvDoc *) data;
        doc->loadfile (doc->mFilestr, false);
        return FALSE;
      }

#ifndef WIN32
  void
    ApvlvDoc::begin_print (GtkPrintOperation *operation,
                           GtkPrintContext   *context,
                           PrintData         *data)
      {
        gtk_print_operation_set_n_pages (operation, data->endpn - data->frmpn + 1);
      }

  void
    ApvlvDoc::draw_page (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         gint               page_nr,
                         PrintData         *data)
      {
        cairo_t *cr = gtk_print_context_get_cairo_context (context);
        PopplerPage *page = poppler_document_get_page (data->doc, data->frmpn + page_nr);
        poppler_page_render_for_printing (page, cr);

        PangoLayout *layout = gtk_print_context_create_pango_layout (context);
        pango_cairo_show_layout (cr, layout);
        g_object_unref (layout);
      }

  void
    ApvlvDoc::end_print (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         PrintData         *data)
      {
        delete data;
      }
#endif

  ApvlvDocCache::ApvlvDocCache (ApvlvDoc *dc)
    {
      mDoc = dc;
      mPagenum = -1;
      mData = NULL;
      mBuf = NULL;
      mLinkMappings = NULL;
#ifdef HAVE_PTHREAD
      mDelay = 0;
      mThreadRunning = false;
      pthread_cond_init (&mCond, NULL);
      pthread_mutex_init (&mMutex, NULL);
#endif
    }

  void
    ApvlvDocCache::set (guint p, bool delay)
      {
#ifdef HAVE_PTHREAD
        if (mThreadRunning)
          {
            pthread_cancel (mTid);
            pthread_mutex_unlock (&rendermutex);
            mThreadRunning = false;
          }
#endif
        mPagenum = p;
        if (mData != NULL)
          {
            delete []mData;
            mData = NULL;
          }
        if (mBuf != NULL)
          {
            g_object_unref (mBuf);
            mBuf = NULL;
          }
#ifdef HAVE_PTHREAD
        if (mDoc->usecache ())
          {
            pthread_cond_init (&mCond, NULL);
            mDelay = delay? 50: 0;
            pthread_create (&mTid, NULL, (void *(*) (void *)) load, this);
          }
        else
#endif
          load (this);
      }

  void
    ApvlvDocCache::load (ApvlvDocCache *ac)
      {
        int c = poppler_document_get_n_pages (ac->mDoc->getdoc ());
        if (ac->mPagenum < 0
            || ac->mPagenum >= c)
          {
            debug ("no this page: %d", ac->mPagenum);
            return;
          }

#ifdef HAVE_PTHREAD
        ac->mThreadRunning = true;
#endif

        PopplerPage *tpage = poppler_document_get_page (ac->mDoc->getdoc (), ac->mPagenum);

        double tpagex, tpagey;
        ac->mDoc->getpagesize (tpage, &tpagex, &tpagey);

        int ix = (int) (tpagex * ac->mDoc->zoomvalue ()), iy = (int) (tpagey * ac->mDoc->zoomvalue ());

        guchar *dat = new guchar[ix * iy * 3];

        GdkPixbuf *bu = gdk_pixbuf_new_from_data (dat, GDK_COLORSPACE_RGB,
                                                  FALSE,
                                                  8,
                                                  ix, iy,
                                                  3 * ix,
                                                  NULL, NULL);

#ifdef HAVE_PTHREAD
        if (ac->mDelay > 0)
          {
            usleep (ac->mDelay * 1000);
          }
        pthread_mutex_lock (&rendermutex);
#endif
        poppler_page_render_to_pixbuf (tpage, 0, 0, ix, iy, ac->mDoc->zoomvalue (), ac->mDoc->getrotate (), bu);
#ifdef HAVE_PTHREAD
        pthread_mutex_unlock (&rendermutex);
#endif

        if (ac->mLinkMappings)
          {
            poppler_page_free_link_mapping (ac->mLinkMappings);
          }
        ac->mLinkMappings = g_list_reverse (poppler_page_get_link_mapping (tpage));
        debug ("has mLinkMappings: %p", ac->mLinkMappings);

        ac->mPage = tpage;
        ac->mData = dat;
        ac->mBuf = bu;

#ifdef HAVE_PTHREAD
        pthread_cond_signal (&ac->mCond);
        ac->mThreadRunning = false;
#endif
      }

  ApvlvDocCache::~ApvlvDocCache ()
    {
#ifdef HAVE_PTHREAD
      if (mThreadRunning)
        {
          pthread_join (mTid, NULL);
          pthread_mutex_unlock (&rendermutex);
          mThreadRunning = false;
        }
#endif
      if (mLinkMappings)
        {
          poppler_page_free_link_mapping (mLinkMappings);
        }

      if (mData != NULL)
        delete []mData;
      if (mBuf != NULL)
        g_object_unref (mBuf);
    }

  PopplerPage *
    ApvlvDocCache::getpage ()
      {
        return mPage;
      }

  guint
    ApvlvDocCache::getpagenum ()
      {
        return mPagenum;
      }

  /*
   * get the cache data
   * @param: wait, if not wait, not wait the buffer be prepared
   * @return: the buffer
   * */
  guchar *ApvlvDocCache::getdata (bool wait)
    {
#ifndef HAVE_PTHREAD
      return mData;
#else
      if (!wait)
        {
          return mData;
        }

      guchar *dat = mData;
      if (dat == NULL)
        {
          pthread_mutex_lock (&mMutex);
          pthread_cond_wait (&mCond, &mMutex);
          dat = mData;
          pthread_mutex_unlock (&mMutex);
        }
      return dat;
#endif
    }

  /*
   * get the cache GdkPixbuf
   * @param: wait, if not wait, not wait the pixbuf be prepared
   * @return: the buffer
   * */
  GdkPixbuf *ApvlvDocCache::getbuf (bool wait)
    {
#ifndef HAVE_PTHREAD
      return mBuf;
#else
      if (!wait)
        {
          return mBuf;
        }

      GdkPixbuf *bu = mBuf;
      if (bu == NULL)
        {
          pthread_cond_wait (&mCond, &mMutex);
          bu = mBuf;
        }
      return bu;
#endif
    }

  GList *
    ApvlvDocCache::getlinks ()
      {
        return mLinkMappings;
      }

  ApvlvDocStatus::ApvlvDocStatus (ApvlvDoc *doc)
    {
      mDoc = doc;
      for (int i=0; i<AD_STATUS_SIZE; ++i)
        {
          mStlab[i] = gtk_label_new ("");
          gtk_box_pack_start (GTK_BOX (mHbox), mStlab[i], FALSE, FALSE, 0);
        }
    }

  ApvlvDocStatus::~ApvlvDocStatus ()
    {
    }

  void
    ApvlvDocStatus::active (bool act)
      {
        GdkColor c;

        if (act)
          {
            c.red = 300;
            c.green = 300;
            c.blue = 300;
          }
        else
          {
            c.red = 30000;
            c.green = 30000;
            c.blue = 30000;
          }

        for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
          {
            gtk_widget_modify_fg (mStlab[i], GTK_STATE_NORMAL, &c);
          }
      }

  void
    ApvlvDocStatus::setsize (int w, int h)
      {
        int sw[AD_STATUS_SIZE];
        sw[0] = w >> 1;
        sw[1] = sw[0] >> 1;
        sw[2] = sw[1] >> 1;
        sw[3] = sw[1] >> 1;
        for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
          {
            gtk_widget_set_usize (mStlab[i], sw[i], h);
          }
      }

  void
    ApvlvDocStatus::show ()
      {
        if (mDoc->filename ())
          {
            char temp[AD_STATUS_SIZE][256];
            gchar *bn;
            bn = g_path_get_basename (mDoc->filename ());
            g_snprintf (temp[0], sizeof temp[0], "%s", bn);
            g_snprintf (temp[1], sizeof temp[1], "%d/%d", mDoc->pagenumber (), mDoc->pagesum ());
            g_snprintf (temp[2], sizeof temp[2], "%d%%", (int) (mDoc->zoomvalue () * 100));
            g_snprintf (temp[3], sizeof temp[3], "%d%%", (int) (mDoc->scrollrate () * 100));
            for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
              {
                gtk_label_set_text (GTK_LABEL (mStlab[i]), temp[i]);
              }
            g_free (bn);
          }
      }
}
