/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  gluk47@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "Cli.h"
#include <iostream>

#define DEFINE_WHEREAMI_MACRO
#include "helpers/cli.h"
#include "Kdb3Database.h"

#include <string>
#include <SecString.h>
#include "Database.h"
#include <QtCore/qstringlist.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <termios.h>
#include <unistd.h>

using namespace helpers::cli;
using namespace std;

void enable_echo(bool on = true) {
    termios settings;
    tcgetattr( STDIN_FILENO, &settings );
    settings.c_lflag = on
                    ? (settings.c_lflag |   ECHO )
                    : (settings.c_lflag & ~(ECHO));
    tcsetattr( STDIN_FILENO, TCSANOW, &settings );
}

struct tmp_echo_disable {
    tmp_echo_disable(){ enable_echo(false); }
    ~tmp_echo_disable() { enable_echo(true); cout << "\n"; }
};

const char* Qstring2constchars(const QString& _) {
    return _.toUtf8().constData();
}
char* strdup(const QString& _) {
    return strdup(Qstring2constchars(_));
}

Cli& Cli::the() {
    static Cli interface;
    return interface;
}

std::ostream& operator<< (std::ostream& _str, const QString& _qstr) {
    return _str << _qstr.toUtf8().constData();
}
bool Cli::openDatabase(const QString& filename, bool IsAuto) {
    if (!QFile::exists(filename)){
        cerr << __WHEREAMI__ << "> File «"
             << filename << "» was not found." << endl;
        return false;
    }
    _Lockfile = filename + ".lock";
    if (QFile::exists(_Lockfile)) {
        const char readonly_ans =
            ReadYesNoChar(tr("The database you are trying to open is locked.\n"
                "This means that either someone else has opened the file or KeePassX crashed last time it opened the database.\n\n"
                "Do you want to open it anyway read only?"
        ).toStdString(),
            string("yд"),
            string("nн"),
            'y');
        if (readonly_ans == 'y') dbReadOnly = true;
        else return false;
    }
//     config->setLastKeyLocation(QString());
//     config->setLastKeyType(PASSWORD);
    db.reset(new Kdb3Database(true));
    if (!dbReadOnly && !QFile::exists(_Lockfile)){
        QFile lock(_Lockfile);
        if (!lock.open(QIODevice::WriteOnly))
            dbReadOnly = true;
    }
    QString pass = ReadPassword(PasswdConfirm::no);
    if (pass.isNull()) {
        cout << "Failed to get the database password.\nAu revoir!\n";
        return false;
    }
    cerr << "db->setKey... ";
    db->setKey(pass, "");
    cerr << "ok\ndb->load...\n";
    if(db->load(filename, dbReadOnly)) {
        cerr << "db opened";
        if (IsLocked)
            resetLock();
        updateCurrentFile(filename);
        saveLastFilename(filename);
        setStateFileOpen(true);
        setStateFileModified(static_cast<Kdb3Database*>(db.get())->hasPasswordEncodingChanged());
    } else {
        if (!dbReadOnly && QFile::exists(_Lockfile))
            QFile::remove(_Lockfile);
        QString error = db->getError();
        if(error.isEmpty())error=tr("Unknown error while loading database.");
        cerr << tr("The following error occured while opening the database:\n")
             << error << "\n";
        if(db->isKeyError()){
            db.reset();
            return openDatabase(filename,IsAuto);
        } else {
            db.reset();
            return false;
        }
    }
    size_t last_slash = filename.lastIndexOf("/");
    if (last_slash != - 1) _Filename = filename.mid(last_slash + 1);
    else _Filename = filename;
    rl_attempted_completion_function = tab_complete;
    return true;
}
void Cli::closeDatabase() {
    if (db.get() != NULL) {
        db->close();
        db.reset();
    }
    if (!dbReadOnly && QFile::exists(_Lockfile)){
        if (!QFile::remove(_Lockfile))
            cerr << tr("Error") << "\n"
            << tr("Couldn't remove database lock file.");
    }
    _Lockfile.clear();
}
void Cli::saveLastFilename(const QString& filename) {
    return;
    ///\todo implement
    if(config->openLastFile()){
        if(config->saveRelativePaths()){
            QString Path=filename.left(filename.lastIndexOf("/"));
            Path=makePathRelative(Path,QDir::currentPath());
            config->setLastFile(Path+filename.right(filename.length()-filename.lastIndexOf("/")-1));
        }
        else
            config->setLastFile(filename);
    }
}
int Cli::Run(const QString& _filename) {
    std::cout << "cli is under construction\n";
    std::cout << "class Cli> openDatabase ("
              << _filename.toStdString() << ")\n";
    bool opened = openDatabase(_filename);
    std::cout << "openDatabase " << (opened? "succeed" : "fail") << "ed\n";
    if (not opened) return 1;
    QStringList cmd = readCmd();
    while (true) {
        if (not cmd.empty() and cmd[0] == "exit") break;
        ProcessCmd(cmd);
        cmd = readCmd();
    }
    if (ModFlag) {
        cout << "The database has not been saved since last modification.\n"
             << "Would you like to save it now before exit?";
        if (helpers::cli::ReadYesNoChar("", "y", "n", 'y')) {
            db->save();
            cout << "The database has been saved.\n";
        }
    }
    closeDatabase();
    return 0;
}
void Cli::search() {
    QList<IEntryHandle*> SearchResults =
        db->search(NULL, "", false, false, false, NULL);
    for (QList<IEntryHandle*>::const_iterator i = SearchResults.constBegin();
            i != SearchResults.constEnd(); ++i) {
        IGroupHandle* group = (*i)->group();
        cerr << "\t @" << group->title().toStdString() << "\n"
             << "\t=== " << (*i)->title().toStdString() << " ===\n"
             << "\turl>      " << (*i)->url().toStdString() << "\n"
             << "\tuser>     " << (*i)->username().toStdString() << "\n";
        SecString passwd = (*i)->password();
        passwd.unlock();
        cerr << "\tpass>     " << passwd.string().toStdString() << "\n";
        passwd.lock();
        cerr << "\tcomment>  " << (*i)->comment().toStdString() << "\n"
             << "\n";
    }
}
QStringList Cli::readCmd() {
    QStringList ret;
    QString prompt = "[kpx@" + _Filename + " "
                    + (_Wd != NULL? _Wd->title(): "/")
                    + "]$ ";
    struct auto_free {
        auto_free(char* _) : data(_){}
        ~auto_free() { free(data); }
        operator char*() const { return data; }
        char operator[](size_t _) const { return data[_]; }
        char* data;
    } line (readline(Qstring2constchars(prompt)));
    if (line == NULL) {
        cout << "Au revoir\n";
        ret.push_back("exit"); return ret;
    }
    if (line[0] == '\0') return ret;
    size_t lexem_start = 0;
    // remove leading spaces
    while (line[lexem_start] == ' ') ++lexem_start;
    // nothing but spaces in the line?
    if (line[lexem_start] == 0) return ret;
    add_history(line);
    size_t i;
    char quote = 0;
    QString lexem;
    for (i = lexem_start; line[i] != 0; ++i) {
        const char c = line[i];
        if (quote != 0) {
            if (c == quote) {
                if (i > lexem_start)
                    lexem += QString::fromUtf8(line + lexem_start,
                                               i - lexem_start);
                quote = 0;
                lexem_start = i + 1;
            }
            continue;
        } else {
            if (c == '\'' or c == '"') {
                quote = c;
                if (i > lexem_start + 1) {
                    lexem += QString::fromUtf8(line + lexem_start,
                                               i - lexem_start);
                }
                lexem_start = i + 1;
                continue;
            }
            if (c != ' ') continue;
            if (i > lexem_start)
                lexem += QString::fromUtf8(line + lexem_start,
                                           i - lexem_start);
            ret.push_back(lexem);
            lexem.clear();
//             ret.push_back(QString::fromUtf8(line + lexem_start, i - lexem_start));
        }
        lexem_start = i;
        while (line[lexem_start] == ' ') ++lexem_start;
        if (line[lexem_start] == 0) return ret;
        i = lexem_start - 1;
    }
    if (lexem_start < i) {
        if (quote != 0) cerr << "warning: autoclosing quote!\n";
        lexem += QString::fromUtf8(line + lexem_start, i - lexem_start);
    }
    if (not lexem.isEmpty()) ret.push_back(lexem);
    return ret;
}
void Cli::ls(bool _quiet) const {
    QList<IGroupHandle*> group = db->groups();
    for (QList<IGroupHandle*>::const_iterator i = group.constBegin();
         i != group.constEnd(); ++i) {
        if ((*i)->parent() != _Wd)
            continue;
        _HSubGroups[(*i)->title()] = *i;
        if (not _quiet) cout << "+ " << (*i)->title() << "\n";
    }

    if (_Wd == NULL) return;
    QList<IEntryHandle*> entries = db->entries(_Wd);
    bool update_entries = _HEntries.isEmpty();
    for (QList<IEntryHandle*>::const_iterator i = entries.constBegin();
         i != entries.constEnd(); ++i) {
        if (update_entries) _HEntries.insert((*i)->title(), *i);
        if (not _quiet) cout << "⋅ " << (*i)->title() << "\n";
    }
}
void Cli::help() const {
    cout << "l, ls — list entries and groups in current group\n"
         << "cd <subgroup>  — change current group to its subgroup\n"
         << "cd .., s — go to parent group\n"
         << "cd -, back, p — go to previous group (double «p» moves to previous, then back to current group)\n"
         << "cd / — go to the root group\n"
         << "cat <entry> — display entry information\n"
         << "passwd <entry> — show passwd stored in entry. Use Ctrl+L to clear screen.\n"
         << "set — modify a database entry. Type «set help» for details.\n"
         << "create — create new entry\n"
         << "rm — completely remove an entry from database\n"
         << "save — save the database to disk\n"
         << "help — display help\n";
}
void Cli::cd(const QString& _subgroup) {
    if (_HSubGroups.empty()) ls(true);
    if (_subgroup == "-") {
        std::swap(_Wd, _Previous_Wd);
        _HSubGroups.clear();
        _HEntries.clear();
        return;
    }
    if (_subgroup == "..") {
        if (_Wd == NULL) return;
        _Previous_Wd = _Wd;
        _Wd = _Wd->parent();
        _HSubGroups.clear();
        _HEntries.clear();
        return;
    }
    if (_subgroup == "/") {
        _Previous_Wd = _Wd;
        _Wd = NULL;
        _HSubGroups.clear();
        _HEntries.clear();
        return;
    }
    if (not _HSubGroups.contains(_subgroup)
        or _HSubGroups[_subgroup] == NULL) {
        cerr << "There is no subgroup «" << _subgroup
             << "» in the current group.\n";
        return;
    }

    _Previous_Wd = _Wd;
    _Wd = _HSubGroups[_subgroup];
    _HSubGroups.clear();
    _HEntries.clear();
}
QList< IEntryHandle* > Cli::find_in_cwd(const QString& _entry) const {
    if (_entry.isEmpty()) return QList< IEntryHandle* >();
    if (_HEntries.empty()) ls(true);
    const QList<IEntryHandle*> keys = _HEntries.values(_entry);
    if (keys.empty())
        cout << "There is no entry with title «" << _entry
             << "» in the current group. Consider using the «l» command or try «?».\n";
    return keys;
}
void Cli::cat(const QString& _entry) const {
    const QList<IEntryHandle*> keys = find_in_cwd(_entry);
    for (QList<IEntryHandle*>::const_iterator i = keys.constBegin();
         i != keys.constEnd(); ++i) {
        cout << "\t=== " << (*i)->title() << " ===\n"
             << "\turl>      " << (*i)->url() << "\n"
             << "\tuser>     " << (*i)->username() << "\n"
             << "\tpass> <type «passwd " << _entry << "» to reveal the password>\n"
             << "\tcomment>  " << (*i)->comment() << "\n"
             << "\n";
    }
}
void Cli::passwd(const QString& _entry) const {
    const QList<IEntryHandle*> keys = find_in_cwd(_entry);
    for (QList<IEntryHandle*>::const_iterator i = keys.constBegin();
         i != keys.constEnd(); ++i) {
        SecString passwd = (*i)->password();
        passwd.unlock();
        cout << (*i)->title() << "> " << passwd.string() << "\n";
        passwd.lock();
    }
}
IEntryHandle* Cli::confirm_first(const QString& _title) {
    QList <IEntryHandle*> entries = find_in_cwd(_title);
    if (entries.empty()) return NULL;
    ///\todo support more than one entry with the same name in one group
    if (entries.size() > 1) {
        cout << "There is more than one record with the name «"
                << _title << "» in the current group. ";
        if (helpers::cli::ReadYesNoChar("The first one will be modified, is it okay?", "y", "n", 'y') == 'n') return NULL;
    }
    return entries[0];
}
void Cli::set(const QStringList& _params) {
    if (_params.isEmpty() or _params[0] == "help" or _params[0] == "?") {
        cout << "\e[1m== set help ==\e[0m\n";
        cout << "Hereafter the word «record» means the title of an existing record in the current group. List of records you can get either using command «ls» sor by pressing <Tab> twice after the whole previous part of the set command.\n";
        cout << " \e[1m⋅\e[0m set title record new_title\n";
        cout << " \e[1m⋅\e[0m set user record new_username\n";
        cout << " \e[1m⋅\e[0m set passwd record new_passwd\n";
        cout << " \e[1m⋅\e[0m set url record new_passwd\n";
        cout << " \e[1m⋅\e[0m set comment record new_passwd\n";
        cout << "if the last argument (new_...) is ommitted, the existing data field will be cleared, but you cannot clear title.";
        return;
    }
    if(_params.size() < 2) {
        cerr << "You need to supply 2 or 3 parameters to the command «set "
        << _params[0] << "»:\nset "
        << _params[0] << " title new_" << _params[0] << "\n"
        << "if new_" << _params[0] << " is ommitted, the existing one will be deleted.";
        return;
    }
    IEntryHandle* _ = confirm_first(_params[1]);
    if (_ == NULL) return;
    if (_params[0] == "title") {
        if (_params.size() < 3) {
            cout << "You cannot delete the tzdanie1itle of the record, sorry. I cannot do it too.";
            return;
        }
        _->setTitle(_params[2]);
        ModFlag = true;
        _HEntries.remove(_params[1], _);
        _HEntries.insert(_params[2], _);
        return;
    }
    if (_params[0] == "passwd") {
        SecString p;
        QString sp (_params[2]);
        p.setString(sp, true);
        _->setPassword(p);
        ModFlag = true;
        return;
    }
    if (_params[0] == "user"
            or _params[0] == "login"
            or _params[0] == "username") {
        _->setUsername(_params[2]);
        ModFlag = true;
        return;
    }
    if (_params[0] == "url") {
        _->setUrl(_params[2]);
        ModFlag = true;
        return;
    }
}
void Cli::create(const QStringList& _entries) {
    for (size_t i = 0; i < _entries.size(); ++i) {
        if (_HEntries.contains(_entries[i])) {
            cout << "Current group already contains an item with title «"
                 << _entries[i] << "». If you create a new one, you won't be able to modify one of the items from this console interface of keepassx. ";
            if (helpers::cli::ReadYesNoChar("Continue?", "y", "n", 'n') == 'n')
                continue;
        }
        IEntryHandle* _new = db->newEntry(_Wd);
        _new->setTitle(_entries[i]);
        ModFlag = true;
        if (not _HEntries.empty())
            _HEntries.insert(_entries[i], _new);
    }
}
void Cli::rm(const QStringList& _entries) {
    for (size_t i = 0; i < _entries.size(); ++i) {
        IEntryHandle* _ = confirm_first(_entries[i]);
        if (_ == NULL) continue;
        db->deleteEntry(_);
    }
    ModFlag = true;
}
void Cli::save(const QStringList& _empty_list) {
    if (not _empty_list.isEmpty()) {
        cerr << "The command «save» has no parameters. If you meant «save as...», by now you'll have to do it using your shell, I'm sorry. Some update will enable the «save as...» semantics, but later.\n";
        return;
    }
    db->save();
    ModFlag = false;
}
QString Cli::ReadPassword(PasswdConfirm::flag _confirmation_needed) {
    QString pass;
   {cout << "Please, enter password: ";
    tmp_echo_disable __rped__;
    string s;
    cin >> s;
    pass = QString::fromUtf8(s.c_str());
   }if (not cin) return QString();
    if (_confirmation_needed == PasswdConfirm::no) return pass;

    QString confirm;
    while (confirm != pass and cin) {
        cout << "Reenter password or hit Ctrl+D to break: ";
        tmp_echo_disable __rped__;
        string s;
        cin >> s;
        confirm = QString::fromUtf8(s.c_str());
    }
    if (cin) return pass;
    return QString(); //isNull, not isEmpty
}
void Cli::ProcessCmd(const QStringList& _) {
    if (_.empty()) return;
    if (_[0] == "ls" || _[0] == "l") ls();
    else if (_[0] == "cd") cd(_.size() > 1 ? _[1] : "/");
    else if (_[0] == "s") cd ("..");
    else if (_[0] == "p" or _[0] == "back") cd ("-");
    else if (_[0] == "cat") {
        QStringList::const_iterator i = _.constBegin();
        while (++i != _.constEnd()) cat (*i);
    }
    else if (_[0] == "passwd") {
        QStringList::const_iterator i = _.constBegin();
        while (++i != _.constEnd()) passwd (*i);
    }
    else if (_[0] == "set") set(_.mid(1));
    else if (_[0] == "create") create(_.mid(1));
    else if (_[0] == "rm") rm(_.mid(1));
    else if (_[0] == "save") save(_.mid(1));
    else if (_[0] == "help" || _[0] == "?") help();
    else cerr << "The command «" << _[0].toStdString()
              << "» is undefined, sorry. Try «help» or «?».\n";
}
// =================== GNU Readline interface ===================
char** Cli::tab_complete(const char* _beginning, int _start, int /*_end*/) {
    rl_attempted_completion_over = true;
    if (_start == 0) return rl_completion_matches(_beginning, cmd_completer);
//     if (_end > 2 and !strcmp (_beginning, "cd"))
/*    cerr << "rl_line_buffer is «" << rl_line_buffer << "»\n";
    cerr << "_beginning is «" << _beginning << "»\n";
    cerr << "_start is " << _start << "\n";*/
    const QString user_input = QString::fromUtf8(rl_line_buffer);
    if (user_input.startsWith("cat ")
        or user_input.startsWith("passwd ")
        or user_input.startsWith("rm ")
       ) 
        return rl_completion_matches(_beginning, entries_completer);
    if (user_input.startsWith("cd "))
        return rl_completion_matches(_beginning, dir_completer);
    if (user_input.startsWith("set ")) {
        QString set_cmd = user_input.mid(4);
        if (not set_cmd.contains(QRegExp("[^ ]+ ")))
            // не введена команда для set
            return rl_completion_matches(_beginning, set_completer);
        if (not set_cmd.contains(QRegExp("[^ ]+ [^ ]+ ")))
            return rl_completion_matches(_beginning, entries_completer);
        return NULL; 
    }
    return NULL;
}
char* complete(const char* _beginning, const QStringList& _list, int& _start) {
    while (++_start < _list.size()) {
        if (_list[_start].startsWith(QString::fromUtf8(_beginning)))
            return strdup(_list[_start]);
    }
    return NULL;
}
char* Cli::entries_completer(const char* _beginning, int _state) {
    if (the()._HEntries.empty()) the().ls(true);
    static int i; if (_state == 0) i = -1;
    return complete(_beginning, the()._HEntries.keys(), i);
}
char* Cli::dir_completer(const char* _beginning, int _state) {
    if (the()._HEntries.empty()) the().ls(true);
    static int i; if (_state == 0) i = -1;
    return complete(_beginning, the()._HSubGroups.keys(), i);
}
QStringList Cli::_AllCmds;
char* Cli::cmd_completer(const char* _beginning, int _state) {
    if (_AllCmds.empty()) {
#define _(cmd) _AllCmds.push_back(cmd)
        _("cd"), _("s"), _("p");
        _("l"), _("ls");
        _("cat"), _("passwd");
        _("set"), _("create"), _("rm");
        _("save");
        _("?"), _("help");
#undef _
    }
    static int i; if (_state == 0) i = -1;
    return complete(_beginning, _AllCmds, i);
}
char* Cli::set_completer(const char* _beginning, int _state) {
    static int i; if (_state == 0) i = -1;
    QStringList cmds;
#define _(cmd) cmds.push_back(cmd)
    _("title"); _("passwd"); _("url"), _("comment"); ("user");
#undef __AllC
    return complete(_beginning, cmds, i);
}
