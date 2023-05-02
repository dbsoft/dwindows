// An example Dynamic Windows application and testing
// ground for Dynamic Windows features in C++.
// By: Brian Smith and Mark Hessling
#include "dw.hpp"
#include <cstdio>
#include <errno.h>

// Select a fixed width font for our platform 
#ifdef __OS2__
#define FIXEDFONT "5.System VIO"
#define PLATFORMFOLDER "os2\\"
#elif defined(__WIN32__)
#define FIXEDFONT "10.Lucida Console"
#define PLATFORMFOLDER "win\\"
#elif defined(__MAC__)
#define FIXEDFONT "9.Monaco"
#define PLATFORMFOLDER "mac/"
#elif defined(__IOS__)
#define FIXEDFONT "9.Monaco"
#elif defined(__ANDROID__)
#define FIXEDFONT "10.Monospace"
#elif GTK_MAJOR_VERSION > 1
#define FIXEDFONT "10.monospace"
#define PLATFORMFOLDER "gtk/"
#else
#define FIXEDFONT "fixed"
#endif

#define SHAPES_DOUBLE_BUFFERED  0
#define SHAPES_DIRECT           1
#define DRAW_FILE               2

#define APP_TITLE "Dynamic Windows C++"
#define APP_EXIT "Are you sure you want to exit?"

#define MAX_WIDGETS 20
#define BUF_SIZE 1024

// Handle the case of very old compilers by using
// A simple non-lambda example instead.
#ifndef DW_LAMBDA

// Simple C++ Dynamic Windows Example

class DWTest : public DW::Window
{
public:
    DW::App *app;

    DWTest() {
        app = DW::App::Init();

        SetText(APP_TITLE);
        SetSize(200, 200);
     }
     int OnDelete() override { 
         if(app->MessageBox(APP_TITLE, DW_MB_YESNO | DW_MB_QUESTION, APP_EXIT) != 0) {
             app->MainQuit();
         }
         return FALSE;
     }
};

int button_clicked(DW::Clickable *classptr)
{
    DW::App *app = DW::App::Init();
    app->MessageBox("Button", DW_MB_OK | DW_MB_INFORMATION, "Clicked!"); 
    return TRUE; 
}

int exit_handler(DW::Clickable *classptr)
{
    DW::App *app = DW::App::Init();
    if(app->MessageBox(APP_TITLE, DW_MB_YESNO | DW_MB_QUESTION, APP_EXIT) != 0) {
        app->MainQuit();
    }
    return TRUE; 
}

int dwmain(int argc, char* argv[]) 
{
    DW::App *app = DW::App::Init(argc, argv, "org.dbsoft.dwindows.dwtestoo");

    app->MessageBox(APP_TITLE, DW_MB_OK | DW_MB_INFORMATION, 
                    "Warning: You are viewing the simplified version of this sample program.\n\n" \
                    "This is because your compiler does not have lambda support.\n\n" \
                    "Please upgrade to Clang 3.3, GCC 5 or Visual Studio 2015 to see the full sample.");

    DWTest *window = new DWTest();
    DW::Button *button = new DW::Button("Test window");

    window->PackStart(button, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, TRUE, 0);
    button->ConnectClicked(&button_clicked);

    DW::MenuBar *mainmenubar = window->MenuBarNew();

    // add menus to the menubar
    DW::Menu *menu = new DW::Menu();
    DW::MenuItem *menuitem = menu->AppendItem("~Quit");
    menuitem->ConnectClicked(&exit_handler);

    // Add the "File" menu to the menubar...
    mainmenubar->AppendItem("~File", menu);

    window->Show();

    app->Main();
    app->Exit(0);

    return 0;
}
#else
class DWTest : public DW::Window
{
private:
    std::string ResolveKeyName(int vk) {
        std::string keyname;
        switch(vk) {
            case  VK_LBUTTON : keyname =  "VK_LBUTTON"; break;
            case  VK_RBUTTON : keyname =  "VK_RBUTTON"; break;
            case  VK_CANCEL  : keyname =  "VK_CANCEL"; break;
            case  VK_MBUTTON : keyname =  "VK_MBUTTON"; break;
            case  VK_TAB     : keyname =  "VK_TAB"; break;
            case  VK_CLEAR   : keyname =  "VK_CLEAR"; break;
            case  VK_RETURN  : keyname =  "VK_RETURN"; break;
            case  VK_PAUSE   : keyname =  "VK_PAUSE"; break;
            case  VK_CAPITAL : keyname =  "VK_CAPITAL"; break;
            case  VK_ESCAPE  : keyname =  "VK_ESCAPE"; break;
            case  VK_SPACE   : keyname =  "VK_SPACE"; break;
            case  VK_PRIOR   : keyname =  "VK_PRIOR"; break;
            case  VK_NEXT    : keyname =  "VK_NEXT"; break;
            case  VK_END     : keyname =  "VK_END"; break;
            case  VK_HOME    : keyname =  "VK_HOME"; break;
            case  VK_LEFT    : keyname =  "VK_LEFT"; break;
            case  VK_UP      : keyname =  "VK_UP"; break;
            case  VK_RIGHT   : keyname =  "VK_RIGHT"; break;
            case  VK_DOWN    : keyname =  "VK_DOWN"; break;
            case  VK_SELECT  : keyname =  "VK_SELECT"; break;
            case  VK_PRINT   : keyname =  "VK_PRINT"; break;
            case  VK_EXECUTE : keyname =  "VK_EXECUTE"; break;
            case  VK_SNAPSHOT: keyname =  "VK_SNAPSHOT"; break;
            case  VK_INSERT  : keyname =  "VK_INSERT"; break;
            case  VK_DELETE  : keyname =  "VK_DELETE"; break;
            case  VK_HELP    : keyname =  "VK_HELP"; break;
            case  VK_LWIN    : keyname =  "VK_LWIN"; break;
            case  VK_RWIN    : keyname =  "VK_RWIN"; break;
            case  VK_NUMPAD0 : keyname =  "VK_NUMPAD0"; break;
            case  VK_NUMPAD1 : keyname =  "VK_NUMPAD1"; break;
            case  VK_NUMPAD2 : keyname =  "VK_NUMPAD2"; break;
            case  VK_NUMPAD3 : keyname =  "VK_NUMPAD3"; break;
            case  VK_NUMPAD4 : keyname =  "VK_NUMPAD4"; break;
            case  VK_NUMPAD5 : keyname =  "VK_NUMPAD5"; break;
            case  VK_NUMPAD6 : keyname =  "VK_NUMPAD6"; break;
            case  VK_NUMPAD7 : keyname =  "VK_NUMPAD7"; break;
            case  VK_NUMPAD8 : keyname =  "VK_NUMPAD8"; break;
            case  VK_NUMPAD9 : keyname =  "VK_NUMPAD9"; break;
            case  VK_MULTIPLY: keyname =  "VK_MULTIPLY"; break;
            case  VK_ADD     : keyname =  "VK_ADD"; break;
            case  VK_SEPARATOR: keyname = "VK_SEPARATOR"; break;
            case  VK_SUBTRACT: keyname =  "VK_SUBTRACT"; break;
            case  VK_DECIMAL : keyname =  "VK_DECIMAL"; break;
            case  VK_DIVIDE  : keyname =  "VK_DIVIDE"; break;
            case  VK_F1      : keyname =  "VK_F1"; break;
            case  VK_F2      : keyname =  "VK_F2"; break;
            case  VK_F3      : keyname =  "VK_F3"; break;
            case  VK_F4      : keyname =  "VK_F4"; break;
            case  VK_F5      : keyname =  "VK_F5"; break;
            case  VK_F6      : keyname =  "VK_F6"; break;
            case  VK_F7      : keyname =  "VK_F7"; break;
            case  VK_F8      : keyname =  "VK_F8"; break;
            case  VK_F9      : keyname =  "VK_F9"; break;
            case  VK_F10     : keyname =  "VK_F10"; break;
            case  VK_F11     : keyname =  "VK_F11"; break;
            case  VK_F12     : keyname =  "VK_F12"; break;
            case  VK_F13     : keyname =  "VK_F13"; break;
            case  VK_F14     : keyname =  "VK_F14"; break;
            case  VK_F15     : keyname =  "VK_F15"; break;
            case  VK_F16     : keyname =  "VK_F16"; break;
            case  VK_F17     : keyname =  "VK_F17"; break;
            case  VK_F18     : keyname =  "VK_F18"; break;
            case  VK_F19     : keyname =  "VK_F19"; break;
            case  VK_F20     : keyname =  "VK_F20"; break;
            case  VK_F21     : keyname =  "VK_F21"; break;
            case  VK_F22     : keyname =  "VK_F22"; break;
            case  VK_F23     : keyname =  "VK_F23"; break;
            case  VK_F24     : keyname =  "VK_F24"; break;
            case  VK_NUMLOCK : keyname =  "VK_NUMLOCK"; break;
            case  VK_SCROLL  : keyname =  "VK_SCROLL"; break;
            case  VK_LSHIFT  : keyname =  "VK_LSHIFT"; break;
            case  VK_RSHIFT  : keyname =  "VK_RSHIFT"; break;
            case  VK_LCONTROL: keyname =  "VK_LCONTROL"; break;
            case  VK_RCONTROL: keyname =  "VK_RCONTROL"; break;
            default: keyname = "<unknown>"; break;
        }
        return keyname;
    }

    std::string ResolveKeyModifiers(int mask) {
        if((mask & KC_CTRL) && (mask & KC_SHIFT) && (mask & KC_ALT))
            return "KC_CTRL KC_SHIFT KC_ALT";
        else if((mask & KC_CTRL) && (mask & KC_SHIFT))
            return "KC_CTRL KC_SHIFT";
        else if((mask & KC_CTRL) && (mask & KC_ALT))
            return "KC_CTRL KC_ALT";
        else if((mask & KC_SHIFT) && (mask & KC_ALT))
            return "KC_SHIFT KC_ALT";
        else if((mask & KC_SHIFT))
            return "KC_SHIFT";
        else if((mask & KC_CTRL))
            return "KC_CTRL";
        else if((mask & KC_ALT))
            return "KC_ALT";
        else return "none";
    }

    char *ReadFile(std::string filename) {
        char *errors = NULL;
        FILE *fp=NULL;
#ifdef __ANDROID__
        int fd = -1;

        // Special way to open for URIs on Android
        if(strstr(filename.c_str(), "://"))
        {
            fd = dw_file_open(filename.c_str(), O_RDONLY);
            fp = fdopen(fd, "r");
        }
        else
#endif
            fp = fopen(filename.c_str(), "r");
        if(!fp)
            errors = strerror(errno);
        else
        {
            int i,len;

            lp = (char **)calloc(1000,sizeof(char *));
            // should test for out of memory
            max_linewidth=0;
            for(i=0; i<1000; i++)
            {
               lp[i] = (char *)calloc(1, 1025);
               if (fgets(lp[i], 1024, fp) == NULL)
                   break;
               len = (int)strlen(lp[i]);
               if (len > max_linewidth)
                   max_linewidth = len;
               if(lp[i][len - 1] == '\n')
                   lp[i][len - 1] = '\0';
            }
            num_lines = i;
            fclose(fp);

            hscrollbar->SetRange(max_linewidth, cols);
            hscrollbar->SetPos(0);
            vscrollbar->SetRange(num_lines, rows);
            vscrollbar->SetPos(0);
        }
#ifdef __ANDROID__
        if(fd != -1)
            close(fd);
#endif
        return errors;
    }

    // When hpm is not NULL we are printing.. so handle things differently
    void DrawFile(int row, int col, int nrows, int fheight, DW::Pixmap *hpm) {
        DW::Pixmap *pixmap = hpm ? hpm : pixmap2;
        char buf[16] = {0};
        int i,y,fileline;
        char *pLine;

        if(current_file.size())
        {
            pixmap->SetForegroundColor(DW_CLR_WHITE);
            if(!hpm)
                pixmap1->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, (int)pixmap1->GetWidth(), (int)pixmap1->GetHeight());
            pixmap->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, (int)pixmap->GetWidth(), (int)pixmap->GetHeight());

            for(i = 0;(i < nrows) && (i+row < num_lines); i++)
            {
                fileline = i + row - 1;
                y = i*fheight;
                pixmap->SetColor(fileline < 0 ? DW_CLR_WHITE : fileline % 16, 1 + (fileline % 15));
                if(!hpm)
                {
                    snprintf(buf, 15, "%6.6d", i+row);
                    pixmap1->DrawText(0, y, buf);
                }
                pLine = lp[i+row];
                pixmap->DrawText(0, y, pLine+col);
            }
        }
    }

    // When hpm is not NULL we are printing.. so handle things differently 
    void DrawShapes(int direct, DW::Pixmap *hpm) {
        DW::Pixmap *pixmap = hpm ? hpm : pixmap2;
        int width = (int)pixmap->GetWidth(), height = (int)pixmap->GetHeight();
        int x[7] = { 20, 180, 180, 230, 180, 180, 20 };
        int y[7] = { 50, 50, 20, 70, 120, 90, 90 };
        DW::Drawable *drawable = direct ? static_cast<DW::Drawable *>(render2) : static_cast<DW::Drawable *>(pixmap);

        drawable->SetForegroundColor(DW_CLR_WHITE);
        drawable->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, width, height);
        drawable->SetForegroundColor(DW_CLR_DARKPINK);
        drawable->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 10, 10, width - 20, height - 20);
        drawable->SetColor(DW_CLR_GREEN, DW_CLR_DARKRED);
        drawable->DrawText(10, 10, "This should be aligned with the edges.");
        drawable->SetForegroundColor(DW_CLR_YELLOW);
        drawable->DrawLine(width - 10, 10, 10, height - 10);
        drawable->SetForegroundColor(DW_CLR_BLUE);
        drawable->DrawPolygon(DW_DRAW_FILL, 7, x, y);
        drawable->SetForegroundColor(DW_CLR_BLACK);
        drawable->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 80, 80, 80, 40);
        drawable->SetForegroundColor(DW_CLR_CYAN);
        // Bottom right corner
        drawable->DrawArc(0, width - 30, height - 30, width - 10, height - 30, width - 30, height - 10);
        // Top right corner
        drawable->DrawArc(0, width - 30, 30, width - 30, 10, width - 10, 30);
        // Bottom left corner
        drawable->DrawArc(0, 30, height - 30, 30, height - 10, 10, height - 30);
        // Full circle in the left top area
        drawable->DrawArc(DW_DRAW_FULL, 120, 100, 80, 80, 160, 120);
        if(image && image->GetHPIXMAP())
        {
            if(imagestretchcheck->Get())
                drawable->BitBltStretch(0, 10, width - 20, height - 20, image, 0, 0, (int)image->GetWidth(), (int)image->GetHeight());
            else
                drawable->BitBlt(imagexspin->GetPos(), imageyspin->GetPos(), (int)image->GetWidth(), (int)image->GetHeight(), image, 0, 0);
        }
    }

    void UpdateRender(void) {
        switch(render_type)
        {
            case SHAPES_DOUBLE_BUFFERED:
                DrawShapes(FALSE, NULL);
                break;
            case SHAPES_DIRECT:
                DrawShapes(TRUE, NULL);
                break;
            case DRAW_FILE:
                DrawFile(current_row, current_col, rows, font_height, NULL);
                break;
        }
    }

    // Request that the render widgets redraw...
    // If not using direct rendering, call UpdateRender() to
    // redraw the in memory pixmaps. Then trigger the expose events.
    // Expose will call UpdateRender() to draw directly or bitblt the pixmaps.
    void RenderDraw() {
        // If we are double buffered, draw to the pixmaps
        if(render_type != SHAPES_DIRECT)
            UpdateRender();
        // Trigger expose event
        render1->Redraw();
        render2->Redraw();
    }

    DW::Menu *ItemContextMenu(DW::StatusText *status_text, std::string text) {
        DW::Menu *menu = new DW::Menu();
        DW::Menu *submenu = new DW::Menu();
        DW::MenuItem *menuitem = submenu->AppendItem("File", 0L, TRUE);
        menuitem->ConnectClicked([status_text, text]() -> int { status_text->SetText(text); return TRUE; });
        menuitem->ConnectClicked([status_text, text]() -> int { status_text->SetText(text); return TRUE; });
        menuitem = submenu->AppendItem("Date", 0L, TRUE);
        menuitem->ConnectClicked([status_text, text]() -> int { status_text->SetText(text); return TRUE; });
        menuitem = submenu->AppendItem("Size", 0L, TRUE);
        menuitem->ConnectClicked([status_text, text]() -> int { status_text->SetText(text); return TRUE; });
        menuitem = submenu->AppendItem("None", 0L, TRUE);
        menuitem->ConnectClicked([status_text, text]() -> int { status_text->SetText(text); return TRUE; });

        menuitem = menu->AppendItem("Sort", submenu);

        menuitem = menu->AppendItem("Make Directory");
        menuitem->ConnectClicked([status_text, text]() -> int { status_text->SetText(text); return TRUE; });

        menuitem = menu->AppendItem(DW_MENU_SEPARATOR);
        menuitem = menu->AppendItem("Rename Entry");
        menuitem->ConnectClicked([status_text, text]() -> int { status_text->SetText(text); return TRUE; });

        menuitem = menu->AppendItem("Delete Entry");
        menuitem->ConnectClicked([status_text, text]() -> int { status_text->SetText(text); return TRUE; });

        menuitem = menu->AppendItem(DW_MENU_SEPARATOR);
        menuitem = menu->AppendItem("View File");
        menuitem->ConnectClicked([status_text, text]() -> int { status_text->SetText(text); return TRUE; });

        return menu;
    }

    DW::ComboBox *ColorCombobox(void) {
        DW::ComboBox *combobox = new DW::ComboBox("DW_CLR_DEFAULT");

        combobox->Append("DW_CLR_DEFAULT");
        combobox->Append("DW_CLR_BLACK");
        combobox->Append("DW_CLR_DARKRED");
        combobox->Append("DW_CLR_DARKGREEN");
        combobox->Append("DW_CLR_BROWN");
        combobox->Append("DW_CLR_DARKBLUE");
        combobox->Append("DW_CLR_DARKPINK");
        combobox->Append("DW_CLR_DARKCYAN");
        combobox->Append("DW_CLR_PALEGRAY");
        combobox->Append("DW_CLR_DARKGRAY");
        combobox->Append("DW_CLR_RED");
        combobox->Append("DW_CLR_GREEN");
        combobox->Append("DW_CLR_YELLOW");
        combobox->Append("DW_CLR_BLUE");
        combobox->Append("DW_CLR_PINK");
        combobox->Append("DW_CLR_CYAN");
        combobox->Append("DW_CLR_WHITE");
        return combobox;
    }

    unsigned long ComboboxColor(std::string colortext) {
        unsigned long color = DW_CLR_DEFAULT;

        if(colortext.compare("DW_CLR_BLACK") == 0)
            color = DW_CLR_BLACK;
        else if(colortext.compare("DW_CLR_DARKRED") == 0)
            color = DW_CLR_DARKRED;
        else if(colortext.compare("DW_CLR_DARKGREEN") == 0)
            color = DW_CLR_DARKGREEN;
        else if(colortext.compare("DW_CLR_BROWN") == 0)
            color = DW_CLR_BROWN;
        else if(colortext.compare("DW_CLR_DARKBLUE") == 0)
            color = DW_CLR_DARKBLUE;
        else if(colortext.compare("DW_CLR_DARKPINK") == 0)
            color = DW_CLR_DARKPINK;
        else if(colortext.compare("DW_CLR_DARKCYAN") == 0)
            color = DW_CLR_DARKCYAN;
        else if(colortext.compare("DW_CLR_PALEGRAY") == 0)
            color = DW_CLR_PALEGRAY;
        else if(colortext.compare("DW_CLR_DARKGRAY") == 0)
            color = DW_CLR_DARKGRAY;
        else if(colortext.compare("DW_CLR_RED") == 0)
            color = DW_CLR_RED;
        else if(colortext.compare("DW_CLR_GREEN") == 0)
            color = DW_CLR_GREEN;
        else if(colortext.compare("DW_CLR_YELLOW") == 0)
            color = DW_CLR_YELLOW;
        else if(colortext.compare("DW_CLR_BLUE") == 0)
            color = DW_CLR_BLUE;
        else if(colortext.compare("DW_CLR_PINK") == 0)
            color = DW_CLR_PINK;
        else if(colortext.compare("DW_CLR_CYAN") == 0)
            color = DW_CLR_CYAN;
        else if(colortext.compare("DW_CLR_WHITE") == 0)
            color = DW_CLR_WHITE;

        return color;
    }

    void MLESetFont(DW::MLE *mle, int fontsize, std::string fontname) {
        if(fontname.size() && fontsize > 0) {
            mle->SetFont(std::to_string(fontsize) + "." + fontname);
        }
        mle->SetFont(NULL);
    }

    // Thread and Event functions
    void UpdateMLE(DW::MLE *threadmle, std::string text, DW::Mutex *mutex) {
        static unsigned int pos = 0;

        // Protect pos from being changed by different threads
        if(mutex)
            mutex->Lock();
        pos = threadmle->Import(text, pos);
        threadmle->SetCursor(pos);
        if(mutex)
            mutex->Unlock();
    }

    void RunThread(int threadnum, DW::Mutex *mutex, DW::Event *controlevent, DW::Event *workevent, DW::MLE *threadmle) {
        std::string buf;

        buf = "Thread " + std::to_string(threadnum) + " started.\r\n";
        UpdateMLE(threadmle, buf, mutex);

        // Increment the ready count while protected by mutex
        mutex->Lock();
        ready++;
        // If all 4 threads have incrememted the ready count...
        // Post the control event semaphore so things will get started.
        if(ready == 4)
            controlevent->Post();
        mutex->Unlock();

        while(!finished)
        {
            int result = workevent->Wait(2000);

            if(result == DW_ERROR_TIMEOUT)
            {
                buf = "Thread " + std::to_string(threadnum) + " timeout waiting for event.\r\n";
                UpdateMLE(threadmle, buf, mutex);
            }
            else if(result == DW_ERROR_NONE)
            {
                buf = "Thread " + std::to_string(threadnum) + " doing some work.\r\n";
                UpdateMLE(threadmle, buf, mutex);
                // Pretend to do some work
                app->MainSleep(1000 * threadnum);

                // Increment the ready count while protected by mutex
                mutex->Lock();
                ready++;
                buf = "Thread " + std::to_string(threadnum) + " work done. ready=" + std::to_string(ready);
                // If all 4 threads have incrememted the ready count...
                // Post the control event semaphore so things will get started.
                if(ready == 4)
                {
                    controlevent->Post();
                    buf += " Control posted.";
                }
                mutex->Unlock();
                buf += "\r\n";
                UpdateMLE(threadmle, buf, mutex);
            }
            else
            {
                buf = "Thread " + std::to_string(threadnum) + " error " + std::to_string(result) + ".\r\n";
                UpdateMLE(threadmle, buf, mutex);
                app->MainSleep(10000);
            }
        }
        buf = "Thread " + std::to_string(threadnum) + " finished.\r\n";
        UpdateMLE(threadmle, buf, mutex);
    }

    void ControlThread(DW::Mutex *mutex, DW::Event *controlevent, DW::Event *workevent, DW::MLE *threadmle) {
        int inprogress = 5;
        std::string buf;

        while(inprogress)
        {
            int result = controlevent->Wait(2000);

            if(result == DW_ERROR_TIMEOUT)
            {
                UpdateMLE(threadmle, "Control thread timeout waiting for event.\r\n", mutex);
            }
            else if(result == DW_ERROR_NONE)
            {
                // Reset the control event
                controlevent->Reset();
                ready = 0;
                buf = "Control thread starting worker threads. Inprogress=" + std::to_string(inprogress) + "\r\n";
                UpdateMLE(threadmle, buf, mutex);
                // Start the work threads
                workevent->Post();
                app->MainSleep(100);
                // Reset the work event
                workevent->Reset();
                inprogress--;
            }
            else
            {
                buf = "Control thread error " + std::to_string(result) + ".\r\n";
                UpdateMLE(threadmle, buf, mutex);
                app->MainSleep(10000);
            }
        }
        // Tell the other threads we are done
        finished = TRUE;
        workevent->Post();
        // Close the control event
        controlevent->Close();
        UpdateMLE(threadmle, "Control thread finished.\r\n", mutex);
    }

    // Add the menus to the window
    void CreateMenus() {
        // Setup the menu
        DW::MenuBar *menubar = this->MenuBarNew();
        
        // add menus to the menubar
        DW::Menu *menu = new DW::Menu();
        DW::MenuItem *menuitem = menu->AppendItem("~Quit");
        menuitem->ConnectClicked([this] () -> int 
        { 
            if(this->app->MessageBox(APP_TITLE, DW_MB_YESNO | DW_MB_QUESTION, APP_EXIT) != 0) {
                this->app->MainQuit();
            }
            return TRUE;
        });

        // Add the "File" menu to the menubar...
        menubar->AppendItem("~File", menu);
        
        menu = new DW::Menu();
        DW::MenuItem *checkable_menuitem = menu->AppendItem("~Checkable Menu Item", 0, TRUE);
        checkable_menuitem->ConnectClicked([this]() -> int 
        {
            this->app->MessageBox("Menu Item Callback", DW_MB_OK | DW_MB_INFORMATION, "\"Checkable Menu Item\" selected");
            return FALSE;
        });
        DW::MenuItem *noncheckable_menuitem = menu->AppendItem("~Non-Checkable Menu Item");
        noncheckable_menuitem->ConnectClicked([this]() -> int 
        {
            this->app->MessageBox("Menu Item Callback", DW_MB_OK | DW_MB_INFORMATION, "\"Non-Checkable Menu Item\" selected");
            return FALSE;
        });
        menuitem = menu->AppendItem("~Disabled menu Item", DW_MIS_DISABLED|DW_MIS_CHECKED, TRUE);
        // separator
        menuitem = menu->AppendItem(DW_MENU_SEPARATOR);
        menuitem = menu->AppendItem("~Menu Items Disabled", 0, TRUE);
        menuitem->ConnectClicked([this, checkable_menuitem, noncheckable_menuitem]() -> int 
        {
            // Toggle the variable
            this->menu_enabled = !this->menu_enabled;
            // Set the ENABLED/DISABLED state on the menu items
            checkable_menuitem->SetStyle(menu_enabled ? DW_MIS_ENABLED : DW_MIS_DISABLED);
            noncheckable_menuitem->SetStyle(menu_enabled ? DW_MIS_ENABLED : DW_MIS_DISABLED);
            return FALSE;
        });
        // Add the "Menu" menu to the menubar...
        menubar->AppendItem("~Menu", menu);

        menu = new DW::Menu();
        menuitem = menu->AppendItem("~About");
        menuitem->ConnectClicked([this]() -> int 
        {
            DWEnv env;

            this->app->GetEnvironment(&env);
            this->app->MessageBox("About dwindows", DW_MB_OK | DW_MB_INFORMATION,
                            "dwindows test\n\nOS: %s %s %s Version: %d.%d.%d.%d\n\nHTML: %s\n\ndwindows Version: %d.%d.%d\n\nScreen: %dx%d %dbpp",
                            env.osName, env.buildDate, env.buildTime,
                            env.MajorVersion, env.MinorVersion, env.MajorBuild, env.MinorBuild,
                            env.htmlEngine,
                            env.DWMajorVersion, env.DWMinorVersion, env.DWSubVersion,
                            this->app->GetScreenWidth(), this->app->GetScreenHeight(), this->app->GetColorDepth());
            return FALSE;
        });
        // Add the "Help" menu to the menubar...
        menubar->AppendItem("~Help", menu);
    }
    
    // Notebook page 1
    void CreateInput(DW::Box *notebookbox) {
        DW::Box *lbbox = new DW::Box(DW_VERT, 10);

        notebookbox->PackStart(lbbox, 150, 70, TRUE, TRUE, 0);

        // Copy and Paste
        DW::Box *browsebox = new DW::Box(DW_HORZ, 0);
        lbbox->PackStart(browsebox, 0, 0, FALSE, FALSE, 0);

        DW::Entryfield *copypastefield = new DW::Entryfield();
        copypastefield->SetLimit(260);
        browsebox->PackStart(copypastefield, TRUE, FALSE, 4);

        DW::Button *copybutton = new DW::Button("Copy");
        browsebox->PackStart(copybutton, FALSE, FALSE, 0);

        DW::Button *pastebutton = new DW::Button("Paste");
        browsebox->PackStart(pastebutton, FALSE, FALSE, 0);

        // Archive Name
        DW::Text *stext = new DW::Text("File to browse");
        stext->SetStyle(DW_DT_VCENTER);
        lbbox->PackStart(stext, 130, 15, TRUE, TRUE, 2);

        browsebox = new DW::Box(DW_HORZ, 0);
        lbbox->PackStart(browsebox, 0, 0, TRUE, TRUE, 0);

        DW::Entryfield *entryfield = new DW::Entryfield();
        entryfield->SetLimit(260);
        browsebox->PackStart(entryfield, 100, 15, TRUE, TRUE, 4);

        DW::Button *browsefilebutton = new DW::Button("Browse File");
        browsebox->PackStart(browsefilebutton, 40, 15, TRUE, TRUE, 0);

        DW::Button *browsefolderbutton = new DW::Button("Browse Folder");
        browsebox->PackStart(browsefolderbutton, 40, 15, TRUE, TRUE, 0);

        browsebox->SetColor(DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
        stext->SetColor(DW_CLR_BLACK, DW_CLR_PALEGRAY);

        // Buttons
        DW::Box *buttonbox = new DW::Box(DW_HORZ, 10);
        lbbox->PackStart(buttonbox, 0, 0, TRUE, TRUE, 0);

        DW::Button *cancelbutton = new DW::Button("Exit");
        buttonbox->PackStart(cancelbutton, 130, 30, TRUE, TRUE, 2);

        DW::Button *cursortogglebutton = new DW::Button("Set Cursor pointer - CLOCK");
        buttonbox->PackStart(cursortogglebutton, 130, 30, TRUE, TRUE, 2);

        DW::Button *okbutton = new DW::Button("Turn Off Annoying Beep!");
        buttonbox->PackStart(okbutton, 130, 30, TRUE, TRUE, 2);

        cancelbutton->Unpack();
        buttonbox->PackStart(cancelbutton, 130, 30, TRUE, TRUE, 2);
        this->ClickDefault(cancelbutton);

        DW::Button *colorchoosebutton = new DW::Button("Color Chooser Dialog");
        buttonbox->PackAtIndex(colorchoosebutton, 1, 130, 30, TRUE, TRUE, 2);

        // Set some nice fonts and colors
        lbbox->SetColor(DW_CLR_DARKCYAN, DW_CLR_PALEGRAY);
        buttonbox->SetColor(DW_CLR_DARKCYAN, DW_CLR_PALEGRAY);
        okbutton->SetColor(DW_CLR_PALEGRAY, DW_CLR_DARKCYAN);
#ifdef COLOR_DEBUG
        copypastefield->SetColor(DW_CLR_WHITE, DW_CLR_RED);
        copybutton->SetColor(DW_CLR_WHITE, DW_CLR_RED);
        // Set a color then clear it to make sure it clears correctly
        entryfield->SetColor(DW_CLR_WHITE, DW_CLR_RED);
        entryfield->SetColor(DW_CLR_DEFAULT, DW_CLR_DEFAULT);
        // Set a color then clear it to make sure it clears correctly... again
        pastebutton->SetColor(DW_CLR_WHITE, DW_CLR_RED);
        pastebutton->SetColor(DW_CLR_DEFAULT, DW_CLR_DEFAULT);
#endif

        // Connect signals
        browsefilebutton->ConnectClicked([this, entryfield, copypastefield]() -> int 
        {
            std::string tmp = this->app->FileBrowse("Pick a file", "dwtest.c", "c", DW_FILE_OPEN);
            if(tmp.size())
            {
                char *errors = ReadFile(tmp);
                std::string title = "New file load";
                std::string image = "image/test.png";
                DW::Notification *notification;

                if(errors)
                    notification = new DW::Notification(title, image, std::string(APP_TITLE) + " failed to load the file into the file browser.");
                else
                    notification = new DW::Notification(title, image, std::string(APP_TITLE) + " loaded the file into the file browser on the Render tab, with \"File Display\" selected from the drop down list.");

                current_file = tmp;
                entryfield->SetText(current_file);
                current_col = current_row = 0;

                RenderDraw();
                notification->ConnectClicked([this]() -> int {
                    this->app->Debug("Notification clicked\n");
                    return TRUE;
                });
                notification->Send();
            }
            copypastefield->SetFocus();
            return FALSE;
        });
        
        browsefolderbutton->ConnectClicked([this]() -> int 
        {
            std::string tmp = this->app->FileBrowse("Pick a folder", ".", "c", DW_DIRECTORY_OPEN);
            this->app->Debug("Folder picked: " + (tmp.size() ? tmp : "None") + "\n");
            return FALSE;
        });

        copybutton->ConnectClicked([this, copypastefield, entryfield]() -> int {
            std::string test = copypastefield->GetText();

            if(test.size()) {
                this->app->SetClipboard(test);
            }
            entryfield->SetFocus();
            return TRUE; 
        });

        pastebutton->ConnectClicked([this, copypastefield]() -> int 
        {
            std::string test = this->app->GetClipboard();
            if(test.size()) {
                copypastefield->SetText(test);
            }
            return TRUE;
        });

        okbutton->ConnectClicked([this]() -> int 
        {
            if(this->timer) {
                delete this->timer;
                this->timer = DW_NULL;
            }
            return TRUE;
        });

        cancelbutton->ConnectClicked([this] () -> int 
        {
            if(this->app->MessageBox(APP_TITLE, DW_MB_YESNO | DW_MB_QUESTION, APP_EXIT) != 0) {
                this->app->MainQuit();
            }
            return TRUE;
        });

        cursortogglebutton->ConnectClicked([this, cursortogglebutton] () -> int 
        {
            cursortogglebutton->SetText(this->cursor_arrow ? "Set Cursor pointer - ARROW" :
                                        "Set Cursor pointer - CLOCK");
            this->SetPointer(this->cursor_arrow ? DW_POINTER_CLOCK : DW_POINTER_DEFAULT);
            this->cursor_arrow = !this->cursor_arrow;
            return FALSE;
        });

        colorchoosebutton->ConnectClicked([this]() -> int
        {
            this->current_color = this->app->ColorChoose(this->current_color);
            return FALSE;
        });
    }

    // Notebook page 2
    void CreateRender(DW::Box *notebookbox) {
        int vscrollbarwidth, hscrollbarheight;
        wchar_t widestring[100] = L"DWTest Wide";
        char *utf8string = app->WideToUTF8(widestring);

        // create a box to pack into the notebook page
        DW::Box *pagebox = new DW::Box(DW_HORZ, 2);
        notebookbox->PackStart(pagebox, 0, 0, TRUE, TRUE, 0);

        // now a status area under this box
        DW::Box *hbox = new DW::Box(DW_HORZ, 1);
        notebookbox->PackStart(hbox, 100, 20, TRUE, FALSE, 1);

        DW::StatusText *status1 = new DW::StatusText();
        hbox->PackStart(status1, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);

        DW::StatusText *status2 = new DW::StatusText();
        hbox->PackStart(status2, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);
        // a box with combobox and button
        hbox = new DW::Box(DW_HORZ, 1 );
        notebookbox->PackStart(hbox, 100, 25, TRUE, FALSE, 1);

        DW::ComboBox *rendcombo = new DW::ComboBox( "Shapes Double Buffered");
        hbox->PackStart(rendcombo, 80, 25, TRUE, TRUE, 0);
        rendcombo->Append("Shapes Double Buffered");
        rendcombo->Append("Shapes Direct");
        rendcombo->Append("File Display");

        DW::Text *label = new DW::Text("Image X:");
        label->SetStyle(DW_DT_VCENTER | DW_DT_CENTER);
        hbox->PackStart(label, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

        imagexspin = new DW::SpinButton("20");
        hbox->PackStart(imagexspin, 25, 25, TRUE, TRUE, 0);

        label = new DW::Text("Y:");
        label->SetStyle(DW_DT_VCENTER | DW_DT_CENTER);
        hbox->PackStart(label, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

        imageyspin = new DW::SpinButton("20");
        hbox->PackStart(imageyspin, 25, 25, TRUE, TRUE, 0);
        imagexspin->SetLimits(2000, 0);
        imageyspin->SetLimits(2000, 0);
        imagexspin->SetPos(20);
        imageyspin->SetPos(20);

        imagestretchcheck = new DW::CheckBox("Stretch");
        hbox->PackStart(imagestretchcheck, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

        DW::Button *refreshbutton = new DW::Button("Refresh");
        hbox->PackStart(refreshbutton, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

        DW::Button *printbutton = new DW::Button("Print");
        hbox->PackStart(printbutton, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

        // Pre-create the scrollbars so we can query their sizes
        vscrollbar = new DW::ScrollBar(DW_VERT, 50);
        hscrollbar = new DW::ScrollBar(DW_HORZ, 50);
        vscrollbar->GetPreferredSize(&vscrollbarwidth, NULL);
        hscrollbar->GetPreferredSize(NULL, &hscrollbarheight);

        // On GTK with overlay scrollbars enabled this returns us 0...
        // so in that case we need to give it some real values.
        if(!vscrollbarwidth)
            vscrollbarwidth = 8;
        if(!hscrollbarheight)
            hscrollbarheight = 8;

        // Create render box for line number pixmap
        render1 = new DW::Render();
        render1->SetFont(FIXEDFONT);
        render1->GetTextExtents("(g", &font_width, &font_height);
        font_width = font_width / 2;

        DW::Box *vscrollbox = new DW::Box(DW_VERT, 0);
        vscrollbox->PackStart(render1, font_width*width1, font_height*rows, FALSE, TRUE, 0);
        vscrollbox->PackStart(DW_NOHWND, (font_width*(width1+1)), hscrollbarheight, FALSE, FALSE, 0);
        pagebox->PackStart(vscrollbox, 0, 0, FALSE, TRUE, 0);

        // pack empty space 1 character wide
        pagebox->PackStart(DW_NOHWND, font_width, 0, FALSE, TRUE, 0);

        // create box for filecontents and horz scrollbar
        DW::Box *textbox = new DW::Box(DW_VERT,0 );
        pagebox->PackStart(textbox, 0, 0, TRUE, TRUE, 0);

        // create render box for filecontents pixmap
        render2 = new DW::Render();
        textbox->PackStart(render2, 10, 10, TRUE, TRUE, 0);
        render2->SetFont(FIXEDFONT);
        // create horizonal scrollbar
        textbox->PackStart(hscrollbar, TRUE, FALSE, 0);

        // create vertical scrollbar
        vscrollbox = new DW::Box(DW_VERT, 0);
        vscrollbox->PackStart(vscrollbar, FALSE, TRUE, 0);
        // Pack an area of empty space of the scrollbar dimensions
        vscrollbox->PackStart(DW_NOHWND, vscrollbarwidth, hscrollbarheight, FALSE, FALSE, 0);
        pagebox->PackStart(vscrollbox, 0, 0, FALSE, TRUE, 0);

        pixmap1 = new DW::Pixmap(render1, font_width*width1, font_height*rows);
        pixmap2 = new DW::Pixmap(render2, font_width*cols, font_height*rows);
        image = new DW::Pixmap(render1, "image/test");
        if(!image || !image->GetHPIXMAP())
            image = new DW::Pixmap(render1, "~/test");
        if(!image || !image->GetHPIXMAP())
        {
            std::string appdir = app->GetDir();
            appdir.append(std::string(1, DW_DIR_SEPARATOR));
            appdir.append("test");
            image = new DW::Pixmap(render1, appdir);
        }
        if(image)
            image->SetTransparentColor(DW_CLR_WHITE);

        app->MessageBox(utf8string ? utf8string : "DWTest", DW_MB_OK|DW_MB_INFORMATION, "Width: %d Height: %d\n", font_width, font_height);
        if(utf8string)
            app->Free(utf8string);
        pixmap1->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, font_width*width1, font_height*rows);
        pixmap2->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, font_width*cols, font_height*rows);

        // Signal handler lambdas
        render1->ConnectButtonPress([this](int x, int y, int buttonmask) -> int
        {
            DW::Menu *menu = new DW::Menu();
            DW::MenuItem *menuitem = menu->AppendItem("~Quit");
            long px, py;

            menuitem->ConnectClicked([this] () -> int 
            {
                if(this->app->MessageBox(APP_TITLE, DW_MB_YESNO | DW_MB_QUESTION, APP_EXIT) != 0) {
                    this->app->MainQuit();
                }
                return TRUE;
            });


            menu->AppendItem(DW_MENU_SEPARATOR);
            menuitem = menu->AppendItem("~Show Window");
            menuitem->ConnectClicked([this]() -> int
            {
                this->Show();
                this->Raise();
                return TRUE;
            });

            this->app->GetPointerPos(&px, &py);
            menu->Popup(this, (int)px, (int)py);
            return TRUE;
        });

        render1->ConnectExpose([this](DWExpose *exp) -> int 
        {
            if(render_type != SHAPES_DIRECT)
            {
                this->render1->BitBlt(0, 0, (int)pixmap1->GetWidth(), (int)pixmap1->GetHeight(), pixmap1, 0, 0);
                render1->Flush();
            }
            else
            {
                UpdateRender();
            }
            return TRUE;
        });

        render2->ConnectExpose([this](DWExpose *exp) -> int 
        {
            if(render_type != SHAPES_DIRECT)
            {
                this->render2->BitBlt(0, 0, (int)pixmap2->GetWidth(), (int)pixmap2->GetHeight(), pixmap2, 0, 0);
                render2->Flush();
            }
            else
            {
                UpdateRender();
            }
            return TRUE;
        });

        render2->ConnectKeyPress([this, status1](char ch, int vk, int state, std::string utf8) -> int
        {
            std::string buf = "Key: ";

            if(ch)
                buf += std::string(1, ch) + "(" + std::to_string((int)ch) + ")";
            else
                buf += ResolveKeyName(vk) + "(" + std::to_string(vk) + ")";

            buf += " Modifiers: " + ResolveKeyModifiers(state) + "(" + std::to_string(state) + ") utf8 " + utf8;

            status1->SetText(buf);
            return FALSE;
        });

        hscrollbar->ConnectValueChanged([this, status1](int value) -> int
        {
            std::string buf = "Row:" + std::to_string(current_row) + " Col:" + std::to_string(current_col) + " Lines:" + std::to_string(num_lines) + " Cols:" + std::to_string(max_linewidth);

            this->current_col = value;
            status1->SetText(buf);
            this->RenderDraw();
            return TRUE;
        });

        vscrollbar->ConnectValueChanged([this, status1](int value) -> int
        {
            std::string buf = "Row:" + std::to_string(current_row) + " Col:" + std::to_string(current_col) + " Lines:" + std::to_string(num_lines) + " Cols:" + std::to_string(max_linewidth);

            this->current_row = value;
            status1->SetText(buf);
            this->RenderDraw();
            return TRUE;
        });

        render2->ConnectMotionNotify([status2](int x, int y, int buttonmask) -> int
        {
            std::string buf = "motion_notify: " + std::to_string(x) + "x" + std::to_string(y) + " buttons " + std::to_string(buttonmask);

            status2->SetText(buf);
            return FALSE;
        });

        render2->ConnectButtonPress([status2](int x, int y, int buttonmask) -> int
        {
            std::string buf = "button_press: " + std::to_string(x) + "x" + std::to_string(y) + " buttons " + std::to_string(buttonmask);

            status2->SetText(buf);
            return FALSE;
        });

        render2->ConnectConfigure([this](int width, int height) -> int
        {
            DW::Pixmap *old1 = this->pixmap1, *old2 = this->pixmap2;

            rows = height / font_height;
            cols = width / font_width;

            // Create new pixmaps with the current sizes
            this->pixmap1 = new DW::Pixmap(this->render1, (unsigned long)(font_width*(width1)), (unsigned long)height);
            this->pixmap2 = new DW::Pixmap(this->render2, (unsigned long)width, (unsigned long)height);

            // Make sure the side area is cleared
            this->pixmap1->SetForegroundColor(DW_CLR_WHITE);
            this->pixmap1->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, (int)this->pixmap1->GetWidth(), (int)this->pixmap1->GetHeight());

            // Destroy the old pixmaps
            delete old1;
            delete old2;

            // Update scrollbar ranges with new values
            this->hscrollbar->SetRange(max_linewidth, cols);
            this->vscrollbar->SetRange(num_lines, rows);

            // Redraw the render widgets
            this->RenderDraw();
            return TRUE;
        });

        imagestretchcheck->ConnectClicked([this]() -> int
        {
            this->RenderDraw();
            return TRUE;
        });

        refreshbutton->ConnectClicked([this]() -> int
        {
            this->RenderDraw();
            return TRUE;
        });

        printbutton->ConnectClicked([this]() -> int
        {
            DW::Print *print = new DW::Print("DWTest Job", 0, 2, [this](DW::Pixmap *pixmap, int page_num) -> int
            {
               pixmap->SetFont(FIXEDFONT);
               if(page_num == 0)
               {
                   this->DrawShapes(FALSE, pixmap);
               }
               else if(page_num == 1)
               {
                   // Get the font size for this printer context...
                   int fheight, fwidth;

                   // If we have a file to display...
                   if(current_file.size())
                   {
                       int nrows;

                       // Calculate new dimensions
                       pixmap->GetTextExtents("(g", NULL, &fheight);
                       nrows = (int)(pixmap->GetHeight() / fheight);

                       // Do the actual drawing
                       this->DrawFile(0, 0, nrows, fheight, pixmap);
                   }
                   else
                   {
                       // We don't have a file so center an error message on the page
                       std::string text = "No file currently selected!";
                       int posx, posy;

                       pixmap->GetTextExtents(text, &fwidth, &fheight);

                       posx = (int)(pixmap->GetWidth() - fwidth)/2;
                       posy = (int)(pixmap->GetHeight() - fheight)/2;

                       pixmap->SetColor(DW_CLR_BLACK, DW_CLR_WHITE);
                       pixmap->DrawText(posx, posy, text);
                   }
               }
               return TRUE;
            });
            print->Run(0);
            return TRUE;
        });

        rendcombo->ConnectListSelect([this](unsigned int index) -> int 
        {
            if(index != this->render_type)
            {
                if(index == DRAW_FILE)
                {
                    this->hscrollbar->SetRange(max_linewidth, cols);
                    this->hscrollbar->SetPos(0);
                    this->vscrollbar->SetRange(num_lines, rows);
                    this->vscrollbar->SetPos(0);
                    this->current_col = this->current_row = 0;
                }
                else
                {
                    this->hscrollbar->SetRange(0, 0);
                    this->hscrollbar->SetPos(0);
                    this->vscrollbar->SetRange(0, 0);
                    this->vscrollbar->SetPos(0);
                }
                this->render_type = index;
                this->RenderDraw();
            }
            return FALSE;
        });

        app->TaskBarInsert(render1, fileicon, "DWTest");
    }

    // Notebook page 3
    void CreateTree(DW::Box *notebookbox) {
        // create a box to pack into the notebook page
        DW::ListBox *listbox = new DW::ListBox(TRUE);
        notebookbox->PackStart(listbox, 500, 200, TRUE, TRUE, 0);
        listbox->Append("Test 1");
        listbox->Append("Test 2");
        listbox->Append("Test 3");
        listbox->Append("Test 4");
        listbox->Append("Test 5");

        // now a tree area under this box 
        DW::Tree *tree = new DW::Tree();
        if(tree->GetHWND())
        {
            notebookbox->PackStart(tree, 500, 200, TRUE, TRUE, 1);

            // and a status area to see whats going on
            DW::StatusText *tree_status = new DW::StatusText();
            notebookbox->PackStart(tree_status, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);

            // set up our signal trappers...
            tree->ConnectItemContext([this, tree_status](std::string text, int x, int y, void *data) -> int
            {
                DW::Menu *popupmenu = ItemContextMenu(tree_status, "Item context menu clicked.");
                std::string buf = "DW_SIGNAL_ITEM_CONTEXT: Text: " + text + " x: " + std::to_string(x) + " y: " + std::to_string(y);
                tree_status->SetText(buf);
                popupmenu->Popup(this, x, y);
                return FALSE;
            });
            tree->ConnectItemSelect([tree_status](HTREEITEM item, std::string text, void *itemdata)
            {
                std::string sitem = std::to_string(DW_POINTER_TO_UINT(item));
                std::string sitemdata = std::to_string(DW_POINTER_TO_UINT(itemdata));
                std::string buf = "DW_SIGNAL_ITEM_SELECT: Item: " + sitem + " Text: " + text + " Itemdata: " + sitemdata;
                tree_status->SetText(buf);
                return FALSE;
            });

            HTREEITEM t1 = tree->Insert("tree folder 1", foldericon, DW_NULL, DW_INT_TO_POINTER(1));
            HTREEITEM t2 = tree->Insert("tree folder 2", foldericon, DW_NULL, DW_INT_TO_POINTER(2));
            tree->Insert("tree file 1", fileicon, t1, DW_INT_TO_POINTER(3));
            tree->Insert("tree file 2", fileicon, t1, DW_INT_TO_POINTER(4));
            tree->Insert("tree file 3", fileicon, t2, DW_INT_TO_POINTER(5));
            tree->Insert("tree file 4", fileicon, t2, DW_INT_TO_POINTER(6));
            tree->Change(t1, "tree folder 1", foldericon);
            tree->Change(t2, "tree folder 2", foldericon);
            tree->SetData(t2, DW_INT_TO_POINTER(100));
            tree->Expand(t1);
            int t1data = DW_POINTER_TO_INT(tree->GetData(t1));
            int t2data = DW_POINTER_TO_INT(tree->GetData(t2));
            std::string message = "t1 title \"" + tree->GetTitle(t1) + "\" data " + std::to_string(t1data) + " t2 data " + std::to_string(t2data) + "\n";

            this->app->Debug(message);
        }
        else
        {
            DW::Text *text = new DW::Text("Tree widget not available.");
            notebookbox->PackStart(text, 500, 200, TRUE, TRUE, 1);
        }
    }

    // Page 4 - Container
    void CreateContainer(DW::Box *notebookbox) {
        CTIME time;
        CDATE date;

        // create a box to pack into the notebook page
        DW::Box *containerbox = new DW::Box(DW_HORZ, 2);
        notebookbox->PackStart(containerbox, 500, 200, TRUE, TRUE, 0);

        // Add a word wrap checkbox
        DW::Box *hbox = new DW::Box(DW_HORZ, 0);

        DW::CheckBox *checkbox = new DW::CheckBox("Word wrap");
        hbox->PackStart(checkbox,  FALSE, TRUE, 1);
        DW::Text *text = new DW::Text("Foreground:");
        text->SetStyle(DW_DT_VCENTER);
        hbox->PackStart(text, FALSE, TRUE, 1);
        DW::ComboBox *mlefore = ColorCombobox();
        hbox->PackStart(mlefore, 150, DW_SIZE_AUTO, TRUE, FALSE, 1);
        text = new DW::Text("Background:");
        text->SetStyle(DW_DT_VCENTER);
        hbox->PackStart(text, FALSE, TRUE, 1);
        DW::ComboBox *mleback = ColorCombobox();
        hbox->PackStart(mleback, 150, DW_SIZE_AUTO, TRUE, FALSE, 1);
        checkbox->Set(TRUE);
        text = new DW::Text("Font:");
        text->SetStyle(DW_DT_VCENTER);
        hbox->PackStart(text, FALSE, TRUE, 1);
        DW::SpinButton *fontsize = new DW::SpinButton("9");
        hbox->PackStart(fontsize, FALSE, FALSE, 1);
        fontsize->SetLimits(100, 5);
        fontsize->SetPos(9);
        DW::ComboBox *fontname = new DW::ComboBox("Default");
        fontname->Append("Default");
        fontname->Append("Arial");
        fontname->Append("Geneva");
        fontname->Append("Verdana");
        fontname->Append("Helvetica");
        fontname->Append("DejaVu Sans");
        fontname->Append("Times New Roman");
        fontname->Append("Times New Roman Bold");
        fontname->Append("Times New Roman Italic");
        fontname->Append("Times New Roman Bold Italic");
        hbox->PackStart(fontname, 150, DW_SIZE_AUTO, TRUE, FALSE, 1);
        notebookbox->PackStart(hbox, TRUE, FALSE, 1);

        // now a container area under this box
        DW::Filesystem *container = new DW::Filesystem(TRUE);
        notebookbox->PackStart(container, 500, 200, TRUE, FALSE, 1);

        // and a status area to see whats going on
        DW::StatusText *container_status = new DW::StatusText();
        notebookbox->PackStart(container_status, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);

        std::vector<std::string> titles = { "Type", "Size", "Time", "Date" };
        std::vector<unsigned long> flags = {
            DW_CFA_BITMAPORICON | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
            DW_CFA_ULONG | DW_CFA_RIGHT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
            DW_CFA_TIME | DW_CFA_CENTER | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
            DW_CFA_DATE | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR };


        container->SetColumnTitle("Test");
        container->Setup(flags, titles);
        container->SetStripe(DW_CLR_DEFAULT, DW_CLR_DEFAULT);
        container->Alloc(3);

        for(int z=0;z<3;z++)
        {
            std::string names = "We can now allocate from the stack: Item: " + std::to_string(z);
            HICN thisicon = (z == 0 ? foldericon : fileicon);

            unsigned long size = z*100;
            container->SetFile(z, "Filename " + std::to_string(z+1), thisicon);
            container->SetItem(0, z, &thisicon);
            container->SetItem(1, z, &size);

            time.seconds = z+10;
            time.minutes = z+10;
            time.hours = z+10;
            container->SetItem(2, z, &time);

            date.day = z+10;
            date.month = z+10;
            date.year = z+2000;
            container->SetItem(3, z, &date);

            container->SetRowTitle(z, names);
            container->SetRowData(z, DW_INT_TO_POINTER(z));
        }

        container->Insert();

        container->Alloc(1);
        container->SetFile(0, "Yikes", foldericon);
        unsigned long size = 324;
        container->SetItem(0, 0, &foldericon);
        container->SetItem(1, 0, &size);
        container->SetItem(2, 0, &time);
        container->SetItem(3, 0, &date);
        container->SetRowTitle(0, "Extra");

        container->Insert();
        container->Optimize();

        DW::MLE *container_mle = new DW::MLE();
        containerbox->PackStart(container_mle, 500, 200, TRUE, TRUE, 0);

        mle_point = container_mle->Import("", -1);
        mle_point = container_mle->Import("[" + std::to_string(mle_point) + "]", mle_point);
        mle_point = container_mle->Import("[" + std::to_string(mle_point) + "]abczxydefijkl", mle_point);
        container_mle->Delete(9, 3);
        mle_point = container_mle->Import("gh", 12);
        unsigned long newpoint;
        container_mle->GetSize(&newpoint, NULL);
        mle_point = (int)newpoint;
        mle_point = container_mle->Import("[" + std::to_string(mle_point) + "]\r\n\r\n", mle_point);
        container_mle->SetCursor(mle_point);

        // connect our event trappers...
        container->ConnectItemEnter([container_status](std::string text, void *itemdata) -> int
        {
            std::string buf = "DW_SIGNAL_ITEM_ENTER: Text: " + text + "Itemdata: " + std::to_string(DW_POINTER_TO_UINT(itemdata));
            container_status->SetText(buf);
            return FALSE;
        });

        container->ConnectItemContext([this, container_status](std::string text, int x, int y, void *itemdata) -> int
        {
            DW::Menu *popupmenu = ItemContextMenu(container_status, "Item context menu clicked.");
            std::string buf = "DW_SIGNAL_ITEM_CONTEXT: Text: " + text + " x: " + std::to_string(x) + " y: " + 
                std::to_string(y) + " Itemdata: " + std::to_string(DW_POINTER_TO_UINT(itemdata));

            container_status->SetText(buf);
            popupmenu->Popup(this, x, y);
            return FALSE;
        });

        container->ConnectItemSelect([this, container_mle, container, container_status](HTREEITEM item, std::string text, void *itemdata) -> int
        {
            std::string sitemdata = std::to_string(DW_POINTER_TO_UINT(itemdata));
            std::string sitem = std::to_string(DW_POINTER_TO_UINT(item));
            std::string buf = "DW_SIGNAL_ITEM_SELECT:Item: " + sitem + " Text: " + text + " Itemdata: " + sitemdata;
            container_status->SetText(buf);
            buf = "\r\nDW_SIGNAL_ITEM_SELECT: Item: " + sitem + " Text: " + text + " Itemdata: " +  sitemdata + "\r\n";
            this->mle_point = container_mle->Import(buf, mle_point);
            std::string str = container->QueryStart(DW_CRA_SELECTED);
            while(str.size())
            {
                std::string buf = "Selected: " + str + "\r\n";
                mle_point = container_mle->Import(buf, mle_point);
                str = container->QueryNext(DW_CRA_SELECTED);
            }
            // Make the last inserted point the cursor location
            container_mle->SetCursor(mle_point);
            // set the details of item 0 to new data
            this->app->Debug("In cb: icon: %x\n", DW_POINTER_TO_INT(fileicon));
            container->ChangeFile(0, "new data", fileicon);
            unsigned long size = 999;
            this->app->Debug("In cb: icon: %x\n",  DW_POINTER_TO_INT(fileicon));
            container->ChangeItem(1, 0, &size);
            return FALSE;
        });

        container->ConnectColumnClick([container, container_status](int column_num) -> int
        {
            std::string type_string = "Filename";

            if(column_num != 0)
            {
                int column_type = container->GetColumnType(column_num-1);

                if(column_type == DW_CFA_STRING)
                    type_string = "String";
                else if(column_type == DW_CFA_ULONG)
                    type_string ="ULong";
                else if(column_type == DW_CFA_DATE)
                    type_string = "Date";
                else if(column_type == DW_CFA_TIME)
                    type_string ="Time";
                else if(column_type == DW_CFA_BITMAPORICON)
                    type_string = "BitmapOrIcon";
                else
                    type_string = "Unknown";
            }
            std::string buf = "DW_SIGNAL_COLUMN_CLICK: Column: " + std::to_string(column_num) + " Type: " + type_string;
            container_status->SetText(buf);
            return FALSE;
        });

        mlefore->ConnectListSelect([this, mlefore, mleback, container_mle](unsigned int pos) -> int
        {
            ULONG fore = DW_CLR_DEFAULT, back = DW_CLR_DEFAULT;

            std::string colortext = mlefore->GetListText(pos);
            fore = ComboboxColor(colortext);
            std::string text = mleback->GetText();

            if(text.size()) {
                back = ComboboxColor(text);
            }
            container_mle->SetColor(fore, back);
            return FALSE;
        });

        mleback->ConnectListSelect([this, mlefore, mleback, container_mle](unsigned int pos) -> int
        {
            ULONG fore = DW_CLR_DEFAULT, back = DW_CLR_DEFAULT;

            std::string colortext = mleback->GetListText(pos);
            back = ComboboxColor(colortext);
            std::string text = mlefore->GetText();

            if(text.size()) {
                fore = ComboboxColor(text);
            }
            container_mle->SetColor(fore, back);
            return FALSE;
        });

        fontname->ConnectListSelect([this, fontname, fontsize, container_mle](unsigned int pos) -> int
        {
            std::string font = fontname->GetListText(pos);
            MLESetFont(container_mle, (int)fontsize->GetPos(), font.compare("Default") == 0 ? NULL : font);
            return FALSE;
        });

        fontsize->ConnectValueChanged([this, fontname, container_mle](int size) -> int
        {
            std::string font = fontname->GetText();

            if(font.size()) {
                MLESetFont(container_mle, size, font.compare("Default") == 0 ? NULL : font);
            }
            else
                MLESetFont(container_mle, size, NULL);
            return FALSE;
        });
    }

    // Page 5 - Buttons
    void CreateButtons(DW::Box *notebookbox) {
        // create a box to pack into the notebook page
        DW::Box *buttonsbox = new DW::Box(DW_VERT, 2);
        notebookbox->PackStart(buttonsbox, 25, 200, TRUE, TRUE, 0);
        buttonsbox->SetColor(DW_CLR_RED, DW_CLR_RED);

        DW::Box *calbox = new DW::Box(DW_HORZ, 0);
        notebookbox->PackStart(calbox, 0, 0, TRUE, FALSE, 1);
        DW::Calendar *cal = new DW::Calendar();
        calbox->PackStart(cal, TRUE, FALSE, 0);

        cal->SetDate(2019, 4, 30);

        // Create our file toolbar boxes...
        DW::Box *buttonboxperm = new DW::Box(DW_VERT, 0);
        buttonsbox->PackStart(buttonboxperm, 25, 0, FALSE, TRUE, 2);
        buttonboxperm->SetColor(DW_CLR_WHITE, DW_CLR_WHITE);
        DW::BitmapButton *topbutton = new DW::BitmapButton("Top Button", fileiconpath);
        buttonboxperm->PackStart(topbutton, 100, 30, FALSE, FALSE, 0 );
        buttonboxperm->PackStart(DW_NOHWND, 25, 5, FALSE, FALSE, 0);
        DW::BitmapButton *iconbutton = new DW::BitmapButton( "Folder Icon", foldericonpath);
        buttonsbox->PackStart(iconbutton, 25, 25, FALSE, FALSE, 0);
        iconbutton->ConnectClicked([this, iconbutton]() -> int
        {
            static int isfoldericon = 0;

            isfoldericon = !isfoldericon;
            if(isfoldericon)
            {
                iconbutton->Set(this->fileiconpath);
                iconbutton->SetTooltip("File Icon");
            }
            else
            {
                iconbutton->Set(this->foldericonpath);
                iconbutton->SetTooltip("Folder Icon");
            }
            return FALSE;
        });

        DW::Box *filetoolbarbox = new DW::Box(DW_VERT, 0);
        buttonboxperm->PackStart(filetoolbarbox, 0, 0, TRUE, TRUE, 0);

        DW::BitmapButton *button = new DW::BitmapButton("Empty image. Should be under Top button", 0, "junk");
        filetoolbarbox->PackStart(button, 25, 25, FALSE, FALSE, 0);
        button->ConnectClicked([buttonsbox]() -> int 
        {
            buttonsbox->SetColor(DW_CLR_RED, DW_CLR_RED);
            return TRUE;
        });
        filetoolbarbox->PackStart(DW_NOHWND, 25, 5, FALSE, FALSE, 0);

        button = new DW::BitmapButton("A borderless bitmapbitton", 0, foldericonpath);
        filetoolbarbox->PackStart(button, 25, 25, FALSE, FALSE, 0);
        button->ConnectClicked([buttonsbox]() -> int 
        {
            buttonsbox->SetColor(DW_CLR_YELLOW, DW_CLR_YELLOW);
            return TRUE;
        });
        filetoolbarbox->PackStart(DW_NOHWND, 25, 5, FALSE, FALSE, 0);
        button->SetStyle(DW_BS_NOBORDER);

        DW::BitmapButton *perbutton = new DW::BitmapButton("A button from data", 0, foldericonpath);
        filetoolbarbox->PackStart(perbutton, 25, 25, FALSE, FALSE, 0);
        filetoolbarbox->PackStart(DW_NOHWND, 25, 5, FALSE, FALSE, 0 );

        // make a combobox
        DW::Box *combox = new DW::Box(DW_VERT, 2);
        notebookbox->PackStart(combox, 25, 200, TRUE, FALSE, 0);
        DW::ComboBox *combobox1 = new DW::ComboBox("fred"); 
        combobox1->Append("fred");
        combox->PackStart(combobox1, TRUE, FALSE, 0);

        int iteration = 0;
        combobox1->ConnectListSelect([this, &iteration](unsigned int index) -> int
        {
            this->app->Debug("got combobox_select_event for index: %d, iteration: %d\n", index, iteration++);
            return FALSE;
        });

        DW::ComboBox *combobox2 = new DW::ComboBox("joe");
        combox->PackStart(combobox2, TRUE, FALSE, 0);
        combobox2->ConnectListSelect([this, &iteration](unsigned int index) -> int
        {
            this->app->Debug("got combobox_select_event for index: %d, iteration: %d\n", index, iteration++);
            return FALSE;
        });

        // add LOTS of items
        app->Debug("before appending 500 items to combobox using DW::ListBox::ListAppend()\n");
        std::vector<std::string> text;
        for(int i = 0; i < 500; i++)
        {
            text.push_back("item " + std::to_string(i));
        }
        combobox2->ListAppend(text);
        app->Debug("after appending 500 items to combobox\n");
        // now insert a couple of items
        combobox2->Insert("inserted item 2", 2);
        combobox2->Insert("inserted item 5", 5);
        // make a spinbutton
        DW::SpinButton *spinbutton = new DW::SpinButton();
        combox->PackStart(spinbutton, TRUE, FALSE, 0);
        spinbutton->SetLimits(100, 1);
        spinbutton->SetPos(30);
        
        spinbutton->ConnectValueChanged([this](int value) -> int
        {
            this->app->MessageBox("DWTest", DW_MB_OK, "New value from spinbutton: %d\n", value);
            return TRUE;
        });
        // make a slider
        DW::Slider *slider = new DW::Slider(FALSE, 11, 0); 
        combox->PackStart(slider, TRUE, FALSE, 0);

        // make a percent
        DW::Percent *percent = new DW::Percent();
        combox->PackStart(percent, TRUE, FALSE, 0);

        topbutton->ConnectClicked([this, combobox1, combobox2, spinbutton, cal]() -> int 
        {
            unsigned int idx = combobox1->Selected();
            std::string buf1 = combobox1->GetListText(idx);
            idx = combobox2->Selected();
            std::string buf2 = combobox2->GetListText(idx);
            unsigned int y,m,d;
            cal->GetDate(&y, &m, &d);
            long spvalue = spinbutton->GetPos();
            std::string buf3 = "spinbutton: " + std::to_string(spvalue) + "\ncombobox1: \"" + buf1 + 
                                "\"\ncombobox2: \"" + buf2 + "\"\ncalendar: " + std::to_string(y) + "-" +
                                std::to_string(m) + "-" + std::to_string(d);
            this->app->MessageBox("Values", DW_MB_OK | DW_MB_INFORMATION, buf3);
            this->app->SetClipboard(buf3);
            return 0;
        });

        perbutton->ConnectClicked([percent]() -> int 
        {
            percent->SetPos(DW_PERCENT_INDETERMINATE);
            return TRUE;
        });

        slider->ConnectValueChanged([percent](int value) -> int
        {
            percent->SetPos(value * 10);
            return TRUE;
        });
    }

    // Page 6 - HTML
    void CreateHTML(DW::Box *notebookbox) {
        DW::HTML *rawhtml = new DW::HTML();
        if(rawhtml && rawhtml->GetHWND())
        {
            DW::Box *hbox = new DW::Box(DW_HORZ, 0);
            DW::ComboBox *javascript = new DW::ComboBox();

            javascript->Append("window.scrollTo(0,500);");
            javascript->Append("window.document.title;");
            javascript->Append("window.navigator.userAgent;");

            notebookbox->PackStart(rawhtml, 0, 100, TRUE, FALSE, 0);
            rawhtml->JavascriptAdd("test");
            rawhtml->ConnectMessage([this](std::string name, std::string message) -> int
            {
                this->app->MessageBox("Javascript Message", DW_MB_OK | DW_MB_INFORMATION,
                              "Name: " + name + " Message: " + message);
                return TRUE;
            });
            rawhtml->Raw(dw_feature_get(DW_FEATURE_HTML_MESSAGE) == DW_FEATURE_ENABLED ?
                         "<html><body><center><h1><a href=\"javascript:test('This is the message');\">dwtest</a></h1></center></body></html>" :
                         "<html><body><center><h1>dwtest</h1></center></body></html>");
            DW::HTML *html = new DW::HTML();

            notebookbox->PackStart(hbox, 0, 0, TRUE, FALSE, 0);

            // Add navigation buttons
            DW::Button *button = new DW::Button("Back");
            hbox->PackStart(button, FALSE, FALSE, 0);
            button->ConnectClicked([html]() -> int 
            {
                html->Action(DW_HTML_GOBACK);
                return TRUE;
            });

            button = new DW::Button("Forward");
            hbox->PackStart(button, FALSE, FALSE, 0);
            button->ConnectClicked([html]() -> int 
            {
                html->Action(DW_HTML_GOFORWARD);
                return TRUE;
            });

            // Put in some extra space
            hbox->PackStart(0, 5, 1, FALSE, FALSE, 0);

            button = new DW::Button("Reload");
            hbox->PackStart(button, FALSE, FALSE, 0);
            button->ConnectClicked([html]() -> int 
            {
                html->Action(DW_HTML_RELOAD);
                return TRUE;
            });

            // Put in some extra space
            hbox->PackStart(0, 5, 1, FALSE, FALSE, 0);
            hbox->PackStart(javascript, TRUE, FALSE, 0);

            button = new DW::Button("Run");
            hbox->PackStart(button, FALSE, FALSE, 0);
            button->ConnectClicked([javascript, html]() -> int
            {
                std::string script = javascript->GetText();

                if(script.size()) {
                    html->JavascriptRun(script);
                }
                return FALSE;
            });
            javascript->ClickDefault(button);

            notebookbox->PackStart(html, 0, 100, TRUE, TRUE, 0);
            html->URL("https://dbsoft.org/dw_help.php");
            DW::StatusText *htmlstatus = new DW::StatusText("HTML status loading...");
            notebookbox->PackStart(htmlstatus, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);

            // Connect the signal handlers
            html->ConnectChanged([htmlstatus](int status, std::string url) -> int
            {
                std::vector<std::string> statusnames = { "none", "started", "redirect", "loading", "complete" };

                if(htmlstatus && status < 5) {
                    htmlstatus->SetText("Status " + statusnames[status] + ": " + url);
                }
                return FALSE;
            });

            html->ConnectResult([this](int status, std::string result, void *script_data) -> int
            {
                this->app->MessageBox("Javascript Result", DW_MB_OK | (status ? DW_MB_ERROR : DW_MB_INFORMATION),
                              result.size() ? result : "Javascript result is not a string value");
                return TRUE;
            });
        }
        else
        {
            DW::Text *htmltext = new DW::Text("HTML widget not available.");
            notebookbox->PackStart(htmltext, 0, 100, TRUE, TRUE, 0);
        }
    }

    // Page 7 - ScrollBox
    void CreateScrollBox(DW::Box *notebookbox) {
        // create a box to pack into the notebook page
        DW::ScrollBox *scrollbox = new DW::ScrollBox(DW_VERT, 0);
        notebookbox->PackStart(scrollbox, 0, 0, TRUE, TRUE, 1);

        DW::Button *adjbutton = new DW::Button("Show Adjustments", 0);
        scrollbox->PackStart(adjbutton, FALSE, FALSE, 0);
        adjbutton->ConnectClicked([this, scrollbox]() -> int 
        {
            int pos = scrollbox->GetPos(DW_VERT);
            int range = scrollbox->GetRange(DW_VERT);
            this->app->Debug("Pos %d Range %d\n", pos, range);
            return FALSE;
        });

        for(int i = 0; i < MAX_WIDGETS; i++)
        {
            DW::Box *tmpbox = new DW::Box(DW_HORZ, 0);
            scrollbox->PackStart(tmpbox, 0, 0, TRUE, FALSE, 2);
            DW::Text *label = new DW::Text("Label " + std::to_string(i));
            tmpbox->PackStart(label, 0, DW_SIZE_AUTO, TRUE, FALSE, 0);
            DW::Entryfield *entry = new DW::Entryfield("Entry " + std::to_string(i) , i);
            tmpbox->PackStart(entry, 0, DW_SIZE_AUTO, TRUE, FALSE, 0);
        }
    }

    // Page 8 - Thread and Event
    void CreateThreadEvent(DW::Box *notebookbox) {
        // create a box to pack into the notebook page
        DW::Box *tmpbox = new DW::Box(DW_VERT, 0);
        notebookbox->PackStart(tmpbox, 0, 0, TRUE, TRUE, 1);

        DW::Button *startbutton = new DW::Button("Start Threads");
        tmpbox->PackStart(startbutton, FALSE, FALSE, 0);

        // Create the base threading components
        DW::MLE *threadmle = new DW::MLE();
        tmpbox->PackStart(threadmle, 1, 1, TRUE, TRUE, 0);
        DW::Mutex *mutex = new DW::Mutex();
        DW::Event *workevent = new DW::Event();

        startbutton->ConnectClicked([this, mutex, workevent, threadmle, startbutton]() -> int
        {
            startbutton->Disable();
            mutex->Lock();
            DW::Event *controlevent = new DW::Event();
            workevent->Reset();
            finished = FALSE;
            ready = 0;
            UpdateMLE(threadmle, "Starting thread 1\r\n", DW_NULL);
            new DW::Thread([this, mutex, controlevent, workevent, threadmle](DW::Thread *thread) -> void {
                this->RunThread(1, mutex, controlevent, workevent, threadmle);
            });
            UpdateMLE(threadmle, "Starting thread 2\r\n", DW_NULL);
            new DW::Thread([this, mutex, controlevent, workevent, threadmle](DW::Thread *thread) -> void {
                this->RunThread(2, mutex, controlevent, workevent, threadmle);
            });
            UpdateMLE(threadmle, "Starting thread 3\r\n", DW_NULL);
            new DW::Thread([this, mutex, controlevent, workevent, threadmle](DW::Thread *thread) -> void {
                this->RunThread(3, mutex, controlevent, workevent, threadmle);
            });
            UpdateMLE(threadmle, "Starting thread 4\r\n", DW_NULL);
            new DW::Thread([this, mutex, controlevent, workevent, threadmle](DW::Thread *thread) -> void {
                this->RunThread(4, mutex, controlevent, workevent, threadmle);
            });
            UpdateMLE(threadmle, "Starting control thread\r\n", DW_NULL);
            new DW::Thread([this, startbutton, mutex, controlevent, workevent, threadmle](DW::Thread *thread) -> void {
                this->ControlThread(mutex, controlevent, workevent, threadmle);
                startbutton->Enable();
            });
            mutex->Unlock();
            return FALSE;
        });
    }
public:
    // Constructor creates the application
    DWTest(std::string title): DW::Window(title) {
        // Get our application singleton
        app = DW::App::Init();

        // Add menus to the window
        CreateMenus();
        
        // Create our notebook and add it to the window
        DW::Box *notebookbox = new DW::Box(DW_VERT, 5);
        this->PackStart(notebookbox, 0, 0, TRUE, TRUE, 0);

        /* First try the current directory */
        foldericon = app->LoadIcon(foldericonpath);
        fileicon = app->LoadIcon(fileiconpath);

#ifdef PLATFORMFOLDER
        // In case we are running from the build directory...
        // also check the appropriate platform subfolder
        if(!foldericon)
        {
            foldericonpath = std::string(PLATFORMFOLDER) + "folder";
            foldericon = app->LoadIcon(foldericonpath);
        }
        if(!fileicon)
        {
            fileiconpath = std::string(PLATFORMFOLDER) + "file";
            fileicon = app->LoadIcon(fileiconpath);
        }
#endif

        // Finally try from the platform application directory
        if(!foldericon && !fileicon)
        {
            std::string appdir = app->GetDir();
            
            appdir.append(std::string(1, DW_DIR_SEPARATOR));
            std::string folderpath = appdir + "folder";
            foldericon = app->LoadIcon(folderpath);
            if(foldericon)
                foldericonpath = folderpath;
            std::string filepath = appdir + "file";
            fileicon = app->LoadIcon(filepath);
            if(fileicon)
                fileiconpath = filepath;
        }

        DW::Notebook *notebook = new DW::Notebook();
        notebookbox->PackStart(notebook, TRUE, TRUE, 0);
        notebook->ConnectSwitchPage([this](unsigned long page_num) -> int 
        {
            this->app->Debug("DW_SIGNAL_SWITCH_PAGE: PageNum: %u\n", page_num);
            return TRUE;
        });

        // Create Notebook Page 1 - Input
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateInput(notebookbox);
        unsigned long notebookpage = notebook->PageNew(0, TRUE);
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "buttons and entry");

        // Create Notebook Page 2 - Render
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateRender(notebookbox);
        notebookpage = notebook->PageNew();
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "render");

        // Create Notebook Page 3 - Tree
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateTree(notebookbox);
        notebookpage = notebook->PageNew();
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "tree");

        // Create Notebook Page 4 - Container
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateContainer(notebookbox);
        notebookpage = notebook->PageNew();
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "container");

        // Create Notebook Page 5 - Buttons
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateButtons(notebookbox);
        notebookpage = notebook->PageNew();
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "buttons");

        // Create Notebook Page 6 - HTML
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateHTML(notebookbox);
        notebookpage = notebook->PageNew();
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "html");

        // Create Notebook Page 7 - ScrollBox
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateScrollBox(notebookbox);
        notebookpage = notebook->PageNew();
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "scrollbox");

        // Create Notebook Page 8 - Thread and Event
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateThreadEvent(notebookbox);
        notebookpage = notebook->PageNew();
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "thread/event");

        // Finalize the window
        this->SetSize(640, 550);

        timer = new DW::Timer(2000, [this]() -> int
        {
            this->app->Beep(200, 200);

            // Return TRUE so we get called again
            return TRUE;
        });
    }

    DW::App *app;

    // Page 1
    DW::Timer *timer;
    int cursor_arrow = TRUE;
    unsigned long current_color;

    // Page 2
    DW::Render *render1, *render2;
    DW::Pixmap *pixmap1, *pixmap2, *image;
    DW::ScrollBar *hscrollbar, *vscrollbar;
    DW::SpinButton *imagexspin, *imageyspin;
    DW::CheckBox *imagestretchcheck;

    int font_width = 8, font_height=12;
    int rows=10,width1=6,cols=80;
    int num_lines=0, max_linewidth=0;
    int current_row=0,current_col=0;
    unsigned int render_type = SHAPES_DOUBLE_BUFFERED;

    char **lp;
    std::string current_file;

    // Page 4
    int mle_point=-1;

    // Page 8
    int finished = FALSE, ready = 0;

    // Miscellaneous
    int menu_enabled = TRUE;
    HICN fileicon, foldericon;
    std::string fileiconpath = "file";
    std::string foldericonpath = "folder";


    int OnDelete() override {
        if(app->MessageBox(APP_TITLE, DW_MB_YESNO | DW_MB_QUESTION, APP_EXIT) != 0) {
            app->MainQuit();
        }
        return TRUE;
    }
};

// Pretty list of features corresponding to the DWFEATURE enum in dw.h
std::vector<std::string> DWFeatureList = {
    "Supports the HTML Widget",
    "Supports the DW_SIGNAL_HTML_RESULT callback",
    "Supports custom window border sizes",
    "Supports window frame transparency",
    "Supports Dark Mode user interface",
    "Supports auto completion in Multi-line Edit boxes",
    "Supports word wrapping in Multi-line Edit boxes",
    "Supports striped line display in container widgets",
    "Supports Multiple Document Interface window frame",
    "Supports status text area on notebook/tabbed controls",
    "Supports sending system notifications",
    "Supports UTF8 encoded Unicode text",
    "Supports Rich Edit based MLE control (Windows)",
    "Supports icons in the taskbar or similar system widget",
    "Supports the Tree Widget",
    "Supports arbitrary window placement",
    "Supports alternate container view modes",
    "Supports the DW_SIGNAL_HTML_MESSAGE callback",
    "Supports render safe drawing mode, limited to expose"
};

// Let's demonstrate the functionality of this library. :)
int dwmain(int argc, char* argv[]) 
{
    // Initialize the Dynamic Windows engine
    DW::App *app = DW::App::Init(argc, argv, "org.dbsoft.dwindows.dwtestoo", "Dynamic Windows Test C++");

    // Enable full dark mode on platforms that support it
    if(getenv("DW_DARK_MODE"))
        app->SetFeature(DW_FEATURE_DARK_MODE, DW_DARK_MODE_FULL);

#ifdef DW_MOBILE
    // Enable multi-line container display on Mobile platforms
    app->SetFeature(DW_FEATURE_CONTAINER_MODE, DW_CONTAINER_MODE_MULTI);
#endif

    // Test all the features and display the results
    for(int intfeat=DW_FEATURE_HTML; intfeat<DW_FEATURE_MAX; intfeat++)
    {
        DWFEATURE feat = static_cast<DWFEATURE>(intfeat);
        int result = app->GetFeature(feat);
        std::string status = "Unsupported";

        if(result == 0)
            status = "Disabled";
        else if(result > 0)
            status = "Enabled";

        app->Debug(DWFeatureList[intfeat] + ": " + status + " (" + std::to_string(result) + ")\n");
    }

    DWTest *window = new DWTest("dwindows test UTF8 中国語 (繁体) cañón");
    window->Show();

    app->Main();
    app->Exit(0);

    return 0;
}
#endif
