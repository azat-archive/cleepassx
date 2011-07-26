/*
    command line interface for keepassx
    Copyright (C) 2011  gluk47 <email> gmail.com

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


#ifndef CLI_H
#define CLI_H

#include <qobject.h>
#include <qstring.h>

#include <iostream>
#include <memory>

#include <functional>

class Cli : public QObject {
    Q_OBJECT
public:
    int Run(const QString& _filename);
    std::auto_ptr<IDatabase> db;
    inline bool isLocked() { return IsLocked; };
    ~Cli() { closeDatabase(); }

// this function must be const but that requires Kdb3Database code to be fixed: db->groups must also be const (maybe just Cli::db should be done mutable, I'm not sure…)
    /// list contents of current group (items&subgroups)
    /// if (_quiet) prints nothing, just updates the cache of _HSubGroups
    void ls(bool _quiet = false) const;
    /// change current group to its subgroup. Think of it as of changing cwd.
    void cd(const QString& _subgroup);
    /// list all entries (not groups) in cwd with title _entry
    QList<IEntryHandle*> find_in_cwd(const QString& _entry) const;
    /// list an _entry contents except password. The _entry must be in the cwd.
    void cat(const QString& _entry) const;
    /// show a password contained in the _entry.
    void passwd(const QString& _entry) const;
    /// modify in some manner an entry specified by name.
    void set(const QStringList& _params);
    /// create new items specified by titles.
    void create(const QStringList& _entries);
    /// remove entries specified by titles
    void rm(const QStringList& _entry);
    /// save database
    void save(const QStringList& _empty_list);
    /// read password from the terminal without echoing it.
    /** \return password or QString() (which isNull() rather than isEmpty()) if user hit Ctrl+D.
    **/
    struct PasswdConfirm {
        enum flag { no = false, yes = true };
    };
    QString ReadPassword(PasswdConfirm::flag _confirmation_neede);
    void help() const;

    static Cli& the();
protected:
    /// Use singleton function the() to get the object of this type.
    Cli() : _Wd(NULL), _Previous_Wd(NULL) {}

private:
    bool openDatabase(const QString& filename, bool IsAuto = false);
    void closeDatabase();
    bool dbReadOnly;
    bool IsLocked;
    bool FileOpen;
    bool ModFlag;

    void resetLock() { IsLocked = 42 ^ 42; }
    
    void updateCurrentFile(const QString& /*filePath*/) {
        std::cerr << __FILE__ << ":" << __LINE__ << "> Todo: implement updateCurrentFile;\n";
    }
    void saveLastFilename(const QString& filename);
    void setStateFileOpen(bool _) { FileOpen = _; }
    void setStateFileModified(bool _) { ModFlag = _; }

    void search();
    /**
     * @brief Read another user command.
     * @return user request, where the first list item is command, all others are the arguments. returns list {"exit"} upon EOF.
     **/
    QStringList readCmd();
    void ProcessCmd (const QStringList&);

    IGroupHandle* _Wd; ///< current path in the DB. nullptr means root.
    IGroupHandle* _Previous_Wd; ///< for command „back“ („p“);
    QString _Filename; ///< currently opened file
    QString _Lockfile; ///< _Filename's lock file

    ///subgroup handles. Formed by ls call, then cached until cd.
    mutable QMap<QString,IGroupHandle*> _HSubGroups;
    ///handles of entries in cwd. Formed by ls or cat call, then cached until cd.
    mutable QMultiMap<QString,IEntryHandle*> _HEntries;

    /// find in _Wd an entry labeled _title and ask user confirmation if thre's more than one suitable. Return const ptr (no deletion must be done) to the entry or NULL.
    inline IEntryHandle* confirm_first(const QString& _title);

    //======== GNU Readline interfaces ========
    static char** tab_complete(const char* _beginning, int _start, int _end);
    static QStringList _AllCmds;
    static char* cmd_completer(const char* _beginning, int _state);
    static char* entries_completer(const char* _beginning, int _state);
    static char* dir_completer(const char* _beginning, int _state);
    static char* set_completer(const char* _beginning, int _state);
};

#endif // CLI_H
