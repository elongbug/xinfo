/**************************************************************************
  xinfo

  Copyright 2014-2018 Samsung Electronics co., Ltd. All Rights Reserved.

  Contact: SooChan Lim <sc1.lim@samsung.com>
  Contact: Gwan-gyeong Mun <kk.moon@samsung.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sub license, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice (including the
  next paragraph) shall be included in all copies or substantial portions
  of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
  IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#ifndef NO_I18N
#include <X11/Xlocale.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <xinfo.h>
#include <wait.h>


static void Display_Window_Id(FILE* fd, Window window, Bool newline_wanted);
static void Display_Pointed_Window_Info(FILE* fd);
static void Display_Focused_Window_Info(FILE* fd);

Display *dpy;
Atom prop_pid;
Atom prop_c_pid;
Atom prop_class;
Atom prop_command;

Atom prop_wm_type;
Atom prop_wm_type_desktop;
Atom prop_wm_type_dock;
Atom prop_wm_type_toolbar;
Atom prop_wm_type_menu;
Atom prop_wm_type_utility;
Atom prop_wm_type_splash;
Atom prop_wm_type_dialog;
Atom prop_wm_type_normal;
Atom prop_wm_type_dropdown_menu;
Atom prop_wm_type_popup_menu;
Atom prop_wm_type_tooltip;
Atom prop_wm_type_notification;
Atom prop_wm_type_combo;
Atom prop_wm_type_dnd;

Atom prop_level;

int      screen = 0;
static const char *window_id_format = "0x%lx";
static int win_cnt = 0;


typedef struct {
    long code;
    const char *name;
} binding;

static const binding _map_states[] = {
    { IsUnmapped, "IsUnMapped" },
    { IsUnviewable, "IsUnviewable" },
    { IsViewable, "IsViewable" },
    { 0, 0 } };

typedef struct  _Wininfo {
    struct _Wininfo *prev, *next;
    int idx;
    /*" PID   WinID     w  h Rel_x Rel_y Abs_x Abs_y Depth          WinName      App_Name*/
    long pid;
    Window winid;
    Window BDid;
    int w;
    int h;
    int rel_x;
    int rel_y;
    int abs_x;
    int abs_y;
    unsigned int depth;
    char type[8];
    char level[4];
    char *winname;
    char *appname;
    char *appname_brief;
    char *map_state;
    char *ping_result;
} Wininfo, *WininfoPtr;

/*
 * Standard fatal error routine - call like printf
 * Does not require dpy or screen defined.
 */
void Fatal_Error(const char *msg, ...)
{
    va_list args;
    fflush(stdout);
    fflush(stderr);
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(0);
}

    static void
Display_Pointed_Window_Info(FILE* fd)
{
    int rel_x, rel_y, abs_x, abs_y;
    Window root_win, pointed_win;
    unsigned int mask;

    XQueryPointer(dpy, RootWindow(dpy, 0), &root_win, &pointed_win, &abs_x,
            &abs_y, &rel_x, &rel_y, &mask);

    if(pointed_win)
    {
        Display_Window_Id(fd, pointed_win, True);
    }
    else
    {
        fprintf(fd, "no pointed window\n");
    }


}

    static void
Display_Focused_Window_Info(FILE* fd)
{
    int revert_to;
    Window focus_win;

    XGetInputFocus( dpy, &focus_win, &revert_to);

    if(focus_win)
    {
        Display_Window_Id(fd, focus_win, True);
    }

}

    static void
Display_Window_Id(FILE* fd, Window window, Bool newline_wanted)
{
#ifdef NO_I18N
    char *win_name;
#else
    XTextProperty tp;
#endif
    fprintf(fd, "[winID:");
    fprintf(fd, window_id_format, window);         /* print id # in hex/dec */
    fprintf(fd, "]");
    if (!window) {
        fprintf(fd, " (none)");
    } else {
        if (window == RootWindow(dpy, screen)) {
            fprintf(fd, " (the root window)");
        }
#ifdef NO_I18N
        if (!XFetchName(dpy, window, &win_name)) { /* Get window name if any */
            fprintf(fd, " (has no name)");
        } else if (win_name) {
            fprintf(fd, " \"%s\"", win_name);
            XFree(win_name);
        }
#else
        if (!XGetWMName(dpy, window, &tp)) { /* Get window name if any */
            fprintf(fd, " (has no name)");
        } else if (tp.nitems > 0) {
            fprintf(fd, " \"");
            {
                int count = 0, i, ret;
                char **list = NULL;
                ret = XmbTextPropertyToTextList(dpy, &tp, &list, &count);
                if((ret == Success || ret > 0) && list != NULL){
                    for(i=0; i<count; i++)
                        fprintf(fd, "%s", list[i]);
                    XFreeStringList(list);
                } else {
                    fprintf(fd, "%s", tp.value);
                }
            }
            fprintf(fd, "\"");
        }
#endif
        else
            fprintf(fd, " (has no name)");
    }

    if (newline_wanted)
        fprintf(fd, "\n");

    return;
}

static int
_get_window_property(Display *dpy, Window win, Atom atom, Atom type, unsigned int *val, unsigned int len)
{
    unsigned char *prop_ret;
    Atom type_ret;
    unsigned long bytes_after, num_ret;
    int format_ret;
    unsigned int i;
    int num;

    prop_ret = NULL;
    if (XGetWindowProperty(dpy, win, atom, 0, 0x7fffffff, False,
                           type, &type_ret, &format_ret, &num_ret,
                           &bytes_after, &prop_ret) != Success) {
        return -1;
    }

    if (type_ret != type || format_ret != 32) {
        num = -1;
    } else if (num_ret == 0 || !prop_ret) {
        num = 0;
    } else {
        if (num_ret < len)
            len = num_ret;
        for (i = 0; i < len; i++)
            val[i] = ((unsigned long *)prop_ret)[i];
        num = len;
    }

    if (prop_ret)
        XFree(prop_ret);

    return num;
}


    static WininfoPtr
alloc_wininfo()
{
    WininfoPtr wininfo;
    wininfo = (WininfoPtr) malloc(sizeof(Wininfo));
    if(!wininfo)
    {
        fprintf(stderr, " alloc error \n");
        exit(1);
    }
    wininfo->idx = win_cnt++;
    wininfo->next = NULL;
    wininfo->prev = NULL;
    return wininfo;
}

    static void
free_wininfo(WininfoPtr wininfo)
{
    WininfoPtr w;
    /* TODO : free winname and appname map_state*/
    w = wininfo;
    while(1)
    {

        if(w && w->next)
            w = w->next;
        else
            break;

    }

    while(1)
    {
        if(w && w->prev)
        {
            w = w->prev;
            if(w && w->next)
                free(w->next);
        }
        else
        {
            if(w)
                free(w);
            break;
        }
    }
}



    static void
get_winname(Window window, char* str)
{
    XTextProperty tp;
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    char *class_name = NULL;


    if (window)
    {
        if (window == RootWindow(dpy, screen))
        {
            printf(" (the root window)");
        }

        if (XGetWMName(dpy, window, &tp))
        {
            if (tp.nitems > 0)
            {
                int count = 0, i, ret;
                char **list = NULL;
                ret = XmbTextPropertyToTextList(dpy, &tp, &list, &count);
                if((ret == Success || ret > 0) && list != NULL)
                {
                    for(i=0; i<count; i++)
                        snprintf(str, 255, "%s", *list);
                    XFreeStringList(list);
                }
            }
        }
        else if (XGetWindowProperty(dpy, window, prop_class,
                    0, 10000000L,
                    False, AnyPropertyType,
                    &actual_type, &actual_format, &nitems,
                    &bytes_after, (unsigned char **)&class_name) == Success
                && nitems && class_name != NULL)
        {
            strncpy(str, class_name, strlen(class_name)+1);
            if(class_name)
            {
                XFree(class_name);
                class_name = NULL;
            }


        }
    }
}

    static void
get_appname_brief(char* brief)
{
    char delim[] = "/";
    char *token = NULL;
    char temp[255] = {0,};
    char *saveptr = NULL;

    token = strtok_r(brief, delim, &saveptr);
    while (token != NULL) {
        memset(temp, 0x00, 255*sizeof(char));
        strncpy(temp, token, 254*sizeof(char));

        token = strtok_r(NULL, delim, &saveptr);
    }

    snprintf(brief, sizeof(temp), "%s", temp);
}

    static void
get_appname_from_pid(long pid, char* str)
{
    FILE* fp;
    int len;
    long app_pid = pid;
    char fn_cmdline[255] = {0,};
    char cmdline[255] = {0,};

    snprintf(fn_cmdline, sizeof(fn_cmdline), "/proc/%ld/cmdline",app_pid);

    fp = fopen(fn_cmdline, "r");
    if(fp==0)
    {
        fprintf(stderr,"cannot file open /proc/%ld/cmdline", app_pid);
        exit(1);
    }
    if (!fgets(cmdline, 255, fp)) {
        fprintf(stderr, "fail to get appname for pid(%ld)\n", app_pid);
        fclose(fp);
        exit(1);
    }
    fclose(fp);

    len = strlen(cmdline);
    if(len < 1)
        memset(cmdline, 0x00,255);
    else
        cmdline[len] = 0;

#if 0
    if(strstr(cmdline, "app-domain") != NULL)
    {
        char temp_buf[255] = {0,};
        char* buf = NULL;
        memset(fn_cmdline, 0x00, 255);
        sprintf(fn_cmdline, "/proc/%ld/maps", app_pid);
        fp = fopen(fn_cmdline, "r");
        if(fp==0)
        {
            fprintf(stderr,"cannot file open /proc/%ld/maps", app_pid);
            exit(1);
        }

        while(!feof(fp))
        {
            fgets(temp_buf, 255, fp);
            if(!(buf = strstr(temp_buf, "/com.samsung")))
                continue;
            if(buf != NULL) {
                buf = strstr(buf, "/lib");
                if(buf != NULL) {
                    if(buf[strlen(buf)-1] == '\n')
                        buf[strlen(buf)-4] = 0;
                    buf += 8;
                    memset(cmdline, 0x00, 255);
                    strncpy(cmdline, buf, strlen(buf)+1);
                }
            }
            break;
        }

        fclose(fp);
    }
#endif
    snprintf(str, sizeof(cmdline), "%s", cmdline);
}

static int
init_atoms(void)
{
    prop_wm_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    if (!prop_wm_type) return 0;

    prop_wm_type_desktop = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    if (!prop_wm_type_desktop) return 0;

    prop_wm_type_dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    if (!prop_wm_type_dock) return 0;

    prop_wm_type_toolbar = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
    if (!prop_wm_type_toolbar) return 0;

    prop_wm_type_menu = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    if (!prop_wm_type_menu) return 0;

    prop_wm_type_utility = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_UTILITY", False);
    if (!prop_wm_type_utility) return 0;

    prop_wm_type_splash = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_SPLASH", False);
    if (!prop_wm_type_splash) return 0;

    prop_wm_type_dialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    if (!prop_wm_type_dialog) return 0;

    prop_wm_type_normal = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    if (!prop_wm_type_normal) return 0;

    prop_wm_type_dropdown_menu = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", False);
    if (!prop_wm_type_dropdown_menu) return 0;

    prop_wm_type_popup_menu = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
    if (!prop_wm_type_popup_menu) return 0;

    prop_wm_type_tooltip = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_TOOLTIP", False);
    if (!prop_wm_type_tooltip) return 0;

    prop_wm_type_notification = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False);
    if (!prop_wm_type_notification) return 0;

    prop_wm_type_combo = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_COMBO", False);
    if (!prop_wm_type_combo) return 0;

    prop_wm_type_dnd = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DND", False);
    if (!prop_wm_type_dnd) return 0;

    prop_level = XInternAtom(dpy, "_E_ILLUME_NOTIFICATION_LEVEL", False);
    if (!prop_level) return 0;

    return 1;
}

static char*
get_type_name(Atom atom)
{
    if (atom == prop_wm_type_normal)
        return "Normal ";
    else if (atom == prop_wm_type_notification)
        return "Notific";
    else if (atom == prop_wm_type_utility)
        return "Utility";
    else if (atom == prop_wm_type_dialog)
        return "Dialog ";
    else if (atom == prop_wm_type_popup_menu)
        return "Popup M";
    else if (atom == prop_wm_type_dock)
        return "Dock   ";
    else if (atom == prop_wm_type_desktop)
        return "Desktop";
    else if (atom == prop_wm_type_toolbar)
        return "Toolbar";
    else if (atom == prop_wm_type_menu)
        return "Menu   ";
    else if (atom == prop_wm_type_splash)
        return "Splash ";
    else if (atom == prop_wm_type_dropdown_menu)
        return "Dropdow";
    else if (atom == prop_wm_type_tooltip)
        return "Tooltip";
    else if (atom == prop_wm_type_combo)
        return "Combo  ";
    else if (atom == prop_wm_type_dnd)
        return "Dnd    ";
    else
        return "Unknown";
}

static void
get_type(Window win, char* type)
{
    int ret;
    char *type_name = NULL;
    Atom win_type = 0;

    ret = _get_window_property(dpy, win, prop_wm_type, XA_ATOM, (unsigned int*)&win_type, 1);
    if (ret < 1)
        win_type = 0;

    type_name = get_type_name(win_type);
    strncpy(type, type_name, 7);
    type[7] = '\0';
}

static void
get_level(Window win, char* level)
{
    int ret;
    unsigned int val = 0;

    ret = _get_window_property(dpy, win, prop_level, XA_CARDINAL, &val, 1);
    if (ret < 1)
        val = 0;

    switch (val) {
        case 0:
            strncpy(level, "---", 3);
            break;
        case 50:
        case 51:
            strncpy(level, "Def", 3);
            break;
        case 100:
        case 101:
            strncpy(level, "Med", 3);
            break;
        case 131:
            strncpy(level, "Hig", 3);
            break;
        case 150:
        case 151:
            strncpy(level, "Top", 3);
            break;
        default:
            strncpy(level, "---", 3);
            break;
    }
    level[3] = '\0';
}


/*
 * Lookup: lookup a code in a table.
 */
static char _lookup_buffer[100];

    static const char *
LookupL(long code, const binding *table)
{
    const char *name;

    snprintf(_lookup_buffer, sizeof(_lookup_buffer),
            "unknown (code = %ld. = 0x%lx)", code, code);
    name = _lookup_buffer;

    while (table->name) {
        if (table->code == code) {
            name = table->name;
            break;
        }
        table++;
    }

    return(name);
}

    static const char *
Lookup(int code, const binding *table)
{
    return LookupL((long)code, table);
}

    static int
get_map_status(Window window)
{
    XWindowAttributes win_attributes;

    if (!XGetWindowAttributes(dpy, window, &win_attributes))
        printf("Can't get window attributes.");

    if( win_attributes.map_state == 0 )
        return 0;
    else
        return win_attributes.map_state;
}

static int Ping_Event_Loop()
{
    XEvent e;

    while (XPending (dpy)) {
        XFlush( dpy );
        XNextEvent(dpy, &e);
        /* draw or redraw the window */
        if (e.type == Expose) {
            fprintf(stderr," Expose \n");
        }
        /* exit on key press */
        if (e.type == KeyPress) {
            fprintf(stderr,"  KeyPress  \n");
            break;
        }
        if (e.type ==  ClientMessage ) {
            return 1;
        }
    }
    return 0;
}

    static void
Send_Ping_to_Window(Window window)
{
    XClientMessageEvent xclient;
    Display* display = dpy;

    memset (&xclient, 0, sizeof (xclient));
    xclient.type = ClientMessage;
    xclient.window = window;

    xclient.message_type = XInternAtom (display, "WM_PROTOCOLS",0);

    xclient.format = 32;

    xclient.data.l[0] = XInternAtom (display, "_NET_WM_PING",0);
    xclient.data.l[1] = 0;
    //    fprintf(stderr,"<0x%x>\n", window);
    XSendEvent (display, window, 0,
            None,
            (XEvent *)&xclient);
    XFlush( dpy );

}



void print_default(FILE* fd, Window root_win, int num_children)
{
    fprintf(fd, " \n");
    fprintf(fd, "  Root window id         :");
    Display_Window_Id(fd, root_win, True);
    fprintf(fd, "  Pointed window         :");
    Display_Pointed_Window_Info(fd);
    fprintf(fd, "  Input Focused window   :");
    Display_Focused_Window_Info(fd);
    fprintf(fd, "\n");
    fprintf(fd, "%d Top level windows\n", num_children);
}

void gen_output(XinfoPtr pXinfo, WininfoPtr pWininfo)
{
    int i;
    FILE* fd;
    pid_t pid;

    int val = pXinfo->xinfo_val;
    WininfoPtr w = pWininfo;
    char* name = pXinfo->xinfovalname;
    char* path = pXinfo->pathname;
    char* file = pXinfo->filename;
    char *argument[6];
    char *argument_props_root[3];
    char *argument_props_id[4];
    int child_status=0;

    Window root_win = RootWindow(dpy, screen);
    int num_children = win_cnt;

    argument[2] = (char *)malloc(sizeof(char)*15);
    argument[4] = (char *)malloc(sizeof(char)*255);

    argument[0] = strdup("xwd");
    argument[1] = strdup("-id");
    argument[3] = strdup("-out");
    argument[5] = NULL;

    fd = pXinfo->output_fd;
    fprintf(stderr, "[%s] Start to logging ", name);
    if(path && file)
        fprintf(stderr, "at %s/%s\n", path, file);
    else
        fprintf(stderr, "\n");
    if(val == XINFO_TOPWINS)
    {
        print_default(fd, root_win, num_children);
        fprintf( fd, "----------------------------------[ %s ]-------------------------------------------------------------------------------\n", name);
        fprintf( fd, " No    PID    WinID     w    h   Rel_x Rel_y Abs_x Abs_y Depth          WinName                      AppName\n" );
        fprintf( fd, "----------------------------------------------------------------------------------------------------------------------------\n" );

        for(i = 0; i < win_cnt; i++)
        {
            fprintf(fd, "%3i %6ld  0x%-7lx %4i %4i %5i %5i %5i %5i %5i  %-35s %-35s\n",
                    (w->idx+1),w->pid,w->winid,w->w,w->h,w->rel_x,w->rel_y,w->abs_x,w->abs_y,w->depth,w->winname,w->appname_brief);

            w = w->next;
        }
    }
    else if(val == XINFO_TOPVWINS)
    {
        print_default(fd, root_win, num_children);
        fprintf( fd, "----------------------------------[ %s ]-------------------------------------------------------------------------------------------------------------\n", name);
        fprintf( fd, " No    PID  BorderID    WinID      w    h   Rel_x Rel_y Abs_x  Abs_y  Depth  Type   Level  WinName                   AppName                     map state \n" );
        fprintf( fd, "-----------------------------------------------------------------------------------------------------------------------------------------------------------\n" );

        for(i = 0; i < win_cnt; i++)
        {
            fprintf( fd, "%3i %6ld  0x%-7lx 0x%-8lx %4i %4i %5i %5i %6i %6i %5i  %-7s  %-3s   %-25s %-25s %s\n",
                    (w->idx+1),w->pid,w->BDid,w->winid,w->w,w->h,w->rel_x,w->rel_y,w->abs_x,w->abs_y,w->depth, w->type, w->level,w->winname, w->appname_brief,w->map_state);
            w = w->next;
        }
    }
    else if(val == XINFO_TOPVWINS_PROPS)
    {
        print_default(fd, root_win, num_children);

        fprintf( fd, "###[ Root  Window ]###\n");
        fprintf( fd, "ID: 0x%lx\n", root_win);
        fprintf( fd, "######\n" );
        argument_props_root[0] = strdup("/usr/bin/xprop");
        argument_props_root[1] = strdup("-root");
        argument_props_root[2] = NULL;
        fflush(fd);
        switch(pid = fork())
        {
            case 0:
                if (execv("/usr/bin/xprop", argument_props_root) == -1)
                {
                    fprintf(fd, "failed to excute 'xprop -root'\n");
                    exit(1);
                }
                break;
            default:
                break;
        }
        waitpid(pid, &child_status, WUNTRACED);
        fflush(fd);

        argument_props_id[0] = strdup("/usr/bin/xprop");
        argument_props_id[1] = strdup("-id");
        argument_props_id[2] = (char *)malloc(sizeof(char)*15);
        argument_props_id[3] = NULL;
        for(i = 0; i < win_cnt; i++)
        {
            fprintf( fd, "\n###[ Window ID: 0x%x ]###\n", (unsigned int)w->winid);
            fprintf( fd, "PID: %ld\n",  w->pid);
            fprintf( fd, "Border ID: 0x%x\n",  (unsigned int)w->BDid);
            fprintf( fd, "###\n" );
            snprintf(argument_props_id[2], 15, "0x%-7x", (unsigned int)w->BDid);
            fflush(fd);
            switch(pid = fork())
            {
                case 0:
                    if (execv("/usr/bin/xprop", argument_props_id) == -1)
                    {
                        fprintf(stderr, "failed to excute 'xprop' \n");
                        exit(1);
                    }
                    break;
                default:
                    break;
            }
            waitpid(pid, &child_status, WUNTRACED);
            fflush(fd);
            fprintf( fd, "###\n" );
            fprintf( fd, "Win Name : %s\n",  w->winname);
            fprintf( fd, "Window ID: 0x%x\n",  (unsigned int)w->winid);
            fprintf( fd, "######\n" );
            snprintf(argument_props_id[2], 15, "0x%-7x", (unsigned int)w->winid);
            fflush(fd);
            switch(pid = fork())
            {
                case 0:
                    if (execv("/usr/bin/xprop", argument_props_id) == -1)
                    {
                        fprintf(stderr, "failed to excute 'xprop' \n");
                        exit(1);
                    }
                    break;
                default:
                    break;
            }
            waitpid(pid, &child_status, WUNTRACED);
            fflush(fd);
            fprintf( fd, "######\n" );

            w = w->next;
        }
        free(argument_props_id[0]);
        free(argument_props_id[1]);
        free(argument_props_id[2]);

        free(argument_props_root[0]);
        free(argument_props_root[1]);
    }
    else if(val == XINFO_PING)
    {
        print_default(fd, root_win, num_children);
        fprintf( fd, "----------------------------------[ %s ]-----------------------------------------------------------------------------------------------------\n", name);
        fprintf( fd, " No    PID    WinID     w    h           WinName              AppName                           Ping                         Command\n" );
        fprintf( fd, "-----------------------------------------------------------------------------------------------------------------------------------------------\n" );

        for(i = 0; i < win_cnt; i++)
        {
            fprintf( fd, "%3i %6ld  0x%-7lx %4i %4i %-30s %-30s %-20s %-40s\n",
                    (w->idx+1),w->pid,w->winid,w->w,w->h, w->winname, w->appname_brief,w->ping_result,w->appname);

            w = w->next;
        }
    }
    else if(val == XINFO_XWD_TOPVWINS)
    {
        fprintf( stderr,  " [START] %s : the path to store the xwd files is %s \n", name, path);

        /* dump root window */
        snprintf(argument[2], 15,"0x%-7lx", root_win);
        snprintf(argument[4], 255,"%s/root_win.xwd", path);

        switch(pid = fork())
        {
            case 0:
                if (execv("/usr/bin/xwd", argument) == -1)
                {
                    fprintf(stderr, "execv error root xwd\n");
                    exit(1);
                }
                break;
            default:
                break;
        }

        for(i = 0; i < win_cnt; i++)
        {
            snprintf(argument[2], 15, "0x%-7lx", w->winid);
            snprintf(argument[4], 255, "%s/0x%-7lx.xwd", path, w->winid);

            switch(pid = fork())
            {
                case 0:
                    if (execv("/usr/bin/xwd", argument) == -1)
                    {
                        fprintf(stderr, "execv error other xwd\n");
                        exit(1);
                    }
                    break;
                default:
                    break;
            }
            w = w->next;
        }
        fprintf( stderr,  " [FINISH] %s : the path to store the xwd files is %s \n", name, path);
    }
    else if(val == XINFO_XWD_WIN)
    {
        fprintf( stderr,  " [START] %s : the path to store the xwd files is %s \n", name, path);

        snprintf(argument[2], 15, "0x%-7x", pXinfo->win);
        snprintf(argument[4], 255, "%s/0x%-7x.xwd", path, pXinfo->win);

        switch(pid = fork())
        {
            case 0:
                if (execv("/usr/bin/xwd", argument) == -1)
                {
                    fprintf(stderr, "execv error window xwd\n");
                    exit(1);
                }
                break;
            default:
                break;
        }
        fprintf( stderr,  " [FINISH] %s : the path to store the xwd files is %s \n", name, path);
    }
    fprintf(stderr, "[%s] Finish to logging ", name);
    if(path && file)
        fprintf(stderr, "at %s/%s\n", path, file);
    else
        fprintf(stderr, "\n");
    free(name);
    if (argument[1])
        free (argument[1]);
    if(argument[2])
        free(argument[2]);
    if(argument[3])
        free(argument[3]);
    if(argument[4])
        free(argument[4]);
}


static Window get_client_window(Window parent)
{
    Window win = 0, root, p2;
    Window *children = NULL;
    XWindowAttributes attr;
    unsigned int num;
    int i;

    if (!XQueryTree(dpy, parent, &root, &p2, &children, &num))
        Fatal_Error("Can't query top-level window tree.");

    for (i = 0; i < (int)num; i++)
    {
        if (!XGetWindowAttributes(dpy, children[i], &attr))
            continue;

        if ((attr.map_state == IsViewable) &&
            (attr.override_redirect == False))
        {
            win = children[i];
            free(children);
            return win;
        }
        else
        {
            win = get_client_window(children[i]);
            if (win)
            {
                free(children);
                return win;
            }
        }
    }

    free(children);
    return 0;
}


static Window get_user_created_window(Window win)
{

    Atom type_ret = 0;
    int ret, size_ret = 0;
    unsigned long num_ret = 0, bytes = 0;
    unsigned char *prop_ret = NULL;
    unsigned int xid;
    Atom prop_user_created_win;
    Window client;

    prop_user_created_win = XInternAtom(dpy, "_E_USER_CREATED_WINDOW", False);

    ret = XGetWindowProperty(dpy, win, prop_user_created_win, 0L, 1L,
            False, XA_WINDOW, &type_ret, &size_ret,
            &num_ret, &bytes, &prop_ret);

    if( ret != Success )
    {
        if( prop_ret ) XFree( (void*)prop_ret );
        goto fallback;
    }
    else if( !prop_ret )
    {
        goto fallback;
    }

    memcpy( &xid, prop_ret, sizeof(unsigned int) );
    XFree( (void *)prop_ret );

    return xid;

fallback:
    client = get_client_window(win);
    if (client) return client;
    else return win;
}

#if 0
static int cb_x_error(Display *disp, XErrorEvent *ev)
{
    return 0;
}
#endif

    void
display_topwins(XinfoPtr pXinfo)
{
    int i;
    int rel_x, rel_y, abs_x, abs_y;
    unsigned int width, height, border, depth;
    Window root_win, parent_win;
    unsigned int num_children;
    Window *child_list;
    long *pid = NULL;
    //    char *command_name = NULL;
    //    char *class_name = NULL;
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    Window window;

    int wininfo_val = pXinfo->xinfo_val;

    /* open a display and get a default screen */
    dpy = XOpenDisplay(0);
    if(!dpy)
    {
        printf("Fail to open display\n");
        exit(0);
    }

    /* get a screen*/
    screen = DefaultScreen(dpy);

    /* get a root window id */
    window = RootWindow(dpy, screen);

    //    /* set a error handler */
    //    m_old_error = XSetErrorHandler(cb_x_error);

    if(wininfo_val == XINFO_PING)
        XSelectInput (dpy, window, ExposureMask|     SubstructureNotifyMask);

    /* XINFO_XWD_WIN do not need to gethering window information */
    if(wininfo_val == XINFO_XWD_WIN)
    {
        gen_output(pXinfo, NULL);
        return;
    }

    /* query a window tree */
    if (!XQueryTree(dpy, window, &root_win, &parent_win, &child_list, &num_children))
        Fatal_Error("Can't query window tree.");

    /* get a properties */
    prop_pid = XInternAtom(dpy, "_NET_WM_PID", False);
    prop_c_pid = XInternAtom(dpy, "X_CLIENT_PID", False);
    prop_class = XInternAtom(dpy, "WM_CLASS", False);
    prop_command = XInternAtom(dpy, "WM_COMMAND", False);

    init_atoms();

    /* gathering the wininfo infomation */
    WininfoPtr prev_wininfo = NULL;
    WininfoPtr cur_wininfo = NULL;
    WininfoPtr origin_wininfo = NULL;
    int map_state;

    /* Make the wininfo list */
    for (i = (int)num_children - 1; i >= 0; i--)
    {
        if(wininfo_val == XINFO_TOPVWINS || wininfo_val == XINFO_XWD_TOPVWINS || wininfo_val == XINFO_TOPVWINS_PROPS)
        {
            /* figure out whether the window is mapped or not */
            map_state = get_map_status(child_list[i]);
            if(!map_state)
                continue;
            else
            {
                cur_wininfo = alloc_wininfo();
                cur_wininfo->map_state = (char*) calloc(1, 255*sizeof(char));
                snprintf(cur_wininfo->map_state, 255, "  %s  ",  Lookup(map_state, _map_states));

                /* get the winid */
                cur_wininfo->winid = child_list[i];
                cur_wininfo->BDid = child_list[i];
            }
        }
        else if(wininfo_val == XINFO_TOPWINS || wininfo_val == XINFO_PING)
        {
            cur_wininfo = alloc_wininfo();

            /* get the winid */
            cur_wininfo->winid = child_list[i];
            cur_wininfo->BDid = child_list[i];
        }
        else
        {
            printf("Error . no valid win_state\n");
            exit(1);
        }

        if(prev_wininfo)
        {
            prev_wininfo->next = cur_wininfo;
            cur_wininfo->prev = prev_wininfo;
        }
        else
            origin_wininfo = cur_wininfo;

        /* set the pre_wininfo is the cur_wininfo now */
        prev_wininfo = cur_wininfo;
    }

    if (origin_wininfo == NULL)
    {
        if (child_list) free(child_list);
        return;
    }

    /* check the window generated in client side not is done by window manager */
    WininfoPtr w = origin_wininfo;
    Window winid = 0;
    for(i = 0; i < win_cnt; i++)
    {
        winid = get_user_created_window(w->winid);
        w->winid = winid;

        /* move to the next wininfo */
        w = w->next;
    }


    /* check other infomation out  */
    w = origin_wininfo;
    for(i = 0; i < win_cnt; i++)
    {
        /* get pid */
        if (XGetWindowProperty(dpy, w->winid, prop_pid,
                    0, 2L,
                    False, XA_CARDINAL,
                    &actual_type, &actual_format, &nitems,
                    &bytes_after, (unsigned char **)&pid) == Success
                && nitems && pid != NULL)
        {
            w->pid = *pid;
        }
        else if (XGetWindowProperty(dpy, w->winid, prop_c_pid,
                    0, 2L,
                    False, XA_CARDINAL,
                    &actual_type, &actual_format, &nitems,
                    &bytes_after, (unsigned char **)&pid) == Success
                && nitems && pid != NULL)
        {
            w->pid = *pid;
        }
        else
        {
            w->pid = 0;
        }

        Window child;
        /* get the geometry info and depth*/
        if (XGetGeometry(dpy, w->winid, &root_win, &rel_x, &rel_y, &width, &height, &border, &depth))
        {
            w->w = width;
            w->h = height;
            w->rel_x = rel_x;
            w->rel_y = rel_y;

            if (XTranslateCoordinates (dpy, w->winid, root_win, 0 ,0, &abs_x, &abs_y, &child))
            {
                w->abs_x = abs_x -border;
                w->abs_y = abs_y - border;
            }
        }

        /* get the depth */
        w->depth = depth;

        get_type(w->winid, w->type);
        get_level(w->winid, w->level);

        /* get the winname */
        w->winname = (char*) calloc(1, 255*sizeof(char));
        get_winname(w->winid, w->winname);
#if 0
        /* get app_name from WM_COMMAND or WM_CLASS */
        w->appname = (char*) calloc(1, 255*sizeof(char));
        if (XGetWindowProperty(dpy, w->winid, prop_command,
                    0, 10000000L,
                    False, AnyPropertyType,
                    &actual_type, &actual_format, &nitems,
                    &bytes_after, (unsigned char **)&command_name) == Success
                && nitems && command_name != NULL)
        {
            strncpy(w->appname, command_name, strlen(command_name)+1);
        }
        else
        {
            if (XGetWindowProperty(dpy, w->winid, prop_class,
                        0, 10000000L,
                        False, AnyPropertyType,
                        &actual_type, &actual_format, &nitems,
                        &bytes_after, (unsigned char **)&class_name) == Success
                    && nitems && class_name != NULL)
            {
                strncpy(w->appname, class_name, strlen(class_name)+1);
            }
            else
            {
                if (w->appname)
                    free (w->appname);
                w->appname = NULL;
            }
        }
        if(command_name)
        {
            XFree(command_name);
            command_name = NULL;
        }
        if(class_name)
        {
            XFree(class_name);
            class_name = NULL;
        }

        if (w->appname) {
            w->appname_brief = (char*) calloc(1, 255*sizeof(char));

            strncpy(w->appname_brief, w->appname, 255*sizeof(char));

            get_appname_brief (w->appname_brief);
        }
#else
        /* get app_name from pid */
        if(w->pid)
        {
            w->appname = (char*) calloc(1, 255*sizeof(char));
            if (w->appname)
            {
                get_appname_from_pid(w->pid, w->appname);
                w->appname_brief = (char*) calloc(1, 255*sizeof(char));

                strncpy(w->appname_brief, w->appname, 255*sizeof(char));

                get_appname_brief (w->appname_brief);
            }
        }
        else
            w->appname = NULL;
#endif
        if (pid)
        {
            XFree(pid);
            pid = NULL;
        }

        /* ping test info */
        if(wininfo_val == XINFO_PING)
        {
            struct timespec tim, tim2;
            tim.tv_sec = 0;
            tim.tv_nsec = 50000000;
            Send_Ping_to_Window(w->winid);
            nanosleep(&tim, &tim2);
            w->ping_result = (char*) calloc(1, 255*sizeof(char));
            if(Ping_Event_Loop())
                snprintf(w->ping_result, sizeof("  Success  "), "  Success  ");
            else
                snprintf(w->ping_result, sizeof("  Fail  "), "  Fail  ");
        }

        /* move to the next wininfo */
        w = w->next;
    }



    /* ping test */
    gen_output(pXinfo, origin_wininfo);

    if (child_list)
        XFree((char *)child_list);

    free_wininfo(origin_wininfo);

}




