/**
 * This file is part of the Zeppelin music player project.
 * Copyright (c) 2013-2014 Zoltan Kovacs, Lajos Santa
 * See http://zeppelin-player.com for more details.
 */

#ifndef LIBRARY_SQLITESTORAGE_H_INCLUDED
#define LIBRARY_SQLITESTORAGE_H_INCLUDED

#include <zeppelin/library/storage.h>

#include <thread/mutex.h>

#include <sqlite3.h>

namespace config
{
struct Library;
}

namespace library
{

/**
 * Storage backend for the music library based on SQLite3.
 */
class SqliteStorage : public zeppelin::library::Storage
{
    public:
	SqliteStorage();
	~SqliteStorage();

	void open(const config::Library& config);

	zeppelin::library::Statistics getStatistics() override;

	std::vector<std::shared_ptr<zeppelin::library::Directory>> getDirectories(const std::vector<int>& ids) override;
	std::vector<int> getSubdirectoryIdsOfDirectory(int id) override;
	int ensureDirectory(const std::string& name, int parentId) override;

	bool addFile(zeppelin::library::File& file) override;

	void clearMark() override;
	void deleteNonMarked() override;

	std::vector<std::shared_ptr<zeppelin::library::File>> getFilesWithoutMetadata() override;

	std::vector<std::shared_ptr<zeppelin::library::File>> getFiles(const std::vector<int>& ids) override;
	std::vector<int> getFileIdsOfAlbum(int albumId) override;
	std::vector<int> getFileIdsOfDirectory(int directoryId) override;

	void setFileMetadata(const zeppelin::library::File& file) override;
	void updateFileMetadata(const zeppelin::library::File& file) override;

	std::vector<std::shared_ptr<zeppelin::library::Artist>> getArtists(const std::vector<int>& ids) override;

	std::vector<int> getAlbumIdsByArtist(int artistId) override;
	std::vector<std::shared_ptr<zeppelin::library::Album>> getAlbums(const std::vector<int>& ids) override;
	std::map<int, std::map<zeppelin::library::Picture::Type, std::shared_ptr<zeppelin::library::Picture>>> getPicturesOfAlbums(const std::vector<int>& ids) override;

	int createPlaylist(const std::string& name) override;
	void deletePlaylist(int id) override;
	int addPlaylistItem(int id, const std::string& type, int itemId) override;
	void deletePlaylistItem(int id) override;
	std::vector<std::shared_ptr<zeppelin::library::Playlist>> getPlaylists(const std::vector<int>& ids) override;

    private:
	void execute(const std::string& sql);
	void prepareStatement(sqlite3_stmt** stmt, const std::string& sql);

	int getFileIdByPath(const std::string& path, const std::string& name);
	int getArtistId(const zeppelin::library::Metadata& metadata);
	int getAlbumId(int artistId, const zeppelin::library::Metadata& metadata);

	static void serializeIntList(std::ostringstream& stream, const std::vector<int>& list);

    private:
	struct StatementHolder
	{
	    StatementHolder(sqlite3* db, const std::string& query);
	    StatementHolder(sqlite3_stmt* stmt);
	    ~StatementHolder();

	    void bindNull(int col);
	    void bindInt(int col, int value);
	    void bindInt64(int col, int64_t value);
	    void bindText(int col, const std::string& value);
	    void bindBlob(int col, const std::vector<unsigned char>& data);
	    // a special bind function that binds NULL if the value is -1, otherwise the numeric value
	    void bindIndex(int col, int value);

	    int step();

	    int columnCount();
	    std::string tableName(int col);
	    std::string columnName(int col);
	    int columnType(int col);

	    int getInt(int col);
	    int64_t getInt64(int col);
	    std::string getText(int col);
	    void getBlob(int col, std::vector<unsigned char>& data);

	    bool isNull(int col);

	    // true when the statement should be finalized by the holder
	    bool m_finalize;
	    sqlite3_stmt* m_stmt;
	};

    private:
	// the database to store the music library
	sqlite3* m_db;

	sqlite3_stmt* m_getDirectory;
	sqlite3_stmt* m_addDirectory;
	sqlite3_stmt* m_getSubdirectoryIds;

	sqlite3_stmt* m_newFile;

	sqlite3_stmt* m_getFileByPath;
	sqlite3_stmt* m_getFilesWithoutMeta;
	sqlite3_stmt* m_getFileIdsOfAlbum;
	sqlite3_stmt* m_getFileIdsOfDirectory;
	sqlite3_stmt* m_getFileStatistics;

	sqlite3_stmt* m_setFileMark;
	sqlite3_stmt* m_setDirectoryMark;

	sqlite3_stmt* m_setFileMeta;
	sqlite3_stmt* m_updateFileMeta;
	sqlite3_stmt* m_addAlbumPicture;

	/// artist handling
	sqlite3_stmt* m_addArtist;
	sqlite3_stmt* m_getArtistIdByName;
	sqlite3_stmt* m_getNumOfArtists;

	/// album handling
	sqlite3_stmt* m_addAlbum;
	sqlite3_stmt* m_getAlbumIdByName;
	sqlite3_stmt* m_getAlbumIdsByArtist;
	sqlite3_stmt* m_getNumOfAlbums;

	// playlist handling
	sqlite3_stmt* m_createPlaylist;
	sqlite3_stmt* m_deletePlaylist;
	sqlite3_stmt* m_addPlaylistItem;
	sqlite3_stmt* m_deletePlaylistItem;

	/// mark handling
	sqlite3_stmt* m_clearFileMarks;
	sqlite3_stmt* m_clearDirectoryMarks;
	sqlite3_stmt* m_deleteNonMarkedFiles;
	sqlite3_stmt* m_deleteNonMarkedDirectories;

	// mutex for the music database
	thread::Mutex m_mutex;
};

}

#endif
