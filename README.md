fofimon
=======

fofimon is a folders and files monitor for Linux.
It is a command line tool based on inotify that watches directories,
tracking the creation, removal, modification and renaming of files and
(sub)directories.

Motivation
----------
It might be useful if:

  - You want to know all the files that are created when installing an
    application or its plugins and extensions.

  - You don't trust an application and want to know what files it modifies.

  - You want to know where an application stores configuration files,
    to then copy them when migrating to a new computer or hopping to a
    new Linux distribution.

Usage
-----

To determine which directories (and their content) should be watched,
you have to define directory zones. A directory zone is defined with
a base path, the maximum depth (depth 0 meaning only the files in the
base path) and optionally inclusion and exclusion filters.
All the directories that exist when fofimon is started or are created
at runtime within the zone are watched.
Note 1: fofimon doesn't follow symbolic links. Symbolic links to a
directory are considered just files.
Note 2: the base path of a directory zone mustn't necessarily
exist when fofimon is started.
Note 3: a directory zone the base path of which is within another
directory zone shadows the ancestor.

Beside directory zones, fofimon also allows to watch single files.

Before you run fofimon it is better to increase the maximum number of
inotify watches (default is just 8192), especially if you want to watch
most of the file system:

    $ sudo sh -c 'echo 128000 >/proc/sys/fs/inotify/max_user_watches'

If it runs out of inotify watches, fofimon stops. It's therefore important
to set the number to a high enough value. To that end you can use fofimon
itself (preferably using sudo)

    $ fofimon  -z /  -m -1 --dont-watch

which returns the current number of watchable directories in your system.

Read the INSTALL file for installation instructions.

Usage examples
--------------

  To watch the changes in your home directory to a depth of 4
  (subdirectories):

    $ fofimon \
      -z ~/   -m 4  \
      -l /tmp/fofi-actions.txt  \
      -o /tmp/fofi-results.txt  \
      --print-watched /tmp/fofi-watched.txt

  To watch the changes in the whole file system except the Trash:

    $ sudo fofimon \
      -z / -m -1  \
      -z ~/.local/share/Trash  --exclude-all  \
      -l  \
      -o /tmp/fofi-results.txt

Limitations
-----------
Because of the limitations of the inotify API, which is inode based,
fofimon can only make a best effort to detect the changed files.

For instance, when a directory containing millions of files is renamed,
the file system just assigns a new path to the directory inode,
a simple operation that might last a few milliseconds. For fofimon, on the
other hand, it means a lot of work. For each file and subdirectory in the
renamed directory tree, it creates two entries, one for the source and one
for the destination path.
This can cause the loss of inotify events and consequently create
inconsistencies in the reported results.
In this kind of scenario it might be better to use inotifywait, which is
available in most Linux distributions.

In particular circumstances fofimon might also fail to detect temporary
files if renaming is done from or to an unwatched directory.

Status
------
This program is alpha (not production ready) and needs a lot more testing.
Also, recovery from inconsistent states (when the results don't match the
current situation on disk) can be improved.

For more info visit http://www.efanomars.com/utilities/fofimon
