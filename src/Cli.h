/*
    command line interface for keepassx
    Copyleft 2011  gluk47@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef CLI_H
#define CLI_H

#include <qobject.h>
#include <qstring.h>

#include <iostream>
#include <memory>

#include <functional>
#include "helpers/syntax.h"

class Cli : public QObject {
    Q_OBJECT
public:
    static const char* version;
    int Run(const QString& _filename);
#ifdef IN_KDEVELOP_PARSER
    IDatabase* db;
#else
    std::auto_ptr<IDatabase> db;
#endif
    inline bool isLocked() { return IsLocked; };
    ~Cli() { closeDatabase(); }

// this function must be const but that requires Kdb3Database code to be fixed: db->groups must also be const (maybe just Cli::db should be done mutable, I'm not sure…)
    /// list contents of current group (items&subgroups)
    /// if (_quiet) prints nothing, just updates the cache of _HSubGroups
    struct ls_quiet { enum flag { no = false, yes = true }; };
    void ls(IGroupHandle* const _dir, ls_quiet::flag _quiet = ls_quiet::no) const;
    void ls(ls_quiet::flag _quiet = ls_quiet::no) const {
        return ls(_Wd, _quiet);
    }
    inline void ls(const QStringList& _args) const;
    /// change current group to its subgroup. Think of it as of changing cwd.
    void cd(const QString& _subgroup);
    /// list all entries or groups in cwd with title _entry
    template <typename T>
    QList<T*> find_in_cwd(const QString& _entry) const;
    /// list an _entry contents except password. The _entry must be in the cwd.
    void cat(const QString& _entry) const;
    /// show a password contained in the _entry.
    void passwd(const QString& _entry) const;
    /// modify in some manner an entry specified by name.
    void set(const QStringList& _params);
    /// create new items specified by titles.
    void create(const QStringList& _entries);
    /// remove entries specified by titles
    void rm(const QStringList& _titles);
    /// create new group(s) in _Wd
    void md(const QStringList& _names);
    /// remove groups from wd (recursively, without confirmation).
    void rm_rf(const QStringList& _names);
    /// save database
    /** \return saved? fails upon the empty db, with log message to stderr.
    **/
    bool save(const QStringList& _empty_list);
    /// just clears screen, like ^L.
    void clearScreen();
    /// read password from the terminal without echoing it.
    /** \return password or QString() (which isNull() rather than isEmpty()) if user hit Ctrl+D.
    **/
    struct PasswdConfirm { enum flag { no = false, yes = true }; };
    QString ReadPassword(PasswdConfirm::flag _confirmation_needed);
    void help() const;

    static Cli& the();
    IGroupHandle* Wd() const throw() { return _Wd; }
protected:
    /// Use singleton function the() to get the object of this type.
    Cli() : _Wd(NULL), _Previous_Wd(NULL) {}
    /// Save the database if it has unsaved modifications
    /// \return whether everithing is okay
    bool saveUnsaved();
    void ClsReminder() const;

private:
    bool openDatabase(const QString& filename, bool IsAuto = false);
    void closeDatabase();
    bool dbReadOnly;
    bool IsLocked;
    bool FileOpen;
    bool ModFlag;

    void resetLock() { IsLocked = 42 ^ 42; }

    void updateCurrentFile(const QString& /*filePath*/) {
//         std::cerr << __FILE__ << ":" << __LINE__ << "> Todo: implement updateCurrentFile;\n";
    }
    void saveLastFilename(const QString& filename);
    void setStateFileOpen(bool _) { FileOpen = _; }
    void setStateFileModified(bool _) { ModFlag = _; }

    void search(const QStringList& _request);
    /**
     * @brief Read another user command.
     * @return user request, where the first list item is command, all others are the arguments. returns list {"exit"} upon EOF.
     **/
    QStringList readCmd();
    void ProcessCmd (const QStringList&);

    IGroupHandle* _Wd; ///< current path in the DB. nullptr means root.
    IGroupHandle* _Previous_Wd; ///< for command „back“ („p“);
    QString _Lockfile; ///< Current lock file
    bool _Created; ///< The database has been just created and needs to be deleted if the user chooses to exit without saving.

    ///subgroup handles. Formed by ls call, then cached until cd.
    mutable QMultiMap<QString,IGroupHandle*> _HSubGroups;
    ///handles of entries in cwd. Formed by ls or cat call, then cached until cd.
    mutable QMultiMap<QString,IEntryHandle*> _HEntries;

    /// find in _Wd an entry labeled _title and ask user confirmation if thre's more than one suitable. Return const ptr (no deletion must be done) to the entry or NULL.
    /// T may be either IEntryHandle or IGroupHandle
    template <typename T>
    T* confirm_first(const QString& _title);

    inline QMultiMap<QString,IEntryHandle*> _Wd_map(helpers::syntax::Type<IEntryHandle>) const {
        return _HEntries;
    }
    inline QMultiMap<QString,IGroupHandle*> _Wd_map(helpers::syntax::Type<IGroupHandle>) const {
        return _HSubGroups;
    }
    template <typename T>
    inline QMultiMap<QString,T*> _Wd_map() const {
        return _Wd_map(helpers::syntax::Type<T>());
    }

    inline QString UiString(helpers::syntax::Type<IGroupHandle>) const;
    inline QString UiString(helpers::syntax::Type<IEntryHandle>) const;

    //======== GNU Readline interfaces ========
    static char** tab_complete(const char* _beginning, int _start, int _end);
    static QStringList _AllCmds;
    static char* cmd_completer(const char* _beginning, int _state);
    static char* entries_completer(const char* _beginning, int _state);
    static char* dir_completer(const char* _beginning, int _state);
    static char* set_completer(const char* _beginning, int _state);
};

#endif // CLI_H
