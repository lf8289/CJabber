#include <popup.h>
#include <program.h>
#include <keycodes.h>
#include <textpopup.h>

CJabberCore::CJabberCore()
{
    initscr();
    cbreak();
    //keypad(stdscr, TRUE);
    height = LINES;
    width = COLS;

    // setting up chat and input window
    borderWindow = createWindow(5,          width - 3, height - 5, 3);
    inputWindow  = createWindow(3,          width - 5, height - 4, 4);
    chatWindow   = createWindow(height - 5, width    , 0         , 0);
    keypad(inputWindow, TRUE);

    // for proper input cursor position
    wrefresh(inputWindow);
    wmove(inputWindow, 0, 0);
    int y, x;
    getparyx(inputWindow, y, x);
    MessageWindow::getInstance().setInputX(x);
    MessageWindow::getInstance().setInputY(y);

    // setting refs for printMsg
    MessageWindow::getInstance().setWindow(chatWindow, height - 5, width);
    scrollok(MessageWindow::getInstance().getWindow(), TRUE);
    jid = JID("");
}

CJabberCore::~CJabberCore()
{
    if (xmpp != NULL)
        delete xmpp;
    destroyWindow(chatWindow);
    destroyWindow(inputWindow);
    destroyWindow(borderWindow);
    endwin();
}

void CJabberCore::repaintDecoration()
{
    mvprintw(height - 4, 0, ">> ");
    refresh();
    box(borderWindow, 0, 0);
    wrefresh(borderWindow);
    wclear(inputWindow);
    wrefresh(inputWindow);
    box(MessageWindow::getInstance().getWindow(), 0, 0);
    wrefresh(MessageWindow::getInstance().getWindow());
    wmove(inputWindow, 0, 0);
}

// XMPP
void CJabberCore::connect(string username, string server, string password, string resource)
{
    if (username.length() <= 0 || server.length() <= 0)
    {
        string header  = "Error!";
        string message = "Check username or password|in configuration file!|(Press any key)";
        Popup p(header, message);
        xmpp = NULL;
    }
    else
    {
        invalidRC = false;
        if (password.length() <= 0)
        {
            TextPopup p("Podaj hasło:", true);
            password = p.getInput();
        }

        xmpp = new XMPPconnection(username, server, resource, password);

        // waiting for connection
        pthread_cond_wait(&connect_var, &connect_mutex);
        repaintDecoration();
    }
}

void CJabberCore::innerEventLoop()
{
    int c;
    bool end = invalidRC;
    string buffer = "";
    stringstream bufferToSend;
    int bufferLength = 0;
    int cursorPosition = 0;

    while(!end)
    {
        c = wgetch(inputWindow);
        switch(c)
        {
            case ESC:
                end = true;
                wclear(inputWindow);
                wrefresh(inputWindow);
                wmove(inputWindow, 0, 0);
                break;
            case ENTER:
                wclear(inputWindow);
                wmove(inputWindow, 0, 0);
                wrefresh(inputWindow);
                if (jid == true && buffer.at(0) != '/')
                {
                    bufferToSend << "ja: " << buffer;
                    MessageWindow::getInstance().printWin(bufferToSend, Me);
                    xmpp->sendMessage(jid, buffer.c_str());

                }
                else
                {
                    Command c = getCommand(buffer);
                    if (c == Quit)
                    {
                        end = true;
                    }
                }
                bufferToSend.str("");
                bufferToSend.clear();
                buffer.clear();
                cursorPosition = 0;
                repaintDecoration();
                break;
            case BACKSPACE:
                if (cursorPosition > 0)
                {
                    cursorPosition--;
                    buffer.erase(cursorPosition);
                    wclear(inputWindow);
                    wprintw(inputWindow, "%s", buffer.c_str());
                    wrefresh(inputWindow);
                }
                break;
            default:
                buffer += (char)c;
                cursorPosition++;
        }
    }
}

Command CJabberCore::getCommand(string command)
{
    stringstream s;
    if (command.length() > 0)
    {
        if (command.at(0) == '/')
        {
            size_t found;
            found = command.find("talk");
            if (found != string::npos)
            {
                string tmp = command.erase(0, 6);
                jid = JID(tmp);
                if (jid != true)
                {
                    printInfo();
                    s << "Błędnie podany identyfikator użytkownika.";
                    MessageWindow::getInstance().printWin(s, Log);
                }
            }
            else
            {
                found = command.find("quit");
                if (found != string::npos)
                {
                    return Quit;
                }
            }
        }
        else
        {
            printInfo();
        }
    }
}

void CJabberCore::printInfo()
{
    stringstream s;
    s << "CJabber: jak korzystać?" << endl
      << endl
      << " /talk user@serwer.com aby rozmawiać z innym użytkownikiem." << endl
      << endl
      << " /quit lub wcisąć <ESC> na klawiaturze, aby wyjść." << endl
      << endl;
    MessageWindow::getInstance().printWin(s, Log);
//     MessageWindow::getInstance().printWin(" ");
//     MessageWindow::getInstance().printWin();
//     MessageWindow::getInstance().printWin();
//     MessageWindow::getInstance().printWin(" ");
}