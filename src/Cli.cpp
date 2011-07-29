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
#include <qmap.h>

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
        cout << __WHEREAMI__ << "> File «"
             << filename << "» was not found." << endl;
//         return false;
        cout << "Would you like to create there a new database? ";
        if (ReadYesNoChar("", "y", "n", 'y') != 'y')
            return false;
        db.reset(new Kdb3Database(true));
        db->create();
        db->changeFile(filename);
        QString passwd = ReadPassword(PasswdConfirm::yes);
        db->setKey(passwd, "");
        db->generateMasterKey();
        QStringList initial_groups;
        initial_groups.push_back("Internet");
        initial_groups.push_back("eMail");
        md(initial_groups);
        db->save();
        return true;
    }
    _Lockfile = filename + ".lock";
    if (QFile::exists(_Lockfile)) {
        dbReadOnly = true;
        const char readonly_ans =
            ReadYesNoChar(tr("The database you are trying to open is locked.\n"
                "This means that either someone else has opened the file or KeePassX crashed last time it opened the database.\n\n"
                "Do you want to open it anyway read only?"
        ).toStdString(),
            string("yд"),
            string("nн"),
            'y');
        if (readonly_ans != 'y') return false;
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
    db->setKey(pass, "");
    if(db->load(filename, dbReadOnly)) {
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
        if (db->groups().isEmpty()) {
            cout << "The database has unsaved modifications but cannot be saved since it is empty. It will be rolled back to the last saved version.\n";
        } else {
            cout << "The database has not been saved since last modification.\n"
                 << "Would you like to save it now before exit?";
            if (helpers::cli::ReadYesNoChar("", "y", "n", 'y')) {
                db->save();
                cout << "The database has been saved.\n";
            }
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
    struct read_last_endl {
        ~read_last_endl() {
            istream::char_type c = 0;
            while (not cin.readsome(&c, 1) > 0) {
                cerr << "Skipping char #" << (int)c << "\n";
                if (c != '\n' and c != '\r') {
                    cerr << "unget\n";
                    cin.unget();
                    return;
                } 
            }
        }
    };
    if (line == NULL) {
        if (ModFlag and db->groups().isEmpty()) {
            
        }
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
void Cli::ls(Cli::ls_quiet::flag _quiet) const {
    QList<IGroupHandle*> group = db->groups();
    _HSubGroups.clear();
    for (QList<IGroupHandle*>::const_iterator i = group.constBegin();
         i != group.constEnd(); ++i) {
        if ((*i)->parent() != _Wd)
            continue;
        _HSubGroups.insert((*i)->title(), *i);
        if (not _quiet) cout << "+ " << (*i)->title() << "\n";
    }

    if (_Wd == NULL) return;
    QList<IEntryHandle*> entries = db->entries(_Wd);
    _HEntries.clear();
    for (QList<IEntryHandle*>::const_iterator i = entries.constBegin();
         i != entries.constEnd(); ++i) {
        _HEntries.insert((*i)->title(), *i);
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
         << "create <entry> — create new entry. Cannot be applied to the root group.\n"
         << "rm <entry> — completely remove an entry from database\n"
         << "md <group>, mkdir <group> — create a new subgroup here\n"
         << "save — save the database to disk\n"
         << "help — display help\n";
}
void Cli::cd(const QString& _subgroup) {
    if (_HSubGroups.empty()) ls(ls_quiet::yes);
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
    IGroupHandle* _ = confirm_first<IGroupHandle>(_subgroup);
    if (_ == NULL) {
        cerr << "There is no subgroup «" << _subgroup
             << "» in the current group.\n";
        return;
    }

    _Previous_Wd = _Wd;
    _Wd = _;
    _HSubGroups.clear();
    _HEntries.clear();
}
template <typename T>
QList<T*> Cli::find_in_cwd(const QString& _entry) const {
    if (_entry.isEmpty()) return QList<T*>();
    const QMultiMap<QString,T*> lst = _Wd_map<T>();
    if (lst.empty()) ls(ls_quiet::yes);
    const QList<T*> keys = lst.values(_entry);
    if (keys.empty())
        cout << "There is no item with title «" << _entry
             << "» in the current group. Consider using the «l» command or try «?».\n";
    return keys;
}
void Cli::cat(const QString& _entry) const {
    const QList<IEntryHandle*> keys = find_in_cwd<IEntryHandle>(_entry);
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
    const QList<IEntryHandle*> keys = find_in_cwd<IEntryHandle>(_entry);
    for (QList<IEntryHandle*>::const_iterator i = keys.constBegin();
         i != keys.constEnd(); ++i) {
        SecString passwd = (*i)->password();
        passwd.unlock();
        cout << (*i)->title() << "> " << passwd.string() << "\n";
        passwd.lock();
    }
}
template<typename T>
T* Cli::confirm_first(const QString& _title) {
    QList <T*> entries = find_in_cwd<T>(_title);
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
    IEntryHandle* _ = confirm_first<IEntryHandle>(_params[1]);
    if (_ == NULL) return;
    if (_params[0] == "comment") {
        if (_params.size() < 3)
            _->setComment(QString());
        else
            _->setComment(_params[2]);
        return;
    }
    if(_params.size() < 3) {
        cerr << "You need to supply 3 parameters to the command «set "
        << _params[0] << "»:\nset "
        << _params[0] << " title new_" << _params[0] << "\n";
        return;
    }
    if (_params[0] == "title") {
        if (_params.size() < 3) {
            cout << "You cannot delete the title of the record, sorry. I cannot do it too...";
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
    if (_Wd == NULL) {
        cout << "It is impossible to create an entry in the root group, sorry.\n";
        return;
    }
    if (_HEntries.isEmpty()) ls(ls_quiet::yes);
    for (size_t i = 0; i < _entries.size(); ++i) {
        if (_HEntries.contains(_entries[i])) {
            cout << "Current group already contains an item with title «"
                 << _entries[i] << "». If you create a new one, you won't be able to modify one of the entries from this console interface of keepassx. ";
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
void Cli::md(const QStringList& _entries) {
    for (size_t i = 0; i < _entries.size(); ++i) {
        if (_HSubGroups.contains(_entries[i])) {
            cout << "Current group already contains a subgroup named «"
                 << _entries[i] << "». If you create a new one, you won't be able to cd to one of the groups from this console interface of keepassx. ";
            if (helpers::cli::ReadYesNoChar("Continue?", "y", "n", 'n') == 'n')
                continue;
        }
        CGroup newGroup;
        newGroup.Title = _entries[i];
        IGroupHandle* _new = db->addGroup(&newGroup, _Wd);
        ModFlag = true;
        if (not _HSubGroups.empty())
            _HSubGroups.insert(_entries[i], _new);
    }
}
void Cli::rm(const QStringList& _entries) {
    for (size_t i = 0; i < _entries.size(); ++i) {
        IEntryHandle* const _ = confirm_first<IEntryHandle>(_entries[i]);
        if (_ == NULL) continue;
        db->deleteEntry(_);
        _HEntries.remove(_entries[i], _);
    }
    ModFlag = true;
}
void Cli::rm_rf(const QStringList& _names) {
    if (_HSubGroups.isEmpty()) ls(ls_quiet::yes);
    for (size_t i = 0; i < _names.size(); ++i) {
        IGroupHandle* _ = confirm_first<IGroupHandle>(_names[i]);
        if (_ == NULL) {
            cerr << "There is no subgroup «"
                 << _names[i] << "» in the current group,\n"
                 << "use «ls» to get the full list.\n";
            continue;
        }
        if (_HSubGroups.size() == 1) {
            cout << "You are about to delete the last group in the database. You won't be able to save it until some new group is created. If you wish to save the database now, enter «n», otherwise answer «y» to continue deletion";
            if (ReadYesNoChar("", "y", "n", 'y') != 'y') return;
        }
        cout << "Are you absolutely sure you wish to irrecoverably erase the whole group «" << _names[i] << "» with all its contents?";
        if (ReadYesNoChar("", "y", "n", 'n') != 'y') continue;
        db->deleteGroup(_);
        if (not _HSubGroups.isEmpty())
            _HSubGroups.remove(_names[i], _);
        ModFlag = true;
    }
}
bool Cli::save(const QStringList& _empty_list) {
    if (not _empty_list.isEmpty()) {
        cerr << "The command «save» has no parameters. If you meant «save as...», by now you'll have to do it using your shell, I'm sorry. Some update will enable the «save as...» semantics, but later.\n";
        return false;
    }
    if (_Wd == NULL) {
        if (db->groups().isEmpty()) {
            // no, that's not cli limitation, db->save() just does absolutely nothing for an empty database.
            cerr << "Sorry, it is impossible to save an empty database.\n";
            return false;
        }
    }
    db->save();
    ModFlag = false;
    return true;
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
    else if (_[0] == "md" || _[0] == "mkdir") md(_.mid(1));
    else if (_[0] == "rd" || _[0] == "rmdir") rm_rf(_.mid(1));
    else if (_[0] == "save") save(_.mid(1));
    else if (_[0] == "help" || _[0] == "?") help();
    else cerr << "The command «" << _[0].toStdString()
              << "» is undefined, sorry. Try «help» or «?».\n";
}
// =================== GNU Readline interface ===================
char** Cli::tab_complete(const char* _beginning, int _start, int /*_end*/) {
    rl_attempted_completion_over = true;
    if (_start == 0) return rl_completion_matches(_beginning, cmd_completer);
    const QString user_input = QString::fromUtf8(rl_line_buffer);
    if (user_input.startsWith("cat ")
        or user_input.startsWith("passwd ")
        or user_input.startsWith("rm ")
       ) 
        return rl_completion_matches(_beginning, entries_completer);
    if (user_input.startsWith("cd ")
        or user_input.startsWith("rd ")
        or user_input.startsWith("rmdir ")
        )
        return rl_completion_matches(_beginning, dir_completer);
    if (user_input.startsWith("set ")) {
        QString set_cmd = user_input.mid(4);
        if (not set_cmd.contains(QRegExp("[^ ]+ ")))
            // awaiting action for set
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
    if (the()._HEntries.empty()) the().ls(ls_quiet::yes);
    static int i; if (_state == 0) i = -1;
    return complete(_beginning, the()._HEntries.keys(), i);
}
char* Cli::dir_completer(const char* _beginning, int _state) {
    if (the()._HSubGroups.empty()) the().ls(ls_quiet::yes);
    static int i; if (_state == 0) i = -1;
    return complete(_beginning, the()._HSubGroups.keys(), i);
}
QStringList Cli::_AllCmds;
char* Cli::cmd_completer(const char* _beginning, int _state) {
    if (_AllCmds.empty()) {
#define _(cmd) _AllCmds.push_back(cmd)
        _("cd"), _("s"), _("p");
        _("l"), _("ls"); _("mkdir"), _("rmdir");
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
    _("title"); _("passwd"); _("url"), _("comment"); _("user");
    _("help");
#undef _
    return complete(_beginning, cmds, i);
}
