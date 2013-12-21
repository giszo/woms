#include "sqlitestorage.h"

#include <thread/blocklock.h>

using library::SqliteStorage;

// =====================================================================================================================
SqliteStorage::SqliteStorage()
    : m_db(NULL)
{
}

// =====================================================================================================================
SqliteStorage::~SqliteStorage()
{
    if (m_db)
	sqlite3_close(m_db);
}

// =====================================================================================================================
void SqliteStorage::open()
{
    if (sqlite3_open("library.db", &m_db) != SQLITE_OK)
	throw StorageException("unable to open database");

    // create db tables
    execute(
	R"(CREATE TABLE IF NOT EXISTS artists(
	      id INTEGER PRIMARY KEY,
	      name TEXT,
              UNIQUE(name)))");
    execute(
	R"(CREATE TABLE IF NOT EXISTS albums(
	    id INTEGER PRIMARY KEY,
	    artist_id INTEGER,
	    name TEXT,
	    UNIQUE(artist_id, name),
	    FOREIGN KEY(artist_id) REFERENCES artists(id)))");
    execute(
	R"(CREATE TABLE IF NOT EXISTS files(
	    id INTEGER PRIMARY KEY,
	    artist_id INTEGER DEFAULT NULL,
	    album_id INTEGER DEFAULT NULL,
	    path TEXT,
	    name TEXT,
	    length INTEGER DEFAULT NULL,
	    title TEXT DEFAULT NULL,
	    year INTEGER DEFAULT NULL,
	    track_index INTEGER DEFAULT NULL,
	    mark INTEGER DEFAULT 1,
	    UNIQUE(path, name),
	    FOREIGN KEY(artist_id) REFERENCES artists(id),
	    FOREIGN KEY(album_id) REFERENCES albums(id)))");

    // prepare statements
    prepareStatement(&m_newFile, "INSERT OR IGNORE INTO files(path, name) VALUES(?, ?)");
    prepareStatement(&m_getFile,
                     R"(SELECT files.path, files.name, files.length, files.title, files.year, files.track_index,
                               albums.name,
                               artists.name
                        FROM files LEFT JOIN albums  ON albums.id = files.album_id
                                   LEFT JOIN artists ON artists.id = files.artist_id
                        WHERE files.id = ?)");
    prepareStatement(&m_getFileByPath, "SELECT id FROM files WHERE path = ? AND name = ?");
    prepareStatement(&m_getFiles,
                     R"(SELECT files.id, files.path, files.name, files.length, files.title, files.year, files.track_index,
                               albums.name,
                               artists.name
                        FROM files LEFT JOIN albums  ON albums.id = files.album_id
                                   LEFT JOIN artists ON artists.id = files.artist_id)");
    prepareStatement(&m_getFilesWithoutMeta, "SELECT id, path, name FROM files WHERE length IS NULL");
    prepareStatement(&m_getFilesOfArtist,
                     "SELECT id, path, name, length, title, year, track_index FROM files WHERE artist_id IS ?");
    prepareStatement(&m_getFilesOfAlbum, R"(SELECT id, path, name, length, title, year, track_index
                                            FROM files
                                            WHERE album_id = ?
                                            ORDER BY track_index)");
    prepareStatement(&m_setFileMark, "UPDATE files SET mark = 1 WHERE id = ?");

    prepareStatement(&m_updateFileMeta,
                     R"(UPDATE files
                        SET artist_id = ?, album_id = ?, length = ?, title = ?, year = ?, track_index = ?
                        WHERE id = ?)");

    // artists
    prepareStatement(&m_addArtist, "INSERT OR IGNORE INTO artists(name) VALUES(?)");
    prepareStatement(&m_getArtists,
                     R"(SELECT artists.id, artists.name, COUNT(DISTINCT files.album_id), COUNT(files.id)
                        FROM files LEFT JOIN artists ON artists.id = files.artist_id
                        GROUP BY files.artist_id
                        ORDER BY artists.name)");

    prepareStatement(&m_getArtistIdByName, "SELECT id FROM artists WHERE name = ?");

    // albums
    prepareStatement(&m_addAlbum, "INSERT OR IGNORE INTO albums(artist_id, name) VALUES(?, ?)");
    prepareStatement(&m_getAlbumIdByName, "SELECT id FROM albums WHERE artist_id IS ? AND name = ?");
    prepareStatement(&m_getAlbums,
                     R"(SELECT albums.id, albums.name, files.artist_id, COUNT(files.id), SUM(files.length)
                        FROM files LEFT JOIN albums ON albums.id = files.album_id
                        GROUP BY files.album_id
                        ORDER BY albums.name)");
    prepareStatement(&m_getAlbumsByArtist,
                     R"(SELECT albums.id, albums.name, COUNT(files.id), SUM(files.length)
                        FROM files LEFT JOIN albums ON albums.id = files.album_id
                        WHERE files.artist_id = ?
                        GROUP BY files.album_id
                        ORDER BY albums.name)");

    // mark
    prepareStatement(&m_clearMark, "UPDATE files SET mark = 0");
    prepareStatement(&m_deleteNonMarked, "DELETE FROM files WHERE mark = 0");
}

// =====================================================================================================================
bool SqliteStorage::addFile(File& file)
{
    thread::BlockLock bl(m_mutex);

    // first check whether the file already exists
    int id = getFileIdByPath(file.m_path, file.m_name);

    // the file was found ...
    if (id != -1)
    {
	// set mark on the file
	sqlite3_bind_int(m_setFileMark, 1, id);
	sqlite3_step(m_setFileMark);
	sqlite3_reset(m_setFileMark);

	return false;
    }

    // add the new file
    bindText(m_newFile, 1, file.m_path);
    bindText(m_newFile, 2, file.m_name);
    sqlite3_step(m_newFile);
    sqlite3_reset(m_newFile);

    // set the ID of the new file
    file.m_id = sqlite3_last_insert_rowid(m_db);

    return true;
}

// =====================================================================================================================
void SqliteStorage::clearMark()
{
    thread::BlockLock bl(m_mutex);

    sqlite3_step(m_clearMark);
    sqlite3_reset(m_clearMark);
}

// =====================================================================================================================
void SqliteStorage::deleteNonMarked()
{
    thread::BlockLock bl(m_mutex);

    sqlite3_step(m_deleteNonMarked);
    sqlite3_reset(m_deleteNonMarked);
}

// =====================================================================================================================
std::shared_ptr<library::File> SqliteStorage::getFile(int id)
{
    std::shared_ptr<File> file;

    thread::BlockLock bl(m_mutex);

    sqlite3_bind_int(m_getFile, 1, id);

    if (sqlite3_step(m_getFile) != SQLITE_ROW)
	throw FileNotFoundException("file not found with ID");

    file = std::make_shared<File>(
	id,
	getText(m_getFile, 0), // path
	getText(m_getFile, 1), // name
	sqlite3_column_int(m_getFile, 2), // length
	getText(m_getFile, 7), // artist
	getText(m_getFile, 6), // album
	getText(m_getFile, 3), // title
	sqlite3_column_int(m_getFile, 4), // year
	sqlite3_column_int(m_getFile, 5) // track index
    );

    sqlite3_reset(m_getFile);

    return file;
}

// =====================================================================================================================
std::vector<std::shared_ptr<library::File>> SqliteStorage::getFiles()
{
    std::vector<std::shared_ptr<File>> files;

    thread::BlockLock bl(m_mutex);

    while (sqlite3_step(m_getFiles) == SQLITE_ROW)
    {
	std::shared_ptr<File> file = std::make_shared<File>(
	    sqlite3_column_int(m_getFiles, 0), // id
	    getText(m_getFiles, 1), // path
	    getText(m_getFiles, 2), // name
	    sqlite3_column_int(m_getFiles, 3), // length
	    getText(m_getFiles, 8), // artist
	    getText(m_getFiles, 7), // album
	    getText(m_getFiles, 4), // title
	    sqlite3_column_int(m_getFiles, 5), // year
	    sqlite3_column_int(m_getFiles, 6) // track index
	);
	files.push_back(file);
    }
    sqlite3_reset(m_getFiles);

    return files;
}

// =====================================================================================================================
std::vector<std::shared_ptr<library::File>> SqliteStorage::getFilesWithoutMetadata()
{
    std::vector<std::shared_ptr<File>> files;

    thread::BlockLock bl(m_mutex);

    while (sqlite3_step(m_getFilesWithoutMeta) == SQLITE_ROW)
    {
	files.push_back(std::make_shared<File>(
	    sqlite3_column_int(m_getFilesWithoutMeta, 0),
	    getText(m_getFilesWithoutMeta, 1),
	    getText(m_getFilesWithoutMeta, 2)
	));
    }
    sqlite3_reset(m_getFilesWithoutMeta);

    return files;
}

// =====================================================================================================================
std::vector<std::shared_ptr<library::File>> SqliteStorage::getFilesOfArtist(int artistId)
{
    std::vector<std::shared_ptr<File>> files;

    thread::BlockLock bl(m_mutex);

    if (artistId == -1)
	sqlite3_bind_null(m_getFilesOfArtist, 1);
    else
	sqlite3_bind_int(m_getFilesOfArtist, 1, artistId);

    while (sqlite3_step(m_getFilesOfArtist) == SQLITE_ROW)
    {
	files.push_back(std::make_shared<File>(
	    sqlite3_column_int(m_getFilesOfArtist, 0),
	    getText(m_getFilesOfArtist, 1),
	    getText(m_getFilesOfArtist, 2),
	    sqlite3_column_int(m_getFilesOfArtist, 3),
	    "", // artist
	    "", // album
	    getText(m_getFilesOfArtist, 4),
	    sqlite3_column_int(m_getFilesOfArtist, 5),
	    sqlite3_column_int(m_getFilesOfArtist, 6)
	));
    }
    sqlite3_reset(m_getFilesOfArtist);

    return files;

}

// =====================================================================================================================
std::vector<std::shared_ptr<library::File>> SqliteStorage::getFilesOfAlbum(int albumId)
{
    std::vector<std::shared_ptr<File>> files;

    thread::BlockLock bl(m_mutex);

    sqlite3_bind_int(m_getFilesOfAlbum, 1, albumId);

    while (sqlite3_step(m_getFilesOfAlbum) == SQLITE_ROW)
    {
	files.push_back(std::make_shared<File>(
	    sqlite3_column_int(m_getFilesOfAlbum, 0),
	    getText(m_getFilesOfAlbum, 1),
	    getText(m_getFilesOfAlbum, 2),
	    sqlite3_column_int(m_getFilesOfAlbum, 3),
	    "", // artist
	    "", // album
	    getText(m_getFilesOfAlbum, 4),
	    sqlite3_column_int(m_getFilesOfAlbum, 5),
	    sqlite3_column_int(m_getFilesOfAlbum, 6)
	));
    }
    sqlite3_reset(m_getFilesOfAlbum);

    return files;

}

// =====================================================================================================================
void SqliteStorage::updateFileMetadata(const library::File& file)
{
    int artistId;
    int albumId;

    thread::BlockLock bl(m_mutex);

    // handle artist
    if (file.m_artist.empty())
	artistId = -1;
    else
    {
	bindText(m_addArtist, 1, file.m_artist);
	if (sqlite3_step(m_addArtist) != SQLITE_DONE)
	    throw StorageException("unable to insert artist");
	sqlite3_reset(m_addArtist);

	bindText(m_getArtistIdByName, 1, file.m_artist);
	if (sqlite3_step(m_getArtistIdByName) != SQLITE_ROW)
	    throw StorageException("unable to get artist after inserting!");
	artistId = sqlite3_column_int(m_getArtistIdByName, 0);
	sqlite3_reset(m_getArtistIdByName);
    }

    // handle album
    if (file.m_album.empty())
	albumId = -1;
    else
    {
	if (artistId == -1)
	    sqlite3_bind_null(m_addAlbum, 1);
	else
	    sqlite3_bind_int(m_addAlbum, 1, artistId);
	bindText(m_addAlbum, 2, file.m_album);
	if (sqlite3_step(m_addAlbum) != SQLITE_DONE)
	    throw StorageException("unable to insert album");
	sqlite3_reset(m_addAlbum);

	if (artistId == -1)
	    sqlite3_bind_null(m_getAlbumIdByName, 1);
	else
	    sqlite3_bind_int(m_getAlbumIdByName, 1, artistId);
	bindText(m_getAlbumIdByName, 2, file.m_album);
	if (sqlite3_step(m_getAlbumIdByName) != SQLITE_ROW)
	    throw StorageException("unable to get album after inserting!");
	albumId = sqlite3_column_int(m_getAlbumIdByName, 0);
	sqlite3_reset(m_getAlbumIdByName);
    }

    if (artistId == -1)
	sqlite3_bind_null(m_updateFileMeta, 1);
    else
	sqlite3_bind_int(m_updateFileMeta, 1, artistId);
    if (albumId == -1)
	sqlite3_bind_null(m_updateFileMeta, 2);
    else
	sqlite3_bind_int(m_updateFileMeta, 2, albumId);
    sqlite3_bind_int(m_updateFileMeta, 3, file.m_length);
    bindText(m_updateFileMeta, 4, file.m_title);
    sqlite3_bind_int(m_updateFileMeta, 5, file.m_year);
    sqlite3_bind_int(m_updateFileMeta, 6, file.m_trackIndex);
    sqlite3_bind_int(m_updateFileMeta, 7, file.m_id);

    sqlite3_step(m_updateFileMeta);
    sqlite3_reset(m_updateFileMeta);
}

// =====================================================================================================================
std::vector<std::shared_ptr<library::Artist>> SqliteStorage::getArtists()
{
    std::vector<std::shared_ptr<Artist>> artists;

    thread::BlockLock bl(m_mutex);

    while (sqlite3_step(m_getArtists) == SQLITE_ROW)
    {
	std::shared_ptr<Artist> artist = std::make_shared<Artist>(
	    sqlite3_column_type(m_getArtists, 0) == SQLITE_NULL ? -1 : sqlite3_column_int(m_getArtists, 0),
	    getText(m_getArtists, 1),
	    sqlite3_column_int(m_getArtists, 2),
	    sqlite3_column_int(m_getArtists, 3));
	artists.push_back(artist);
    }

    sqlite3_reset(m_getArtists);

    return artists;
}

// =====================================================================================================================
std::vector<std::shared_ptr<library::Album>> SqliteStorage::getAlbums()
{
    std::vector<std::shared_ptr<Album>> albums;

    thread::BlockLock bl(m_mutex);

    while (sqlite3_step(m_getAlbums) == SQLITE_ROW)
    {
	std::shared_ptr<Album> album = std::make_shared<Album>(
	    sqlite3_column_int(m_getAlbums, 0),
	    getText(m_getAlbums, 1),
	    sqlite3_column_int(m_getAlbums, 2),
	    sqlite3_column_int(m_getAlbums, 3),
	    sqlite3_column_int(m_getAlbums, 4));
	albums.push_back(album);
    }

    sqlite3_reset(m_getAlbums);

    return albums;
}

// =====================================================================================================================
std::vector<std::shared_ptr<library::Album>> SqliteStorage::getAlbumsByArtist(int artistId)
{
    std::vector<std::shared_ptr<Album>> albums;

    thread::BlockLock bl(m_mutex);

    sqlite3_bind_int(m_getAlbumsByArtist, 1, artistId);

    while (sqlite3_step(m_getAlbumsByArtist) == SQLITE_ROW)
    {
	std::shared_ptr<Album> album = std::make_shared<Album>(
	    sqlite3_column_int(m_getAlbumsByArtist, 0),
	    getText(m_getAlbumsByArtist, 1),
	    artistId,
	    sqlite3_column_int(m_getAlbumsByArtist, 2),
	    sqlite3_column_int(m_getAlbumsByArtist, 3));
	albums.push_back(album);
    }

    sqlite3_reset(m_getAlbumsByArtist);

    return albums;
}

// =====================================================================================================================
void SqliteStorage::execute(const std::string& sql)
{
    char* error;

    if (sqlite3_exec(m_db, sql.c_str(), NULL, NULL, &error) != SQLITE_OK)
	throw StorageException("unable to execute query");
}

// =====================================================================================================================
void SqliteStorage::prepareStatement(sqlite3_stmt** stmt, const std::string& sql)
{
    if (sqlite3_prepare_v2(m_db, sql.c_str(), sql.length() + 1, stmt, NULL) != SQLITE_OK)
	throw StorageException("unable to prepare statement: " + sql);
}

// =====================================================================================================================
std::string SqliteStorage::getText(sqlite3_stmt* stmt, int col)
{
    const char* s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));

    if (!s)
	return "";

    return s;
}

// =====================================================================================================================
void SqliteStorage::bindText(sqlite3_stmt* stmt, int col, const std::string& s)
{
    sqlite3_bind_text(stmt, col, s.c_str(), s.length(), NULL);
}

// =====================================================================================================================
int SqliteStorage::getFileIdByPath(const std::string& path, const std::string& name)
{
    int id;

    bindText(m_getFileByPath, 1, path);
    bindText(m_getFileByPath, 2, name);

    if (sqlite3_step(m_getFileByPath) == SQLITE_ROW)
	id = sqlite3_column_int(m_getFileByPath, 0);
    else
	id = -1;

    sqlite3_reset(m_getFileByPath);

    return id;
}
