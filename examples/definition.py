import time

# types of file, for full list of types visit https://docs.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants 
FaFile = 0
FaHidden = 2
FaDirectory = 16

# get_file result statused - complete list in fsplugin.chm
FileOK = 0
FileExists = 1
FileNotFound = 2

files = None

def iter_get_file():
    t = next(files, None)
    if t == None:
        return  { "filename": None }
    return { 
        "filename": t[0]          # file name
        , "attributes": t[1]          # distinguish at least directory from files
        , "size": t[2]                  # size of file
        , "updated" : int(time.time())  # posix timestamp
    }


def find_first(path):
    global files
    if (path == '\\'):
        files = iter([("a.txt", FaFile, 1), ("b.txt", FaFile, 2), ("c.txt", FaFile | FaHidden, 3), ("some_dir", FaDirectory, 0)])
    else:
        files = iter([("inner.txt", FaFile, 1), ("inner.pdf", FaFile, 2), ("inner.xml", FaFile | FaHidden, 3)])
        
    return iter_get_file()

    # return if you have file or directory
    return { 
        "filename": "file.txt"          # file name
        , "attributes": FaFile          # distinguish at least directory from files
        , "size": 123                   # size of file or directory. directory is usually zero
        , "updated" : int(time.time())  # posix timestamp, not supported yet
    }

    # if directory is empty return None
    return { "filename": None }

def find_next():
    return iter_get_file()



def get_file(remote_name, local_name):
    # copy content from remote (virtual) file to local (temp) file
    with open(local_name, "w") as f:
        f.write(f"this is content of file '{remote_name}'")
    return FileOK
#
#print(find_first('\\'))