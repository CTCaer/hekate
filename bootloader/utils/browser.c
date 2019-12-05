/*
 * Copyright (c) 2019 Mattytrog
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//start_dir = dir to start browsing from
//required_extn = files of certain extension only
//browser_caption = Title in browser
//get_dir = will return dir or file. If disabled and a folder chosen, error displayed
//on fail, display error, automatically go to sd root

#include <string.h>
#include <stdlib.h>
#include "browser.h"
#include "../config/ini.h"
#include "../gfx/gfx.h"
#include "../gfx/tui.h"
#include "../libs/fatfs/ff.h"
#include "../utils/btn.h"
#include "../utils/util.h"

char * file_browser(const char * start_dir,
  const char * required_extn,
    const char * browser_caption,
      const bool get_dir,
        const bool goto_root_on_fail,
          const bool ASCII_order) {

  u8 max_entries = 55;
  char * browse_dir = (char * ) calloc('0', 256);
  DIR dir;
  FILINFO fno;

  if (start_dir) memcpy(browse_dir + 0, start_dir, strlen(start_dir) + 1);

  char * filelist;
  char * file_sec;
  char * file_sec_untrimmed;
  char * select_display = malloc(256);
  u32 i = 0;
  u8 trimsize = 0;
  u8 trimlen;
  u8 err;
  bool home_selected;
  bool select_item;
  bool back_selected;
  bool directory_selected;
  ment_t * ments = (ment_t * ) malloc(sizeof(ment_t) * (max_entries + 9));

  start:
    while (true) {
      filelist = (char * ) calloc(max_entries, 256);

      file_sec = (char * ) calloc('0', 256);
      file_sec_untrimmed = (char * ) calloc('0', 256);
      trimlen = 0;
      i = 0;
      err = 0;
      home_selected = false;
      select_item = false;
      back_selected = false;
      directory_selected = false;
      gfx_clear(BG_COL);
      gfx_con_setpos(0, 0);

      //j = 0, k = 0;

      memcpy(select_display + 0, "SD:", 4);
      memcpy(select_display + strlen(select_display), browse_dir, strlen(browse_dir) + 1);
      if (!f_opendir( & dir, browse_dir)) {
        for (;;) {
          int res = f_readdir( & dir, & fno);
          if (res || !fno.fname[0]) {

            break;
          }

          if (fno.fattrib & AM_DIR) {
            memcpy(fno.fname + strlen(fno.fname), " <Dir>", 7);
            memcpy(filelist + ((i + 9) * 256), fno.fname, strlen(fno.fname) + 1);
          }

          if (!(fno.fattrib & AM_DIR)) memcpy(filelist + ((i + 9) * 256), fno.fname, strlen(fno.fname) + 1);

          i++;
          if ((i + 9) > (max_entries - 1))
            break;
        }
        f_closedir( & dir);
      }

      if ((i + 9) == 9) {
        err = 1;
        goto out;
      }

      //alphabetical order. Not worth it due to size
      if (ASCII_order) {
        u32 j = 0, k = 0;
        char * temp = (char * ) calloc(1, 256);
        k = i;
        for (i = 0; i < k - 1; i++) {
          for (j = i + 1; j < k; j++) {
            if (strcmp( & filelist[(i + 9) * 256], & filelist[(j + 9) * 256]) > 0) {
              memcpy(temp, & filelist[(i + 9) * 256], strlen( & filelist[(i + 9) * 256]) + 1);
              memcpy( & filelist[(i + 9) * 256], & filelist[(j + 9) * 256], strlen( & filelist[(j + 9) * 256]) + 1);
              memcpy( & filelist[(j + 9) * 256], temp, strlen(temp) + 1);
            }
          }
        }
        free(temp);
      }
      i = 0;
      if (filelist) {
        // Build configuration menu.
        ments[0].type = MENT_CAPTION;
        ments[0].caption = "Select Path:";
        ments[0].color = INFOCOL;
        ments[1].type = MENT_CHGLINE;
        ments[2].type = MENT_DATA;
        ments[2].caption = select_display;
        ments[2].data = "<selected_item>";
        //ments[0].handler = exit_loop;
        ments[3].type = MENT_CHGLINE;
        ments[4].type = MENT_CHGLINE;
        ments[5].type = MENT_DATA;
        ments[5].caption = "Exit";
        ments[5].data = "<home_menu>";
        ments[6].type = MENT_CHGLINE;
        ments[7].type = MENT_DATA;
        ments[7].caption = "Parent (..)";
        ments[7].data = "<go_back>";
        ments[8].type = MENT_CHGLINE;

        while (true) {
          if (i > max_entries || !filelist[(i + 9) * 256])
            break;
          ments[i + 9].type = INI_CHOICE;
          ments[i + 9].caption = & filelist[(i + 9) * 256];
          ments[i + 9].data = & filelist[(i + 9) * 256];

          i++;
        }
      }

      if (!browser_caption) browser_caption = (char * )
      "Select A File";
      if (i > 0) {
        memset( & ments[i + 9], 0, sizeof(ment_t));
        menu_t menu = {
          ments,
          browser_caption,
          0,
          0
        };
        //free(file_sec_untrimmed);
        file_sec_untrimmed = (char * ) tui_do_menu( & menu);
      }

      trimsize = 0;

      if (!strcmp(file_sec_untrimmed + strlen(file_sec_untrimmed) - 11, "<home_menu>")) {
        home_selected = true;
        break;
      } else if (!strcmp(file_sec_untrimmed + strlen(file_sec_untrimmed) - 15, "<selected_item>")) {
        select_item = true;
        trimsize = 15;
        break;
      } else if (!strcmp(file_sec_untrimmed + strlen(file_sec_untrimmed) - 9, "<go_back>")) {
        back_selected = true;
        trimsize = 9;
        break;
      } else if (!strcmp(file_sec_untrimmed + strlen(file_sec_untrimmed) - 6, " <Dir>")) {
        directory_selected = true;
        trimsize = 6;
        break;
      } else break;

      if (i == 0) break;
    }
  trimlen = (strlen(file_sec_untrimmed) - trimsize);
  memcpy(file_sec + 0, file_sec_untrimmed, trimlen);
  if (strlen(browse_dir) == 1) memcpy(browse_dir + 0, "\0", 2);
  recheck:
    if (back_selected) {

      memcpy(browse_dir + strlen(browse_dir), file_sec_untrimmed, trimlen);
      char * back_trim_len = strrchr(browse_dir, '/');
      memcpy(browse_dir + (strlen(browse_dir) - strlen(back_trim_len)), "\0", 2);
      if (strlen(back_trim_len) == 1024) memcpy(browse_dir + 0, "/", 2);
      goto start;
    }

  else if (directory_selected) {

    memcpy(browse_dir + strlen(browse_dir), "/", 2);
    memcpy(browse_dir + strlen(browse_dir), file_sec, strlen(file_sec) + 1);
    goto start;
  } else if (select_item) {
    err = 3;
    goto out;
  } else {
    memcpy(browse_dir + strlen(browse_dir), "/", 2);
    memcpy(browse_dir + strlen(browse_dir), file_sec, strlen(file_sec) + 1);
    if (required_extn) {
      if (strcmp(required_extn, browse_dir + strlen(browse_dir) - strlen(required_extn)))
        err = 2;
    }
  }

  if (home_selected) {
    err = 0;
    browse_dir = NULL;
  }

  out:
    gfx_clear(BG_COL);
  gfx_con_setpos(0, 0);

  if (err == 1) {
    if (goto_root_on_fail) {
      gfx_printf("Empty folder. Please choose another");
      msleep(2000);
      back_selected = true;
      goto recheck;
    } else {
      gfx_printf("Invalid Selection. Exiting");
      msleep(2000);
      return NULL;
    }
  }
  if (err == 2) {
    gfx_printf("File extension incorrect");
    msleep(2000);
    back_selected = true;
    goto recheck;
  }
  if (err == 3) {
    if (!get_dir) {
      gfx_printf("Directory chosen. Choose a file instead");
      msleep(2000);
      back_selected = true;
      goto recheck;
    } else return browse_dir;
  }

  return browse_dir;
}